#ifndef NEURAL_NETWORKS_TEST_TORCH_NETWORK_H
#define NEURAL_NETWORKS_TEST_TORCH_NETWORK_H

#include "torch_network.h"

#include "../heuristic.h"
#include "../option_parser.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"

namespace neural_networks {

class TestTorchNetwork : public TorchNetwork {
protected:
    int last_h = Heuristic::NO_VALUE;
    std::vector<int> last_h_batch;


    virtual std::vector<at::Tensor> get_input_tensors(const State &state) override;
    virtual void parse_output(const torch::jit::IValue &output) override;
    virtual void clear_output() override;
public:
    explicit TestTorchNetwork(const Options &opts);
    TestTorchNetwork(const TestTorchNetwork &orig) = delete;
    virtual ~TestTorchNetwork() override;

    virtual bool is_heuristic() override;
    virtual int get_heuristic() override;
    virtual const std::vector<int> &get_heuristics() override;
};
}
#endif /* NEURAL_NETWORKS_TEST_TORCH_NETWORK_H */
