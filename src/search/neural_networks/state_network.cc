#include "state_network.h"

#include "abstract_network.h"

#include "../option_parser.h"
#include "../plugin.h"

#include <algorithm>
#include <fstream>
#include <memory>
#include <string>

using namespace std;
using namespace tensorflow;
namespace neural_networks {


vector<int> get_domain_sizes(
        TaskProxy &task_proxy, bool domain_max_is_undefined) {
    vector<int> domain_sizes;
    domain_sizes.reserve(task_proxy.get_variables().size());
    for (const VariableProxy &variable_proxy : task_proxy.get_variables()) {
        domain_sizes.push_back(
                variable_proxy.get_domain_size() +
                (domain_max_is_undefined ? -1 : 0));
    }
    return domain_sizes;
}

vector<string> get_atoms(vector<string> atoms) {
    if (atoms.size() == 1 && atoms[0].rfind("file ", 0) == 0){
        vector<string> loaded_atoms;
        ifstream myfile (atoms[0].substr(5));
        string line;
        if (myfile.is_open()) {
            while (getline (myfile,line, ';')) {
                loaded_atoms.push_back(line);
            }
            myfile.close();
        }
        return loaded_atoms;
    } else {
        return atoms;
    }
}


vector<int> get_defaults(vector<string> defaults) {
    if (defaults.size() == 1 && defaults[0].rfind("file ", 0) == 0){
        vector<int> loaded_defaults;
        ifstream myfile (defaults[0].substr(5));
        string line;
        if (myfile.is_open()) {
            while (getline (myfile,line, ';')) {
                loaded_defaults.push_back(stoi(line));
            }
            myfile.close();
        }
        return loaded_defaults;
    } else {
        vector<int> converted_defaults;
        converted_defaults.reserve(defaults.size());
        for (auto d : defaults) {
            converted_defaults.push_back(stoi(d));
        }
        return converted_defaults;
    }
}

vector<FactMapping> get_fact_mappings(
        const vector<int> &domain_sizes,
        const vector<string> &state_atoms,
        const TaskProxy &task_proxy,
        bool allow_unused_atoms
        ) {
    if (state_atoms.empty()) {
        return {};
    }

    //KEEP FACT_MAPPING SORTED BY VARIABLE, VALUE
    unordered_map<string, int> atom2idx;
    for (size_t atom_idx = 0; atom_idx < state_atoms.size(); ++atom_idx) {
        string atom = state_atoms[atom_idx];
        //HACK! to fix bad string parsing
        if (atom[atom.size() - 1] != ')') {
            atom += "()";
        }
        atom2idx[atom] = atom_idx;
    }

    vector<FactMapping> fact_mappings;
    vector<string> unused_facts;
    for (unsigned int var = 0; var < domain_sizes.size(); ++var) {
        for (int value = 0; value < domain_sizes[var]; ++value) {
            string fact_name = task_proxy.get_variables()[var].get_fact(value).get_name();
            fact_name = options::stringify_tree(options::generate_parse_tree(fact_name));
            if (fact_name[fact_name.size() - 1] != ')') {
                fact_name += "()";
            }
            auto iter = atom2idx.find(fact_name);
            if (iter != atom2idx.end()) {
                fact_mappings.push_back(
                        FactMapping(var, value, iter->second));
            } else {
                unused_facts.push_back(fact_name);
            }
        }
    }
    if (task_proxy.get_operators().size() == 0){
        cout << "Task is trivial" << endl;
    } else if (!unused_facts.empty() && !allow_unused_atoms) {
        cerr << "The current tasks has facts that are not used by the network: ";
        for (const string &s: unused_facts) {
            cerr << s << ", ";
        }
        cerr << endl;
        utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
    } else if (!unused_facts.empty()){
        cout << "The current tasks has facts that are not used by the network: ";
        for (const string &s: unused_facts) {
            cout << s << ", ";
        }
        cout << endl << endl;
    }
    return fact_mappings;
}

StateNetwork::StateNetwork(const Options &opts)
    : ProtobufNetwork(opts),
      rng(utils::parse_rng_from_options(opts)),
      domain_max_is_undefined(opts.get<bool>("domain_max_is_undefined")),
      domain_sizes(get_domain_sizes(task_proxy, domain_max_is_undefined)),
      _state_layer_name(opts.get<string>("state_layer")),
      _output_layer_names(opts.get_list<string>("output_layers")),
      state_atoms(get_atoms(opts.get_list<string>("atoms"))),
      state_defaults(get_defaults(opts.get_list<string>("defaults"))),
      allow_unused_atoms(opts.get<bool>("allow_unused_atoms")),
      fact_mappings(get_fact_mappings(
              domain_sizes, state_atoms, task_proxy, allow_unused_atoms)),
      output_type(get_output_type(opts.get<string>("type"))),
      unary_threshold(opts.get<double>("unary_threshold")),
      bin_size(opts.get<int>("bin_size")),
      exponentiate_heuristic(opts.get<bool>("exponentiate_heuristic")){
    if (output_type != OutputType::Classification
        && output_type != OutputType::Regression) {
        cerr << "Invalid output type for network: " << output_type << endl;
        utils::exit_with(utils::ExitCode::SEARCH_UNSUPPORTED);
    }

    if (state_defaults.size() != state_atoms.size()) {
        cerr << "The number of specified atoms does not agree with the "
                "number of given default values: " << state_atoms.size()
             << ", " << state_defaults.size() << endl;
        utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
    }

    if (unary_threshold < 0 && unary_threshold != -1 && unary_threshold != -2
        && unary_threshold != -3) {
        cerr << "Invalid unary_threshold: " << unary_threshold << endl;
        utils::exit_with(utils::ExitCode::SEARCH_UNSUPPORTED);
    }

    if (bin_size < 1) {
        cerr << "Invalid bin_size: " << bin_size << endl;
        utils::exit_with(utils::ExitCode::SEARCH_UNSUPPORTED);
    }

    cout << "Network State Input: ";
    if (state_atoms.empty()) {
    for (size_t var = 0; var < domain_sizes.size(); var++) {
        for (int value = 0; value < domain_sizes[var]; value++) {
            cout << task_proxy.get_task()->get_fact_name(
                    FactPair(var, value)) << ", ";
        }
    }
    } else {
        if (state_atoms.size() != state_defaults.size()) {
            cerr << "Invalid state_atoms, state_defaults definition. "
                 << "Sizes varys (atoms, defaults)" << endl;

        }
        for (unsigned int i = 0; i < state_atoms.size() - 1; ++i) {
            cout << state_atoms[i] << "(" << state_defaults[i] << "), ";
        }
        cout << state_atoms[state_atoms.size() - 1] << "("
             << state_defaults[state_atoms.size() - 1] << ")" << endl;
    }
}

StateNetwork::~StateNetwork() {

}


bool StateNetwork::is_heuristic() {
    return true;
}

bool StateNetwork::is_heuristic_confidence() {
    return true;
}

int StateNetwork::get_heuristic() {
    return last_h;
}

double StateNetwork::get_heuristic_confidence() {
    return last_h_confidence;
}

const vector<int> &StateNetwork::get_heuristics() {
    return last_h_batch;
}

const vector<double> &StateNetwork::get_heuristic_confidences() {
    return last_h_confidence_batch;
}

int StateNetwork::get_state_input_size() const {
    if (state_atoms.empty()) {
        int input_size = 0;
        for (int domain_size : domain_sizes) {
            input_size += domain_size;
        }
        return input_size;
    } else {
        return state_atoms.size();
    }
}


Tensor StateNetwork::get_state_input_tensor(int input_size) const {
    Tensor t(DT_FLOAT, TensorShape({batch_size, input_size}));
    if (!state_defaults.empty()) {
        auto state_tensor_buffer = t.matrix<float>();
        for (int batch_idx = 0; batch_idx < batch_size; ++batch_idx) {
            for (unsigned int idx = 0; idx < state_defaults.size(); ++idx) {
                state_tensor_buffer(batch_idx, idx) = state_defaults[idx];
            }
        }
    } // else every value will be overwritten every time
    return t;
}
void StateNetwork::initialize_inputs() {
    int input_size = get_state_input_size();
    inputs = {{_state_layer_name, get_state_input_tensor(input_size)}, };
}

void StateNetwork::initialize_output_layers() {
    output_layers = _output_layer_names;
}


void StateNetwork::fill_input(const State &state, int batch_idx) {
    state.unpack();
    const vector<int> &values = state.get_values();
    auto tensor_buffer = inputs[0].second.matrix<float>();
    if (state_atoms.empty()) {
        int idx = 0;
        for (size_t var = 0; var < domain_sizes.size(); var++) {
            int chosen;
            if (domain_max_is_undefined && values[var] == domain_sizes[var]) {
                chosen = (*rng)(domain_sizes[var]);
            } else {
                chosen = values[var];
            }

            for (int value = 0; value < domain_sizes[var]; value++) {
                if (value == chosen) {
                    tensor_buffer(batch_idx, idx) = 1;
                } else {
                    tensor_buffer(batch_idx, idx) = 0;
                }
                idx++;
            }
        }
    } else {
        for (const FactMapping &single_mapping: fact_mappings) {
            tensor_buffer(batch_idx, single_mapping.index) =
                (values[single_mapping.var] == single_mapping.value) ? 1 : 0;
        }
    }
}

static float inline extract_unary_confidence(
        TTypes<float>::Matrix & tensor_buffer,
        int batch_idx, int h_value) {
    assert(h_value < tensor_buffer.dimension(1));
    if (h_value == -1) { // aka DEAD_END
        return 1 - tensor_buffer(batch_idx, h_value + 1);
    } else if (h_value == tensor_buffer.dimension(1) - 1) {
        return tensor_buffer(batch_idx, h_value);
    } else {
        return tensor_buffer(batch_idx, h_value) - tensor_buffer(batch_idx, h_value + 1);
    }
}

void StateNetwork::extract_output(long count) {
    auto tensor_buffer = outputs[0].matrix<float>();
    assert(tensor_buffer.dimensions().size() == 2);
    assert(tensor_buffer.dimension(0) == batch_size);
    if (count == -1) {
        count = tensor_buffer.dimension(0);
    }
    double h = -2;
    for (int batch_idx = 0;
         batch_idx < min(count, (long) tensor_buffer.dimension(0));
         ++batch_idx) {

        if (output_type == OutputType::Regression) {
            h = round(tensor_buffer(batch_idx, 0));
            last_h_confidence = -1;
        } else if (output_type == OutputType::Classification) {
            if (unary_threshold == 0) {
                // plain classification
                int maxIdx = 0;
                float maxVal = tensor_buffer(batch_idx, 0);
                for (int i = 0; i < tensor_buffer.dimension(1); i++) {
                    if (tensor_buffer(batch_idx, i) > maxVal) {
                        maxIdx = i;
                        maxVal = tensor_buffer(batch_idx, i);
                    }
                }
                h = maxIdx;
                last_h_confidence = maxVal; //assuming softmax activation in final layer
            } else if (unary_threshold > 0) {
                // 0 is encoded as 1 0 0 0 ...
                // nothing is encoded as 0 0 0 0 0 ...,
                // thus, we interpret it as dead end (-1)

                h = -2;
                for (int i = 0; i < tensor_buffer.dimension(1); i++) {
                    if (tensor_buffer(batch_idx, i) < unary_threshold) {
                        h = i - 1;
                        last_h_confidence = extract_unary_confidence(
                                tensor_buffer, batch_idx, last_h);
                        break;
                    }
                }
                if (h == -2) {
                    h = tensor_buffer.dimension(1) - 1;
                    last_h_confidence = -1;
                }
            } else if (unary_threshold == -1) {
                // max confidence choosen
                int max_h = Heuristic::DEAD_END;
                float max_confidence = extract_unary_confidence(
                        tensor_buffer, batch_idx, max_h);
                for (int i = 0; i < tensor_buffer.dimension(1); i++) {
                    float new_confidence = extract_unary_confidence(
                            tensor_buffer, batch_idx, i);
                    if (new_confidence > max_confidence) {
                        max_h = i;
                        max_confidence = new_confidence;
                    }
                }
                h = max_h;
                last_h_confidence = max_confidence;
            } else if (unary_threshold == -2) {
                //mean heuristic value
                float mean_h = 0;
                for (int i = 0; i < tensor_buffer.dimension(1); i++) {
                    float new_confidence = extract_unary_confidence(
                            tensor_buffer, batch_idx, i);
                    mean_h += new_confidence * i;
                }
                h = mean_h;
                last_h_confidence = -1;
            } else if (unary_threshold == -3) {
                //draw h value form distribution
                vector<double> weights;
                weights.reserve(tensor_buffer.dimension(1));
                for (int i = 0; i < tensor_buffer.dimension(1); i++) {
                    float new_confidence = extract_unary_confidence(
                            tensor_buffer, batch_idx, i);
                    weights.push_back(new_confidence);
                    if (weights[i] < 0) {
                        weights[i] = 0;
                    }
                }
                h = rng->weighted_choose_index(weights);
                last_h_confidence = weights[last_h];
            } else {
                cerr << "Invalid unary_threshold: " << unary_threshold << endl;
                utils::exit_with(utils::ExitCode::SEARCH_UNSUPPORTED);
            }
        }
        assert(h >= 0);
        if (bin_size > 1) {
            h *= bin_size;
        }
        if (exponentiate_heuristic) {
            h = exp(h);
        }

        last_h = round(h);
        last_h_batch.push_back(last_h);
        last_h_confidence_batch.push_back(last_h_confidence);
    }
}

void StateNetwork::clear_output() {
    last_h = Heuristic::NO_VALUE;
    last_h_batch.clear();
    last_h_confidence_batch.clear();
}

void StateNetwork::add_state_network_options_to_parser(options::OptionParser &parser) {
    parser.add_option<string>("type",
                              "Type of network output (regression or classification)");
    parser.add_option<string>("state_layer", "Name of the input layer in "
                                             "the computation graph to insert the current state.");
    parser.add_list_option<string>("output_layers", "Name of the output layer "
                                              "from which to extract the network output.");
    parser.add_list_option<string>("atoms", "(Optional) Description of the atoms"
                                            "in the input state of the network. Provide a list of atom names exactly"
                                            "as they are used by Fast Downward. The order of the list has to fit to"
                                            "the order the network expects them. Alternatively, write [file filepath]"
                                            "to load the atoms from filepath. In the file the entries are separated "
                                            "by ;.", "[]");
    parser.add_list_option<string>("defaults", "(Optional) If not given, then"
                                            "all state atoms values are defaulted to 0. If given, then this needs to "
                                            "be of the same size as \"atoms\" or as the number of not atoms"
                                            " not pruned by Fast Downward (if only this is given and not \"atoms\"."
                                            " Provide 1 for atoms present by default and 0 for atoms absent. (The "
                                            "goal atoms default always to 0).Alternatively, write [file filepath]"
                                            "to load the defaults from filepath. The entries shall be separated by ;",
                                            "[]");
    parser.add_option<bool>("allow_unused_atoms",
            "If \"atoms\" is used, then more atoms could be defined that are"
            "not given in the current task. This options forbids this.",
            "true");
    parser.add_option<double>(
            "unary_threshold",
            "Requires classification network. The network outputs the heuristic "
            "values in unary encoding. Use this parameter as threshold when a "
            "predicted digit is still counted as 1. If set to 0, the feature is "
            "deactivated. If it is set to -1, then it will predict the class"
            "with the highest confidence (p(class_i) - p(class_{i+1})",
            "0");
    parser.add_option<int>(
            "bin_size",
            "Size of the bins in which the heuristic values are sorted",
            "1");
    parser.add_option<bool>(
            "domain_max_is_undefined",
            "Specifies whether the final value in each domain represents"
            "that the variable is not defined. An undefined variable is not fed"
            "into the NN.",
            "false"
            );
    parser.add_option<bool>(
            "exponentiate_heuristic",
            "The float heuristic value predicted by the NN will be"
            "exponentiated (e ^ prediction) prior to being rounded to an "
            "integer. Use this, if you transformed the label via ln(label)"
            "during training",
            "false"
    );
    utils::add_rng_options(parser);
}
}

static shared_ptr<neural_networks::AbstractNetwork> _parse(OptionParser &parser) {
    neural_networks::ProtobufNetwork::add_options_to_parser(parser);
    neural_networks::StateNetwork::add_state_network_options_to_parser(
            parser);
    Options opts = parser.parse();

    shared_ptr<neural_networks::StateNetwork> network;
    if (!parser.dry_run()) {
        network = make_shared<neural_networks::StateNetwork>(opts);
    }

    return network;
}

static Plugin<neural_networks::AbstractNetwork> _plugin("snet", _parse);
