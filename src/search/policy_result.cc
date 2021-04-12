#include "policy_result.h"

using namespace std;


PolicyResult::PolicyResult() : initialized(false) {}
PolicyResult::PolicyResult(
        vector<OperatorID> &&preferred_operators,
        vector<float> &&operator_preferences,
        bool count_evaluation)
    : preferred_operators(move(preferred_operators)),
      operator_preferences(move(operator_preferences)),
      count_evaluation(count_evaluation),
      initialized(true) {}



bool PolicyResult::is_uninitialized() const {
    return !initialized;
}

bool PolicyResult::is_dead_end() const {
    return preferred_operators.empty();
}

const vector<OperatorID> &PolicyResult::get_preferred_operators() const {
    return preferred_operators;
}

const vector<float> &PolicyResult::get_operator_preferences() const {
    return operator_preferences;
}


bool PolicyResult::get_count_evaluation() const {
    return count_evaluation;
}

void PolicyResult::set_preferred_operators(
    vector<OperatorID> &&preferred_ops) {
    initialized = true;
    preferred_operators = move(preferred_ops);
}

void PolicyResult::set_operator_preferences(
        vector<float> &&operator_prefs) {
    operator_preferences = move(operator_prefs);
}

void PolicyResult::set_count_evaluation(bool count_eval) {
    count_evaluation = count_eval;
}
