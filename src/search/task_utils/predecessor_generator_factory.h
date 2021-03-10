#ifndef TASK_UTILS_PREDECESSOR_GENERATOR_FACTORY_H
#define TASK_UTILS_PREDECESSOR_GENERATOR_FACTORY_H

#include "operator_generator_factory.h"

class RegressionTaskProxy;

using namespace operator_generator;

namespace predecessor_generator {

class PredecessorGeneratorFactory : public OperatorGeneratorFactory {
    const RegressionTaskProxy &regression_task_proxy;

protected:
    virtual VariablesProxy get_variables() const override;
public:
    explicit PredecessorGeneratorFactory(
            const RegressionTaskProxy &regression_task_proxy);
    virtual GeneratorPtr create() override;

};
}

#endif
