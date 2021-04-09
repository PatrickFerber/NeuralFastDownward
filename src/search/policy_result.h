#ifndef POLICY_RESULT_H
#define POLICY_RESULT_H

#include "operator_id.h"

#include <limits>
#include <vector>

class PolicyResult {
    std::vector<OperatorID> preferred_operators;
    std::vector<float> operator_preferences;
    bool count_evaluation;
    bool initialized;
public:
    PolicyResult();
    PolicyResult(std::vector<OperatorID> &&preferred_operators,
                 std::vector<float> && operator_preferences,
                 bool count_evaluation);

    bool is_uninitialized() const;
    bool is_dead_end() const;

    bool get_count_evaluation() const;
    const std::vector<OperatorID> &get_preferred_operators() const;
    const std::vector<float> &get_operator_preferences() const;

    void set_preferred_operators(std::vector<OperatorID> &&preferred_operators);
    void set_operator_preferences(std::vector<float> &&operator_preferences);
    void set_count_evaluation(bool count_eval);
};

#endif
