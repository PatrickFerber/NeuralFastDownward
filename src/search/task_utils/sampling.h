#ifndef TASK_UTILS_SAMPLING_H
#define TASK_UTILS_SAMPLING_H

#include "../task_proxy.h"
#include "regression_task_proxy.h"

#include <functional>
#include <memory>
#include <vector>

class PartialAssignment;
class State;

namespace successor_generator {
class SuccessorGenerator;
}

namespace predecessor_generator {
class PredecessorGenerator;
}

namespace utils {
class RandomNumberGenerator;
}

using DeadEndDetector = std::function<bool (State &)>;
using PartialDeadEndDetector = std::function<bool (PartialAssignment &)>;
using ValidStateDetector = std::function<bool (PartialAssignment &)>;
using PartialAssignmentBias = std::function<int (PartialAssignment &)>;
using StateBias = std::function<int (State &)>;

namespace sampling {
/*
  Sample states with random walks.
*/
class RandomWalkSampler {
    const OperatorsProxy operators;
    const std::unique_ptr<successor_generator::SuccessorGenerator> successor_generator;
    const State initial_state;
    const double average_operator_costs;
    utils::RandomNumberGenerator &rng;

public:
    RandomWalkSampler(
        const TaskProxy &task_proxy,
        utils::RandomNumberGenerator &rng);
    ~RandomWalkSampler();

    /*
      Perform a single random walk and return the last visited state.

      The walk length is taken from a binomial distribution centered around the
      estimated plan length, which is computed as the ratio of the h value of
      the initial state divided by the average operator costs. Whenever a dead
      end is detected or a state has no successors, restart from the initial
      state. The function 'is_dead_end' should return whether a given state is
      a dead end. If omitted, no dead end detection is performed. The 'init_h'
      value should be an estimate of the solution cost.
    */
    State sample_state(
        int init_h,
        const DeadEndDetector &is_dead_end = [](const State &) {return false;}) const;

    State sample_state_length(
        const State &init_state,
        int length,
        const DeadEndDetector &is_dead_end = [](const State &) {return false;},
        bool deprioritize_undoing_steps = false,
        const StateBias *bias = nullptr,
        bool probabilistic_bias=true,
        double adapt_bias=-1
        ) const;

    std::vector<State> sample_states(
        int num_samples,
        int init_h,
        const DeadEndDetector &is_dead_end = [](const State &) {return false;}) const;
};


class PartialAssignmentRegistry{
protected:
    std::vector<PartialAssignment> id2assignment;
    utils::HashMap<PartialAssignment, size_t> assignment2id;

public:
    PartialAssignmentRegistry() {};
    PartialAssignmentRegistry(PartialAssignmentRegistry &&other)
            : id2assignment(std::move(other.id2assignment)),
              assignment2id(std::move(other.assignment2id)) {}


    const PartialAssignment& lookup_by_id(size_t id) const {
        assert(id < id2assignment.size());
        return id2assignment[id];
    }

    int lookup_or_insert_by_assignment(PartialAssignment &partial_assignment){
        auto iter = assignment2id.find(partial_assignment);
        if (iter == assignment2id.end()) {
            assignment2id[partial_assignment] = id2assignment.size();
            id2assignment.push_back(std::move(partial_assignment));
            return id2assignment.size() - 1;
        } else {
            return iter->second;
        }
    }
};


class RandomRegressionWalkSampler {
    const RegressionTaskProxy regression_task_proxy;
    const std::unique_ptr<predecessor_generator::PredecessorGenerator> predecessor_generator;
    const PartialAssignment goals;
    const double average_operator_costs;
    utils::RandomNumberGenerator &rng;

public:
    RandomRegressionWalkSampler(
        const RegressionTaskProxy &regression_task_proxy,
        utils::RandomNumberGenerator &rng);
    ~RandomRegressionWalkSampler();

    /*
      Perform a single random walk and return the last visited state.

      The walk length is taken from a binomial distribution centered around the
      estimated plan length, which is computed as the ratio of the h value of
      the initial state divided by the average operator costs. Whenever a dead
      end is detected or a state has no successors, restart from the initial
      state. The function 'is_dead_end' should return whether a given state is
      a dead end. If omitted, no dead end detection is performed. The 'init_h'
      value should be an estimate of the solution cost.
    */
    PartialAssignment sample_state(
        int init_h,
        bool deprioritize_undoing_steps = false,
        const ValidStateDetector &is_valid_state = [](const PartialAssignment &) {return true;},
        const PartialAssignmentBias *bias = nullptr,
        bool probabilistic_bias=true,
        const PartialDeadEndDetector &is_dead_end = [](const PartialAssignment &) {return false;}) const;

    PartialAssignment sample_state_length(
        const PartialAssignment &goals, int length,
        bool deprioritize_undoing_steps = false,
        const ValidStateDetector &is_valid_state = [](const PartialAssignment &) {return true;},
        const PartialAssignmentBias *bias = nullptr,
        bool probabilistic_bias=true,
        double adapt_bias=-1,
        const PartialDeadEndDetector &is_dead_end = [](const PartialAssignment &) {return false;}) const;

    std::vector<PartialAssignment> sample_states(
        int num_samples,
        int init_h,
        bool deprioritize_undoing_steps = false,
        const ValidStateDetector &is_valid_state = [](const PartialAssignment &) {return true;},
        const PartialAssignmentBias *bias = nullptr,
        bool probabilistic_bias=true,
        const PartialDeadEndDetector &is_dead_end = [](const PartialAssignment &) {return false;}) const;

    std::pair<PartialAssignmentRegistry, utils::HashMap<size_t, int>> sample_area(
        const PartialAssignment &initial,
        const int max_cost,
        const int max_states,
        bool check_mutexes = true
        ) const;
};
}

#endif
