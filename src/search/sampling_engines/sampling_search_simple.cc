#include "sampling_search_simple.h"

#include "sampling_search_base.h"
#include "sampling_engine.h"

#include "../option_parser.h"
#include "../plugin.h"

#include "../task_utils/task_properties.h"

#include <sstream>
#include <string>

using namespace std;

namespace sampling_engine {

string SamplingSearchSimple::construct_header() const {
    ostringstream oss;
    oss << "# Format: ";
    if (store_plan_cost){
        oss << "<PlanCost>" << field_separator;
    }
    if (store_state) {
        oss << "<State>" << field_separator;
    }
    if (store_operator) {
        oss << "<Operator>" << field_separator;
    }
    oss.seekp(-1,oss.cur);
    oss << endl;
    if (store_plan_cost){
        oss << "#<PlanCost>=single integer value" << endl;
    }
    if (store_state) {
        oss << "#<State>=";
        for (const FactPair &fp: relevant_facts) {
            oss << task->get_fact_name(fp) << state_separator;
        }
        oss.seekp(-1,oss.cur);
        oss << endl;
    }
    if (store_operator) {
        oss << "#<Operator>=";
        for (int idx = 0; idx < task->get_num_operators(); ++idx) {
            oss << task->get_operator_name(idx, false) << state_separator;
        }
        oss.seekp(-1,oss.cur);
        oss << endl;
    }
    oss << "#";
    return oss.str();
}

string SamplingSearchSimple::sample_file_header() const {
    return header;
}

vector<string> SamplingSearchSimple::extract_samples() {
    vector<string> samples;

    OperatorsProxy ops = task_proxy.get_operators();
    Trajectory trajectory;
    engine->get_search_space().trace_path(engine->get_goal_state(), trajectory);
    Plan plan = engine->get_plan();
    assert(plan.size() == trajectory.size() - 1);
    // Sequence:    s0 a0 s1 a1 s2
    // Plan:        a0 a1
    // Trajectory:  s0 s1 s2
    int cost = 0;
    for (size_t idx_t = trajectory.size(); idx_t-- > 0;) {
        ostringstream oss;

        if (store_plan_cost) {
            if (idx_t != trajectory.size() - 1){
                cost += ops[plan[idx_t]].get_cost();
            }
            oss << cost << field_separator;
        }

        if (store_state) {
            State state = engine->get_state_registry().lookup_state(trajectory[idx_t]);
            state.unpack();
            vector<int> values = state.get_values();
            for (const FactPair &fp: relevant_facts) {
                oss << (values[fp.var] == fp.value ? 1 : 0) << state_separator;
            }
            oss.seekp(-1,oss.cur);
            oss << field_separator;
        }

        if (store_operator) {
            // Select no operator for the goal state
            int next_op = idx_t >= plan.size() ?
                    OperatorID::no_operator.get_index() :
                    plan[idx_t].get_index();

            for (int idx = 0; idx < task->get_num_operators(); ++idx) {
                oss << (next_op == idx ? 1 : 0) << state_separator;
            }
            oss.seekp(-1,oss.cur);
            oss << field_separator;
        }

        string s = oss.str();
        s.pop_back();
        samples.push_back(s);
    }
    samples.push_back("# ---");
    return samples;
}


SamplingSearchSimple::SamplingSearchSimple(const options::Options &opts)
    : SamplingSearchBase(opts),
      store_plan_cost(opts.get<bool>("store_plan_cost")),
      store_state(opts.get<bool>("store_state")),
      store_operator(opts.get<bool>("store_operator")),
      relevant_facts(task_properties::get_strips_fact_pairs(task.get())),
      header(construct_header()){


}


static shared_ptr<SearchEngine> _parse_sampling_search_simple(OptionParser &parser) {
    parser.document_synopsis("Sampling Search Manager", "");

    sampling_engine::SamplingSearchBase::add_sampling_search_base_options(parser);
    sampling_engine::SamplingEngine::add_sampling_options(parser);
    sampling_engine::SamplingStateEngine::add_sampling_state_options(
            parser, "fields", "pddl", ";", ";");

    parser.add_option<bool>(
            "store_plan_cost",
            "Store for every state its cost along the plan to the goal",
            "true");

    parser.add_option<bool>(
            "store_state",
            "Store every state along the plan",
            "true");

    parser.add_option<bool>(
            "store_operator",
            "Store for every state along the plan the next chosen operator",
            "true");

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
