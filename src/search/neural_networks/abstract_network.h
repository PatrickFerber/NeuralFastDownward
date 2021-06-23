#ifndef NEURAL_NETWORKS_ABSTRACT_NETWORK_H
#define NEURAL_NETWORKS_ABSTRACT_NETWORK_H

#include "../task_proxy.h"

#include "../algorithms/ordered_set.h"
#include "../utils/system.h"


namespace neural_networks {
enum OutputType {
    /*Different types of basic output a network can produce
     (not how we later interpret the output*/
    Classification, Regression
};

/*Get for a given string the output type it is associated with*/
extern OutputType get_output_type(const std::string &type);

/**
 * Base class for all networks.
 * Add new is_PROPERTY, verify_PROPERTY, get_PROPERTY if adding new properties
 * a network can produce as output (those are interpreted outputs types like
 * heuristic values or preferred operators)
 */
class AbstractNetwork {
public:
    AbstractNetwork() = default;
    AbstractNetwork(const AbstractNetwork &orig) = delete;
    virtual ~AbstractNetwork() = default;

    /**
     * Initialize the network. Call this function after the constructor and
     * prior to the first usage.
     */
    virtual void initialize() {}
    /**
     * Evaluate the given state. To obtain the evaluation results use the
     * get* methods.
     * @param state state object to evaluate in the network
     */
    virtual void evaluate(const State &state) = 0;
    /**
     * Evaluate the given vector of states.  To obtain the evaluation results
     * use the get* methods.
     * @param states
     */
    virtual void evaluate(const std::vector<State> &states) = 0;


    virtual bool is_heuristic();
    void verify_heuristic();
    virtual int get_heuristic();
    virtual const std::vector<int> &get_heuristics();

    virtual bool is_heuristic_confidence();
    void verify_heuristic_confidence();
    virtual double get_heuristic_confidence();
    virtual const std::vector<double> &get_heuristic_confidences();

    virtual bool is_preferred();
    void verify_preferred();
    virtual ordered_set::OrderedSet<OperatorID> &get_preferred();
    virtual std::vector<ordered_set::OrderedSet<OperatorID>> &get_preferreds();
    virtual std::vector<float> &get_operator_preferences();
};


std::vector<FactPair> get_fact_mapping(
        const AbstractTask *task, const std::vector<std::string> &facts);
std::vector<int> get_default_inputs(
        const std::vector<std::string> &default_inputs);
void check_facts_and_default_inputs(
        const std::vector<FactPair> &relevant_facts,
        const std::vector<int> &default_input_values);
}

#endif /* NEURAL_NETWORKS_NEURAL_NETWORK_H */
