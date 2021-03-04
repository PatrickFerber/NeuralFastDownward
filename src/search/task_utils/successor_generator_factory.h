#ifndef TASK_UTILS_SUCCESSOR_GENERATOR_FACTORY_H
#define TASK_UTILS_SUCCESSOR_GENERATOR_FACTORY_H

#include "../task_proxy.h"
#include <memory>
#include <vector>

class TaskProxy;

namespace successor_generator {
class GeneratorBase;

using GeneratorPtr = std::unique_ptr<GeneratorBase>;

struct OperatorRange {
    int begin;
    int end;

    OperatorRange(int begin, int end)
            : begin(begin), end(end) {
    }

    bool empty() const {
        return begin == end;
    }

    int span() const {
        return end - begin;
    }
};


class OperatorInfo {
    /*
      The attributes are not const because we must support
      assignment/swapping to sort vector<OperatorInfo>.
    */
    OperatorID op;
    std::vector<FactPair> precondition;
public:
    OperatorInfo(OperatorID op, std::vector<FactPair> precondition)
            : op(op),
              precondition(move(precondition)) {
    }

    bool operator<(const OperatorInfo &other) const {
        return precondition < other.precondition;
    }

    OperatorID get_op() const {
        return op;
    }

    // Returns -1 as a past-the-end sentinel.
    int get_var(int depth) const {
        if (depth == static_cast<int>(precondition.size())) {
            return -1;
        } else {
            return precondition[depth].var;
        }
    }

    int get_value(int depth) const {
        return precondition[depth].value;
    }
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

    const OperatorInfo &get_current_op_info() const {
        assert(!range.empty());
        return operator_infos[range.begin];
    }

    int get_current_group_key() const {
        const OperatorInfo &op_info = get_current_op_info();
        if (group_by == GroupOperatorsBy::VAR) {
            return op_info.get_var(depth);
        } else {
            assert(group_by == GroupOperatorsBy::VALUE);
            return op_info.get_value(depth);
        }
    }
public:
    explicit OperatorGrouper(
            const std::vector<OperatorInfo> &operator_infos,
            int depth,
            GroupOperatorsBy group_by,
            OperatorRange range)
            : operator_infos(operator_infos),
              depth(depth),
              group_by(group_by),
              range(range) {
    }

    bool done() const {
        return range.empty();
    }

    std::pair<int, OperatorRange> next() {
        assert(!range.empty());
        int key = get_current_group_key();
        int group_begin = range.begin;
        do {
            ++range.begin;
        } while (!range.empty() && get_current_group_key() == key);
        OperatorRange group_range(group_begin, range.begin);
        return std::make_pair(key, group_range);
    }
};

class SuccessorGeneratorFactory {
    using ValuesAndGenerators = std::vector<std::pair<int, GeneratorPtr>>;

    const TaskProxy &task_proxy;
    std::vector<OperatorInfo> operator_infos;

    GeneratorPtr construct_fork(std::vector<GeneratorPtr> nodes) const;
    GeneratorPtr construct_leaf(OperatorRange range) const;
    GeneratorPtr construct_switch(
        int switch_var_id, ValuesAndGenerators values_and_generators) const;
    GeneratorPtr construct_recursive(int depth, OperatorRange range) const;
public:
    explicit SuccessorGeneratorFactory(const TaskProxy &task_proxy);
    // Destructor cannot be implicit because OperatorInfo is forward-declared.
    ~SuccessorGeneratorFactory();
    GeneratorPtr create();
};
}

#endif
