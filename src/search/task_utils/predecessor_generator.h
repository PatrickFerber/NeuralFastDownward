#ifndef TASK_UTILS_PREDECESSOR_GENERATOR_H
#define TASK_UTILS_PREDECESSOR_GENERATOR_H

#include "regression_task_proxy.h"
#include "successor_generator_internals.h"
#include "../per_task_information.h"

#include <memory>
#include <vector>

class OperatorID;
class PartialAssignment;
class State;
class TaskProxy;


namespace predecessor_generator {
    using namespace successor_generator;

class PredecessorGenerator {
    std::unique_ptr<GeneratorBase> root;
    const RegressionOperatorsProxy ops;
public:
    explicit PredecessorGenerator(const TaskProxy &task_proxy);
    explicit PredecessorGenerator(
            const RegressionTaskProxy &regression_task_proxy);
    /*
      We cannot use the default destructor (implicitly or explicitly)
      here because GeneratorBase is a forward declaration and the
      incomplete type cannot be destroyed.
    */
    ~PredecessorGenerator();

    void generate_applicable_ops(
        const PartialAssignment &assignment, std::vector<OperatorID> &applicable_ops) const;
    // Transitional method, used until the search is switched to the new task interface.
    //void generate_applicable_ops(
//        const GlobalState &state, std::vector<OperatorID> &applicable_ops) const;
};

extern PerTaskInformation<PredecessorGenerator> g_predecessor_generators;
}

#endif
