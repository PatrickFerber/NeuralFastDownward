#include "abstract_network.h"

#include "../plugin.h"

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

}
