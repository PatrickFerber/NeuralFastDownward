#ifndef HEURISTICS_NETWORK_HEURISTIC_H
#define HEURISTICS_NETWORK_HEURISTIC_H

#include "../heuristic.h"

#include "../neural_networks/abstract_network.h"

#include <memory>

namespace network_heuristic {

class NetworkHeuristic : public Heuristic {
protected:
    std::shared_ptr<neural_networks::AbstractNetwork> network;

    virtual int compute_heuristic(const State &ancestor_state) override;
    std::pair<int, double> compute_heuristic_and_confidence(const State &state) override;


public:
    explicit NetworkHeuristic(const options::Options &options);
    ~NetworkHeuristic();
    
    virtual std::vector<EvaluationResult> compute_results(
        std::vector<EvaluationContext> &eval_contexts) override;
};
}

#endif
