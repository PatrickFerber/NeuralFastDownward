#ifndef SEARCH_ENGINES_SAMPLING_SEARCH_H
#define SEARCH_ENGINES_SAMPLING_SEARCH_H

#include "sampling_state_engine.h"

#include "../open_list.h"

#include "../sampling_techniques/sampling_technique.h"

#include <deque>
#include <functional>
#include <memory>
#include <ostream>
#include <sstream>
#include <tree.hh>
#include <vector>

class Evaluator;
class Heuristic;
class PruningMethod;

namespace options {
class Options;
struct ParseNode;
class Predefinitions;
class Registry;
using ParseTree = tree<ParseNode>;
}


namespace sampling_engine {

enum SampleType {
    TRAJECTORY_SOLUTION, TRAJECTORY_OTHER, STATE_OTHER
};


class SamplingSearch : public SamplingStateEngine {
protected:
    // Internal
    const options::ParseTree search_parse_tree;
    options::Registry registry;
    options::Predefinitions predefinitions;

    // Sample Sources
    const bool store_solution_trajectories;
    const bool store_other_trajectories;
    const bool store_all_states;

    const bool store_initial_state;
    const bool store_intermediate_state;
    const bool expand_trajectory;

    // What to store
    const bool store_expansions;
    const bool store_expansions_unsolved;
    const bool skip_goal_field;
    const bool skip_snd_state_field;
    const bool skip_action_field;
    const bool add_unsolved_samples;
    const std::vector<std::string> use_evaluators;
    const std::string problem_hash;
    const int network_reload_frequency;
    int network_reload_count = 0;
    const std::string constructed_sample_file_header;
    const std::string entry_meta_general;
    const std::string entry_meta_heuristics;
    
    std::shared_ptr<SearchEngine> engine;
    std::vector<std::shared_ptr<Evaluator>> ptr_use_evaluators;
    utils::HashMap<int, size_t> successfully_solved;
    utils::HashMap<int, std::deque<bool>> successfully_solved_history;
    const size_t successfully_solved_history_size = 500;
    const size_t successfully_solved_increment_threshold = successfully_solved_history_size * 0.9;
    
    /* Internal Methods*/
    void next_engine();
    void update_solved_log(std::vector<std::string> &samples);
    std::vector<std::string> extract_samples();
    std::string extract_modification_hash(State init, GoalsProxy goals) const;
    std::string construct_meta(
            size_t modification_hash,
            const std::string &sample_type,
            const std::string &snd_state);
    void add_entry(
        std::vector<std::string> &new_entries, const std::string &meta,
        const State &state,
        const std::string &goal, const StateID &second_state_id,
        const OperatorID &op_id, const std::vector<int> *heuristics,
        const OperatorsProxy &ops, const StateRegistry &sr);
    int extract_entries_trajectories(
        std::vector<std::string> &new_entries,
        const StateRegistry &sr, const OperatorsProxy &ops,
        const std::string &meta_init, const std::string &meta_inter,
        const Plan &plan, const Trajectory &trajectory,
        const std::string &pddl_goal = "");
    int extract_entries_all_states(
        std::vector<std::string> &new_entries,
        const StateRegistry &sr, OperatorsProxy &ops, const SearchSpace &ss,
        const std::string &meta, StateID &sid,
        const std::string &goal_description);
    
    /* Overwritten Methods*/
    virtual void initialize() override;
    std::vector<std::string> sample(
        std::shared_ptr<AbstractTask> task) override;
    virtual std::string sample_file_header() const override;

    

public:
    explicit SamplingSearch(const options::Options &opts);
    virtual ~SamplingSearch() = default;

    static void add_sampling_search_options(options::OptionParser &parser);
};
}
#endif
