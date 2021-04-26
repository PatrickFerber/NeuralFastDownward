#include "sampling_search.h"

#include "../evaluator.h"
#include "../evaluation_context.h"
#include "../heuristic.h"
#include "../plugin.h"

#include "../task_utils/successor_generator.h"
#include "../task_utils/task_properties.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <set>
#include <string>

using namespace std;

namespace sampling_engine {
static const std::hash<std::string> SHASH;


static void calculate_use_heuristics(
    vector<shared_ptr<Evaluator>> &ptr_use_evaluators,
    const State &state, vector<int> &heuristic_values) {
    for (const shared_ptr<Evaluator> &evaluator: ptr_use_evaluators) {
        EvaluationContext eval_context(state);
        heuristic_values.push_back(
            evaluator->compute_result(eval_context).get_evaluator_value());
    }
}

static size_t calculate_modification_hash(
    const State &init, const GoalsProxy &goals) {
    ostringstream oss;
    task_properties::dump_pddl(init, oss, "\t");
    task_properties::dump_goals(goals, oss, "\t");
    return SHASH(oss.str());
}

string SamplingSearch::construct_meta(
        size_t modification_hash,
        const string &sample_type,
        const string &snd_state) {
    return "{" + entry_meta_general + to_string(modification_hash) + "\", "
           "\"sample_type\": \"" + sample_type + "\", "
           "\"fields\": [{\"name\": \"current_state\", \"type\": \"state\", \"format\": \"FD\"}, " +
           ((skip_goal_field) ? "" : R"({"name": "goals", "type": "state", "format": "FD"}, )") +
           ((skip_snd_state_field) ? "" : (R"({"name": ")" + snd_state + R"(", "type": "state", "format": "FD"}, )")) +
           ((skip_action_field) ? "" : R"({"name": "action", "type": "string"}, )") +
           ((add_unsolved_samples) ? R"({"name": "unsolved", "type": "bool"}, )": "") +
           entry_meta_heuristics + "]}";
}


void SamplingSearch::add_entry(
    vector<string> &new_entries, const string &meta, const State &state,
    const string &goal, const StateID &second_state_id,
    const OperatorID &op_id, const vector<int> *heuristics,
    const OperatorsProxy &ops, const StateRegistry &sr) {
    ostringstream stream;
    if (sample_format == SampleFormat::FIELDS) {
        stream << meta;
        convert_and_push_state(stream, state);

        if (!skip_goal_field) {
            stream << field_separator << goal;
        }
        if (!skip_snd_state_field) {
            stream << field_separator;
            if (second_state_id != StateID::no_state) {
                convert_and_push_state(
                        stream, sr.lookup_state(second_state_id));
            }
        }
        if (!skip_action_field) {
            stream << field_separator;
            if (op_id != OperatorID::no_operator) {
                stream << ops[op_id].get_name();
            }
        }

        if (add_unsolved_samples) {
            stream << field_separator;
            stream << !engine->found_solution();
        }
        if (heuristics != nullptr) {
            for (int h : *heuristics) {
                stream << field_separator << h;
            }
        }
    } else if (sample_format == SampleFormat::CSV) {
        if (heuristics != nullptr) {
            for (int h : *heuristics) {
                stream << h << field_separator;
            }
        }

        if (add_unsolved_samples) {
            stream  << engine->found_solution() << field_separator;
        }

        convert_and_push_state(stream, state);

        if (!skip_goal_field) {
            stream << field_separator << goal;
        }
        if (!skip_snd_state_field) {
            stream << field_separator;
            if (second_state_id != StateID::no_state) {
                convert_and_push_state(
                        stream, sr.lookup_state(second_state_id));
            }
        }
        if (!skip_action_field) {
            stream << field_separator;
            if (op_id != OperatorID::no_operator) {
                stream << ops[op_id].get_name();
            }
        }

    } else {
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
    new_entries.push_back(stream.str());
}



/*
 store_init = store initial state of trajectory
 store_intermediate = number of random intermediate states (including init and
     goal) to store
 store_trajectory = store all states on trajectory. As this includes any of
     the previously chosen states, this sets the previous to store_* to false.
     if store_trajectory is false, then expand_trajectory is set to false.
 expand_trajectory = make all combinations of N states with M-N+1 successor
     states as goal
 pddl_goal = if not given, then the trajectories final state is the goal. If
     given, then this is used as goal UNLESS 'expand_trajectory' is used, then
     the final state is again used as goal
 */
int SamplingSearch::extract_entries_trajectories(
    vector<string> &new_entries,
    const StateRegistry &sr, const OperatorsProxy &ops,
    const string &meta_init, const string &meta_inter,
    const Plan &plan, const Trajectory &trajectory,
    const string &pddl_goal) {
    //!(store_initial_state || store_intermediate_state)
    int counter = 0;
    string string_pddl_goal = pddl_goal;
    bool store_init = store_initial_state;
    int store_intermediate = store_intermediate_state;
    bool store_trajectory = !(store_initial_state || store_intermediate_state);
    bool expand = expand_trajectory;
    if (store_trajectory) {
        store_init = false;
        store_intermediate = 0;
    } else {
        expand = false;
    }
    vector<int> intermediate = rng->choose_n_of_N(store_intermediate, trajectory.size());

    size_t min_idx_goal = (expand) ? (1) : (trajectory.size() - 1);

    for (size_t idx_goal = trajectory.size(); idx_goal-- > min_idx_goal;) {
        if (pddl_goal.empty() || expand) {
            // TODO: Goal is to precise, can we make it more exact via Regression?
            ostringstream stream_pddl_goal;
            convert_and_push_state(
                    stream_pddl_goal, sr.lookup_state(trajectory[idx_goal]));
            string_pddl_goal = stream_pddl_goal.str();
        }
        int heuristic = 0;

        for (size_t idx_init = idx_goal + 1; idx_init-- > 0;) {
            if (idx_init != idx_goal) {
                heuristic += ops[plan[idx_init]].get_cost();
            }
            if (!(store_trajectory ||
                  (store_init && idx_init == 0) ||
                  (store_intermediate &&
                   std::find(intermediate.begin(),
                             intermediate.end(), idx_init) != intermediate.end()))) {
                continue;
            }

            const State init = sr.lookup_state(trajectory[idx_init]);
            const StateID next_state_id = (idx_init == idx_goal) ? StateID::no_state : trajectory[idx_init + 1];
            const OperatorID &op_id = (idx_init == idx_goal)? OperatorID::no_operator : plan[idx_init];

            vector<int> heuristics;
            heuristics.push_back(heuristic);
            // Registered heuristics calculate w.r.t. the true goal. Use only
            // for trajectories ending in the true goal.
            if (idx_goal == trajectory.size() - 1) {
                calculate_use_heuristics(ptr_use_evaluators, init, heuristics);
            } else {
                for (size_t idx_h = 0;
                     idx_h < ptr_use_evaluators.size(); ++idx_h) {
                    heuristics.push_back(Heuristic::NO_VALUE);
                }
            }

            add_entry(new_entries, (idx_init == 0) ? meta_init : meta_inter,
                init, string_pddl_goal, next_state_id,
                op_id, &heuristics, ops, sr);
            counter++;
        }
    }
    return counter;
}


int SamplingSearch::extract_entries_all_states(
    vector<string> &new_entries,
    const StateRegistry &sr, OperatorsProxy &ops, const SearchSpace &ss,
    const string &meta, StateID &sid, const string &goal_description) {
    const State global_state = sr.lookup_state(sid);
    const StateID parent_id = ss.get_parent_id(global_state);
    const OperatorID op_id = ss.get_creating_operator(global_state);

    vector<int> heuristics;
    heuristics.push_back(Heuristic::NO_VALUE);
    calculate_use_heuristics(ptr_use_evaluators, global_state, heuristics);

    add_entry(new_entries, meta, global_state,
              goal_description, parent_id, op_id,
              &heuristics, ops, sr);
    return 1;
}


/* Methods to use in the constructor */

string construct_sample_file_header(
        const SampleFormat &sample_format,
        const string &field_separator) {
    ostringstream header;
    if (sample_format == SampleFormat::FIELDS) {
        header << SAMPLE_FILE_MAGIC_WORD << endl
               << "# Everything in a line after '#' is a comment" << endl
               << "# Entry format: [JSON describing the entry] ([FIELD]"
               << field_separator << ")*" << endl
               << "# Entry attributes:" << endl
               << "#\tproblem_hash: hash of the problem file" << endl
               << "#\tmodification_hash: has for the modification done on the initial state and goal" << endl
               << "#\tdelimiter: string used to separate the different fields of the entry" << endl
               << "#\tsample_type: describes where this entry comes from. Available:"
               << " init (initial state of a found solution), "
               << "plan (from the found solution plan), trajectory (from a "
               << "trajectory any search algorithm could log), or visited (a "
               << "state visited during the search)" << endl
               << "#\tfields: List of one dictionary per field to describe the data entry" << endl
               << "# General Field attributes:" << endl
               << "#\tname: name for the field" << endl
               << "#\ttype: type of the field. Available types: \"state\", \"heuristic\", \"int\", \"string\"" << endl
               << "#State Field attributes:" << endl
               << "#\tformat: format in which the state is stored, e.g. \"FD\" for the format Fast Downward uses"
               << endl;
    } else {
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
    return header.str();
}

string construct_meta_general(
        const string & field_separator,
        const string & problem_hash) {
    return string(R"("delimiter": ")") + field_separator + "\", "
        "\"problem_hash\": \"" + problem_hash + "\", "
        "\"modification_hash\": \"";
}

string construct_meta_heuristics(const vector<string> &use_evaluators) {
    ostringstream heuristic_fields;
    heuristic_fields << R"({"name": "hplan", "type": "heuristic"})";
    for (const string &hname : use_evaluators) {
        heuristic_fields << R"(, {"name": ")" << hname
                         << R"(", "type": "heuristic"})";
    }
    return heuristic_fields.str();
}

/* Constructor */
SamplingSearch::SamplingSearch(const options::Options &opts)
    : SamplingSearchBase(opts),
      store_solution_trajectories(opts.get<bool>("store_solution_trajectory")),
      store_other_trajectories(opts.get<bool>("store_other_trajectories")),
      store_all_states(opts.get<bool>("store_all_states")),

      store_initial_state(opts.get<bool>("store_initial_state")),
      store_intermediate_state(opts.get<bool>("store_intermediate_state")),
      expand_trajectory(opts.get<bool>("expand_trajectory")),

      store_expansions(opts.get<bool>("store_expansions")),
      store_expansions_unsolved(opts.get<bool>("store_expansions_unsolved")),
      skip_goal_field(opts.get<bool>("skip_goal_field")),
      skip_snd_state_field(opts.get<bool>("skip_second_state_field")),
      skip_action_field(opts.get<bool>("skip_action_field")),
      add_unsolved_samples(opts.get<bool>("add_unsolved_samples")),
      use_evaluators(opts.get_list<string>("use_predefined_evaluators")),
      problem_hash(opts.get<string>("hash")),
      network_reload_frequency(opts.get<int>("network_reload_frequency")),
      constructed_sample_file_header(
              sample_format == SampleFormat::FIELDS ?
              construct_sample_file_header(sample_format, field_separator) :
              SamplingStateEngine::sample_file_header()),
      entry_meta_general(construct_meta_general(
        field_separator, problem_hash)),
      entry_meta_heuristics(construct_meta_heuristics(use_evaluators)),
      successfully_solved_history_size(opts.get<int>("upgrade_history_size")),
      successfully_solved_increment_threshold(
              successfully_solved_history_size * opts.get<double>("upgrade_solved_rate")){
    if (sample_format != SampleFormat::FIELDS &&
        sample_format != SampleFormat::CSV) {
        cerr << "Invalid sample format for sampling_search" << endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
    if (state_format != StateFormat::PDDL &&
        state_format != StateFormat::FDR) {
        cerr << "invalid state format for sampling_search" << endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
    for (auto &st: sampling_techniques) {
        successfully_solved[st->id] = 0;
        successfully_solved_history[st->id] = deque<bool>();
    }
    if (store_expansions && (
            store_solution_trajectories || store_other_trajectories ||
            store_all_states)) {
        cerr << "Invalid options. 'store_expansions' cannot be used with any"
                "option that stores more than the initial state of the actual"
                "sampling search" << endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
    if ((store_all_states || store_other_trajectories)
            && add_unsolved_samples) {
        cerr << "Storing of unsolved (aka timed out samples) not implemented "
                "for sampling all visited states or states on other "
                "trajectories." << endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
}


vector<string> SamplingSearch::extract_samples() {
    /*For the sampling format, please take a look at sample_file_header*/
    const StateRegistry &sr = engine->get_state_registry();
    const SearchSpace &ss = engine->get_search_space();
    const TaskProxy &tp = engine->get_task_proxy();
    OperatorsProxy ops = tp.get_operators();
    const GoalsProxy gps = tp.get_goals();
    size_t mod_hash = calculate_modification_hash(
        tp.get_initial_state(), gps);

    vector<string> new_entries;

    if (store_expansions){
        if (engine->found_solution() || store_expansions_unsolved) {
            ostringstream stream_pddl_goal;
            gps.dump_pddl(stream_pddl_goal, "\t");
            vector<int> expansions = {engine->get_statistics().get_expanded()};
            add_entry(new_entries,
                      construct_meta(mod_hash, "init", "next_state"),
                      tp.get_initial_state(), stream_pddl_goal.str(),
                      StateID::no_state, OperatorID::no_operator,
                      &expansions, ops, sr);
        } else if (add_unsolved_samples) {
            ostringstream stream_pddl_goal;
            gps.dump_pddl(stream_pddl_goal, "\t");
            vector<int> expansions = {-1};
            add_entry(new_entries,
                      construct_meta(mod_hash, "init", "next_state"),
                      tp.get_initial_state(), stream_pddl_goal.str(),
                      StateID::no_state, OperatorID::no_operator,
                      &expansions, ops, sr);
        }
    }
    if (store_solution_trajectories) {
        if (engine->found_solution()) {
            Trajectory trajectory;
            ss.trace_path(engine->get_goal_state(), trajectory);

            extract_entries_trajectories(
                    new_entries, sr, ops,
                    construct_meta(mod_hash, "init", "next_state"),
                    construct_meta(mod_hash, "inter", "next_state"),
                    engine->get_plan(), trajectory);
        } else if (add_unsolved_samples) {
            ostringstream stream_pddl_goal;
            gps.dump_pddl(stream_pddl_goal, "\t");
            vector<int> h_value = {-1};
            add_entry(new_entries,
                      construct_meta(mod_hash, "init", "next_state"),
                      tp.get_initial_state(), stream_pddl_goal.str(),
                      StateID::no_state, OperatorID::no_operator,
                      &h_value, ops, sr);
        }
    }

    if (store_other_trajectories) {
        const string meta_init = construct_meta(
                mod_hash, "trajectory_init", "next_state");
        const string meta_inter = construct_meta(
                mod_hash, "trajectory_inter", "next_state");

        for (const Path &path : sampling_engine::paths) {
            extract_entries_trajectories(
                new_entries, sr, ops,
                meta_init, meta_inter,
                path.get_plan(), path.get_trajectory());
        }
    }

    if (store_all_states) {
        const string meta_visited = construct_meta(
             mod_hash, "visited", "previous_state");
        ostringstream stream_pddl_goal;
        gps.dump_pddl(stream_pddl_goal, "\t");
        const string pddl_goal = stream_pddl_goal.str();

        for (StateRegistry::const_iterator iter = sr.begin(); iter != sr.end();
             ++iter) {
            StateID state = *iter;
            extract_entries_all_states(
                new_entries, sr, ops, ss, meta_visited,state, pddl_goal);
        }
    }
    return new_entries;
}


void SamplingSearch::next_engine() {
    network_reload_count++;
    if (network_reload_count >= network_reload_frequency) {
        ignore_repredefinitions.erase("network");
        network_reload_count = 0;
    }
    SamplingSearchBase::next_engine();

    ignore_repredefinitions.insert("network");

    ptr_use_evaluators.clear();
    for (const string &name : use_evaluators) {
        ptr_use_evaluators.push_back(
                predefinitions.get<shared_ptr<Evaluator>>(name));
    }
}


void SamplingSearch::post_search(std::vector<std::string> &samples) {
    if ((*current_technique)->has_upgradeable_parameters()) {
        auto &history = successfully_solved_history.find((*current_technique)->id)->second;
        size_t nb_solved = successfully_solved.find((*current_technique)->id)->second;
        history.push_back(engine->found_solution());
        if (engine->found_solution()) {
            nb_solved++;
        }
        while (history.size() > successfully_solved_history_size) {
            if (history.front()) {
                nb_solved--;
            }
            history.pop_front();
        }
        if (nb_solved >= successfully_solved_increment_threshold) {
            (*current_technique)->upgrade_parameters();
            successfully_solved[(*current_technique)->id] = 0;
            history.clear();
            ostringstream oss;
            oss << "#Solves tasks generated by sampling technique "
                << (*current_technique)->id << ":";
            (*current_technique)->dump_upgradable_parameters(oss);

            samples.push_back(oss.str());
            cout << oss.str() << endl;
        } else {
            successfully_solved[(*current_technique)->id] = nb_solved;
        }
    }

}


std::string SamplingSearch::sample_file_header() const {
    return constructed_sample_file_header;
}

void SamplingSearch::add_sampling_search_options(options::OptionParser &parser) {
    // Sources for samples to store
    parser.add_option<bool> ("store_solution_trajectory",
                             "Stores for every state on the solution path which operator was chosen"
                             "next to reach the goal, the next state reached, and the heuristic "
                             "values estimated for the state.", "true");
    parser.add_option<bool> ("store_other_trajectories",
                             "Stores for every state on the other trajectories (has only an effect,"
                             "if the used search engine stores other trajectories) which operator "
                             "was chosen next to reach the goal, the next state reached, and the "
                             "heuristic values estimated for the state.", "false");
    parser.add_option<bool> ("store_all_states",
                             "Stores for every state visited the operator chosen to reach it,"
                             ", its parent state, and the heuristic "
                             "values estimated for the state.", "false");

    //Which samples from the trajectory sources to store
    parser.add_option<bool> (
        "store_initial_state", "Stores for the initial state itself, its cost "
        "(estimated by the plan found), the operator selected as well as the goal "
        "and the heuristic estimates of the heuristics registered via "
        "\"evaluators\" for each trajectory given (solution_trajectory and "
        "other_trajectories). This prevents storing the whole trajectory.", "true");

    parser.add_option<bool> (
        "store_intermediate_state", "Stores a random uniformly chosen state per "
        "trajectory given, its cost (estimated by the plan found), the "
        "operator selected as well as the goal and the heuristic estimates of "
        "the heuristics registered via \"evaluators\" for each trajectory "
        "given (solution_trajectory and other_trajectories. This prevents"
        "storing the whole trajectory.", "false");

    parser.add_option<bool> ("expand_trajectory",
                             "Stores for every state combination on a trajectory which operator was chosen"
                             "next to from the first to the second state, the next state reached, and the heuristic "
                             "values estimated for the state.", "false");

    // What/How to store
    parser.add_option<bool> (
            "store_expansions",
            "instead of storing the plan length, the expanded states of the"
            "sampling search are stored. This option is mutually exclusive"
            "with storing anything except the initial state of the sampling search.",
            "false");
    parser.add_option<bool> ("store_expansions_unsolved",
                              "stores also for the task not solved by the sampling search"
                              "the expansions (timed out states will have a large expansion"
                              "counts which informs the network to not choose such states.",
                              "false");
    parser.add_option<bool> ("skip_goal_field", "does not store the goal "
                             "field of an entry.", "false");
    parser.add_option<bool> ("skip_second_state_field", "does not store the second state "
                             "(depending on entry type successor or predecessor) field of "
                             "an entry.", "false");
    parser.add_option<bool> ("skip_action_field", "does not store the selected action "
                             "field of an entry.", "false");
    parser.add_option<bool> ("add_unsolved_samples",
                             "adds those samples which timed out to the samples. This also adds"
                             "a new field which can take the values 0 and 1",
                             "false");

    parser.add_list_option<std::string> ("use_predefined_evaluators",
                                         "Calculates for every sample to store also their evaluator values for "
                                         "the predefined evaluators listed here", "[]");

    parser.add_option<std::string> ("hash",
                                    "MD5 hash of the input problem. This can be used to "
                                    "differentiate which problems created which entries.", "none");

    parser.add_option<int> (
        "network_reload_frequency",
        "Specify how often the network predefinitions"
        " will be reloaded. -1 = never, "
        "0 = always, N > 0 = every N sample calls. ATTENTIONS: The network might "
        "take during initialization a task pointer and uses this task for "
        "its evaluations. Reduce reloading frequency only if you know that it "
        "is not harmful.",
        "0");
    parser.add_option<int>(
            "upgrade_history_size",
            "If sufficiently many tasks (this options * upgrade_solved_rate)"
            " have been solved in the last time,"
            "then the sampling technique parameters are upgraded.",
            "1000");
    parser.add_option<double>(
            "upgrade_solved_rate",
            "Fraction of tasks that have to be solve to upgrade the"
            "sampling technique parameters",
            "0.95"
            );
    utils::add_rng_options(parser);

}
}
