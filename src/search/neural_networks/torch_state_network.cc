#include "torch_state_network.h"

#include "abstract_network.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../task_utils/task_properties.h"

#include <algorithm>
#include <fstream>
#include <memory>
#include <string>

using namespace std;
namespace neural_networks {

TorchStateNetwork::TorchStateNetwork(const Options &opts)
    : TorchNetwork(opts),
      heuristic_shift(opts.get<int>("shift")),
      heuristic_multiplier(opts.get<int>("multiplier")),
      relevant_facts(get_fact_mapping(task.get(), opts.get_list<string>("facts"))),
      default_input_values(get_default_inputs(opts.get_list<string>("defaults"))) {
    check_facts_and_default_inputs(relevant_facts, default_input_values);
}

TorchStateNetwork::~TorchStateNetwork() {}


bool TorchStateNetwork::is_heuristic() {
    return true;
}

int TorchStateNetwork::get_heuristic() {
    return last_h;
}

const vector<int> &TorchStateNetwork::get_heuristics() {
    return last_h_batch;
}

int tmp = 0;
vector<at::Tensor> TorchStateNetwork::get_input_tensors(const State &state) {
    state.unpack();
    const vector<int> &values = state.get_values();

    at::Tensor tensor = torch::ones({1, static_cast<long>(relevant_facts.size())});
    auto accessor = tensor.accessor<float, 2>();
    size_t idx = 0;
    for (const FactPair &fp: relevant_facts) {
        if (fp == FactPair::no_fact) {
            accessor[0][idx] = default_input_values[idx];
        } else {
            accessor[0][idx] = values[fp.var] == fp.value;
        }
        idx++;
    }
    return {tensor};
}

void TorchStateNetwork::parse_output(const torch::jit::IValue &output) {
    at::Tensor tensor = output.toTensor();
    auto accessor = tensor.accessor<float, 2>();
    for (int64_t i = 0; i < tensor.size(0); ++i){
        last_h = (accessor[i][0]+heuristic_shift) * heuristic_multiplier;
        last_h_batch.push_back(last_h);
    }
}

void TorchStateNetwork::clear_output() {
    last_h = Heuristic::NO_VALUE;
    last_h_batch.clear();
}
}

static shared_ptr<neural_networks::AbstractNetwork> _parse(OptionParser &parser) {
    parser.document_synopsis(
        "Torch State Network",
        "Takes a trained PyTorch model and evaluates it on a given state."
        "The output is read as and provided as a heuristic.");
    neural_networks::TorchNetwork::add_options_to_parser(parser);
    parser.add_option<int>(
            "shift",
            "shift the predicted heuristic value (useful, if the model"
            "output is expected to be negative up to a certain bound.", "0");
    parser.add_option<int>(
            "multiplier",
            "Multiply the predicted (and shifted) heuristic value (useful, if "
            "the model predicts small float values, but heuristics have to be "
            "integers", "1");
    parser.add_list_option<string>(
            "facts",
            "if the SAS facts after translation can differ from the facts"
            "during training (e.g. some are pruned or their order changed),"
            "provide here the order of facts during training.",
            "[]");
    parser.add_list_option<string>(
            "defaults",
            "Default values for the facts given in option 'facts'",
            "[]");
    Options opts = parser.parse();

    shared_ptr<neural_networks::TorchStateNetwork> network;
    if (!parser.dry_run()) {
        network = make_shared<neural_networks::TorchStateNetwork>(opts);
    }

    return network;
}

static Plugin<neural_networks::AbstractNetwork> _plugin("torch_state_network", _parse);
