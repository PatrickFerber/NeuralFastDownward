#include "regression_task_proxy.h"

#include "../task_utils/task_properties.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

using namespace std;

RegressionCondition::RegressionCondition(int var, int value)
    : data(FactPair(var, value)) { }

bool RegressionCondition::is_satisfied(const PartialAssignment &assignment) const {
    return !assignment.assigned(data.var) || data.value == assignment[data.var].get_value();
    //int current_val = assignment[data.var].get_value();
    //return current_val == PartialAssignment::UNASSIGNED || current_val == data.value;
}

RegressionConditionProxy::RegressionConditionProxy(const AbstractTask &task, const RegressionCondition &condition)
    : task(&task), condition(condition) { }

RegressionConditionProxy::RegressionConditionProxy(const AbstractTask &task, int var_id, int value)
    : RegressionConditionProxy(task, RegressionCondition(var_id, value)) { }

RegressionEffect::RegressionEffect(int var, int value)
    : data(FactPair(var, value)) { }

RegressionEffectProxy::RegressionEffectProxy(const AbstractTask &task, const RegressionEffect &effect)
    : task(&task), effect(effect) { }

RegressionEffectProxy::RegressionEffectProxy(const AbstractTask &task, int var_id, int value)
    : RegressionEffectProxy(task, RegressionEffect(var_id, value)) { }

RegressionOperator::RegressionOperator(OperatorProxy &op)
    : original_index(op.get_id()),
      cost(op.get_cost()),
      name(op.get_name()),
      is_an_axiom(op.is_axiom()) {
    /*
      1) pre(v) = x, eff(v) = y  ==>  rpre(v) = y, reff(v) = x
      2) pre(v) = x, eff(v) = -  ==>  rpre(v) = x, reff(v) = x
                                <=/=>  rpre(v) = x, reff(v) = -, because x could
                                be satisfied by unknown, and should afterwards
                                be x
      3) pre(v) = -, eff(v) = y  ==>  rpre(v) = y, reff(v) = u
      4) (do per partial assignment)
          exists v s.t. eff(v) inter partial assignment is not empty
     */
    unordered_set<int> precondition_vars;
    unordered_map<int, int> vars_to_effect_values;

    for (EffectProxy effect : op.get_effects()) {
        FactProxy fact = effect.get_fact();
        int var_id = fact.get_variable().get_id();
        vars_to_effect_values[var_id] = fact.get_value();
        original_effect_vars.insert(var_id);
    }

    // Handle cases 1 and 2 where preconditions are defined.
    for (FactProxy precondition : op.get_preconditions()) {
        int var_id = precondition.get_variable().get_id();
        precondition_vars.insert(var_id);
        effects.emplace_back(var_id, precondition.get_value());
        if (vars_to_effect_values.count(var_id)) {
            // Case 1, effect defined.
            preconditions.emplace_back(var_id, vars_to_effect_values[var_id]);
        } else {
            // Case 2, effect undefined.
            preconditions.emplace_back(var_id, precondition.get_value());
        }
    }

    // Handle case 3 where preconditions are undefined.
    for (EffectProxy effect : op.get_effects()) {
        FactProxy fact = effect.get_fact();
        int var_id = fact.get_variable().get_id();
        if (precondition_vars.count(var_id) == 0) {
            preconditions.emplace_back(var_id, fact.get_value());
            effects.emplace_back(var_id, PartialAssignment::UNASSIGNED);
        }
    }
}

bool RegressionOperator::achieves_subgoal(const PartialAssignment &assignment) const {
    return any_of(original_effect_vars.begin(), original_effect_vars.end(),
                  [&] (int var_id) {
                      return assignment.assigned(var_id);
                  });
}

bool RegressionOperator::is_applicable(const PartialAssignment &assignment) const {
    return achieves_subgoal(assignment) &&
        all_of(preconditions.begin(), preconditions.end(),
            [&](const RegressionCondition &condition) {
                return condition.is_satisfied(assignment);
        });
}


inline shared_ptr<vector<RegressionOperator>> extract_regression_operators(const AbstractTask &task, TaskProxy &tp) {
    task_properties::verify_no_axioms(tp);
    task_properties::verify_no_conditional_effects(tp);

    auto rops = make_shared<vector<RegressionOperator>>();
    for (OperatorProxy op : OperatorsProxy(task)) {
        RegressionOperator o(op);
        rops->emplace_back(op);
    }
    return rops;
}

RegressionTaskProxy::RegressionTaskProxy(const AbstractTask &task)
    : TaskProxy(task),
      operators(extract_regression_operators(task, *this)) { }

