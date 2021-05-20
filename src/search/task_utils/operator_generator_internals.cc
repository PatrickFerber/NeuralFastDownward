#include "operator_generator_internals.h"

#include "../task_proxy.h"

#include <cassert>

using namespace std;

/*
  Notes on possible optimizations:

  - Using specialized allocators (e.g. an arena allocator) could
    improve cache locality and reduce memory.

  - We could keep the different nodes in a single vector (for example
    of type unique_ptr<GeneratorBase>) and then use indices rather
    than pointers for representing child nodes. This would reduce the
    memory overhead for pointers in 64-bit builds. However, this
    overhead is not as bad as it used to be.

  - Going further down this route, on the more extreme end of the
    spectrum, we could use a "byte-code" style representation, where
    the successor generator is just a long vector of ints combining
    information about node type with node payload.

    For example, we could represent different node types as follows,
    where BINARY_FORK etc. are symbolic constants for tagging node
    types:

    - binary fork: [BINARY_FORK, child_1, child_2]
    - multi-fork:  [MULTI_FORK, n, child_1, ..., child_n]
    - vector switch: [VECTOR_SWITCH, var_id, child_1, ..., child_k]
    - single switch: [SINGLE_SWITCH, var_id, value, child_index]
    - hash switch: [HASH_SWITCH, var_id, map_no]
      where map_no is an index into a separate vector of hash maps
      (represented out of band)
    - single leaf: [SINGLE_LEAF, op_id]
    - vector leaf: [VECTOR_LEAF, n, op_id_1, ..., op_id_n]

    We could compact this further by permitting to use operator IDs
    directly wherever child nodes are used, by using e.g. negative
    numbers for operatorIDs and positive numbers for node IDs,
    obviating the need for SINGLE_LEAF. This would also make
    VECTOR_LEAF redundant, as MULTI_FORK could be used instead.

    Further, if the other symbolic constants are negative numbers,
    we could represent forks just as [n, child_1, ..., child_n] without
    symbolic constant at the start, unifying binary and multi-forks.

    To completely unify the representation, not needing hash values
    out of band, we might consider switches of the form [SWITCH, k,
    var_id, value_1, child_1, ..., value_k, child_k] that permit
    binary searching. This would only leave switch and fork nodes, and
    we could do away with the type tags by just using +k for one node
    type and -k for the other. (But it may be useful to leave the
    possibility of the current vector switches for very dense switch
    nodes, which could be used in the case where k equals the domain
    size of the variable in question.)

  - More modestly, we could stick with the current polymorphic code,
    but just use more types of nodes, such as switch nodes that stores
    a vector of (value, child) pairs to be scanned linearly or with
    binary search.

  - We can also try to optimize memory usage of the existing nodes
    further, e.g. by replacing vectors with something smaller, like a
    zero-terminated heap-allocated array.
*/

namespace operator_generator {

inline OperatorID min_op_id(OperatorID &op_id1, OperatorID &op_id2) {
    if (op_id1.get_index() == -1) {
        return op_id2;
    } else if (op_id2.get_index() == -1) {
        return op_id1;
    } else {
        return (op_id1.get_index() < op_id2.get_index())? op_id1: op_id2;
    }
}

inline OperatorID max_op_id(OperatorID &op_id1, OperatorID &op_id2) {
    if (op_id1.get_index() == -1) {
        return op_id2;
    } else if (op_id2.get_index() == -1) {
        return op_id1;
    } else {
        return (op_id1.get_index() > op_id2.get_index())? op_id1: op_id2;
    }
}

GeneratorForkBinary::GeneratorForkBinary(
    unique_ptr<GeneratorBase> generator1,
    unique_ptr<GeneratorBase> generator2)
    : generator1(move(generator1)),
      generator2(move(generator2)) {
    /* There is no reason to use a fork if only one of the generators exists.
       Use the existing generator directly if one of them exists or a nullptr
       otherwise. */
    assert(this->generator1);
    assert(this->generator2);
}

OperatorID GeneratorForkBinary::generate_min_applicable_op(
        const vector<int> &state,
        bool reject_unassigned) const {
    OperatorID op_id1 = generator1->generate_min_applicable_op(state, reject_unassigned);
    OperatorID op_id2 = generator2->generate_min_applicable_op(state, reject_unassigned);

    return min_op_id(op_id1, op_id2);
}
void GeneratorForkBinary::generate_applicable_ops(
    const vector<int> &state, vector<OperatorID> &applicable_ops) const {
    generator1->generate_applicable_ops(state, applicable_ops);
    generator2->generate_applicable_ops(state, applicable_ops);
}

GeneratorForkMulti::GeneratorForkMulti(vector<unique_ptr<GeneratorBase>> children)
    : children(move(children)) {
    /* Note that we permit 0-ary forks as a way to define empty
       successor generators (for tasks with no operators). It is
       the responsibility of the factory code to make sure they
       are not generated in other circumstances. */
    assert(this->children.empty() || this->children.size() >= 2);
}

OperatorID GeneratorForkMulti::generate_min_applicable_op(
        const vector<int> &state,
        bool reject_unassigned) const {
    OperatorID min_id(-1);
    for (const auto &generator : children) {
        OperatorID op_id = generator->generate_min_applicable_op(
                state, reject_unassigned);
        min_id = min_op_id(min_id, op_id);
    }
    return min_id;
}

void GeneratorForkMulti::generate_applicable_ops(
    const vector<int> &state, vector<OperatorID> &applicable_ops) const {
    for (const auto &generator : children)
        generator->generate_applicable_ops(state, applicable_ops);
}


GeneratorSwitchBase::GeneratorSwitchBase(
        int switch_var_id, bool covers_all_values)
        : switch_var_id(switch_var_id),
          covers_all_values(covers_all_values) {}

GeneratorSwitchVector::GeneratorSwitchVector(
    int switch_var_id, vector<unique_ptr<GeneratorBase>> &&generator_for_value,
    bool covers_all_values)
    : GeneratorSwitchBase(switch_var_id, covers_all_values),
      generator_for_value(move(generator_for_value)) {
}

OperatorID GeneratorSwitchVector::generate_min_applicable_op(
        const vector<int> &state,
        bool reject_unassigned) const {
    int val = state[switch_var_id];
    if (val != PartialAssignment::UNASSIGNED) {
        const unique_ptr<GeneratorBase> &generator_for_val = generator_for_value[val];
        if (generator_for_val) {
            return generator_for_val->generate_min_applicable_op(state, reject_unassigned);
        }
        return OperatorID(-1);
    } else if (reject_unassigned) {
        if (covers_all_values) {
            // choose worst best case
            OperatorID max_id(-1);
            for (auto &generator_for_val: generator_for_value) {
                if (generator_for_val) {
                    OperatorID op_id = generator_for_val->generate_min_applicable_op(state, reject_unassigned);
                    max_id = max_op_id(max_id, op_id);
                }
            }
            return max_id;
        } else {
            return OperatorID(-1);
        }
    } else {
        OperatorID min_id(-1);
        for (auto &generator_for_val: generator_for_value) {
            if (generator_for_val) {
                OperatorID op_id = generator_for_val->generate_min_applicable_op(state, reject_unassigned);
                min_id = min_op_id(min_id, op_id);
            }
        }
        return min_id;
    }
}
void GeneratorSwitchVector::generate_applicable_ops(
    const vector<int> &state, vector<OperatorID> &applicable_ops) const {
    //todo
    int val = state[switch_var_id];
    if (val != PartialAssignment::UNASSIGNED) {
        const unique_ptr<GeneratorBase> &generator_for_val = generator_for_value[val];
        if (generator_for_val) {
            generator_for_val->generate_applicable_ops(state, applicable_ops);
        }
    } else {
       for (auto &generator_for_val: generator_for_value) {
            if (generator_for_val) {
                generator_for_val->generate_applicable_ops(state, applicable_ops);
            }
        }
    }
}

GeneratorSwitchHash::GeneratorSwitchHash(
    int switch_var_id,
    unordered_map<int, unique_ptr<GeneratorBase>> &&generator_for_value,
    bool covers_all_values)
    : GeneratorSwitchBase(switch_var_id, covers_all_values),
      generator_for_value(move(generator_for_value)) {
}

OperatorID GeneratorSwitchHash::generate_min_applicable_op(
        const vector<int> &state,
        bool reject_unassigned) const {
    int val = state[switch_var_id];
    if (val != PartialAssignment::UNASSIGNED) {
        const auto &child = generator_for_value.find(val);
        if (child != generator_for_value.end()) {
            const unique_ptr<GeneratorBase> &generator_for_val = child->second;
            return generator_for_val->generate_min_applicable_op(state, reject_unassigned);
        }
        return OperatorID(-1);
    } else if(reject_unassigned) {
        if (covers_all_values) {
            OperatorID max_id(-1);
            for (auto &iter: generator_for_value){
                OperatorID op_id = iter.second->generate_min_applicable_op(state, reject_unassigned);
                max_id = max_op_id(max_id, op_id);
            }
            return max_id;
        } else {
            return OperatorID(-1);
        }
    } else {
        OperatorID min_id(-1);
        for (auto &iter: generator_for_value){
            OperatorID op_id = iter.second->generate_min_applicable_op(state, reject_unassigned);
            min_id = min_op_id(min_id, op_id);
        }
        return min_id;
    }
}

void GeneratorSwitchHash::generate_applicable_ops(
    const vector<int> &state, vector<OperatorID> &applicable_ops) const {
    int val = state[switch_var_id];
    if (val != PartialAssignment::UNASSIGNED) {
        const auto &child = generator_for_value.find(val);
        if (child != generator_for_value.end()) {
            const unique_ptr<GeneratorBase> &generator_for_val = child->second;
            generator_for_val->generate_applicable_ops(state, applicable_ops);
        }
    } else {
        for (auto &iter: generator_for_value){
            iter.second->generate_applicable_ops(state, applicable_ops);
        }
    }
}

GeneratorSwitchSingle::GeneratorSwitchSingle(
    int switch_var_id, int value, unique_ptr<GeneratorBase> generator_for_value,
    bool covers_all_values)
    : GeneratorSwitchBase(switch_var_id, covers_all_values),
      value(value),
      generator_for_value(move(generator_for_value)) {
}

OperatorID GeneratorSwitchSingle::generate_min_applicable_op(
        const vector<int> &state,
        bool reject_unassigned) const {
    int val = state[switch_var_id];
    if ((val == PartialAssignment::UNASSIGNED && (!reject_unassigned || covers_all_values)) ||
         value == val) {
        return generator_for_value->generate_min_applicable_op(state, reject_unassigned);
    }
    return OperatorID(-1);
}
void GeneratorSwitchSingle::generate_applicable_ops(
    const vector<int> &state, vector<OperatorID> &applicable_ops) const {
    if (value == state[switch_var_id] || state[switch_var_id] == PartialAssignment::UNASSIGNED) {
        generator_for_value->generate_applicable_ops(state, applicable_ops);
    }
}

GeneratorLeafVector::GeneratorLeafVector(vector<OperatorID> &&applicable_operators)
    : applicable_operators(move(applicable_operators)) {
}

OperatorID GeneratorLeafVector::generate_min_applicable_op(
        const vector<int> &, bool) const {
    return (applicable_operators.empty()) ? OperatorID(-1): applicable_operators.back();
}

void GeneratorLeafVector::generate_applicable_ops(
    const vector<int> &, vector<OperatorID> &applicable_ops) const {
    /*
      In our experiments (issue688), a loop over push_back was faster
      here than doing this with a single insert call because the
      containers are typically very small. However, we have changed
      the container type from list to vector since then, so this might
      no longer apply.
    */
    for (OperatorID id : applicable_operators) {
        applicable_ops.push_back(id);
    }
}

GeneratorLeafSingle::GeneratorLeafSingle(OperatorID applicable_operator)
    : applicable_operator(applicable_operator) {
}

OperatorID GeneratorLeafSingle::generate_min_applicable_op(
        const vector<int> &, bool) const {
    return applicable_operator;
}
void GeneratorLeafSingle::generate_applicable_ops(
    const vector<int> &, vector<OperatorID> &applicable_ops) const {
    applicable_ops.push_back(applicable_operator);
}
}
