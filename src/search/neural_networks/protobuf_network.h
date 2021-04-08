#ifndef NEURAL_NETWORKS_PROTOBUF_NETWORK_H
#define NEURAL_NETWORKS_PROTOBUF_NETWORK_H

#include "abstract_network.h"

#include "../option_parser.h"

#include "tensorflow/core/platform/env.h"
#include "tensorflow/core/public/session.h"

#include <vector>
#include <string>

namespace neural_networks {
/**Base class for all networks which use tensorflow and are in the protobuf
 * format.*/
class ProtobufNetwork :  public AbstractNetwork {
protected:
    const std::shared_ptr<AbstractTask> task;
    TaskProxy task_proxy;

    /** Path to the the stored network to use */
    const std::string path;
    /** Tensorflow session to use */
    tensorflow::Session *session;
    /** Number of how many states shall be evaluated together */
    const int batch_size;
    /** The network input to evaluate */
    std::vector<std::pair<std::string, tensorflow::Tensor>> inputs;
    /** List of names of the networks output nodes */
    std::vector<std::string> output_layers;
    /** Unprocessed network output */
    std::vector<tensorflow::Tensor> outputs;
    /** Flags if the network was already initialized */
    bool is_initialized = false;

    /**
     * Initialize the inputs variable which fed to the network.
     */
    virtual void initialize_inputs() = 0;
    /**
     * Initialize which outputs to take from the network.
     */
    virtual void initialize_output_layers() = 0;
    /**
     * Write the input features for the evaluation of a state into the
     * network input tensors.
     * @param state State to evaluate
     * @param batch_idx location in the input tensor
     */
    virtual void fill_input(const State &state, int batch_idx = 0) = 0;
    /**
     * Extract the raw output of the network and process it.
     * @param count Number of predictions of the network to extract (
     * by default all)
     */
    virtual void extract_output(long count = -1) = 0;
    /**
     * Clear the output left from potential previous network evaluations.
     */
    virtual void clear_output() = 0;

    virtual void initialize() override;
    virtual void evaluate(const State &state) override;
    virtual void evaluate(const std::vector<State> &states) override;

public:
    explicit ProtobufNetwork(const Options &opts);
    ProtobufNetwork(const ProtobufNetwork &orig) = delete;
    virtual ~ProtobufNetwork() override;

    static void add_options_to_parser(options::OptionParser &parser);
};
}
#endif /* NEURAL_NETWORKS_PROTOBUF_NETWORK_H */
