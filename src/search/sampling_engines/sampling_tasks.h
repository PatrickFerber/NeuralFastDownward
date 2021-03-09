#ifndef SEARCH_ENGINES_SAMPLING_TASKS_H
#define SEARCH_ENGINES_SAMPLING_TASKS_H

#include "sampling_engine.h"

#include <functional>
#include <memory>
#include <ostream>
#include <sstream>
#include <tree.hh>
#include <vector>

namespace options {
class Options;
}

namespace sampling_engine {
class SamplingTasks : public SamplingEngine {
protected:
    virtual std::vector<std::string> sample(
            std::shared_ptr<AbstractTask> task) override;
    virtual std::string sample_file_header() const override;
public:
    explicit SamplingTasks(const options::Options &opts);
    virtual ~SamplingTasks() override = default;
};
}
#endif
