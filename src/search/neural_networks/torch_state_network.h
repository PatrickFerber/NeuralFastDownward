#ifndef NEURAL_NETWORKS_TORCH_STATE_NETWORK_H
#define NEURAL_NETWORKS_TORCH_STATE_NETWORK_H

#include "torch_network.h"

#include "../heuristic.h"

namespace neural_networks {

class TorchStateNetwork : public TorchNetwork {
protected:
    const int heuristic_shift;
    const int heuristic_multiplier;
    const std::vector<FactPair> relevant_facts;
    const std::vector<int> default_input_values;

    int last_h = Heuristic::NO_VALUE;
    std::vector<int> last_h_batch;


    virtual std::vector<at::Tensor> get_input_tensors(const State &state) override;
    virtual void parse_output(const torch::jit::IValue &output) override;
    virtual void clear_output() override;
public:
    explicit TorchStateNetwork(const Options &opts);
    TorchStateNetwork(const TorchStateNetwork &orig) = delete;
    virtual ~TorchStateNetwork() override;

    virtual bool is_heuristic() override;
    virtual int get_heuristic() override;
    virtual const std::vector<int> &get_heuristics() override;
};
}
#endif /* NEURAL_NETWORKS_TEST_TORCH_NETWORK_H */
