#include "heuristic.h"

#include "evaluation_context.h"
#include "evaluation_result.h"
#include "option_parser.h"
#include "plugin.h"

#include "task_utils/task_properties.h"
#include "tasks/cost_adapted_task.h"
#include "tasks/root_task.h"

#include <cassert>
#include <cstdlib>
#include <limits>

using namespace std;

Heuristic::Heuristic(const Options &opts)
    : Evaluator(
            opts.get<string>("alt_name", "<none>") == "<none>" ? opts.get_unparsed_config() : opts.get<string>("alt_name"),
                    true, true, true),
      heuristic_cache(HEntry(NO_VALUE, true)), //TODO: is true really a good idea here?
      cache_evaluator_values(opts.get<bool>("cache_estimates")),
      task(opts.get<shared_ptr<AbstractTask>>("transform")),
      task_proxy(*task) {
}

Heuristic::~Heuristic() {
}

void Heuristic::set_preferred(const OperatorProxy &op) {
    preferred_operators.insert(op.get_ancestor_operator_id(tasks::g_root_task.get()));
}

State Heuristic::convert_ancestor_state(const State &ancestor_state) const {
    return task_proxy.convert_ancestor_state(ancestor_state);
}

void Heuristic::add_options_to_parser(OptionParser &parser) {
    parser.add_option<shared_ptr<AbstractTask>>(
        "transform",
        "Optional task transformation for the heuristic."
        " Currently, adapt_costs(), sampling_transform(), and no_transform() are "
        "available.",
        "no_transform()");
    parser.add_option<bool>("cache_estimates", "cache heuristic estimates", "true");
    parser.add_option<string>("alt_name", "alternative name for printing the evaluators name", "<none>");
}

std::pair<int, double> Heuristic::compute_heuristic_and_confidence(const State &state) {
    return {compute_heuristic(state), DEAD_END};
}

EvaluationResult Heuristic::compute_result(EvaluationContext &eval_context) {
    EvaluationResult result;

    assert(preferred_operators.empty());

    const State &state = eval_context.get_state();
    bool calculate_preferred = eval_context.get_calculate_preferred();
    bool report_confidence = eval_context.get_report_confidence();

    int heuristic = NO_VALUE;
    double confidence = DEAD_END;

    if (!report_confidence && !calculate_preferred && cache_evaluator_values &&
        heuristic_cache[state].h != NO_VALUE && !heuristic_cache[state].dirty) {
        heuristic = heuristic_cache[state].h;
        result.set_count_evaluation(false);
    } else {
        if (report_confidence) {
            auto heuristic_confidence = compute_heuristic_and_confidence(state);
            heuristic = heuristic_confidence.first;
            confidence = heuristic_confidence.second;
        } else {
            heuristic = compute_heuristic(state);
        }
        if (cache_evaluator_values) {
            heuristic_cache[state] = HEntry(heuristic, false);
        }
        result.set_count_evaluation(true);
    }

    assert(heuristic == DEAD_END || heuristic >= 0);
    assert(confidence == DEAD_END || (0 <= confidence && confidence <= 1));

    if (heuristic == DEAD_END) {
        /*
          It is permissible to mark preferred operators for dead-end
          states (thus allowing a heuristic to mark them on-the-fly
          before knowing the final result), but if it turns out we
          have a dead end, we don't want to actually report any
          preferred operators.
        */
        preferred_operators.clear();
        heuristic = EvaluationResult::INFTY;
    }

#ifndef NDEBUG
    TaskProxy global_task_proxy = state.get_task();
    OperatorsProxy global_operators = global_task_proxy.get_operators();
    if (heuristic != EvaluationResult::INFTY) {
        for (OperatorID op_id : preferred_operators)
            assert(task_properties::is_applicable(global_operators[op_id], state));
    }
#endif

    result.set_evaluator_value(heuristic);
    result.set_preferred_operators(preferred_operators.pop_as_vector());
    result.set_confidence(confidence);
    assert(preferred_operators.empty());

    return result;
}

bool Heuristic::does_cache_estimates() const {
    return cache_evaluator_values;
}

bool Heuristic::is_estimate_cached(const State &state) const {
    return heuristic_cache[state].h != NO_VALUE;
}

int Heuristic::get_cached_estimate(const State &state) const {
    assert(is_estimate_cached(state));
    return heuristic_cache[state].h;
}


static PluginTypePlugin<Heuristic> _type_plugin(
        "Heuristic",
        "An evaluator specification is either a newly created evaluator "
        "instance or an evaluator that has been defined previously. "
        "This page describes how one can specify a new evaluator instance. "
        "For re-using evaluators, see OptionSyntax#Evaluator_Predefinitions.\n\n"
        "If the evaluator is a heuristic, "
        "definitions of //properties// in the descriptions below:\n\n"
        " * **admissible:** h(s) <= h*(s) for all states s\n"
        " * **consistent:** h(s) <= c(s, s') + h(s') for all states s "
        "connected to states s' by an action with cost c(s, s')\n"
        " * **safe:** h(s) = infinity is only true for states "
        "with h*(s) = infinity\n"
        " * **preferred operators:** this heuristic identifies "
        "preferred operators ");
