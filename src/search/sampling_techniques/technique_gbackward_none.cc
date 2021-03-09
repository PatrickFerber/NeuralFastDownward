#include "technique_gbackward_none.h"

#include "../evaluation_result.h"
#include "../heuristic.h"
#include "../plugin.h"

#include "../tasks/modified_init_goals_task.h"
#include "../tasks/partial_state_wrapper_task.h"

#include "../task_utils/sampling.h"

using namespace std;

namespace sampling_technique {
const std::string TechniqueGBackwardNone::name = "gbackward_none";

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
          bias_reload_counter(0) {
}

std::shared_ptr<AbstractTask> TechniqueGBackwardNone::create_next(
        shared_ptr<AbstractTask> seed_task, const TaskProxy &task_proxy) {
    if (seed_task != last_task) {
        regression_task_proxy = make_shared<RegressionTaskProxy>(*seed_task);
        state_registry = make_shared<StateRegistry>(task_proxy);
        rrws = make_shared<sampling::RandomRegressionWalkSampler>(*regression_task_proxy, *rng);
    }
    bias_reload_counter++;
    if (!bias_evaluator_tree.empty() &&
        (seed_task != last_task ||
         (bias_reload_frequency != -1 &&
          bias_reload_counter > bias_reload_frequency))) {
        options::OptionParser bias_parser(bias_evaluator_tree, *registry, *predefinitions, false);
        bias = bias_parser.start_parsing<shared_ptr<Heuristic>>();
        bias_reload_counter = 0;
        cache.clear();
    }


    auto is_valid_state = [&](PartialAssignment &partial_assignment) {
        return !(is_valid_walk) || regression_task_proxy->convert_to_full_state(
                partial_assignment, true, *rng).first;
    };
    PartialAssignmentBias *func_bias = nullptr;
    PartialAssignmentBias pab = [&](PartialAssignment &partial_assignment) {
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

    while (true) {
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
                        partial_assignment[i].get_value() :
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
        options::OptionParser &parser) {
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
    options::Options opts = parser.parse();

    shared_ptr<TechniqueGBackwardNone> technique;
    if (!parser.dry_run()) {
        technique = make_shared<TechniqueGBackwardNone>(opts);
    }
    return technique;
}

static Plugin<SamplingTechnique> _plugin_technique_gbackward_none(
        TechniqueGBackwardNone::name, _parse_technique_gbackward_none);

}
