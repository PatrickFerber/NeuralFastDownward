#ifndef SEARCH_ENGINES_POLICY_SEARCH_H
#define SEARCH_ENGINES_POLICY_SEARCH_H

#include "../evaluation_context.h"
#include "../heuristic.h"
#include "../open_list.h"
#include "../search_engine.h"

#include <utility>
#include <vector>

namespace options {
class Options;
}

class Policy;

namespace search_engines {

/*
  Policy Search, following a given Policy by naively choosing the
  (first of all) most probable operator(s)
*/
class PolicySearch : public SearchEngine {
    std::shared_ptr<Policy> policy;
    std::vector<std::shared_ptr<Evaluator>> dead_end_evaluators;
    const int trajectory_limit;
    EvaluationContext current_context;
    int trajectory_length = 0;

    bool is_dead_end();
protected:
    virtual void initialize() override;
    virtual SearchStatus step() override;

public:
    explicit PolicySearch(const options::Options &opts);
    virtual ~PolicySearch() override;

    virtual void print_statistics() const override;
};
}

#endif
