#ifndef SAMPLING_TECHNIQUES_TECHNIQUE_NULL_H
#define SAMPLING_TECHNIQUES_TECHNIQUE_NULL_H

#include "sampling_technique.h"

namespace sampling_technique {

class TechniqueNull : public SamplingTechnique {
protected:
    virtual std::shared_ptr<AbstractTask> create_next(
            std::shared_ptr<AbstractTask> seed_task,
            const TaskProxy &task_proxy) override;

public:
    TechniqueNull();

    virtual ~TechniqueNull() override = default;

    virtual const std::string &get_name() const override;

    const static std::string name;
};
}
#endif
