#include "sampling_tasks.h"

#include "../task_proxy.h"

#include "../algorithms/ordered_set.h"
#include "../task_utils/successor_generator.h"
//#include "../task_utils/predecessor_generator.h"

#include <fstream>
#include <iostream>
#include <set>

#include <memory>
#include <numeric>
#include <string>

using namespace std;

namespace sampling_engine {
SamplingTasks::SamplingTasks(const Options &opts)
    : SamplingEngine(opts) {}


string SamplingTasks::sample_file_header() const {
    return "";
}
vector<string> SamplingTasks::sample(shared_ptr<AbstractTask> task) {
    return vector<string> {task->get_sas()};
}
}
