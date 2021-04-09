#include "policy_search.h"

#include "../policy.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../task_utils/successor_generator.h"
#include "../task_utils/task_properties.h"

#include <algorithm>
#include <memory>
#include <optional.hh>


using namespace std;
using utils::ExitCode;

namespace search_engines {
    using Plan = std::vector<OperatorID>;

    PolicySearch::PolicySearch(
    const Options &opts)
    : SearchEngine(opts),
      policy(opts.get<shared_ptr<Policy>>("policy")),
      dead_end_evaluators(opts.get_list<shared_ptr<Evaluator>>("dead_end_evaluators")),
      trajectory_limit(opts.get<int>("trajectory_limit")),
      current_context(EvaluationContext(state_registry.get_initial_state(), 0, true, &statistics))
      {
        if (trajectory_limit != -1 && trajectory_limit <= 0) {
            cerr << "Trajectory limit has to be positive!" << endl;
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
        }
    }

    PolicySearch::~PolicySearch() {
    }

    bool PolicySearch::is_dead_end() {
        return any_of(dead_end_evaluators.begin(), dead_end_evaluators.end(),
                   [&] (shared_ptr<Evaluator> eval){
            return current_context.is_evaluator_value_infinite(eval.get());});
    }
    void PolicySearch::initialize() {
        cout << "Conducting policy search" << endl;
        if (is_dead_end()) {
            cout << "Initial state is a dead end, no solution" << endl;
            utils::exit_with(ExitCode::SEARCH_UNSOLVABLE);
        }
        statistics.inc_evaluated_states();

        SearchNode node = search_space.get_node(current_context.get_state());
        node.open_initial();
    }

    SearchStatus PolicySearch::step() {
        const State &state = current_context.get_state();
        SearchNode node = search_space.get_node(state);
        node.close();
        assert(!node.is_dead_end());

        if (check_goal_and_set_plan(state)) {
            return SOLVED;
        }

        statistics.inc_expanded();

        if (trajectory_limit != -1 && trajectory_length >= trajectory_limit) {
            cout << "No solution - trajectory limit reached" << endl;
            return SearchStatus::FAILED;
        }

        // collect policy output in current EvaluationContext
        vector<OperatorID> operator_ids = current_context.get_preferred_operators(policy.get());
        vector<float> operator_prefs = current_context.get_operator_preferences(policy.get());
        assert(!operator_ids.empty());
        assert(operator_ids.size() == operator_prefs.size());

        // find most probable/ preferenced operator
        OperatorID best_op = OperatorID::no_operator;
        float highest_probability = 0;
        for (size_t index = 0; index < operator_ids.size(); index++) {
            if (operator_prefs[index] > highest_probability) {
                highest_probability = operator_prefs[index];
                best_op = operator_ids[index];
            }
        }
        assert(best_op != OperatorID::no_operator);
        OperatorProxy op_proxy  = task_proxy.get_operators()[best_op];
        assert(task_properties::is_applicable(op_proxy, state));

        const State succ_state = state_registry.get_successor_state(state, op_proxy);
        cout << "Policy reached state with id " << succ_state.get_id()
             << " by applying op " << op_proxy.get_name()
             << " which had probability " << highest_probability << endl;
        SearchNode succ_node = search_space.get_node(succ_state);
        statistics.inc_generated();

        // succ_node cannot be dead end, because then we would have evaluated
        // it earlier and stopped the search.
        if (succ_node.is_new()) {
            current_context = EvaluationContext(
                    succ_state, node.get_g() + get_adjusted_cost(op_proxy),
                    true, &statistics);
            statistics.inc_evaluated_states();

            if (is_dead_end()) {
                cout << "No solution - dead end reached!" << endl;
                return FAILED;
            }
            succ_node.open(node, op_proxy, get_adjusted_cost(op_proxy));

            trajectory_length++;
            return IN_PROGRESS;
        }

        cout << "No solution - FAILED" << endl;
        return FAILED;
    }

    void PolicySearch::print_statistics() const {
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
        SearchEngine::add_options_to_parser(parser);
        Options opts = parser.parse();

        shared_ptr<PolicySearch> engine;
        if (!parser.dry_run()) {
            engine = make_shared<PolicySearch>(opts);
        }
        return engine;
    }

    static Plugin<SearchEngine> _plugin("policy_search", _parse);
}
