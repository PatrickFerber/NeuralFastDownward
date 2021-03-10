#ifndef TASK_UTILS_PREDECESSOR_GENERATOR_H
#define TASK_UTILS_PREDECESSOR_GENERATOR_H

#include "operator_generator_factory.h"
#include "regression_task_proxy.h"

#include "../per_task_information.h"

#include <memory>
#include <vector>

class OperatorID;
class PartialAssignment;
class TaskProxy;

namespace predecessor_generator {

class PredecessorGenerator {
    std::unique_ptr<operator_generator::GeneratorBase> root;
    const RegressionOperatorsProxy ops;
public:
    explicit PredecessorGenerator(const TaskProxy &task_proxy);
    explicit PredecessorGenerator(
            const RegressionTaskProxy &regression_task_proxy);

    ~PredecessorGenerator();

    void generate_applicable_ops(
        const PartialAssignment &assignment, std::vector<OperatorID> &applicable_ops) const;
};

extern PerTaskInformation<PredecessorGenerator> g_predecessor_generators;
}

#endif
