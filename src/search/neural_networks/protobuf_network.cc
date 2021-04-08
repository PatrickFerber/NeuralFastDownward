#include "protobuf_network.h"

#include <algorithm>

using namespace std;
using namespace tensorflow;
namespace neural_networks {
ProtobufNetwork::ProtobufNetwork(const Options &opts)
    : AbstractNetwork(),
      task(opts.get<shared_ptr<AbstractTask>>("transform")),
      task_proxy(*task),
      path(opts.get<string>("path")),
      session(nullptr),
      batch_size(opts.get<int>("batch_size")){
}

ProtobufNetwork::~ProtobufNetwork() {
    Status status = session->Close();
    if (!status.ok()) {
        std::cerr << "Session close error: "
                  << status.ToString() << "\n";
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
}

void ProtobufNetwork::initialize() {
    if (!is_initialized) {
        AbstractNetwork::initialize();
        // Initialize a tensorflow session
        Status status = NewSession(SessionOptions(), &session);
        if (!status.ok()) {
            std::cerr << "Tensorflow session error: "
                      << status.ToString() << "\n";
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
        }

        GraphDef graph_def;
        status = ReadBinaryProto(Env::Default(), path, &graph_def);
        if (!status.ok()) {
            std::cerr << "Protobuf loading error: "
                      << status.ToString() << "\n";
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
        }

        // Add the graph to the session
        status = session->Create(graph_def);
        if (!status.ok()) {
            std::cerr << "Session graph creation error: "
                      << status.ToString() << "\n";
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
        }
        initialize_inputs();
        initialize_output_layers();
        is_initialized = true;
    }
}

void ProtobufNetwork::evaluate(const State &state) {
    clear_output();
    fill_input(state);

    Status status = session->Run(inputs, output_layers, {}, &outputs);
    if (!status.ok()) {
        std::cerr << "Network evaluation error: " << status.ToString() << "\n";
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }

    extract_output(1);
}

void ProtobufNetwork::evaluate(const vector<State> &states) {
    clear_output();
    for (size_t batch_round = 0; batch_round < states.size();
            batch_round += batch_size) {
        int batch_round_size = min(
                batch_size, (int) (states.size() - batch_round));
        for (int batch_idx = 0; batch_idx < batch_round_size; ++batch_idx) {
            fill_input(states[batch_round + batch_idx], batch_idx);
        }

        Status status = session->Run(inputs, output_layers, {}, &outputs);

        if (!status.ok()) {
            std::cerr << "Network evaluation error: " << status.ToString() << "\n";
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
        }

        extract_output(batch_round_size);
    }
}

void ProtobufNetwork::add_options_to_parser(options::OptionParser &parser) {
    parser.add_option<shared_ptr<AbstractTask>>(
        "transform",
        "Optional task transformation for the network."
        " Currently, adapt_costs(), sampling_transform(), and no_transform() are "
        "available.",
        "no_transform()");
    parser.add_option<int>(
            "batch_size",
            "Size of the tensor to evaluate states in a batch. By default,"
            "this causes all evaluation to run in a batch of the given size. "
            "Thus, evaluating  not full batches has some overhead",
            "1"
    );
    parser.add_option<string>("path", "Path to networks protobuf file.");
}
}
