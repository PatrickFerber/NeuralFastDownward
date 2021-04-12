#ifndef POLICIES_PREF_OPS_POLICY_H
#define POLICIES_PREF_OPS_POLICY_H

#include "../policy.h"

#include <vector>
#include <memory>

namespace successor_generator {
    class SuccessorGenerator;
}

class Evaluator;
class FactProxy;
class OperatorProxy;
class PolicyResult;

namespace policies {

/*
    Policy which uses internally an heuristic which computes
    preferred operators which are then uniformally used as
    an policy
    Currently used additive heuristic! (set in constructor)
*/
class PrefOpsPolicy : public Policy {
    std::shared_ptr<Evaluator> evaluator;
protected:
    const successor_generator::SuccessorGenerator &successor_generator;

    virtual PolicyResult compute_policy(const State &state) override;
public:
    PrefOpsPolicy(const options::Options &options);
    ~PrefOpsPolicy();
    virtual bool dead_ends_are_reliable() const override;
};
}

#endif
