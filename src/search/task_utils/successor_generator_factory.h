#ifndef TASK_UTILS_SUCCESSOR_GENERATOR_FACTORY_H
#define TASK_UTILS_SUCCESSOR_GENERATOR_FACTORY_H

#include "operator_generator_factory.h"

#include "../task_proxy.h"

class TaskProxy;

using namespace operator_generator;

namespace successor_generator {

class SuccessorGeneratorFactory : public OperatorGeneratorFactory {
    const TaskProxy &task_proxy;

protected:
    virtual VariablesProxy get_variables() const override;

public:
    explicit SuccessorGeneratorFactory(const TaskProxy &task_proxy);
    virtual GeneratorPtr create() override;
};
}

#endif
