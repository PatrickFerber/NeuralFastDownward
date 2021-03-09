#include "technique_none_none.h"

#include "../plugin.h"

using namespace std;

namespace sampling_technique {
const string TechniqueNoneNone::name = "none_none";

const string &TechniqueNoneNone::get_name() const {
    return name;
}

TechniqueNoneNone::TechniqueNoneNone(const options::Options &opts)
        : SamplingTechnique(opts) {}

TechniqueNoneNone::TechniqueNoneNone(int count,
                                     bool check_mutexes, bool check_solvable)
        : SamplingTechnique(count, check_mutexes, check_solvable) {}

std::shared_ptr<AbstractTask> TechniqueNoneNone::create_next(
        shared_ptr<AbstractTask> seed_task, const TaskProxy &) {
    return seed_task;
}


static shared_ptr<SamplingTechnique> _parse_technique_none_none(
        options::OptionParser &parser) {
    SamplingTechnique::add_options_to_parser(parser);
    options::Options opts = parser.parse();

    shared_ptr<TechniqueNoneNone> technique;
    if (!parser.dry_run()) {
        technique = make_shared<TechniqueNoneNone>(opts);
    }
    return technique;
}

static Plugin<SamplingTechnique> _plugin_technique_none_none(
        TechniqueNoneNone::name, _parse_technique_none_none);


}