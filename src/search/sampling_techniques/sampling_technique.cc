#include "sampling_technique.h"

#include "../task_utils//sampling.h"
#include "../task_utils/successor_generator.h"

#include "../evaluation_context.h"
#include "../evaluation_result.h"
#include "../heuristic.h"
#include "../plugin.h"
#include "../state_registry.h"

#include "../tasks/modified_init_goals_task.h"

#include "../tasks/partial_state_wrapper_task.h"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <unordered_map>

using namespace std;


namespace sampling_technique {
/*
 * Use modified_task as transformed task in heuristic (and now also sampling).
 * We use this to instantiate search engines with our desired modified tasks.
 */
    std::shared_ptr<AbstractTask> modified_task = nullptr;

    static shared_ptr<AbstractTask> _parse_sampling_transform(
            OptionParser &parser) {
        if (parser.dry_run()) {
            return nullptr;
        } else {
            if (modified_task == nullptr) {
                return tasks::g_root_task;
            } else {
                return modified_task;
            }
        }
    }

    static Plugin<AbstractTask> _plugin_sampling_transform(
            "sampling_transform", _parse_sampling_transform);


static int compute_heuristic(
        const TaskProxy &task_proxy, Heuristic *bias,
        utils::RandomNumberGenerator &rng, const PartialAssignment &assignment) {
    auto pair_success_state = assignment.get_full_state(
            true, rng);
    if (!pair_success_state.first) {
        return EvaluationResult::INFTY;
    }
    StateRegistry state_registry(task_proxy);
    vector<int> initial_facts= pair_success_state.second.get_values();
    State state = state_registry.insert_state(move(initial_facts));

    return bias->compute_heuristic(state);
}


const string SamplingTechnique::no_dump_directory = "false";
    int SamplingTechnique::next_id = 0;

//REDUNDANCY
    void check_magic(istream &in, const string &magic) {
        string word;
        in >> word;
        if (word != magic) {
            cerr << "Failed to match magic word '" << magic << "'." << endl
                 << "Got '" << word << "'." << endl;
            if (magic == "begin_version") {
                cerr << "Possible cause: you are running the planner "
                     << "on a translator output file from " << endl
                     << "an older version." << endl;
            }
            utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
        }
    }

//REDUNDANCY
    bool are_facts_mutex(const vector<vector<set<FactPair>>> &mutexes,
                         const FactPair &fact1,
                         const FactPair &fact2) {
        if (fact1.var == fact2.var) {
            // Same variable: mutex iff different value.
            return fact1.value != fact2.value;
        }
        assert(utils::in_bounds(fact1.var, mutexes));
        assert(utils::in_bounds(fact1.value, mutexes[fact1.var]));
        return bool(mutexes[fact1.var][fact1.value].count(fact2));
    }

    static vector<vector<string>> load_mutexes(const string &path) {
        if (path == "none") {
            return vector<vector<string>>();
        }

        ifstream in;
        in.open(path);
        if (!in) {
            cerr << "Unable to open external mutex file: " << path << endl;
            utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
        }

        //Compare mutex loading to root_task.cc
        int num_mutex_groups;
        in >> num_mutex_groups;
        vector<vector<string>> mutexes(num_mutex_groups);
        for (int i = 0; i < num_mutex_groups; ++i) {
            check_magic(in, "begin_mutex_group");

            int num_facts;
            in >> num_facts;
            in >> ws;
            mutexes[i].reserve(num_facts);

            for (int j = 0; j < num_facts; ++j) {
                string atom;
                getline(in, atom);
                mutexes[i].push_back(atom);
            }
            check_magic(in, "end_mutex_group");
        }
        in.close();
        return mutexes;
    }

/* START DEFINITION SAMPLING_TECHNIQUE */
    SamplingTechnique::SamplingTechnique(const options::Options &opts)
            : id(next_id++),
              registry(opts.get_registry()),
              predefinitions(opts.get_predefinitions()),
              count(opts.get<int>("count")),
              dump_directory(opts.get<string>("dump")),
              check_mutexes(opts.get<bool>("check_mutexes")),
              check_solvable(opts.get<bool>("check_solvable")),
              use_alternative_mutexes(opts.get<string>("mutexes") != "none"),
              alternative_mutexes(load_mutexes(opts.get<string>("mutexes"))),
              eval_parse_tree(opts.get_parse_tree("evals")),
              option_parser(utils::make_unique_ptr<OptionParser>(
                      eval_parse_tree,
                      *opts.get_registry(), *opts.get_predefinitions(),
                      false, false)),
              remaining_upgrades(opts.get<int>("max_upgrades", 0)),
              rng(utils::parse_rng_from_options(opts)) {
        for (const shared_ptr<Evaluator> &e:
                option_parser->start_parsing<vector<shared_ptr<Evaluator>>>()) {
            if (!e->dead_ends_are_reliable()) {
                cout << "Warning: A given dead end detection evaluator is not safe."
                     << endl;
            }
        }
    }

    SamplingTechnique::SamplingTechnique(
            int count, string dump_directory, bool check_mutexes,
            bool check_solvable, mt19937 &mt)
            : id(next_id++),
              count(count),
              dump_directory(move(dump_directory)),
              check_mutexes(check_mutexes),
              check_solvable(check_solvable),
              use_alternative_mutexes(false),
              alternative_mutexes(),
              eval_parse_tree(options::generate_parse_tree("[]")),
              option_parser(nullptr),
              remaining_upgrades(0),
              rng(make_shared<utils::RandomNumberGenerator>(mt)) {}

    int SamplingTechnique::get_count() const {
        return count;
    }

    int SamplingTechnique::get_counter() const {
        return counter;
    }

    bool SamplingTechnique::empty() const {
        return counter >= count;
    }

    shared_ptr<AbstractTask> SamplingTechnique::next(
            shared_ptr<AbstractTask> seed_task) {
        return next(seed_task, TaskProxy(*seed_task));
    }

    shared_ptr<AbstractTask> SamplingTechnique::next(
            shared_ptr<AbstractTask> seed_task,
            const TaskProxy &task_proxy) {
        if (empty()) {
            return nullptr;
        } else {
            update_alternative_task_mutexes(seed_task);
            counter++;
            while (true) {
                const shared_ptr<AbstractTask> next_task = create_next(
                        seed_task, task_proxy);
                modified_task = next_task;
                if ((check_mutexes && !test_mutexes(next_task)) ||
                    (check_solvable && !test_solvable(
                            TaskProxy(*next_task)))) {
                    //cout << "Generated task invalid, try anew." << endl;
                    continue;
                }
                last_task = seed_task;
                return next_task;
            }
        }
    }

    void SamplingTechnique::update_alternative_task_mutexes(
            std::shared_ptr<AbstractTask> task) {
        if (!use_alternative_mutexes || task == last_task) {
            return;
        }

        vector<FactPair> facts;
        unordered_map<string, int> names;
        for (int var = 0; var < task->get_num_variables(); ++var) {
            for (int val = 0; val < task->get_variable_domain_size(var); ++val) {
                facts.emplace_back(var, val);
                names[task->get_fact_name(facts.back())] = facts.size() - 1;
            }
        }

        vector<vector<set<FactPair>>> inconsistent_facts(task->get_num_variables());
        for (int i = 0; i < task->get_num_variables(); ++i)
            inconsistent_facts[i].resize(task->get_variable_domain_size(i));

        for (const vector<string> &mutex: alternative_mutexes) {
            for (const string &sfact1: mutex) {
                auto ifact1 = names.find(sfact1);
                if (ifact1 == names.end()) {
                    continue;
                }
                int fact1 = ifact1->second;
                for (const string &sfact2: mutex) {
                    auto ifact2 = names.find(sfact2);
                    if (ifact2 == names.end()) {
                        continue;
                    }
                    int fact2 = ifact2->second;
                    inconsistent_facts[facts[fact1].var][facts[fact1].value].insert(
                            FactPair(facts[fact2].var, facts[fact2].value));
                }
            }
        }
        alternative_task_mutexes = inconsistent_facts;
    }

    bool SamplingTechnique::has_upgradeable_parameters() const {
        return remaining_upgrades > 0 || remaining_upgrades == -1;
    }

    void SamplingTechnique::do_upgrade_parameters() {
        cerr << "Either this sampling technique has not upgradable parameters"
                "or the upgrade method has to be implemented." << endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }

    void SamplingTechnique::upgrade_parameters() {
        if (remaining_upgrades == -1) {
            do_upgrade_parameters();
        } else if (remaining_upgrades > 0) {
            do_upgrade_parameters();
            remaining_upgrades--;
        } else {
            cerr << "Either this sampling technique has not upgradable parameters"
                    "or no upgrades are remaining." << endl;
            utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
        }
    }

    void SamplingTechnique::dump_upgradable_parameters(ostream &/*stream*/) const {
        cerr << "Either this sampling technique has not upgradable parameters"
                "or the dump parameter method has to be implemented." << endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }

    bool SamplingTechnique::test_mutexes(shared_ptr<AbstractTask> task) const {
        //Check initial state
        vector<int> init = task->get_initial_state_values();
        for (size_t i = 0; i < init.size(); ++i) {
            FactPair fi(i, init[i]);
            for (size_t j = i + 1; j < init.size(); ++j) {
                FactPair fj(j, init[j]);
                if ((use_alternative_mutexes &&
                     are_facts_mutex(alternative_task_mutexes, fi, fj)) ||
                    (!use_alternative_mutexes && task->are_facts_mutex(fi, fj))) {
                    return false;
                }
            }
        }
        //Check goal facts
        for (int i = 0; i < task->get_num_goals(); ++i) {
            FactPair fi = task->get_goal_fact(i);
            for (int j = i + 1; j < task->get_num_goals(); ++j) {
                FactPair fj = task->get_goal_fact(j);
                if ((use_alternative_mutexes &&
                     are_facts_mutex(alternative_task_mutexes, fi, fj)) ||
                    (!use_alternative_mutexes && task->are_facts_mutex(fi, fj))) {
                    return false;
                }
            }
        }
        return true;
    }

    bool SamplingTechnique::test_solvable(const TaskProxy &task_proxy) const {
        if (option_parser == nullptr) {
            return true;
        }
        StateRegistry state_registry(task_proxy);
        const State &state = state_registry.get_initial_state();
        EvaluationContext eval_context(state);
        int round = 0;
        for (const shared_ptr<Evaluator> &e:
                option_parser->start_parsing<vector<shared_ptr<Evaluator>>>()) {
            EvaluationResult eval_result = e->compute_result(eval_context);
            if (eval_result.is_infinite()) {
                cout << "Task unsolvable said by evaluator: " << round << endl;
                return false;
            }
            round++;
        }
        return true;
    }

    void SamplingTechnique::dump_modifications(
            std::shared_ptr<AbstractTask> /*task*/) const {
        if (dump_directory.empty() || dump_directory == "false") {
            return;
        }
        //ostringstream path;
        //path << dump_directory << "/" << counter << "-" << get_name();
        //ostringstream content;
        cout << "SamplingTechnique problem dump currently not supported" << endl;
        utils::exit_with(utils::ExitCode::SEARCH_UNSUPPORTED);
    }

    void SamplingTechnique::add_options_to_parser(options::OptionParser &parser) {
        parser.add_option<int>("count", "Number of times this sampling "
                                        "technique shall be used.");
        parser.add_list_option<shared_ptr<Evaluator>>(
                "evals",
                "evaluators for dead-end detection (use only save ones to not "
                "reject a non dead end). If any of the evaluators detects a dead "
                "end, the assignment is rejected. \nATTENTON: The evaluators are "
                "initialize for each task to test anew. To be initialized "
                "correctly, they need to have the attribute "
                "'transform=sampling_transform'", "[]");
        parser.add_option<string>(
                "mutexes",
                "Path to a file describing mutexes. Uses those mutexes instead of "
                "the task mutexes if given.",
                "none");
        parser.add_option<string>(
                "dump",
                "Path to a directory where to dump the modifications or \"false\" "
                "to not dump.",
                SamplingTechnique::no_dump_directory);
        parser.add_option<bool>("check_mutexes", "Boolean flag to set whether to "
                                                 "to check that a generated task does not violate "
                                                 "mutexes.", "true");
        parser.add_option<bool>("check_solvable", "Boolean flag to set whether "
                                                  "the generated task is solvable.",
                                "true");
        utils::add_rng_options(parser);
    }

    vector<int> SamplingTechnique::extractInitialState(const State &state) {
        vector<int> values;
        values.reserve(state.size());
        for (int val : state.get_values()) {
            values.push_back(val);
        }
        return values;
    }

    vector<FactPair> SamplingTechnique::extractGoalFacts(
            const GoalsProxy &goals_proxy) {
        vector<FactPair> goals;
        goals.reserve(goals_proxy.size());
        for (size_t i = 0; i < goals_proxy.size(); i++) {
            goals.push_back(goals_proxy[i].get_pair());
        }
        return goals;
    }

    vector<FactPair> SamplingTechnique::extractGoalFacts(const State &state) {
        vector<FactPair> goals;
        goals.reserve(state.size());
        int var = 0;
        for (int val : state.get_values()) {
            goals.emplace_back(var, val);
            var++;
        }
        return goals;
    }

/* START DEFINITION TECHNIQUE_NULL */
    const std::string TechniqueNull::name = "null";

    const string &TechniqueNull::get_name() const {
        return name;
    }

    TechniqueNull::TechniqueNull()
            : SamplingTechnique(0, SamplingTechnique::no_dump_directory, false, false) {}


    std::shared_ptr<AbstractTask> TechniqueNull::create_next(
            shared_ptr<AbstractTask> /*seed_task*/, const TaskProxy &) {
        return nullptr;
    }


/* START DEFINITION TECHNIQUE_NONE_NONE */
    const std::string TechniqueNoneNone::name = "none_none";

    const string &TechniqueNoneNone::get_name() const {
        return name;
    }

    TechniqueNoneNone::TechniqueNoneNone(const options::Options &opts)
            : SamplingTechnique(opts) {}

    TechniqueNoneNone::TechniqueNoneNone(int count, string dump_directory,
                                         bool check_mutexes, bool check_solvable)
            : SamplingTechnique(count, dump_directory, check_mutexes, check_solvable) {}

    std::shared_ptr<AbstractTask> TechniqueNoneNone::create_next(
            shared_ptr<AbstractTask> seed_task, const TaskProxy &) {
        return seed_task;
    }


/* PARSING TECHNIQUE_NONE_NONE*/
    static shared_ptr<SamplingTechnique> _parse_technique_none_none(
            OptionParser &parser) {
        SamplingTechnique::add_options_to_parser(parser);
        Options opts = parser.parse();

        shared_ptr<TechniqueNoneNone> technique;
        if (!parser.dry_run()) {
            technique = make_shared<TechniqueNoneNone>(opts);
        }
        return technique;
    }

    static Plugin<SamplingTechnique> _plugin_technique_none_none(
            TechniqueNoneNone::name, _parse_technique_none_none);


/* START DEFINITION TECHNIQUE_IFORWARD_NONE */
    const std::string TechniqueIForwardNone::name = "iforward_none";

    const string &TechniqueIForwardNone::get_name() const {
        return name;
    }

    TechniqueIForwardNone::TechniqueIForwardNone(const options::Options &opts)
            : SamplingTechnique(opts),
              steps(opts.get<shared_ptr<utils::DiscreteDistribution>>("distribution")),
              deprioritize_undoing_steps(opts.get<bool>("deprioritize_undoing_steps")),
              bias_evaluator_tree(opts.get_parse_tree("bias", options::ParseTree())),
              bias_probabilistic(opts.get<bool>("bias_probabilistic")),
              bias_adapt(opts.get<double>("bias_adapt")),
              bias_reload_frequency(opts.get<int>("bias_reload_frequency")),
              bias_reload_counter(0) {
        if (bias_evaluator_tree.size() > 0 && bias_adapt == -1) {
            cerr << "Bias for " << name << " requires bias_adapt set." << endl;
            utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
        }
    }

    std::shared_ptr<AbstractTask> TechniqueIForwardNone::create_next(
            shared_ptr<AbstractTask> seed_task, const TaskProxy &task_proxy) {
        if (seed_task != last_task) {
            rws = make_shared<sampling::RandomWalkSampler>(task_proxy, *rng);
        }
        bias_reload_counter++;
        if (bias_evaluator_tree.size() > 0 &&
            (seed_task != last_task ||
             (bias_reload_frequency != -1 &&
              bias_reload_counter > bias_reload_frequency))) {
            OptionParser bias_parser(bias_evaluator_tree, *registry, *predefinitions, false);
            bias = bias_parser.start_parsing<shared_ptr<Heuristic>>();
            bias_reload_counter = 0;
            cache.clear();
        }

        StateBias *func_bias = nullptr;
        StateBias pab = [&](State &state) {
            auto iter = cache.find(state);
            if (iter != cache.end()) {
                return iter->second;
            } else {
                int h = -bias->compute_heuristic(state);
                cache[state] = h;
                return h;
            }
        };
        if (bias != nullptr) {
            func_bias = &pab;
        }

        //add option bias, deprioritize undoing steps, bias probabilistic, bias_adapt
        State new_init = rws->sample_state_length(
                task_proxy.get_initial_state(), steps->next(), [](const State &) { return false; },
                deprioritize_undoing_steps, func_bias, bias_probabilistic, bias_adapt);
        return make_shared<extra_tasks::ModifiedInitGoalsTask>(seed_task,
                                                               extractInitialState(new_init),
                                                               extractGoalFacts(task_proxy.get_goals()));
    }

/* PARSING TECHNIQUE_IFORWARD_NONE*/
    static shared_ptr<SamplingTechnique> _parse_technique_iforward_none(
            OptionParser &parser) {
        SamplingTechnique::add_options_to_parser(parser);
        parser.add_option<shared_ptr<utils::DiscreteDistribution>>("distribution",
                                                                   "Discrete random distribution to determine the random walk length used"
                                                                   " by this technique.");
        parser.add_option<bool>(
                "deprioritize_undoing_steps",
                "Deprioritizes actions which undo the previous action",
                "false");

        parser.add_option<shared_ptr<Heuristic>>(
                "bias",
                "bias heuristic",
                "<none>"
        );
        parser.add_option<int>(
                "bias_reload_frequency",
                "the bias is reloaded everytime the tasks for which state are"
                "generated changes or if it has not been reloaded for "
                "bias_reload_frequency steps. Use -1 to prevent reloading.",
                "-1"
        );
        parser.add_option<bool>(
                "bias_probabilistic",
                "uses the bias values as weights for selecting the next state"
                "on the walk. Otherwise selects a random state among those with "
                "maximum bias",
                "true"
        );
        parser.add_option<double>(
                "bias_adapt",
                "if using the probabilistic bias, then the bias values calculated"
                "for the successors s1,..., sn of the state s are adapted as "
                "bias_adapt^(b(s1) - b(s)). This gets right of the issue that for"
                "large bias values, there was close to no difference between the "
                "states probabilities and focuses more on states increasing the bias.",
                "-1"
        );

        Options opts = parser.parse();

        shared_ptr<TechniqueIForwardNone> technique;
        if (!parser.dry_run()) {
            technique = make_shared<TechniqueIForwardNone>(opts);
        }
        return technique;
    }

    static Plugin<SamplingTechnique> _plugin_technique_iforward_none(
            TechniqueIForwardNone::name, _parse_technique_iforward_none);


/* START DEFINITION TECHNIQUE_GBACKWARD_NONE */
const std::string TechniqueGBackwardNone::name = "gbackward_none";

const string &TechniqueGBackwardNone::get_name() const {
    return name;
}


TechniqueGBackwardNone::TechniqueGBackwardNone(const options::Options &opts)
    : SamplingTechnique(opts),
      steps(opts.get<shared_ptr<utils::DiscreteDistribution>>("distribution")),
      wrap_partial_assignment(opts.get<bool>("wrap_partial_assignment")),
      deprioritize_undoing_steps(opts.get<bool>("deprioritize_undoing_steps")),
      is_valid_walk(opts.get<bool>("is_valid_walk")),
      bias_evaluator_tree(opts.get_parse_tree("bias", options::ParseTree())),
      bias_probabilistic(opts.get<bool>("bias_probabilistic")),
      bias_adapt(opts.get<double>("bias_adapt")),
      bias_reload_frequency(opts.get<int>("bias_reload_frequency")),
      bias_reload_counter(0){
}

PartialAssignment TechniqueGBackwardNone::create_next_initial(
        std::shared_ptr<AbstractTask> seed_task, const TaskProxy &task_proxy) {
    assert(!empty());
    counter++;
    if (seed_task != last_task) {
        regression_task_proxy = make_shared<RegressionTaskProxy>(*seed_task);
        state_registry = make_shared<StateRegistry>(task_proxy);
        rrws = make_shared<sampling::RandomRegressionWalkSampler>(*regression_task_proxy, *rng);
    }
    bias_reload_counter++;
    if (bias_evaluator_tree.size() > 0 &&
        (seed_task != last_task ||
         (bias_reload_frequency != -1 &&
          bias_reload_counter > bias_reload_frequency))) {
        OptionParser bias_parser(bias_evaluator_tree, *registry, *predefinitions, false);
        bias = bias_parser.start_parsing<shared_ptr<Heuristic>>();
        bias_reload_counter = 0;
        cache.clear();
    }


    auto is_valid_state = [&](PartialAssignment &partial_assignment) {
        return (is_valid_walk)?regression_task_proxy->convert_to_full_state(
                partial_assignment, true, *rng).first: true;
    };
    PartialAssignmentBias *func_bias = nullptr;
    PartialAssignmentBias pab = [&] (PartialAssignment &partial_assignment){
        auto iter = cache.find(partial_assignment);
        if (iter != cache.end()) {
            return iter->second;
        } else {
            int h = compute_heuristic(task_proxy, bias.get(), *rng, partial_assignment);
            cache[partial_assignment] = h;
            return h;
        }
    };
    if (bias != nullptr) {
        func_bias = &pab;
    }
    while(true) {
        PartialAssignment partial_assignment =
                rrws->sample_state_length(
                        regression_task_proxy->get_goal_assignment(),
                        steps->next(),
                        deprioritize_undoing_steps,
                        is_valid_state,
                        func_bias,
                        bias_probabilistic,
                        bias_adapt);
        auto complete_assignment = regression_task_proxy->convert_to_full_state(
                partial_assignment, true, *rng);
        if (!complete_assignment.first) {
            continue;
        }
        return partial_assignment;
    }
}

std::shared_ptr<AbstractTask> TechniqueGBackwardNone::create_next(
    shared_ptr<AbstractTask> seed_task, const TaskProxy &task_proxy) {
    if (seed_task != last_task) {
        regression_task_proxy = make_shared<RegressionTaskProxy>(*seed_task);
        state_registry = make_shared<StateRegistry>(task_proxy);
        rrws = make_shared<sampling::RandomRegressionWalkSampler>(*regression_task_proxy, *rng);
    }
    bias_reload_counter++;
    if (bias_evaluator_tree.size() > 0 &&
            (seed_task != last_task ||
             (bias_reload_frequency != -1 &&
              bias_reload_counter > bias_reload_frequency))) {
        OptionParser bias_parser(bias_evaluator_tree, *registry, *predefinitions, false);
        bias = bias_parser.start_parsing<shared_ptr<Heuristic>>();
        bias_reload_counter = 0;
        cache.clear();
    }


    auto is_valid_state = [&](PartialAssignment &partial_assignment) {
        return (is_valid_walk)?regression_task_proxy->convert_to_full_state(
                partial_assignment, true, *rng).first: true;
    };
    PartialAssignmentBias *func_bias = nullptr;
    PartialAssignmentBias pab = [&] (PartialAssignment &partial_assignment){
        auto iter = cache.find(partial_assignment);
        if (iter != cache.end()) {
            return iter->second;
        } else {
            int h = compute_heuristic(task_proxy, bias.get(), *rng, partial_assignment);
            cache[partial_assignment] = h;
            return h;
        }
    };
    if (bias != nullptr) {
        func_bias = &pab;
    }

    while(true) {
        cout << "A" << endl;
        PartialAssignment partial_assignment =
            rrws->sample_state_length(
                    regression_task_proxy->get_goal_assignment(),
                    steps->next(),
                    deprioritize_undoing_steps,
                    is_valid_state,
                    func_bias,
                    bias_probabilistic,
                    bias_adapt);

        auto complete_assignment = partial_assignment.get_full_state(true, *rng);
        cout << "B" << endl;
        if (!complete_assignment.first) {
            continue;
        }
        if (wrap_partial_assignment) {
            if (last_task != seed_task) {
                last_partial_wrap_task = make_shared<extra_tasks::PartialStateWrapperTask>(seed_task);
            }

            vector<int> new_init_values;
            new_init_values.reserve(partial_assignment.size());
            for (size_t i = 0; i < partial_assignment.size(); ++i) {
                new_init_values.push_back(
                        partial_assignment.assigned(i) ?
                        partial_assignment[i].get_value():
                        seed_task->get_variable_domain_size(i));
            }
            return make_shared<extra_tasks::ModifiedInitGoalsTask>(
                    last_partial_wrap_task,
                    move(new_init_values),
                    extractGoalFacts(regression_task_proxy->get_goals()));
        } else {
            return make_shared<extra_tasks::ModifiedInitGoalsTask>(
                seed_task,
                extractInitialState(complete_assignment.second),
                extractGoalFacts(regression_task_proxy->get_goals()));

        }
    }
}

void TechniqueGBackwardNone::do_upgrade_parameters() {
    steps->upgrade_parameters();
}

void TechniqueGBackwardNone::dump_upgradable_parameters(std::ostream &stream) const {
    steps->dump_parameters(stream);
}

/* PARSING TECHNIQUE_GBACKWARD_NONE*/
static shared_ptr<TechniqueGBackwardNone> _parse_technique_gbackward_none(
    OptionParser &parser) {
    SamplingTechnique::add_options_to_parser(parser);
    parser.add_option<shared_ptr<utils::DiscreteDistribution>>(
        "distribution",
        "Discrete random distribution to determine the random walk length used"
        " by this technique.");
    parser.add_option<bool>(
        "wrap_partial_assignment",
        "If set, wraps a partial assignment obtained by the regression for the "
        "initial state into a task which has additional values for undefined "
        "variables. By default, the undefined variables are random uniformly "
        "set (satisfying the mutexes).",
        "false"
    );
    parser.add_option<bool>(
        "deprioritize_undoing_steps",
        "Deprioritizes actions which undo the previous action",
        "false");
    parser.add_option<bool>(
            "is_valid_walk",
            "enforces states during random walk are avalid states w.r.t. "
            "the KNOWN mutexes",
           "true");
    parser.add_option<shared_ptr<Heuristic>>(
            "bias",
            "bias heuristic",
            "<none>"
            );
    parser.add_option<int>(
            "bias_reload_frequency",
            "the bias is reloaded everytime the tasks for which state are"
            "generated changes or if it has not been reloaded for "
            "bias_reload_frequency steps. Use -1 to prevent reloading.",
            "-1"
            );
    parser.add_option<bool>(
            "bias_probabilistic",
            "uses the bias values as weights for selecting the next state"
            "on the walk. Otherwise selects a random state among those with "
            "maximum bias",
            "true"
            );
    parser.add_option<double>(
            "bias_adapt",
            "if using the probabilistic bias, then the bias values calculated"
            "for the successors s1,..., sn of the state s are adapted as "
            "bias_adapt^(b(s1) - b(s)). This gets right of the issue that for"
            "large bias values, there was close to no difference between the "
            "states probabilities and focuses more on states increasing the bias.",
            "-1"
            );
    parser.add_option<int>(
            "max_upgrades",
            "Maximum number of times this sampling technique can upgrade its"
            "parameters. Use -1 for infinite times.",
            "0");
    Options opts = parser.parse();

    shared_ptr<TechniqueGBackwardNone> technique;
    if (!parser.dry_run()) {
        technique = make_shared<TechniqueGBackwardNone>(opts);
    }
    return technique;
}


static Plugin<SamplingTechnique> _plugin_technique_gbackward_none(
    TechniqueGBackwardNone::name, _parse_technique_gbackward_none);
//static Plugin<TechniqueGBackwardNone> _plugin_technique_gbackward_none2(
//        TechniqueGBackwardNone::name, _parse_technique_gbackward_none, "", false);


/* START DEFINITION TECHNIQUE_UNIFORM_NONE */
const std::string TechniqueUniformNone::name = "uniform_none";

const string &TechniqueUniformNone::get_name() const {
    return name;
}

TechniqueUniformNone::TechniqueUniformNone(const options::Options &opts)
    : SamplingTechnique(opts) { }

std::shared_ptr<AbstractTask> TechniqueUniformNone::create_next(
    shared_ptr<AbstractTask> seed_task, const TaskProxy &) {
    TaskProxy seed_task_proxy(*seed_task);
    int c = 0;
    while (true) {
        vector<int> unassigned(
                seed_task->get_num_variables(), PartialAssignment::UNASSIGNED);
        auto state = PartialAssignment(*seed_task, move(unassigned))
                .get_full_state(check_mutexes, *rng);
        if (state.first) {
            return make_shared<extra_tasks::ModifiedInitGoalsTask>(
                    seed_task, move(state.second.get_values()), extractGoalFacts(seed_task_proxy.get_goals()));
        } else {
            c++;
        }
    }
}

/* PARSING TECHNIQUE_UNIFORM_NONE*/
static shared_ptr<SamplingTechnique> _parse_technique_uniform_none(
    OptionParser &parser) {
    SamplingTechnique::add_options_to_parser(parser);


    Options opts = parser.parse();

    shared_ptr<TechniqueUniformNone> technique;
    if (!parser.dry_run()) {
        technique = make_shared<TechniqueUniformNone>(opts);
    }
    return technique;
}

static Plugin<SamplingTechnique> _plugin_technique_uniform_none(
    TechniqueUniformNone::name, _parse_technique_uniform_none);


static PluginTypePlugin<TechniqueGBackwardNone> _type_plugin_gbackward_none(
        "GBackwardNone",
        "Object to modify the given task.");

static PluginTypePlugin<SamplingTechnique> _type_plugin(
        "SamplingTechnique",
        "Generates from a given task a new one.");
}

