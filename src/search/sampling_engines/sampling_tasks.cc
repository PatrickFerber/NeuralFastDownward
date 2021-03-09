#include "sampling_tasks.h"

#include "../task_utils/successor_generator.h"

#include <memory>
#include <string>

using namespace std;

namespace sampling_engine {
SamplingTasks::SamplingTasks(const options::Options &opts)
    : SamplingEngine(opts) {}


string SamplingTasks::sample_file_header() const {
    return "";
}
vector<string> SamplingTasks::sample(shared_ptr<AbstractTask> task) {
    return vector<string> {task->get_sas()};
}
}
