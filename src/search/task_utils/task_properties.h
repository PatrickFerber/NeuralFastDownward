#ifndef TASK_UTILS_TASK_PROPERTIES_H
#define TASK_UTILS_TASK_PROPERTIES_H

#include "../per_task_information.h"
#include "../task_proxy.h"
#include "../utils/logging.h"

#include "../algorithms/int_packer.h"

#include <algorithm>
#include <iostream>
#include <limits>

namespace task_properties {

inline bool is_applicable(OperatorProxy op, const PartialAssignment &partial_assignment) {
    for (FactProxy precondition : op.get_preconditions()) {
        if (partial_assignment[precondition.get_variable()] != precondition)
            return false;
    }
    return true;
}

inline bool is_applicable(OperatorProxy op, const State &state) {
    for (FactProxy precondition : op.get_preconditions()) {
        if (state[precondition.get_variable()] != precondition)
            return false;
    }
    return true;
}

inline bool is_goal_state(TaskProxy task, const State &state) {
    for (FactProxy goal : task.get_goals()) {
        if (state[goal.get_variable()] != goal)
            return false;
    }
    return true;
}

inline bool is_goal_assignment(
        const TaskProxy &task, const PartialAssignment &partial_assignment) {
    for (FactProxy goal : task.get_goals()) {
        if (partial_assignment[goal.get_variable()] != goal)
            return false;
    }
    return true;
}

inline bool is_strips_fact(const std::string &fact_name) {
    return fact_name != "<none of those>" &&
        fact_name.rfind("NegatedAtom", 0) == std::string::npos;
}

inline bool is_strips_fact(const AbstractTask *task, const FactPair &fact_pair) {
    return is_strips_fact(task->get_fact_name(fact_pair));
}

extern std::vector<FactPair> get_strips_fact_pairs(const AbstractTask *task);

/*
  Return true iff all operators have cost 1.

  Runtime: O(n), where n is the number of operators.
*/
extern bool is_unit_cost(TaskProxy task);

// Runtime: O(1)
extern bool has_axioms(TaskProxy task);

/*
  Report an error and exit with ExitCode::UNSUPPORTED if the task has axioms.
  Runtime: O(1)
*/
extern void verify_no_axioms(TaskProxy task);

// Runtime: O(n), where n is the number of operators.
extern bool has_conditional_effects(TaskProxy task);

/*
  Report an error and exit with ExitCode::UNSUPPORTED if the task has
  conditional effects.
  Runtime: O(n), where n is the number of operators.
*/
extern void verify_no_conditional_effects(TaskProxy task);

extern std::vector<int> get_operator_costs(const TaskProxy &task_proxy);

template <typename AnyTaskProxy>
double get_average_operator_cost(AnyTaskProxy task_proxy) {
    double average_operator_cost = 0;
    for (OperatorProxy op : task_proxy.get_operators()) {
        average_operator_cost += op.get_cost();
    }
    average_operator_cost /= task_proxy.get_operators().size();
    return average_operator_cost;
}

template <typename AnyTaskProxy>
int get_min_operator_cost(AnyTaskProxy task_proxy) {
    int min_cost = std::numeric_limits<int>::max();
    for (OperatorProxy op : task_proxy.get_operators()) {
        min_cost = std::min(min_cost, op.get_cost());
    }
    return min_cost;
}
/*
  Return the number of facts of the task.
  Runtime: O(n), where n is the number of state variables.
*/
extern int get_num_facts(const TaskProxy &task_proxy);

/*
  Return the total number of effects of the task, including the
  effects of axioms.
  Runtime: O(n), where n is the number of operators and axioms.
*/
extern int get_num_total_effects(const TaskProxy &task_proxy);

template<class FactProxyCollection>
std::vector<FactPair> get_fact_pairs(const FactProxyCollection &facts) {
    std::vector<FactPair> fact_pairs;
    fact_pairs.reserve(facts.size());
    for (FactProxy fact : facts) {
        fact_pairs.push_back(fact.get_pair());
    }
    return fact_pairs;
}

extern void print_variable_statistics(const TaskProxy &task_proxy);
template <typename T>
void dump_pddl(const State &state, T &stream = utils::g_log, const std::string &separator = "\n", bool skip_undefined = false){
    for (FactProxy fact : state) {
        std::string fact_name = fact.get_name();
        if (skip_undefined and fact_name == "<undefined>") {
            continue;
        }
        if (fact_name != "<none of those>")
            stream << fact_name << separator;
    }
    stream << std::flush;
}
template <typename T>
void dump_fdr(const State &state, T &stream = utils::g_log, const std::string &separator = "\n"){
    for (FactProxy fact : state) {
        VariableProxy var = fact.get_variable();
        stream << "  #" << var.get_id() << " [" << var.get_name() << "] -> "
               << fact.get_value() << separator;
    }
    stream << std::flush;
}
template <typename T>
void dump_pddl(const PartialAssignment &state, T &stream = utils::g_log, const std::string &separator = "\n"){
    for (unsigned int var = 0; var < state.size(); ++var) {
        if (state.assigned(var)) {
            FactProxy fact = state[var];
            std::string fact_name = fact.get_name();
            if (fact_name != "<none of those>")
                stream << fact_name << separator;
        }
    }
    stream << std::flush;
}
template <typename T>
void dump_fdr(const PartialAssignment &state, T &stream = utils::g_log, const std::string &separator = "\n"){
    for (unsigned int var = 0; var < state.size(); ++var) {
        if (state.assigned(var)) {
            FactProxy fact = state[var];
            VariableProxy var = fact.get_variable();
            stream << "  #" << var.get_id() << " [" << var.get_name() << "] -> "
                   << fact.get_value() << separator;
        }
    }
    stream << std::flush;
}
template <typename T>
void dump_goals(const GoalsProxy &goals, T &stream = utils::g_log, const std::string &separator = "\n"){
    stream << "Goal conditions:" << separator;
    for (FactProxy goal : goals) {
        stream << "  " << goal.get_variable().get_name() << ": "
               << goal.get_value() << separator;
    }
    stream << std::flush;
}
template <typename T>
void dump_task(const TaskProxy &task_proxy, T &stream = utils::g_log, const std::string &separator = "\n"){
    OperatorsProxy operators = task_proxy.get_operators();
    int min_action_cost = std::numeric_limits<int>::max();
    int max_action_cost = 0;
    for (OperatorProxy op : operators) {
        min_action_cost = std::min(min_action_cost, op.get_cost());
        max_action_cost = std::max(max_action_cost, op.get_cost());
    }
    stream << "Min action cost: " << min_action_cost << separator;
    stream << "Max action cost: " << max_action_cost << separator;

    VariablesProxy variables = task_proxy.get_variables();
    stream << "Variables (" << variables.size() << "):" << separator;
    for (VariableProxy var : variables) {
        stream << "  " << var.get_name()
               << " (range " << var.get_domain_size() << ")" << separator;
        for (int val = 0; val < var.get_domain_size(); ++val) {
            stream << "    " << val << ": " << var.get_fact(val).get_name() << separator;
        }
    }
    State initial_state = task_proxy.get_initial_state();
    stream << "Initial state (PDDL):" << separator;
    dump_pddl(initial_state, stream, separator);
    stream << "Initial state (FDR):" << separator;
    dump_fdr(initial_state, stream, separator);
    dump_goals(task_proxy.get_goals(), stream, separator);
    stream << std::flush;
}

//<<<<
    inline void dump_pddl(const State &state, std::ostream &stream = std::cout, const std::string &separator = "\n", bool skip_undefined = false){
        dump_pddl<std::ostream>(state, stream, separator, skip_undefined);
    }
    inline void dump_fdr(const State &state, std::ostream &stream = std::cout, const std::string &separator = "\n") {
        dump_fdr<std::ostream>(state, stream, separator);
    }
    inline void dump_pddl(const PartialAssignment &state, std::ostream &stream = std::cout, const std::string &separator = "\n") {
        dump_pddl<std::ostream>(state, stream, separator);
    }
    inline void dump_fdr(const PartialAssignment &state, std::ostream &stream = std::cout, const std::string &separator = "\n") {
        dump_fdr<std::ostream>(state, stream, separator);
    }
    inline void dump_goals(const GoalsProxy &goals, std::ostream &stream = std::cout, const std::string &separator = "\n") {
        dump_goals<std::ostream>(goals, stream, separator);
    }
    inline void dump_task(const TaskProxy &task_proxy, std::ostream &stream = std::cout, const std::string &separator = "\n") {
        dump_task<std::ostream>(task_proxy, stream, separator);
    }

    inline void dump_pddl(const State &state, utils::Log log, const std::string &separator = "\n", bool skip_undefined = false){
        dump_pddl<utils::Log>(state, log, separator, skip_undefined);
    }
    inline void dump_fdr(const State &state, utils::Log log, const std::string &separator = "\n") {
        dump_fdr<utils::Log>(state, log, separator);
    }
    inline void dump_pddl(const PartialAssignment &state, utils::Log log, const std::string &separator = "\n") {
        dump_pddl<utils::Log>(state, log, separator);
    }
    inline void dump_fdr(const PartialAssignment &state, utils::Log log, const std::string &separator = "\n") {
        dump_fdr<utils::Log>(state, log, separator);
    }
    inline void dump_goals(const GoalsProxy &goals, utils::Log log, const std::string &separator = "\n") {
        dump_goals<utils::Log>(goals, log, separator);
    }
    inline void dump_task(const TaskProxy &task_proxy, utils::Log log, const std::string &separator = "\n") {
        dump_task<utils::Log>(task_proxy, log, separator);
    }
//>>>>

extern PerTaskInformation<int_packer::IntPacker> g_state_packers;
}

#endif
