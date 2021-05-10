#include "task_properties.h"

#include "../utils/memory.h"
#include "../utils/system.h"

#include <algorithm>
#include <iostream>

using namespace std;
using utils::ExitCode;


namespace task_properties {

vector<FactPair> get_strips_fact_pairs(const AbstractTask *task) {
    std::vector<FactPair> facts;
    for (int var = 0; var < task->get_num_variables(); ++var) {
        for (int val = 0; val < task->get_variable_domain_size(var); ++val) {
            facts.emplace_back(var, val);
            if (!task_properties::is_strips_fact(task, facts.back())) {
                facts.pop_back();
            }
        }
    }
    return facts;
}
bool is_unit_cost(TaskProxy task) {
    for (OperatorProxy op : task.get_operators()) {
        if (op.get_cost() != 1)
            return false;
    }
    return true;
}

bool has_axioms(TaskProxy task) {
    return !task.get_axioms().empty();
}

void verify_no_axioms(TaskProxy task) {
    if (has_axioms(task)) {
        cerr << "This configuration does not support axioms!"
             << endl << "Terminating." << endl;
        utils::exit_with(ExitCode::SEARCH_UNSUPPORTED);
    }
}


static int get_first_conditional_effects_op_id(TaskProxy task) {
    for (OperatorProxy op : task.get_operators()) {
        for (EffectProxy effect : op.get_effects()) {
            if (!effect.get_conditions().empty())
                return op.get_id();
        }
    }
    return -1;
}

bool has_conditional_effects(TaskProxy task) {
    return get_first_conditional_effects_op_id(task) != -1;
}

void verify_no_conditional_effects(TaskProxy task) {
    int op_id = get_first_conditional_effects_op_id(task);
    if (op_id != -1) {
        OperatorProxy op = task.get_operators()[op_id];
        cerr << "This configuration does not support conditional effects "
             << "(operator " << op.get_name() << ")!" << endl
             << "Terminating." << endl;
        utils::exit_with(ExitCode::SEARCH_UNSUPPORTED);
    }
}

vector<int> get_operator_costs(const TaskProxy &task_proxy) {
    vector<int> costs;
    OperatorsProxy operators = task_proxy.get_operators();
    costs.reserve(operators.size());
    for (OperatorProxy op : operators)
        costs.push_back(op.get_cost());
    return costs;
}


int get_num_facts(const TaskProxy &task_proxy) {
    int num_facts = 0;
    for (VariableProxy var : task_proxy.get_variables())
        num_facts += var.get_domain_size();
    return num_facts;
}

int get_num_total_effects(const TaskProxy &task_proxy) {
    int num_effects = 0;
    for (OperatorProxy op : task_proxy.get_operators())
        num_effects += op.get_effects().size();
    num_effects += task_proxy.get_axioms().size();
    return num_effects;
}

void print_variable_statistics(const TaskProxy &task_proxy) {
    const int_packer::IntPacker &state_packer = g_state_packers[task_proxy];

    int num_facts = 0;
    VariablesProxy variables = task_proxy.get_variables();
    for (VariableProxy var : variables)
        num_facts += var.get_domain_size();

    utils::g_log << "Variables: " << variables.size() << endl;
    utils::g_log << "FactPairs: " << num_facts << endl;
    utils::g_log << "Bytes per state: "
                 << state_packer.get_num_bins() * sizeof(int_packer::IntPacker::Bin)
                 << endl;
}

PerTaskInformation<int_packer::IntPacker> g_state_packers(
    [](const TaskProxy &task_proxy) {
        VariablesProxy variables = task_proxy.get_variables();
        vector<int> variable_ranges;
        variable_ranges.reserve(variables.size());
        for (VariableProxy var : variables) {
            variable_ranges.push_back(var.get_domain_size());
        }
        return utils::make_unique_ptr<int_packer::IntPacker>(variable_ranges);
    }
    );
}
