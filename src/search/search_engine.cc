#include "search_engine.h"

#include "evaluation_context.h"
#include "evaluator.h"
#include "option_parser.h"
#include "plugin.h"

#include "algorithms/ordered_set.h"
#include "task_utils/successor_generator.h"
#include "task_utils/task_properties.h"
#include "tasks/root_task.h"
#include "task_utils/successor_generator.h"
#include "utils/logging.h"
#include "utils/rng_options.h"
#include "utils/system.h"
#include "utils/timer.h"
#include "utils/countdown_timer.h"

#include <cassert>
#include <iostream>
#include <limits>

using namespace std;
using utils::ExitCode;

class PruningMethod;

SearchEngine::SearchEngine(const Options &opts)
    : status(IN_PROGRESS),
      solution_found(false),
      task(opts.get<shared_ptr<AbstractTask>>("transform")),
      task_proxy(*task),
      state_registry(task_proxy),
      successor_generator(successor_generator::get_successor_generator(task_proxy)),
      search_space(state_registry),
      search_progress(opts.get<utils::Verbosity>("verbosity")),
      statistics(SearchStatistics(opts.get<utils::Verbosity>("verbosity"), opts.get<int>("max_expansions"))),
      statistics_interval(opts.get<double>("statistics_interval")),
      cost_type(opts.get<OperatorCost>("cost_type")),
      is_unit_cost(task_properties::is_unit_cost(task_proxy)),
      max_time(opts.get<double>("max_time")),
      timer(nullptr),
      verbosity(opts.get<utils::Verbosity>("verbosity")) {
    if (opts.get<int>("bound") < 0) {
        cerr << "error: negative cost bound " << opts.get<int>("bound") << endl;
        utils::exit_with(ExitCode::SEARCH_INPUT_ERROR);
    }
    bound = opts.get<int>("bound");
    task_properties::print_variable_statistics(task_proxy);
}

SearchEngine::~SearchEngine() {
}

void SearchEngine::print_statistics() const {
    utils::g_log << "Bytes per state: "
                 << state_registry.get_state_size_in_bytes() << endl;
}

void SearchEngine::print_timed_statistics() const {
    utils::g_log << "[Timed Statistics: ";
    statistics.print_basic_statistics();
    utils::g_log << "]" << endl;
}

bool SearchEngine::found_solution() const {
    return solution_found;
}

SearchStatus SearchEngine::get_status() const {
    return status;
}

const Plan &SearchEngine::get_plan() const {
    assert(solution_found);
    return plan;
}

const State SearchEngine::get_goal_state() const {
    assert(solution_found);
    return state_registry.lookup_state(goal_id);
}


void SearchEngine::set_plan(const Plan &p) {
    solution_found = true;
    plan = p;
}

const StateRegistry &SearchEngine::get_state_registry() const {
    return state_registry;
}

const SearchSpace &SearchEngine::get_search_space() const {
    return search_space;
}

const TaskProxy &SearchEngine::get_task_proxy() const {
    return task_proxy;
}

void SearchEngine::search() {
    initialize();
    assert(!timer);
    timer = make_unique<utils::CountdownTimer>(max_time);
    double last_statistic_time  = timer->get_elapsed_time();
    while (status == IN_PROGRESS) {
        try {
            status = step();
        } catch (MaximumExpansionsError e) {
            cout << "Maximum number of expansions reached. Abort search."
                << endl;
            break;
        }
        if (timer->is_expired()) {
            utils::g_log << "Time limit reached. Abort search." << endl;
            status = TIMEOUT;
            break;
        }
        if(statistics_interval > 0 &&
           timer->get_elapsed_time() - last_statistic_time > statistics_interval) {
            print_timed_statistics();
            last_statistic_time = timer->get_elapsed_time();
        }
    }
    // TODO: Revise when and which search times are logged.
    utils::g_log << "Actual search time: " << timer->get_elapsed_time() << endl;
}

bool SearchEngine::check_goal_and_set_plan(const State &state) {
    if (task_properties::is_goal_state(task_proxy, state)) {
        utils::g_log << "Solution found!" << endl;
        goal_id = state.get_id();
        Plan plan;
        search_space.trace_path(state, plan);
        set_plan(plan);
        return true;
    }
    return false;
}

void SearchEngine::save_plan_if_necessary() {
    if (found_solution()) {
        plan_manager.save_plan(get_plan(), task_proxy);
    }
}

int SearchEngine::get_adjusted_cost(const OperatorProxy &op) const {
    return get_adjusted_action_cost(op, cost_type, is_unit_cost);
}

double SearchEngine::get_max_time() {
    return max_time;
}
void SearchEngine::reduce_max_time(double new_max_time) {
    if (timer) {
        cerr << "The time limit of a search cannot be reduced after it was"
                "started" << endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    } else if (new_max_time > max_time){
        cerr << "You cannot increase the time limit of a search!" << endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    } else {
        max_time = new_max_time;
    }
}
/* TODO: merge this into add_options_to_parser when all search
         engines support pruning.

   Method doesn't belong here because it's only useful for certain derived classes.
   TODO: Figure out where it belongs and move it there. */
void SearchEngine::add_pruning_option(OptionParser &parser) {
    parser.add_option<shared_ptr<PruningMethod>>(
        "pruning",
        "Pruning methods can prune or reorder the set of applicable operators in "
        "each state and thereby influence the number and order of successor states "
        "that are considered.",
        "null()");
}

void SearchEngine::add_options_to_parser(OptionParser &parser) {
    ::add_cost_type_option_to_parser(parser);
    parser.add_option<int>(
        "bound",
        "exclusive depth bound on g-values. Cutoffs are always performed according to "
        "the real cost, regardless of the cost_type parameter", "infinity");
    parser.add_option<double>(
        "max_time",
        "maximum time in seconds the search is allowed to run for. The "
        "timeout is only checked after each complete search step "
        "(usually a node expansion), so the actual runtime can be arbitrarily "
        "longer. Therefore, this parameter should not be used for time-limiting "
        "experiments. Timed-out searches are treated as failed searches, "
        "just like incomplete search algorithms that exhaust their search space.",
        "infinity");
    utils::add_verbosity_option_to_parser(parser);
    parser.add_option<int>(
        "max_expansions",
        "maximum number of expansions the search is allowed to run for.",
        "infinity");
    parser.add_option<double>(
            "statistics_interval",
            "Prints every interval seconds some statistics on the search."
            "Use a negative value to disable. Default: 30",
            "30"
            );
    parser.add_option<shared_ptr<AbstractTask>>(
        "transform",
        "Optional task transformation for the search algorithm."
        " Currently, adapt_costs(), sampling_transform(), and no_transform() are "
        "available.",
        "no_transform()");
}

/* Method doesn't belong here because it's only useful for certain derived classes.
   TODO: Figure out where it belongs and move it there. */
void SearchEngine::add_succ_order_options(OptionParser &parser) {
    vector<string> options;
    parser.add_option<bool>(
        "randomize_successors",
        "randomize the order in which successors are generated",
        "false");
    parser.add_option<bool>(
        "preferred_successors_first",
        "consider preferred operators first",
        "false");
    parser.document_note(
        "Successor ordering",
        "When using randomize_successors=true and "
        "preferred_successors_first=true, randomization happens before "
        "preferred operators are moved to the front.");
    utils::add_rng_options(parser);
}

void print_initial_evaluator_values(const EvaluationContext &eval_context) {
    eval_context.get_cache().for_each_evaluator_result(
        [] (const Evaluator *eval, const EvaluationResult &result) {
            if (eval->is_used_for_reporting_minima()) {
                eval->report_value_for_initial_state(result);
            }
        }
        );
}

static PluginTypePlugin<SearchEngine> _type_plugin(
    "SearchEngine",
    // TODO: Replace empty string by synopsis for the wiki page.
    "");

void collect_preferred_operators(
    EvaluationContext &eval_context,
    Evaluator *preferred_operator_evaluator,
    ordered_set::OrderedSet<OperatorID> &preferred_operators) {
    if (!eval_context.is_evaluator_value_infinite(preferred_operator_evaluator)) {
        for (OperatorID op_id : eval_context.get_preferred_operators(preferred_operator_evaluator)) {
            preferred_operators.insert(op_id);
        }
    }
}
