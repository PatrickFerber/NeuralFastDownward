#ifndef POLICY_H
#define POLICY_H

#include "per_state_information.h"
#include "task_proxy.h"

#include <vector>

namespace options {
class OptionParser;
class Options;
}

class EvaluationContext;
class OperatorID;
class PolicyResult;
class StateRegistry;

class Policy {
    /*
        Entries for Policy results including
        dirty: true if vectors/ values are not set yet 
        operator_ids: vector of IDs for operators the policy considers
        operator_preferences: vector of operator preferences (= probabilities) for the same operators
            with index matching the operator IDs of the previous vector
    */
    struct PEntry {
        bool dirty;
        std::vector<OperatorID> operator_ids;
        std::vector<float> operator_preferences;

        // non-dirty constructor with ids and preferences
        PEntry(std::vector<OperatorID> operator_ids, std::vector<float> operator_preferences)
            : dirty(false), operator_ids(operator_ids), operator_preferences(operator_preferences) {
        }

        /*
            non-dirty constructor only using ids (e.g. can be used when all operators should have same
            preference which is handled in Policy::compute_result)
        */
        PEntry(std::vector<OperatorID> operator_ids)
            : dirty(false), operator_ids(operator_ids), operator_preferences(std::vector<float>()) {
        }

        // dirty empty constructor
        PEntry() : dirty(true), operator_ids(std::vector<OperatorID>()), operator_preferences(std::vector<float>()) {
        }
    };

protected:
    /*
      Cache for saving policy results
      Before accessing this cache always make sure that the cache_policy_values
      flag is set to true - as soon as the cache is accessed it will create
      entries for all existing states
    */
    PerStateInformation<PEntry> policy_cache;
    bool cache_policy_values;

    // Hold a reference to the task implementation and pass it to objects that need it.
    const std::shared_ptr<AbstractTask> task;
    // Use task_proxy to access task information.
    TaskProxy task_proxy;
    /*
        registration name and flag for g_registered_policies map in globals
    */
//    const std::string register_name;
//    const bool registered;

protected:

    /*
        main function to implement for concrete policies returning the policy
        result as pair of operator ids and preferences for a given state
    */
    virtual PolicyResult compute_policy(const State &state) = 0;

    State convert_ancestor_state(const State &ancestor_state) const;

public:
    explicit Policy(const options::Options &options);
    virtual ~Policy();

    virtual bool dead_ends_are_reliable() const = 0;

    static void add_options_to_parser(options::OptionParser &parser);

    virtual PolicyResult compute_result(
        EvaluationContext &eval_context);
};

#endif
