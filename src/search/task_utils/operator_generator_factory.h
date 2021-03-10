#ifndef TASK_UTILS_OPERATOR_GENERATOR_FACTORY_H
#define TASK_UTILS_OPERATOR_GENERATOR_FACTORY_H

#include "../task_proxy.h"

#include <memory>
#include <vector>


namespace operator_generator {
class GeneratorBase;

using GeneratorPtr = std::unique_ptr<GeneratorBase>;

struct OperatorRange {
    int begin;
    int end;

    OperatorRange(int begin, int end);
    bool empty() const;
    int span() const;
};


class OperatorInfo {
    /*
      The attributes are not const because we must support
      assignment/swapping to sort vector<OperatorInfo>.
    */
    OperatorID op;
    std::vector<FactPair> precondition;
public:
    OperatorInfo(OperatorID op, std::vector<FactPair> precondition);

    bool operator<(const OperatorInfo &other) const;
    OperatorID get_op() const;
    // Returns -1 as a past-the-end sentinel.
    int get_var(int depth) const;
    int get_value(int depth) const;
};

enum class GroupOperatorsBy {
    VAR,
    VALUE
};

class OperatorGrouper {
    const std::vector<OperatorInfo> &operator_infos;
    const int depth;
    const GroupOperatorsBy group_by;
    OperatorRange range;

    const OperatorInfo &get_current_op_info() const;
    int get_current_group_key() const;

public:
    explicit OperatorGrouper(
            const std::vector<OperatorInfo> &operator_infos,
            int depth,
            GroupOperatorsBy group_by,
            OperatorRange range);

    bool done() const;
    std::pair<int, OperatorRange> next();
};

template <typename AnyOperatorProxy>
std::vector<FactPair> build_sorted_precondition(const AnyOperatorProxy &op) {
    std::vector<FactPair> precond;
    precond.reserve(op.get_preconditions().size());
    for (FactProxy pre : op.get_preconditions())
        precond.emplace_back(pre.get_pair());
    // Preconditions must be sorted by variable.
    sort(precond.begin(), precond.end());
    return precond;
}

class OperatorGeneratorFactory {
    using ValuesAndGenerators = std::vector<std::pair<int, GeneratorPtr>>;

    std::vector<OperatorInfo> operator_infos;

    GeneratorPtr construct_fork(std::vector<GeneratorPtr> nodes) const;
    GeneratorPtr construct_leaf(OperatorRange range) const;
    GeneratorPtr construct_switch(
            int switch_var_id, ValuesAndGenerators values_and_generators) const;
    GeneratorPtr construct_recursive(int depth, OperatorRange range) const;

protected:
    virtual VariablesProxy get_variables() const = 0;
    template <typename AnyOperatorsProxy> GeneratorPtr internal_create(AnyOperatorsProxy operators) {
        operator_infos.reserve(operators.size());
        for (auto op : operators) {
            operator_infos.emplace_back(
                    OperatorID(op.get_id()), build_sorted_precondition(op));
        }
        /* Use stable_sort rather than sort for reproducibility.
           This amounts to breaking ties by operator ID. */
        stable_sort(operator_infos.begin(), operator_infos.end());

        OperatorRange full_range(0, operator_infos.size());
        GeneratorPtr root = construct_recursive(0, full_range);
        operator_infos.clear();
        return root;
    }

public:
    virtual ~OperatorGeneratorFactory() = default;
    virtual GeneratorPtr create() = 0;
};
}

#endif
