#include "network_heuristic.h"

#include "../evaluation_context.h"
#include "../option_parser.h"
#include "../plugin.h"

#include "../task_utils/task_properties.h"

using namespace std;

namespace network_heuristic {
NetworkHeuristic::NetworkHeuristic(const Options &opts)
    : Heuristic(opts),
      network(opts.get<shared_ptr<neural_networks::AbstractNetwork>>("network")) {
    cout << "Initializing network heuristic..." << endl;
    network->verify_heuristic();
    network->initialize();
}

NetworkHeuristic::~NetworkHeuristic() {
}

int NetworkHeuristic::compute_heuristic(const State &state) {
    if (task_properties::is_goal_state(task_proxy, state)) {
        return 0;
    }
    network->evaluate(state);
    int h = network->get_heuristic();

    if (network->is_preferred()) {
        for (const OperatorID &oid: network->get_preferred()) {
            preferred_operators.insert(oid);
        }
    }
    return h;
}

std::pair<int, double> NetworkHeuristic::compute_heuristic_and_confidence(const State &state) {
    int h = compute_heuristic(state);
    double confidence = Heuristic::DEAD_END;
    if (network->is_heuristic_confidence()) {
        confidence = network->get_heuristic_confidence();
    }
    return {h, confidence};
}

vector<EvaluationResult> NetworkHeuristic::compute_results(
        vector<EvaluationContext> &eval_contexts) {
    vector<EvaluationResult> results;
    results.reserve(eval_contexts.size());

    vector<State> eval_states;
    vector<int> old_heuristics;
    old_heuristics.reserve(eval_contexts.size());

    for (size_t idx_ec = 0; idx_ec < eval_contexts.size(); ++idx_ec) {
        const State &state = eval_contexts[idx_ec].get_state();
        bool calculate_preferred = eval_contexts[idx_ec].get_calculate_preferred();

        if (!calculate_preferred && cache_evaluator_values &&
            heuristic_cache[state].h != NO_VALUE && !heuristic_cache[state].dirty) {
            old_heuristics.push_back(heuristic_cache[state].h);
        } else if (!calculate_preferred && task_properties::is_goal_state(task_proxy, state)) {
            old_heuristics.push_back(0);
            if (cache_evaluator_values) {
                heuristic_cache[state] = HEntry(0, false);
            }
        } else {
            old_heuristics.push_back(NO_VALUE);
            eval_states.push_back(state);
        }
    }

    network->evaluate(eval_states);
    size_t idx_evaluated_states = 0;

    for (size_t idx_ec = 0; idx_ec < eval_contexts.size(); ++idx_ec) {
        EvaluationResult er;
        int heuristic;
        if (old_heuristics[idx_ec] == NO_VALUE) {
            heuristic = network->get_heuristics()[idx_evaluated_states];
            if (cache_evaluator_values) {
                heuristic_cache[eval_contexts[idx_ec].get_state()] =
                        HEntry(heuristic, false);
            }

            if (network->is_preferred() && heuristic != DEAD_END) {

                er.set_preferred_operators(
                   network->get_preferreds()[idx_evaluated_states].
                   pop_as_vector());
            }
            er.set_count_evaluation(true);
            idx_evaluated_states++;
        } else {
            heuristic = old_heuristics[idx_ec];
            er.set_count_evaluation(false);
        }

        assert(heuristic == DEAD_END || heuristic >= 0);
        if (heuristic == DEAD_END) {
            heuristic = EvaluationResult::INFTY;
        }
        er.set_evaluator_value(heuristic);
        results.push_back(move(er));
    }
    assert(idx_evaluated_states == eval_states.size());
    return results;
}



static shared_ptr<Heuristic> _parse(OptionParser &parser) {
    parser.document_synopsis("network heuristic", "");
    parser.document_language_support("action costs", "supported");
    parser.document_language_support("conditional effects", "supported");
    parser.document_language_support("axioms", "supported");
    parser.document_property("admissible", "no");
    parser.document_property("consistent", "no");
    parser.document_property("safe", "no");
    parser.document_property("preferred operators",
                             "maybe (depends on network)");


    parser.add_option<shared_ptr<neural_networks::AbstractNetwork>>(
        "network", "Network for state evaluations.");
    Heuristic::add_options_to_parser(parser);
    Options opts = parser.parse();
    if (parser.dry_run())
        return 0;
    else
        return make_shared<NetworkHeuristic>(opts);
}

static Plugin<Evaluator> _plugin("nh", _parse);
static Plugin<Heuristic> _plugin2("hnh", _parse);
}
