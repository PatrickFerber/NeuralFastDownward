#include "predecessor_generator.h"

#include "operator_generator_internals.h"
#include "predecessor_generator_factory.h"
#include "regression_task_proxy.h"

#include "../task_proxy.h"
#include "../task_utils/task_properties.h"

using namespace std;

namespace predecessor_generator {
PredecessorGenerator::PredecessorGenerator(const RegressionTaskProxy
    &regression_task_proxy)
    : root(PredecessorGeneratorFactory(regression_task_proxy).create()),
      ops(regression_task_proxy.get_regression_operators()){
    task_properties::verify_no_axioms(regression_task_proxy);
    task_properties::verify_no_conditional_effects(regression_task_proxy);
}

PredecessorGenerator::PredecessorGenerator(const TaskProxy &task_proxy)
    : PredecessorGenerator(RegressionTaskProxy(*task_proxy.get_task())) {
}

PredecessorGenerator::~PredecessorGenerator() = default;

void PredecessorGenerator::generate_applicable_ops(
    const PartialAssignment &assignment, vector<OperatorID> &applicable_ops) const {
    root->generate_applicable_ops(assignment.get_unpacked_values(), applicable_ops);
    applicable_ops.erase(
        std::remove_if(applicable_ops.begin(), applicable_ops.end(),
                           [&](const OperatorID & op_id) {
            return !ops[op_id.get_index()].achieves_subgoal(assignment); }),
        applicable_ops.end());
}

PerTaskInformation<PredecessorGenerator> g_predecessor_generators;
}
