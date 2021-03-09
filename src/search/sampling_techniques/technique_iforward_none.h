#ifndef SAMPLING_TECHNIQUES_TECHNIQUE_IFORWARD_NONE_H
#define SAMPLING_TECHNIQUES_TECHNIQUE_IFORWARD_NONE_H

#include "sampling_technique.h"

#include "../task_proxy.h"

#include "../utils/distribution.h"

namespace sampling{
class RandomWalkSampler;
}

namespace sampling_technique {
class TechniqueIForwardNone : public SamplingTechnique {
protected:
    std::shared_ptr<utils::DiscreteDistribution> steps;
    const bool deprioritize_undoing_steps;
    const options::ParseTree bias_evaluator_tree;
    const bool bias_probabilistic;
    const double bias_adapt;
    utils::HashMap<PartialAssignment, int> cache;
    std::shared_ptr<Heuristic> bias = nullptr;
    const int bias_reload_frequency;
    int bias_reload_counter;
    std::shared_ptr<sampling::RandomWalkSampler> rws = nullptr;

    virtual std::shared_ptr<AbstractTask> create_next(
            std::shared_ptr<AbstractTask> seed_task,
            const TaskProxy &task_proxy) override;

public:
    explicit TechniqueIForwardNone(const options::Options &opts);
    virtual ~TechniqueIForwardNone() override = default;

    virtual const std::string &get_name() const override;
    const static std::string name;
};
}
#endif
