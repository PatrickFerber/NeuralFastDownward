#include "technique_uniform_none.h"

#include "../plugin.h"
#include "../task_proxy.h"

#include "../tasks/modified_init_goals_task.h"

using namespace std;

namespace sampling_technique {
const std::string TechniqueUniformNone::name = "uniform_none";

const string &TechniqueUniformNone::get_name() const {
    return name;
}

TechniqueUniformNone::TechniqueUniformNone(const options::Options &opts)
        : SamplingTechnique(opts) { }

std::shared_ptr<AbstractTask> TechniqueUniformNone::create_next(
        shared_ptr<AbstractTask> seed_task, const TaskProxy &) {
    TaskProxy seed_task_proxy(*seed_task);
    int c = 0;
    while (true) {
        vector<int> unassigned(
                seed_task->get_num_variables(), PartialAssignment::UNASSIGNED);
        auto state = PartialAssignment(*seed_task, move(unassigned))
                .get_full_state(check_mutexes, *rng);
        if (state.first) {
            vector<int> values = state.second.get_values();
            return make_shared<extra_tasks::ModifiedInitGoalsTask>(
                    seed_task, move(values), extractGoalFacts(seed_task_proxy.get_goals()));
        } else {
            c++;
        }
    }
}

/* PARSING TECHNIQUE_UNIFORM_NONE*/
static shared_ptr<SamplingTechnique> _parse_technique_uniform_none(
        options::OptionParser &parser) {
    SamplingTechnique::add_options_to_parser(parser);


    options::Options opts = parser.parse();

    shared_ptr<TechniqueUniformNone> technique;
    if (!parser.dry_run()) {
        technique = make_shared<TechniqueUniformNone>(opts);
    }
    return technique;
}

static Plugin<SamplingTechnique> _plugin_technique_uniform_none(
        TechniqueUniformNone::name, _parse_technique_uniform_none);


}
