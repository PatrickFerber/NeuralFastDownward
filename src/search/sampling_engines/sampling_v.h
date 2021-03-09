#ifndef SEARCH_ENGINES_SAMPLING_V_H
#define SEARCH_ENGINES_SAMPLING_V_H

#include "sampling_state_engine.h"

#include "../options/predefinitions.h"
#include "../options/registries.h"

#include <functional>
#include <memory>
#include <ostream>
#include <sstream>
#include <tree.hh>
#include <vector>

class EvaluationContext;
class Evaluator;
class Heuristic;
class PruningMethod;

namespace options {
class Options;
struct ParseNode;
using ParseTree = tree<ParseNode>;
}

namespace sampling_engine {

// Optimize to use directly GlobalState and std::move
struct StateTree {
    const StateID state_id;
    const int parent;
    const int action_cost;
    double value;
    StateTree(
        const StateID &state_id,
        int parent,
        int action_cost,
        double value);
};

class SamplingV : public SamplingStateEngine {
protected:
    // Internal
    const options::ParseTree evaluator_parse_tree;
    options::Registry registry;
    options::Predefinitions predefinitions;

    const int lookahead;
//    const int expand_goal;

    const int evaluator_reload_frequency;
    const int task_reload_frequency;
    int evaluator_reload_counter;
    int task_reload_counter;

    /* Options for the output file */
    const bool add_goal_to_output;
    
    
    std::shared_ptr<Evaluator> qevaluator;
    std::shared_ptr<AbstractTask> q_task;
    std::shared_ptr<TaskProxy> q_task_proxy;
    std::shared_ptr<StateRegistry> q_state_registry;
//    std::shared_ptr<assignment_cost_generator::AssignmentCostGenerator> expanded_goals;
//    const int expand_goals_cost_limit;
//    const int expand_goal_state_limit;
//    const bool reload_expanded_goals;
    std::string q_task_goal;
    
    /* Internal Methods*/
    void reload_evaluator(std::shared_ptr<AbstractTask> task);
    void reload_task(std::shared_ptr<AbstractTask> task);
    //std::string convert_state_to_output();
    std::pair<int, std::vector<StateTree>> construct_state_tree(
        const State &state);
    double evaluate_q_value(const State &state);
    std::string convert_output(const State &state, double q_value);
    
    /* Overwritten Methods*/
    virtual void initialize() override;
    std::vector<std::string> sample(
        std::shared_ptr<AbstractTask> task) override;
    

public:
    explicit SamplingV(const options::Options &opts);
    virtual ~SamplingV() override = default;

    static void add_sampling_v_options(options::OptionParser &parser);
};
}
#endif
