#include "technique_null.h"

using namespace std;

namespace sampling_technique {
const std::string TechniqueNull::name = "null";

const string &TechniqueNull::get_name() const {
    return name;
}

TechniqueNull::TechniqueNull()
        : SamplingTechnique(0, false, false) {}


std::shared_ptr<AbstractTask> TechniqueNull::create_next(
        shared_ptr<AbstractTask> /*seed_task*/, const TaskProxy &) {
    return nullptr;
}

}