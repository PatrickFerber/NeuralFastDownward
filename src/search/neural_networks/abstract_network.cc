#include "abstract_network.h"

#include "../plugin.h"

#include "../task_utils/task_properties.h"

#include <algorithm>
#include <iostream>

using namespace std;
namespace neural_networks {
OutputType get_output_type(const std::string &type) {
    if (type == "regression") {
        return OutputType::Regression;
    } else if (type == "classification") {
        return OutputType::Classification;
    } else {
        std::cerr << "Invalid network output type: " << type << std::endl;
        utils::exit_with(utils::ExitCode::SEARCH_UNSUPPORTED);
    }
}

bool AbstractNetwork::is_heuristic() {
    return false;
}

void AbstractNetwork::verify_heuristic() {
    if (!is_heuristic()) {
        cerr << "Network does not support heuristic estimates." << endl
             << "Terminating." << endl;
        utils::exit_with(utils::ExitCode::SEARCH_UNSUPPORTED);
    }
}

int AbstractNetwork::get_heuristic() {
    cerr << "Network does not support heuristic estimates." << endl
         << "Terminating." << endl;
    utils::exit_with(utils::ExitCode::SEARCH_UNSUPPORTED);
}

const vector<int> &AbstractNetwork::get_heuristics() {
    cerr << "Network does not support heuristic estimates." << endl
         << "Terminating." << endl;
    utils::exit_with(utils::ExitCode::SEARCH_UNSUPPORTED);
}

bool AbstractNetwork::is_heuristic_confidence() {
    return false;
}

void AbstractNetwork::verify_heuristic_confidence() {
    if (!is_heuristic_confidence()) {
        cerr << "Network does not support heuristic confidence estimates." << endl
             << "Terminating." << endl;
        utils::exit_with(utils::ExitCode::SEARCH_UNSUPPORTED);
    }
}

double AbstractNetwork::get_heuristic_confidence() {
    cerr << "Network does not support confidences for heuristic estimates." << endl
         << "Terminating." << endl;
    utils::exit_with(utils::ExitCode::SEARCH_UNSUPPORTED);
}

const std::vector<double> & AbstractNetwork::get_heuristic_confidences() {
    cerr << "Network does not support confidences for heuristic estimates." << endl
         << "Terminating." << endl;
    utils::exit_with(utils::ExitCode::SEARCH_UNSUPPORTED);
}

bool AbstractNetwork::is_preferred() {
    return false;
}

void AbstractNetwork::verify_preferred() {
    if (!is_preferred()) {
        cerr << "Network does not support preferred operator estimates." << endl
             << "Terminating." << endl;
        utils::exit_with(utils::ExitCode::SEARCH_UNSUPPORTED);
    }
}

ordered_set::OrderedSet<OperatorID> &AbstractNetwork::get_preferred() {
    cerr << "Network does not support preferred operator estimates." << endl
         << "Terminating." << endl;
    utils::exit_with(utils::ExitCode::SEARCH_UNSUPPORTED);
}

vector<ordered_set::OrderedSet<OperatorID>> &AbstractNetwork::get_preferreds() {
    cerr << "Network does not support preferred operator estimates." << endl
         << "Terminating." << endl;
    utils::exit_with(utils::ExitCode::SEARCH_UNSUPPORTED);
}

std::vector<float> &AbstractNetwork::get_operator_preferences() {
    cerr << "Network does not support preferred operator preferences." << endl
         << "Terminating." << endl;
    utils::exit_with(utils::ExitCode::SEARCH_UNSUPPORTED);
}

static PluginTypePlugin<AbstractNetwork> _type_plugin(
"AbstractNetwork",
// TODO: Replace empty string by synopsis for the wiki page.
"");


std::vector<FactPair> get_fact_mapping(
        const AbstractTask *task, const vector<string> &facts) {
    if (facts.empty()) {
        return task_properties::get_strips_fact_pairs(task);
    }

    unordered_map<string, FactPair> name2factpair;
    for (const FactPair &fp: task_properties::get_strips_fact_pairs(task)) {
        string fact_name = task->get_fact_name(fp);
        fact_name = options::stringify_tree(options::generate_parse_tree(fact_name));
        name2factpair.insert({fact_name, fp});
    }

    vector<FactPair> order;
    int not_found = 0;
    for (const string &fact: facts) {
        auto iter = name2factpair.find(fact);
        if (iter == name2factpair.end()) {
            not_found++;
            cout << "Unknown fact: " << fact << endl;
            order.push_back(FactPair::no_fact);
        } else {
            order.push_back(iter->second);
        }
    }
    cout << "Number of unknown facts:" << not_found << endl;
    return order;
}

std::vector<int> get_default_inputs(
        const std::vector<std::string> &default_inputs) {
    vector<int> converted;
    converted.reserve(default_inputs.size());
    transform(default_inputs.begin(), default_inputs.end(),
              back_inserter(converted),
              [](const std::string& str) { return std::stoi(str); });
    return converted;
}

void check_facts_and_default_inputs(
        const vector<FactPair> &relevant_facts,
        const vector<int> &default_input_values) {
    if (default_input_values.empty()) {
        if (any_of(relevant_facts.begin(), relevant_facts.end(),
                   [](const FactPair &fp){
                       return fp == FactPair::no_fact;})) {
            cerr << "If no default values are given for the facts, then"
                    "every specified fact has to be present in the given"
                    "SAS task." << endl;
            utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
        }
    } else {
        if (default_input_values.size() != relevant_facts.size()) {

            cerr << "If default values are given for the facts, then exactly one"
                    "value has to be given for every (STRIPS) fact (facts: "
                 << relevant_facts.size()
                 << ", default values: " << default_input_values.size()
                 << endl;
            utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
        }
    }
}
}
