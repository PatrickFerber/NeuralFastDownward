#ifndef NEURAL_NETWORKS_STATE_GOAL_NETWORK_H
#define NEURAL_NETWORKS_STATE_GOAL_NETWORK_H

#include "state_network.h"

#include "../heuristic.h"
#include "../option_parser.h"

#include <tuple>
#include <vector>

namespace neural_networks {
/**
 * This network takes as input a state and the goal description (the goal
 * description is given by telling for every predicate in the state 1 if it is
 * part of the goal and 0 otherwise).
 * The input state can be larger than the state Fast Downward works on. Via
 * command line arguments the predicates of the state and their default values
 * can be described.
 */
class StateGoalNetwork : public StateNetwork {
protected:
    const std::string _goal_layer_name;
    const std::vector<int> goal_defaults;

    tensorflow::Tensor  get_goal_input_tensor(int input_size) const;

public:
    explicit StateGoalNetwork(const Options &opts);
    StateGoalNetwork(const StateGoalNetwork &orig) = delete;
    virtual ~StateGoalNetwork() override;

    virtual void initialize_inputs() override;
};
}
#endif /* NEURAL_NETWORKS_STATE_GOAL_NETWORK_H */
