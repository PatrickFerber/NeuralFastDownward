#ifndef SAMPLING_TECHNIQUES_TECHNIQUE_UNIFORM_NONE_H
#define SAMPLING_TECHNIQUES_TECHNIQUE_UNIFORM_NONE_H

#include "sampling_technique.h"

namespace sampling_technique {

class TechniqueUniformNone : public SamplingTechnique {
protected:
    virtual std::shared_ptr<AbstractTask> create_next(
            std::shared_ptr<AbstractTask> seed_task,
            const TaskProxy &task_proxy) override;

public:
    explicit TechniqueUniformNone(const options::Options &opts);
    virtual ~TechniqueUniformNone() override = default;

    virtual const std::string &get_name() const override;
    const static std::string name;
};

}
#endif
