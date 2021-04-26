#include "sampling_search_simple.h"

#include "sampling_search_base.h"
#include "sampling_engine.h"

#include "../option_parser.h"
#include "../plugin.h"

#include <sstream>
#include <string>

using namespace std;

namespace sampling_engine {
string SamplingSearchSimple::sample_file_header() const {
    return "# Format: <Cost to Go> <State>";
}

vector<string> SamplingSearchSimple::extract_samples() {
    vector<string> samples;

    OperatorsProxy ops = task_proxy.get_operators();
    Trajectory trajectory;
    engine->get_search_space().trace_path(engine->get_goal_state(), trajectory);
    Plan plan = engine->get_plan();
    assert(plan.size() == trajectory.size() - 1);

    int cost = 0;
    for (size_t idx_t = trajectory.size(); idx_t-- > 0;) {
        State state = engine->get_state_registry().lookup_state(trajectory[idx_t]);
        state.unpack();
        vector<int> values = state.get_values();
        ostringstream oss;
        oss << cost << field_separator;
        for (int var = 0; var < task->get_num_variables(); ++var) {
            for (int val = 0; val < task->get_variable_domain_size(var); ++val) {
                string fact_name = task->get_fact_name(FactPair(var, val));
                if (fact_name != "<none of those>" && fact_name.rfind("NegatedAtom", 0) == string::npos) {
                    oss << (var == 0 && val == 0 ? "" : state_separator)
                        << (values[var] == val ? 1 : 0);
                }
            }
        }
        if (idx_t > 0) {
            cost += ops[plan[idx_t - 1]].get_cost();
        }
        samples.push_back(oss.str());
    }
    samples.push_back("#---");
    return samples;
}

SamplingSearchSimple::SamplingSearchSimple(const options::Options &opts)
    : SamplingSearchBase(opts) {
}


static shared_ptr<SearchEngine> _parse_sampling_search_simple(OptionParser &parser) {
    parser.document_synopsis("Sampling Search Manager", "");

    sampling_engine::SamplingSearchBase::add_sampling_search_base_options(parser);
    sampling_engine::SamplingEngine::add_sampling_options(parser);
    sampling_engine::SamplingStateEngine::add_sampling_state_options(
            parser, "fields", "pddl", ";", ";");
    SearchEngine::add_options_to_parser(parser);
    Options opts = parser.parse();
    shared_ptr<sampling_engine::SamplingSearchSimple> engine;
    if (!parser.dry_run()) {
        engine = make_shared<sampling_engine::SamplingSearchSimple>(opts);
    }

    return engine;
}

static Plugin<SearchEngine> _plugin_search("sampling_search_simple", _parse_sampling_search_simple);

}
