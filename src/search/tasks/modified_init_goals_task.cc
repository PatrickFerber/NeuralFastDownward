#include "modified_init_goals_task.h"

using namespace std;

namespace extra_tasks {
ModifiedInitGoalsTask::ModifiedInitGoalsTask(
    const shared_ptr<AbstractTask> &parent,
    const vector<int> &&initial_state,
    const vector<FactPair> &&goals)
    : DelegatingTask(parent),
      initial_state(move(initial_state)),
      goals(move(goals)) {
}

int ModifiedInitGoalsTask::get_num_goals() const {
    return goals.size();
}

FactPair ModifiedInitGoalsTask::get_goal_fact(int index) const {
    return goals[index];
}

vector<int> ModifiedInitGoalsTask::get_initial_state_values() const {
    return initial_state;
}
}
