#include "network_policy.h"

#include "../option_parser.h"
#include "../plugin.h"
#include "../policy_result.h"

#include "../neural_networks/abstract_network.h"
#include "../task_utils/task_properties.h"

#include <cassert>
#include <set>

using namespace std;

namespace network_policy {
NetworkPolicy::NetworkPolicy(const Options &opts)
    : Policy(opts),
      network(opts.get<shared_ptr<neural_networks::AbstractNetwork>>("network")) {
    cout << "Initializing network policy..." << endl;
    network->verify_preferred();
    network->initialize();
}

NetworkPolicy::~NetworkPolicy() {
}

PolicyResult NetworkPolicy::compute_policy(const State &state) {
    network->evaluate(state);
    PolicyResult result;
    const ordered_set::OrderedSet<OperatorID> &prefs = network->get_preferred();
    result.set_preferred_operators(vector<OperatorID>(prefs.begin(), prefs.end()));

    vector<float> opPrefs = network->get_operator_preferences();
    for (int i = 0; i < prefs.size(); i++) {
        OperatorID op = prefs[i];
        auto op_proxy = task_proxy.get_operators()[op];
        if (!task_properties::is_applicable(op_proxy, state)) {
            opPrefs[i] = 0.0;
        }
    }
    result.set_operator_preferences(move(opPrefs));
    return result;
}

bool NetworkPolicy::dead_ends_are_reliable() const {
    return false;
}

static shared_ptr<Policy> _parse(OptionParser &parser) {
    parser.document_synopsis("network policy", "");
    parser.document_property("preferred operators",
        "yes (representing policy)");

    Policy::add_options_to_parser(parser);
    parser.add_option<shared_ptr<neural_networks::AbstractNetwork>>("network",
        "Network for state evaluations.");
    Options opts = parser.parse();
    shared_ptr<Policy> policy;
    if (!parser.dry_run()) {
        policy = make_shared<NetworkPolicy>(opts);
    }
    return policy;
}

static Plugin<Policy> _plugin("np", _parse);
}
