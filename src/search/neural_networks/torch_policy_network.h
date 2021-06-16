#ifndef NEURAL_NETWORKS_TORCH_POLICY_NETWORK_H
#define NEURAL_NETWORKS_TORCH_POLICY_NETWORK_H

#include "torch_network.h"

namespace neural_networks {

    class TorchPolicyNetwork : public TorchNetwork {
        const std::vector<FactPair> relevant_facts;

        ordered_set::OrderedSet<OperatorID> last_preferred;
        std::vector<float> last_preferences;

        virtual std::vector<at::Tensor> get_input_tensors(const State &state) override;
        virtual void parse_output(const torch::jit::IValue &output) override;
        virtual void clear_output() override;
    public:
        explicit TorchPolicyNetwork(const Options &opts);
        TorchPolicyNetwork(const TorchPolicyNetwork &orig) = delete;
        virtual bool is_preferred() override;
        virtual ordered_set::OrderedSet<OperatorID> &get_preferred() override;
        virtual std::vector<float> &get_operator_preferences() override;
    };

}
#endif
