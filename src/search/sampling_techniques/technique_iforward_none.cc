#include "technique_iforward_none.h"

#include "../heuristic.h"
#include "../plugin.h"
#include "../task_utils/sampling.h"

#include "../tasks/modified_init_goals_task.h"

using namespace std;

namespace sampling_technique {
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
    if (!bias_evaluator_tree.empty() && bias_adapt == -1) {
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
    if (!bias_evaluator_tree.empty() &&
        (seed_task != last_task ||
         (bias_reload_frequency != -1 &&
          bias_reload_counter > bias_reload_frequency))) {
        options::OptionParser bias_parser(bias_evaluator_tree, *registry, *predefinitions, false);
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
        options::OptionParser &parser) {
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

    options::Options opts = parser.parse();

    shared_ptr<TechniqueIForwardNone> technique;
    if (!parser.dry_run()) {
        technique = make_shared<TechniqueIForwardNone>(opts);
    }
    return technique;
}

static Plugin<SamplingTechnique> _plugin_technique_iforward_none(
        TechniqueIForwardNone::name, _parse_technique_iforward_none);

}