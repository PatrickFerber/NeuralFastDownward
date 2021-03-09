#include "sampling_v.h"

#include "../evaluation_context.h"
#include "../heuristic.h"
#include "../plugin.h"

#include "../algorithms/ordered_set.h"
#include "../task_utils/successor_generator.h"
#include "../task_utils/task_properties.h"
//#include "../task_utils/assignment_cost_generator.h"

#include <cassert>
#include <cstdlib>
#include <iostream>
#include <set>
#include <memory>
#include <string>

using namespace std;

namespace sampling_engine {

StateTree::StateTree(const StateID state_id, const int parent, const int action_cost, double value)
: state_id(state_id),
parent(parent),
action_cost(action_cost),
value(value) { }


/* Methods to use in the constructor */


options::ParseTree prepare_evaluator_parse_tree(
    const std::string &unparsed_config) {
    options::ParseTree pt = options::generate_parse_tree(unparsed_config);
    return subtree(pt, options::first_child_of_root(pt));
}


/* Constructor */
SamplingV::SamplingV(const Options &opts)
    : SamplingStateEngine(opts),
      evaluator_parse_tree(prepare_evaluator_parse_tree(opts.get_unparsed_config())),
      registry(*opts.get_registry()),
      predefinitions(*opts.get_predefinitions()),
      lookahead(opts.get<int>("lookahead")),
//      expand_goal(opts.get<int>("expand_goal")),
      evaluator_reload_frequency(opts.get<int>("evaluator_reload_frequency")),
      task_reload_frequency(opts.get<int>("task_reload_frequency")),
      evaluator_reload_counter(0),
      task_reload_counter(0),
      add_goal_to_output(opts.get<bool>("add_goal")),
      qevaluator(nullptr),
      q_task_proxy(nullptr),
      q_state_registry(nullptr)
//      expanded_goals(nullptr),
//      expand_goals_cost_limit(opts.get<int>("expand_goal")),
//      expand_goal_state_limit(opts.get<int>("expand_goal_state_limit")),
//      reload_expanded_goals(opts.get<bool>("reload_expanded_goals"))
      {
    if (sample_format != SampleFormat::CSV) {
        cerr << "Invalid sample format for q_sampling" << endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
    if (state_format != StateFormat::FDR) {
        cerr << "invalid state format for q_sampling" << endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
//    if (expand_goal < 0) {
//        cerr << "expand_goal has to be 0 or positive" << endl;
//        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
//    }
}

void SamplingV::initialize() {
    SamplingStateEngine::initialize();
    cout << "Initializing Sampling Q...";
    cout << "done." << endl;
}

void SamplingV::reload_evaluator(shared_ptr<AbstractTask> task) {
    sampling_technique::modified_task = task;
    registry.handle_repredefinition("evaluator", predefinitions);
    registry.handle_repredefinition("heuristic", predefinitions);

    OptionParser evaluator_parser(
        evaluator_parse_tree, registry, predefinitions, false);
    qevaluator = evaluator_parser.start_parsing<shared_ptr < Evaluator >> ();
}

void SamplingV::reload_task(shared_ptr<AbstractTask> task) {
    q_task = task;
    q_task_proxy = make_shared<TaskProxy>(*q_task);
    q_state_registry = make_shared<StateRegistry>(*q_task_proxy);
    assert(task_properties::is_unit_cost(*q_task_proxy));
    ostringstream oss;
    convert_and_push_goal(oss, *q_task);
    q_task_goal = oss.str();

//    if (expand_goals_cost_limit > 0 && (expanded_goals == nullptr || reload_expanded_goals)){
//        cout << "Expand Goal..." << flush;
//        RegressionTaskProxy rtp(*task);
//        sampling::RandomRegressionWalkSampler rrws(rtp, *rng);
//        vector<int> goal_values(
//                task->get_num_variables(), PartialAssignment::UNASSIGNED);
//        for (int i = 0; i < task->get_num_goals(); ++i){
//            FactPair goal_fact = task->get_goal_fact(i);
//            goal_values[goal_fact.var] = goal_fact.value;
//        }
//        PartialAssignment goal_assignment(*task, move(goal_values));
//        auto registry_costs = rrws.sample_area(
//                goal_assignment, expand_goals_cost_limit,
//                expand_goal_state_limit, true);
//        expanded_goals = make_shared<assignment_cost_generator::AssignmentCostGenerator>(
//                *q_task_proxy, registry_costs.first,registry_costs.second);
//        cout << "Done." << endl;
//    }
}

std::pair<int, std::vector<StateTree> > SamplingV::construct_state_tree(
    const State& state) {
    vector<StateTree> states;
    states.emplace_back(state.get_id(), -1, 0, -1.0);
    
    successor_generator::SuccessorGenerator &successor_generator = 
        successor_generator::g_successor_generators[*q_task_proxy];
    
    size_t final_layer = 0;
    size_t pre_final_layer = -1;
    size_t i = 0;
    int depth = lookahead - 1;
    
    while (i < states.size() && depth >= 0) {
        if (states[i].value == -1) {
            // add all children
            vector<OperatorID> applicable_ops;
            const State curr_state = q_state_registry->
                    lookup_state(states[i].state_id);
            successor_generator.generate_applicable_ops(
                    curr_state,
                    applicable_ops);
            if (applicable_ops.empty()) {
                states.emplace_back(states[i].state_id, i, 0, -1);
            } else {
                for (OperatorID op_id : applicable_ops) {
                    OperatorProxy op = q_task_proxy->get_operators()[op_id];
                    const State succ_state =
                        q_state_registry->get_successor_state(
                                curr_state, op);

                    states.emplace_back(
                        succ_state.get_id(), i, op.get_cost(), -1);
                    if (task_properties::is_goal_state(
                        *q_task_proxy, succ_state)) {
                        states.back().value = 0.0;
//                    } else if (expanded_goals != nullptr){
//                        int c = expanded_goals->lookup_cost(succ_state.unpack());
//                        if (c != -1) {
//                            states.back().value = c;
//                        }
                    }

                }
            }
        }
        // update final layer marker
        if (i == final_layer) {
            pre_final_layer = final_layer;
            final_layer = states.size() - 1;
            --depth;
        }
        ++i;
    }
    return {pre_final_layer + 1, states};
 }

double SamplingV::evaluate_q_value(
        const State &state) {
    state.unpack();
    if (task_properties::is_goal_state(*q_task_proxy, state)) {
        return 0.0;
    }
//    if (expanded_goals != nullptr) {
//        int c = expanded_goals->lookup_cost(state);
//        if (c > -1) {
//            return c;
//        }
//    }
    
    pair<int, vector<StateTree>> state_tree = 
        construct_state_tree(state);

    vector<EvaluationContext> eval_contexts;
    for (size_t i = state_tree.first; i < state_tree.second.size(); ++i){
        if (state_tree.second[i].value == -1) {
            eval_contexts.emplace_back(q_state_registry->lookup_state(
                    state_tree.second[i].state_id));
        }
    }
    vector<EvaluationResult> eval_results =
        qevaluator->compute_results(eval_contexts);
    size_t idx_eval_results = 0;
    for (size_t i = state_tree.first; i < state_tree.second.size(); ++i) {
        if (state_tree.second[i].value == -1) {
            state_tree.second[i].value = eval_results[idx_eval_results++].
                    get_evaluator_value();
        }
    }
    assert (idx_eval_results == eval_results.size());

    // evaluate all states in final layer
    for (auto node = state_tree.second.rbegin();
        node != state_tree.second.rend(); ++node ) {
        assert(node->value != -1);
        
        if (node->parent == -1) {
            return node->value;
        }
        
        StateTree &parent = state_tree.second[node->parent];
        if (parent.value == -1) {
            parent.value = node->value + node->action_cost;
        } else {
            parent.value = std::min(
                parent.value,
                node->value + node->action_cost
                );
        }
    }
    utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
}


string SamplingV::convert_output(
    const State& state,
    double q_value) {
    ostringstream sout;
    sout << q_value << field_separator;
    convert_and_push_state(sout, state);
    if (add_goal_to_output) {
        sout << field_separator << q_task_goal;
    }
    return sout.str();
 }


vector<string> SamplingV::sample(shared_ptr<AbstractTask> task) {
    if (qevaluator == nullptr ||
        (evaluator_reload_frequency != -1 && 
         evaluator_reload_counter >= evaluator_reload_frequency)) {
        reload_evaluator(task);
        evaluator_reload_counter = 0;
    }
    if (q_task == nullptr ||
        (task_reload_frequency != -1 && 
         task_reload_counter >= task_reload_frequency)) {
        reload_task(task);
        task_reload_counter = 0;
    }


    const State state = q_state_registry->insert_state(
            task->get_initial_state_values());
    evaluator_reload_counter++;
    task_reload_counter++;

    return {convert_output(state, evaluate_q_value(state))};
}


void SamplingV::add_sampling_v_options(OptionParser &parser) {
    parser.add_option<shared_ptr < Evaluator >> (
        "vevaluator", "evaluator to use to estimate the v values");
    parser.add_option<int> (
        "lookahead",
        "look ahead depth for the v value calculation",
        "1"
    );
    parser.add_option<int> (
        "evaluator_reload_frequency",
        "Specify how often the evaluator will be reloaded. -1 = never, "
        "0 = always, N > 0 = every N sample calls. ATTENTIONS: The evaluator "
        "takes during initialization a task pointer and uses this task for "
        "its evaluations. Evaluating states from a different task (different "
        "action costs/goal/actions/...) does not necessarily return the "
        "expected values. Reduce reloading frequency only if you know that it "
        "is not harmful (e.g. only the initial state changes and the evaluator "
        "is fine with that).",
        "0");
    parser.add_option<int> (
        "task_reload_frequency",
        "Specify how often the task (state_registry,...) will be reloaded. "
        "-1 = never, "
        "0 = always, N > 0 = every N sample calls. ATTENTIONS: Not reloading "
        "when the goal/actions/... change can lead to wrong results."
        "(currently only goal and actions are relevant, but this can change).",
        "0");
    parser.add_option<bool>(
        "add_goal",
        "If true (default), the goal will also be stored",
        "true"
        );
//    parser.add_option<int>(
//            "expand_goal",
//            "performs an max x cost breath-first search around the goal. States"
//            "which within the lookahead which are in the breath-first search get"
//            "their h* value and are not evaluated by the network. Use -1 to"
//            "deactivate this (and no genius, do not use 0 which is equal to"
//            "not eqpanding anything, but might occure overhead (if I am to "
//            "lazy to check this))",
//            "0"
//            );
//    parser.add_option<int>(
//            "expand_goal_state_limit",
//            "limits how many states are searched for the goal expansions.",
//            "-1"
//            );
//    parser.add_option<bool>(
//            "reload_expanded_goals",
//            "reloads the expanded goal when the task is reloaded. By default"
//            "this is active. Depending on your task transformations you can"
//            "and should deactivate this, because the expansion is expensive.",
//            "true"
//            );
}
}
