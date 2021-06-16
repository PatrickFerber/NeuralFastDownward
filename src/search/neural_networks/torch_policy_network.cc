//
// Created by dh on 10.06.21.
//

#include "torch_policy_network.h"
#include "../task_utils/task_properties.h"
#include "../option_parser.h"
#include "../plugin.h"

using namespace std;
namespace neural_networks {
    TorchPolicyNetwork::TorchPolicyNetwork(const Options &opts) : TorchNetwork(opts),
            relevant_facts(task_properties::get_strips_fact_pairs(task.get())) {
        // The network is expected to output a preference for EVERY grounded
        // operator, thus, the list of preferred operators is always the same.
        for (auto o : task_proxy.get_operators()) {
            OperatorID id = OperatorID(o.get_id());
            this->last_preferred.insert(id);
        }
    }

    std::vector<at::Tensor> TorchPolicyNetwork::get_input_tensors(const State &state) {
        state.unpack();
        const vector<int> &values = state.get_values();

        at::Tensor tensor = torch::ones({1, static_cast<long>(relevant_facts.size())});
        auto accessor = tensor.accessor<float, 2>();
        size_t idx = 0;
        for (const FactPair &fp: relevant_facts) {
            accessor[0][idx++] = values[fp.var] == fp.value;
        }
        return {tensor};
    }

    void TorchPolicyNetwork::parse_output(const torch::IValue &output) {
        at::Tensor tensor = output.toTensor();
        auto accessor = tensor.accessor<float, 2>();
        for(int j = 0; j < accessor.size(1); j++) {
            last_preferences.push_back(accessor[0][j]);
        }
        assert(last_preferences.size() == (size_t) last_preferred.size());
    }

    void TorchPolicyNetwork::clear_output() {
        last_preferences.clear();
    }

    bool TorchPolicyNetwork::is_preferred() {
        return true;
    }

    ordered_set::OrderedSet<OperatorID> &TorchPolicyNetwork::get_preferred() {
        return last_preferred;
    }

    std::vector<float> &TorchPolicyNetwork::get_operator_preferences() {
        return last_preferences;
    }
}

static shared_ptr<neural_networks::AbstractNetwork> _parse(OptionParser &parser) {
    parser.document_synopsis(
            "Torch Policy Network",
            "Takes a trained PyTorch model and evaluates it on a given state."
            "The output is interpreted as action preferences.");
    neural_networks::TorchNetwork::add_options_to_parser(parser);
    Options opts = parser.parse();

    shared_ptr<neural_networks::TorchPolicyNetwork> network;
    if (!parser.dry_run()) {
        network = make_shared<neural_networks::TorchPolicyNetwork>(opts);
    }

    return network;
}

static Plugin<neural_networks::AbstractNetwork> _plugin("torch_policy_network", _parse);
