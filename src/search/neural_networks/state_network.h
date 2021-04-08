#ifndef NEURAL_NETWORKS_STATE_NETWORK_H
#define NEURAL_NETWORKS_STATE_NETWORK_H

#include "protobuf_network.h"

#include "../heuristic.h"
#include "../option_parser.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"

namespace neural_networks {

struct FactMapping {
    int var;
    int value;
    int index;
    FactMapping(int var, int value, int index)
    : var(var), value(value), index(index) {}
};
/*A simple example network. This network takes a trained computation graph and
 loads it. At every state, it feeds the state into the network as sequence of
 0s and 1s and extract a heuristic estimate for the state.*/
class StateNetwork : public ProtobufNetwork {
protected:
    std::shared_ptr<utils::RandomNumberGenerator> rng;
    const bool domain_max_is_undefined;
    const std::vector<int> domain_sizes;
    const std::string _state_layer_name;
    const std::vector<std::string> _output_layer_names;

    /* List of all facts by name which shall be fed to the network. If not
    * given, aka empty(), then all facts of FastDownward are fed in their
    * variable/value order.
    */
    const std::vector<std::string> state_atoms;
    /*
     * If not given, then and atoms is specified, the defaults as 0 are assumed.
     * */
    const std::vector<int> state_defaults;
    /*
     * Allow/Disallow that the current task contains facts not present in the
     * state_atoms list (if state_atoms is given).
     */
    const bool allow_unused_atoms;
    /*Tells for all facts which are used as input of the network their index in
    the input tensor. Every tuple in the list has the three fields
    <VarID, Value, Idx in tensor>. Variables not used in the tensor have no
    tuples.
    The  tuples are sorted by their VarID and then their Value!
    state_atoms is empty, then this is empty too.
    */
    const std::vector<FactMapping> fact_mappings;

    const OutputType output_type;
    const double unary_threshold;
    const int bin_size;
    const bool exponentiate_heuristic;
    int last_h = Heuristic::NO_VALUE;
    double last_h_confidence = Heuristic::DEAD_END;
    std::vector<int> last_h_batch;
    std::vector<double> last_h_confidence_batch;

    int get_state_input_size() const;
    tensorflow::Tensor  get_state_input_tensor(int input_size) const;

    virtual void initialize_inputs() override;
    virtual void initialize_output_layers() override;

    virtual void fill_input(const State &state, int batch_index=0) override;
    virtual void extract_output(long count) override;
    virtual void clear_output() override;
public:
    explicit StateNetwork(const Options &opts);
    StateNetwork(const StateNetwork &orig) = delete;
    virtual ~StateNetwork() override;

    virtual bool is_heuristic() override;
    virtual bool is_heuristic_confidence() override;
    virtual int get_heuristic() override;
    virtual const std::vector<int> &get_heuristics() override;
    virtual double get_heuristic_confidence() override;
    virtual const std::vector<double> &get_heuristic_confidences() override;

    static void add_state_network_options_to_parser(OptionParser &parser);
};
}
#endif /* NEURAL_NETWORKS_PROBLEM_NETWORK_H */
