#include "predecessor_generator_factory.h"

#include "operator_generator_internals.h"
#include "regression_task_proxy.h"

using namespace std;

namespace predecessor_generator {

PredecessorGeneratorFactory::PredecessorGeneratorFactory(
    const RegressionTaskProxy &regression_task_proxy)
    : OperatorGeneratorFactory(), regression_task_proxy(regression_task_proxy) {
}

VariablesProxy PredecessorGeneratorFactory::get_variables() const {
    return regression_task_proxy.get_variables();
}

GeneratorPtr PredecessorGeneratorFactory::create() {
    return internal_create(regression_task_proxy.get_regression_operators());
}
}
