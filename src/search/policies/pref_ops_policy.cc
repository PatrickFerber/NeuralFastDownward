#include "pref_ops_policy.h"

#include "../evaluator.h"
#include "../option_parser.h"
#include "../plugin.h"
#include "../policy_result.h"
#include "../evaluation_context.h"
#include "../search_statistics.h"

#include "../task_utils/successor_generator.h"

using namespace std;

namespace policies {
PrefOpsPolicy::PrefOpsPolicy(const Options &opts)
    : Policy(opts),
      evaluator(opts.get<shared_ptr<Evaluator>>("evaluator")),
      successor_generator(successor_generator::get_successor_generator(task_proxy)) {
    cout << "Initializing pref ops policy..." << endl;
}

PrefOpsPolicy::~PrefOpsPolicy() {
}

PolicyResult PrefOpsPolicy::compute_policy(const State &state) {
    EvaluationContext context = EvaluationContext(state, nullptr, true);
    EvaluationResult heuristic_result = evaluator->compute_result(context);
    vector<OperatorID> preferred_operators = heuristic_result.get_preferred_operators();
    return PolicyResult(move(preferred_operators), vector<float>(), true);
}

bool PrefOpsPolicy::dead_ends_are_reliable() const {
    return evaluator->dead_ends_are_reliable();
}

static shared_ptr<Policy> _parse(OptionParser &parser) {
    parser.document_synopsis("Heuristic Preferred Operators Policy", "");
    parser.add_option<shared_ptr<Evaluator>> ("evaluator",
    "heuristic function which is used to compute preferred operators. "
    "These are followed along the policy.");
    Policy::add_options_to_parser(parser);
    Options opts = parser.parse();

    shared_ptr<Policy> policy;
    if (!parser.dry_run()) {
        policy = make_shared<PrefOpsPolicy>(opts);
    }
    return policy;
}


static Plugin<Policy> _plugin("pref_ops_policy", _parse);
}
