#ifndef POLICIES_NETWORK_POLICY_H
#define POLICIES_NETWORK_POLICY_H

#include "../policy.h"

#include <memory>

namespace neural_networks {
class AbstractNetwork;
}

namespace network_policy {
class NetworkPolicy : public Policy {
protected:
    std::shared_ptr<neural_networks::AbstractNetwork> network;

    virtual PolicyResult compute_policy(const State &state) override;
public:
    explicit NetworkPolicy(const options::Options &options);
    ~NetworkPolicy();
    virtual bool dead_ends_are_reliable() const override;
};
}

#endif
