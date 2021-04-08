#include "state_goal_network.h"

#include "../option_parser.h"
#include "../plugin.h"

#include <algorithm>
#include <memory>

using namespace std;
using namespace tensorflow;
namespace neural_networks {

vector<int> get_goal_defaults(
        const TaskProxy &task_proxy,
        const vector<int> &domain_sizes,
        const vector<FactMapping> &fact_mappings) {

    vector<int> goal_idx;
    if (fact_mappings.empty()) {
        vector<int> shift;
        shift.reserve(domain_sizes.size() + 1);
        shift.push_back(0);
        for (int domain_size : domain_sizes) {
            shift.push_back(shift.back() + domain_size);
        }

        for (const FactProxy &fp: task_proxy.get_goals()) {
            goal_idx.push_back(
                    shift[fp.get_variable().get_id()] + fp.get_value());
        }

    } else {
        unordered_map<int, unordered_set<int>> goals;
        for (const FactProxy &fp: task_proxy.get_goals()) {
            FactPair fact = fp.get_pair();
            goals[fact.var].insert(fact.value);
        }

        for (const FactMapping &single_mapping: fact_mappings) {
            if (goals.find(single_mapping.var) != goals.end() &&
                goals[single_mapping.var].find(single_mapping.value)
                != goals[single_mapping.var].end()) {
                goal_idx.push_back(single_mapping.index);
            }
        }
    }
    return goal_idx;
}


StateGoalNetwork::StateGoalNetwork(const Options &opts)
    : StateNetwork(opts),
      _goal_layer_name(opts.get<string>("goal_layer")),
      goal_defaults(get_goal_defaults(
              task_proxy, domain_sizes, fact_mappings)) {
}

StateGoalNetwork::~StateGoalNetwork() {
}

Tensor StateGoalNetwork::get_goal_input_tensor(int input_size) const {
    Tensor goal_tensor(
            DT_FLOAT,
     TensorShape({batch_size, input_size}));

    // Fill goal tensor with default values
    auto goal_tensor_buffer = goal_tensor.matrix<float>();
    assert(goal_tensor_buffer.dimensions().size() == 2);
    assert(goal_tensor_buffer.dimension(0) == batch_size);
    for (int batch_idx = 0; batch_idx < batch_size; ++batch_idx) {
        for (int idx = 0; idx < input_size; ++idx) {
            goal_tensor_buffer(batch_idx, idx) = 0;
        }
        for (int idx : goal_defaults) {
            goal_tensor_buffer(batch_idx, idx) = 1;
        }
    }
    return goal_tensor;
}

void StateGoalNetwork::initialize_inputs() {
    int input_size = get_state_input_size();
    Tensor goal_tensor(DT_FLOAT, TensorShape({1, input_size}));
    inputs = {
        {_state_layer_name, get_state_input_tensor(input_size)},
        {_goal_layer_name, get_goal_input_tensor(input_size)}
    };
}
}

static shared_ptr<neural_networks::AbstractNetwork> _parse(OptionParser &parser) {
    neural_networks::ProtobufNetwork::add_options_to_parser(parser);
    neural_networks::StateNetwork::add_state_network_options_to_parser(parser);

    parser.add_option<string>("goal_layer", "Name of the input layer in"
                              "the computation graph to insert the current goal.");
    
    Options opts = parser.parse();
    shared_ptr<neural_networks::StateGoalNetwork> network;
    if (!parser.dry_run()) {
        network = make_shared<neural_networks::StateGoalNetwork>(opts);
    }

    return network;
}

static Plugin<neural_networks::AbstractNetwork> _plugin("sgnet", _parse);
