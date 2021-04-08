#include "test_torch_network.h"

#include "abstract_network.h"

#include "../option_parser.h"
#include "../plugin.h"

#include <algorithm>
#include <fstream>
#include <memory>
#include <string>

using namespace std;
namespace neural_networks {

TestTorchNetwork::TestTorchNetwork(const Options &opts)
    : TorchNetwork(opts) {}

TestTorchNetwork::~TestTorchNetwork() {}


bool TestTorchNetwork::is_heuristic() {
    return true;
}

int TestTorchNetwork::get_heuristic() {
    return last_h;
}

const vector<int> &TestTorchNetwork::get_heuristics() {
    return last_h_batch;
}

int tmp = 0;
vector<at::Tensor> TestTorchNetwork::get_input_tensors(const State &/*state*/) {
    at::Tensor tensor = torch::ones({1, 3});
    auto accessor = tensor.accessor<float, 2>();
    accessor[0][0] = 0;
    accessor[0][1] = 0;
    accessor[0][2] = 0;

    if (tmp < 3) {
        accessor[0][tmp] = 1;
    } else {
        accessor[0][0] = 1;
        accessor[0][1] = 1;
        accessor[0][2] = 1;
    }
    tmp += 1;
    return {tensor};
}

void TestTorchNetwork::parse_output(const torch::jit::IValue &output) {
    at::Tensor tensor = output.toTensor();
    auto accessor = tensor.accessor<float, 1>();
    for (int64_t i = 0; i < tensor.size(0); ++i){
        last_h = accessor[i];
        last_h_batch.push_back(last_h);
        cout << "TestTorch Output: " << accessor[i] << endl;
    }
}

void TestTorchNetwork::clear_output() {
    last_h = Heuristic::NO_VALUE;
    last_h_batch.clear();
}
}

static shared_ptr<neural_networks::AbstractNetwork> _parse(OptionParser &parser) {
    neural_networks::TorchNetwork::add_options_to_parser(parser);
    Options opts = parser.parse();

    shared_ptr<neural_networks::TestTorchNetwork> network;
    if (!parser.dry_run()) {
        network = make_shared<neural_networks::TestTorchNetwork>(opts);
    }

    return network;
}

static Plugin<neural_networks::AbstractNetwork> _plugin("ttest", _parse);
