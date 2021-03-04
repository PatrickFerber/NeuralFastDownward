#include "abstract_task.h"

#include "per_task_information.h"
#include "plugin.h"

#include <iostream>
#include <sstream>

using namespace std;

const FactPair FactPair::no_fact = FactPair(-1, -1);

ostream &operator<<(ostream &os, const FactPair &fact_pair) {
    os << fact_pair.var << "=" << fact_pair.value;
    return os;
}


string AbstractTask::get_sas() const {
    ostringstream sas;
    bool use_metric = false;
    for (int idx_op = 0; idx_op < get_num_operators(); ++idx_op) {
        if (get_operator_cost(idx_op, false) != 1) {
            use_metric = true;
            break;
        }
    }

    // SAS Head
    sas << "begin_version" << endl
        << "3" << endl
        << "end_version" << endl
        << "begin_metric" << endl
        << use_metric << endl
        << "end_metric" << endl;

    // Variable section
    sas << get_num_variables() << endl;
    for (int var = 0; var < get_num_variables(); ++var) {
        sas << "begin_variable" << endl
            << get_variable_name(var) << endl
            << get_variable_axiom_layer(var) << endl
            << get_variable_domain_size(var) << endl;
        for (int value = 0; value < get_variable_domain_size(var); ++value) {
            sas << get_fact_name(FactPair(var, value)) << endl;
        }
        sas << "end_variable" << endl;
    }

    // Mutex section
    sas << "INVALID" << endl << "MUTEX" << endl << "SECTION" << endl;

    // Initial state section
    sas << "begin_state" << endl;
    for (int value : get_initial_state_values()) {
        sas << value << endl;
    }
    sas << "end_state" << endl;

    // Goal Section
    sas << "begin_goal" << endl
        << get_num_goals() << endl;
    for (int idx_goal = 0; idx_goal < get_num_goals(); ++idx_goal) {
        const FactPair goal_fact = get_goal_fact(idx_goal);
        sas << goal_fact.var << " " << goal_fact.value << endl;
    }
    sas << "end_goal" << endl;

    // Operator section
    sas << get_num_operators() << endl;
    for (int idx_op = 0; idx_op < get_num_operators(); ++idx_op) {
        sas << "begin_operator" << endl
            << get_operator_name(idx_op, false) << endl;
        // Preconditions
        sas << get_num_operator_preconditions(idx_op, false) << endl;
        for (int idx_pre = 0;
            idx_pre < get_num_operator_preconditions(idx_op, false);
            ++idx_pre) {
            FactPair pre = get_operator_precondition(idx_op, idx_pre, false);
            sas << pre.var << " " << pre.value << endl;
            
        }
        // Operator effects
        sas << get_num_operator_effects(idx_op, false) << endl;
        for (int idx_eff = 0;
            idx_eff < get_num_operator_effects(idx_op, false);
            ++idx_eff) {
            // Operator effect conditions
            sas << get_num_operator_effect_conditions(idx_op, idx_eff, false);
            for (int idx_eff_cnd = 0;
                idx_eff_cnd < get_num_operator_effect_conditions(
                idx_op, idx_eff, false);
                ++idx_eff_cnd){
                FactPair eff_cnd = get_operator_effect_condition(
                    idx_op, idx_eff, idx_eff_cnd, false);
                sas << " " <<eff_cnd.var << " " << eff_cnd.value;
            }
            // Operator precondition, variable, new value
            FactPair eff = get_operator_effect(idx_op, idx_eff, false);
            sas << eff.var << " " << -1 << " " << eff.value << endl;
        }
        sas << get_operator_cost(idx_op, false) << endl
            << "end_operator" << endl;
    }

    // Axiom section
    sas << get_num_axioms() << endl;
    for (int idx_ax = 0; idx_ax < get_num_axioms(); ++idx_ax) {
        sas << "begin_rule" << endl;
        // Preconditions
        sas << get_num_operator_preconditions(idx_ax, true) << endl;
        for (int idx_pre = 0;
            idx_pre < get_num_operator_preconditions(idx_ax, true);
            ++idx_pre) {
            FactPair pre = get_operator_precondition(idx_ax, idx_pre, true);
            sas << pre.var << " " << pre.value << endl;
            
        }
        // Axiom head
        assert (get_num_operator_effects(idx_ax, true) == 1);
        FactPair eff = get_operator_effect(idx_ax, 1, true);
        sas << eff.var << " " << -1 << " " << eff.value << endl
            << "end_rule" << endl;
    }

    return sas.str();
}

static PluginTypePlugin<AbstractTask> _type_plugin(
    "AbstractTask",
    // TODO: Replace empty string by synopsis for the wiki page.
    "");
