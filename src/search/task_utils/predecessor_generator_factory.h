#ifndef TASK_UTILS_PREDECESSOR_GENERATOR_FACTORY_H
#define TASK_UTILS_PREDECESSOR_GENERATOR_FACTORY_H

#include "regression_task_proxy.h"
#include "successor_generator_factory.h"

#include <memory>
#include <vector>

class TaskProxy;

namespace predecessor_generator {
    using namespace successor_generator;

    class PredecessorGeneratorFactory {
        using ValuesAndGenerators = std::vector<std::pair<int, GeneratorPtr>>;

        const RegressionTaskProxy &regression_task_proxy;
        std::vector<OperatorInfo> operator_infos;

        GeneratorPtr construct_fork(std::vector<GeneratorPtr> nodes) const;
        GeneratorPtr construct_leaf(OperatorRange range) const;
        GeneratorPtr construct_switch(
                int switch_var_id, ValuesAndGenerators values_and_generators) const;
        GeneratorPtr construct_recursive(int depth, OperatorRange range) const;
    public:
        explicit PredecessorGeneratorFactory(const RegressionTaskProxy
                                             &regression_task_proxy);
        // Destructor cannot be implicit because OperatorInfo is forward-declared.
        ~PredecessorGeneratorFactory();
        GeneratorPtr create();
    };
}

#endif
