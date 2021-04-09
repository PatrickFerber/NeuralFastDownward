#include "evaluation_context.h"

#include "evaluation_result.h"
#include "evaluator.h"
#include "policy.h"
#include "search_statistics.h"

#include <cassert>

using namespace std;

EvaluationContext::EvaluationContext(
    const EvaluatorCache &cache,
    const PolicyCache &policy_cache,
    const State &state, int g_value,
    bool is_preferred, SearchStatistics *statistics,
    bool calculate_preferred,
    bool report_confidence)
    : cache(cache),
      policy_cache(policy_cache),
      state(state),
      g_value(g_value),
      preferred(is_preferred),
      statistics(statistics),
      calculate_preferred(calculate_preferred),
      report_confidence(report_confidence) {
}


EvaluationContext::EvaluationContext(
    const EvaluationContext &other, int g_value,
    bool is_preferred, SearchStatistics *statistics, bool calculate_preferred, bool report_confidence)
    : EvaluationContext(other.cache, other.policy_cache, other.state, g_value, is_preferred,
                        statistics, calculate_preferred, report_confidence) {
}

EvaluationContext::EvaluationContext(
    const State &state, int g_value, bool is_preferred,
    SearchStatistics *statistics, bool calculate_preferred, bool report_confidence)
    : EvaluationContext(EvaluatorCache(), PolicyCache(), state, g_value, is_preferred,
                        statistics, calculate_preferred, report_confidence) {
}

EvaluationContext::EvaluationContext(
    const State &state,
    SearchStatistics *statistics, bool calculate_preferred, bool report_confidence)
    : EvaluationContext(EvaluatorCache(), PolicyCache(), state, INVALID, false,
                        statistics, calculate_preferred, report_confidence) {
}

const EvaluationResult &EvaluationContext::get_result(Evaluator *evaluator) {
    EvaluationResult &result = cache[evaluator];
    if (result.is_uninitialized()) {
        result = evaluator->compute_result(*this);
        if (statistics &&
            evaluator->is_used_for_counting_evaluations() &&
            result.get_count_evaluation()) {
            statistics->inc_evaluations();
        }
    }
    return result;
}

const PolicyResult &EvaluationContext::get_result(Policy *policy) {
    PolicyResult &result = policy_cache[policy];
    if (result.is_uninitialized()) {
        result = policy->compute_result(*this);
        if (statistics &&
            result.get_count_evaluation()) {
            statistics->inc_evaluations();
        }
    }
    return result;
}

const EvaluatorCache &EvaluationContext::get_cache() const {
    return cache;
}

const State &EvaluationContext::get_state() const {
    return state;
}

int EvaluationContext::get_g_value() const {
    assert(g_value != INVALID);
    return g_value;
}

bool EvaluationContext::is_preferred() const {
    assert(g_value != INVALID);
    return preferred;
}

bool EvaluationContext::is_evaluator_value_infinite(Evaluator *eval) {
    return get_result(eval).is_infinite();
}

bool EvaluationContext::is_policy_dead_end(Policy *policy) {
    return get_result(policy).is_dead_end();
}

int EvaluationContext::get_evaluator_value(Evaluator *eval) {
    int h = get_result(eval).get_evaluator_value();
    assert(h != EvaluationResult::INFTY);
    return h;
}

int EvaluationContext::get_evaluator_value_or_infinity(Evaluator *eval) {
    return get_result(eval).get_evaluator_value();
}

const vector<OperatorID> &
EvaluationContext::get_preferred_operators(Evaluator *eval) {
    return get_result(eval).get_preferred_operators();
}

const vector<OperatorID> &
EvaluationContext::get_preferred_operators(Policy *policy) {
    return get_result(policy).get_preferred_operators();
}


const vector<float> &
EvaluationContext::get_operator_preferences(Policy *policy) {
    return get_result(policy).get_operator_preferences();
}

bool EvaluationContext::get_calculate_preferred() const {
    return calculate_preferred;
}

bool EvaluationContext::get_report_confidence() const {
    return report_confidence;
}
