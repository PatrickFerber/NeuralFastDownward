#include "successor_generator_factory.h"

#include "operator_generator_internals.h"

using namespace std;

namespace successor_generator {

SuccessorGeneratorFactory::SuccessorGeneratorFactory(
    const TaskProxy &task_proxy)
    : OperatorGeneratorFactory(), task_proxy(task_proxy) {
}

VariablesProxy SuccessorGeneratorFactory::get_variables() const {
    return task_proxy.get_variables();
}

GeneratorPtr SuccessorGeneratorFactory::create() {
    return internal_create(task_proxy.get_operators());
}
}
