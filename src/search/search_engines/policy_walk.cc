#include "policy_walk.h"

#include "../policy.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../task_utils/successor_generator.h"
#include "../task_utils/task_properties.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"
#include "../utils/system.h"

#include <algorithm>
#include <memory>
#include <optional.hh>


using namespace std;
using utils::ExitCode;

namespace search_engines {
    using Plan = std::vector<OperatorID>;

    OperatorSelection get_operator_selection(string selection) {
        if (selection == "first") {
            return OperatorSelection::First;
        } else if (selection == "best") {
            return OperatorSelection::Best;
        } else if (selection == "probability") {
            return OperatorSelection::Probability;
        } else {
            cerr << "Unknown operator selection option: " << selection << endl;
            utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
        }
    }

    PolicyWalk::PolicyWalk(
    const Options &opts)
    : SearchEngine(opts),
      policy(opts.get<shared_ptr<Policy>>("policy")),
      dead_end_evaluators(opts.get_list<shared_ptr<Evaluator>>("dead_end_evaluators")),
      trajectory_limit(opts.get<int>("trajectory_limit")),
      reopen(opts.get<bool>("reopen")),
      op_select(get_operator_selection(opts.get<string>("operator_selection"))),
      rng(utils::parse_rng_from_options(opts)),
      current_state(state_registry.get_initial_state())
      {
        if (trajectory_limit != -1 && trajectory_limit <= 0) {
            cerr << "Trajectory limit has to be positive!" << endl;
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
        }
    }

    PolicyWalk::~PolicyWalk() {
    }

    bool PolicyWalk::is_dead_end(EvaluationContext &eval_context) {
        return any_of(dead_end_evaluators.begin(), dead_end_evaluators.end(),
                   [&] (shared_ptr<Evaluator> eval){
            return eval_context.is_evaluator_value_infinite(eval.get());});
    }

    void PolicyWalk::initialize() {
        cout << "Conducting policy search" << endl;
        SearchNode current_node = search_space.get_node(current_state);
        EvaluationContext eval_context(
                current_node.get_state(), 0,
                true, &statistics);
        if (is_dead_end(eval_context)) {
            current_node.mark_as_dead_end();
            statistics.inc_dead_ends();
            cout << "Initial state is a dead end, no solution" << endl;
            utils::exit_with(ExitCode::SEARCH_UNSOLVABLE);
        }
        current_node.open_initial();
    }

    SearchStatus PolicyWalk::step() {
        SearchNode current_node = search_space.get_node(current_state);
        assert(!current_node.is_dead_end());
        current_node.close();

        if (check_goal_and_set_plan(current_state)) {
            return SOLVED;
        }
        statistics.inc_expanded();

        if (trajectory_limit != -1 && trajectory_length >= trajectory_limit) {
            cout << "No solution - trajectory limit reached" << endl;
            return SearchStatus::FAILED;
        }

        EvaluationContext eval_context(current_state, current_node.get_g(), true, &statistics);
        vector<OperatorID> operator_ids = eval_context.get_preferred_operators(policy.get());
        vector<float> operator_prefs = eval_context.get_operator_preferences(policy.get());

        while(!operator_ids.empty()) {
            assert(operator_ids.size() == operator_prefs.size());

            size_t op_idx;
            if (op_select == OperatorSelection::Probability) {
                op_idx = rng->weighted_choose_index(operator_prefs);
            } else if (op_select == OperatorSelection::First
                    || op_select == OperatorSelection::Best) {
                vector<size_t> best;
                float highest_probability = 0;
                for (size_t index = 0; index < operator_ids.size(); index++) {
                    if (operator_prefs[index] > highest_probability) {
                        highest_probability = operator_prefs[index];
                        best.clear();
                    }
                    if(operator_prefs[index] == highest_probability) {
                        best.push_back(index);
                    }
                }
                if (op_select == OperatorSelection::First) {
                    op_idx = best.front();
                } else {
                    op_idx = *rng->choose(best);
                }
            } else {
                cerr << "Internal error: Unknown operator selection mode" << endl;
                utils::exit_with(ExitCode::SEARCH_CRITICAL_ERROR);
            }

            OperatorID succ_op = operator_ids[op_idx];
            assert(succ_op != OperatorID::no_operator);
            OperatorProxy op_proxy = task_proxy.get_operators()[succ_op];
            assert(task_properties::is_applicable(op_proxy, current_state));
            const State succ_state = state_registry.get_successor_state(
                    current_state, op_proxy);
            SearchNode succ_node = search_space.get_node(succ_state);
            statistics.inc_generated();

            cout << "Policy selected operator " << op_proxy.get_name() << endl;

            // succ_node cannot be flagged as dead end, because then we would
            // have evaluated it earlier and stopped the search.
            eval_context = EvaluationContext(
                    succ_state, current_node.get_g() + get_adjusted_cost(op_proxy),
                    true, &statistics);
            if (succ_node.is_new()) {
                statistics.inc_evaluated_states();
                if (is_dead_end(eval_context)) {
                    operator_ids.erase(operator_ids.begin() + op_idx);
                    operator_prefs.erase(operator_prefs.begin() + op_idx);
                    succ_node.mark_as_dead_end();
                }
            }

            if(succ_node.is_new()) {
                succ_node.open(current_node, op_proxy, get_adjusted_cost(op_proxy));
            } else if (succ_node.is_closed()) {
                if (reopen) {
                    int adjusted_cost = get_adjusted_cost(op_proxy);
                    int new_g = current_node.get_g() + adjusted_cost;
                    if (new_g < succ_node.get_g()) {
                        succ_node.reopen(current_node, op_proxy, adjusted_cost);
                        statistics.inc_reopened();
                    } else {
                        succ_node.reopen();
                    }
                } else {
                    cout << "Policy Walked in a cycle and reopening is not enabled."
                         << endl;
                    return FAILED;
                }
            } else if (succ_node.is_dead_end()) {
                /* Either because this node was new and we detected above that
                 * it is a dead-end or we evaluated it already as dead end (e.g.
                 * because reached the state on another path*/
                continue;
            } else {
                cerr << "Internal error. An open search node should not be"
                        "reached in the policy walk" << endl;
                utils::exit_with(ExitCode::SEARCH_CRITICAL_ERROR);
            }

            current_state = move(succ_state);
            trajectory_length++;
            return IN_PROGRESS;
        }

        cout << "No solution - FAILED" << endl;
        return FAILED;
    }

    void PolicyWalk::print_statistics() const {
        statistics.print_detailed_statistics();
        search_space.print_statistics();
    }

    static shared_ptr<SearchEngine> _parse(OptionParser &parser) {
        parser.document_synopsis("Policy search", "");
        parser.add_option<shared_ptr<Policy>>("policy", "policy");
        parser.add_list_option<shared_ptr<Evaluator>>("dead_end_evaluators",
        "list of evaluators used for early dead-end detection", "[]");
        parser.add_option<int> ("trajectory_limit",
        "Int to represent the length limit for explored trajectories during "
        "network policy exploration", "-1");
        parser.add_option<string> (
            "operator_selection",
            "Selection mode for operators. Choose 'first' to select always"
            "the first operator with the highest probability. 'best' to select"
            "a random operator with the highest probability, or 'probability' to"
            "select operators depending on their probability.",
            "first");
        parser.add_option<bool> (
                "reopen",
                "Reopen nodes already visited. This makes only sense if "
                "either the policy is not deterministic or the walk is not "
                "deterministic.", "false");
        SearchEngine::add_options_to_parser(parser);
        utils::add_rng_options(parser);
        Options opts = parser.parse();

        shared_ptr<SearchEngine> engine;
        if (!parser.dry_run()) {
            engine = make_shared<PolicyWalk>(opts);
        }
        return engine;
    }

    static Plugin<SearchEngine> _plugin("policy_walk", _parse);
}
