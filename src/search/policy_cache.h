#ifndef POLICY_CACHE_H
#define POLICY_CACHE_H

#include "policy_result.h"

#include <unordered_map>

class Policy;

using PolicyResults = std::unordered_map<Policy *, PolicyResult>;

/*
  Store evaluation results for evaluators.
*/
class PolicyCache {
    PolicyResults policy_results;

public:
    PolicyResult &operator[](Policy *policy);

    template<class Callback>
    void for_each_policy_result(const Callback &callback) const {
        for (const auto &element : policy_results) {
            const Policy *policy = element.first;
            const PolicyResult &result = element.second;
            callback(policy, result);
        }
    }
};

#endif
