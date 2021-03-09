#ifndef SAMPLING_TECHNIQUES_TECHNIQUE_NONE_NONE_H
#define SAMPLING_TECHNIQUES_TECHNIQUE_NONE_NONE_H

#include "sampling_technique.h"

namespace sampling_technique {

class TechniqueNoneNone : public SamplingTechnique {
protected:
    virtual std::shared_ptr<AbstractTask> create_next(
            std::shared_ptr<AbstractTask> seed_task,
            const TaskProxy &task_proxy) override;
public:
    explicit TechniqueNoneNone(const options::Options &opts);
    TechniqueNoneNone(int count, bool check_mutexes,
                      bool check_solvable);
    virtual ~TechniqueNoneNone() override = default;

    virtual const std::string &get_name() const override;
    const static std::string name;
};
}
#endif
