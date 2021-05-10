#include "torch_network.h"

#include <algorithm>

using namespace std;
namespace neural_networks {
TorchNetwork::TorchNetwork(const Options &opts)
    : AbstractNetwork(),
      task(opts.get<shared_ptr<AbstractTask>>("transform")),
      task_proxy(*task),
      path(opts.get<string>("path")) {}

TorchNetwork::~TorchNetwork() {}

void TorchNetwork::initialize() {
    if (!is_initialized) {
        AbstractNetwork::initialize();
        module = torch::jit::load(path);
        is_initialized = true;
    }
}

void TorchNetwork::evaluate(const State &state) {
    clear_output();
    vector<torch::jit::IValue> inputs;
    auto sample = get_input_tensors(state);
    inputs.insert(inputs.end(), sample.begin(), sample.end());
    parse_output(module.forward(inputs));
}

void TorchNetwork::evaluate(const vector<State> &states) {
    cerr << "Evaluating batches of samples is currently not working for Torch."
         << endl;
    utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);

    clear_output();
    vector<vector<at::Tensor>> samples;
    for (auto &state : states) {
        vector<at::Tensor> sample = get_input_tensors(state);
        if (samples.empty()) {
            for (size_t i = 0; i < sample.size(); ++i) {
                samples.push_back(vector<at::Tensor>(state.size()));
            }
        }
        for (size_t idx = 0; idx < sample.size(); ++idx){
            samples[idx].push_back(sample[idx]);
        }
    }
    vector<torch::jit::IValue> inputs;
    for (const vector<at::Tensor> &input: samples) {
        inputs.push_back(torch::cat(input));
    }
    parse_output(module.forward(inputs));
}

void TorchNetwork::add_options_to_parser(options::OptionParser &parser) {
    parser.add_option<string>("path", "Path to networks protobuf file.");
    parser.add_option<shared_ptr<AbstractTask>>(
        "transform",
        "Optional task transformation for the network."
        " Currently, adapt_costs(), sampling_transform(), and no_transform() are "
        "available.",
        "no_transform()");
}
}
