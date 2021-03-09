#ifndef TASK_UTILS_SAMPLING_TECHNIQUE_H
#define TASK_UTILS_SAMPLING_TECHNIQUE_H

#include "regression_task_proxy.h"
#include "sampling.h"

#include "../abstract_task.h"
#include "../option_parser.h"
#include "../task_proxy.h"
#include "../evaluator.h"
#include "../heuristic.h"

#include "../tasks/modified_goals_task.h"
#include "../tasks/root_task.h"
#include "../utils/hash.h"
#include "../utils/rng.h"
#include "../utils/rng_options.h"
#include "../utils/distribution.h"

#include <memory>
#include <ostream>
#include <random>
#include <set>
#include <string>
#include <vector>

class PartialAssignment;

namespace sampling_technique {
extern std::shared_ptr<AbstractTask> modified_task;

class SamplingTechnique {
private:
    static int next_id;
public:
    const int id;
protected:
    options::Registry *registry;
    const options::Predefinitions *predefinitions;
    const int count;
    const std::string dump_directory;
    const bool check_mutexes;
    const bool check_solvable;
    const bool use_alternative_mutexes;
    const std::vector<std::vector<std::string>> alternative_mutexes;
    const options::ParseTree eval_parse_tree;
    const std::unique_ptr<options::OptionParser> option_parser;
    int counter = 0;

protected:
    int remaining_upgrades;

    std::shared_ptr<utils::RandomNumberGenerator> rng;
    std::vector<std::vector<std::set<FactPair>>> alternative_task_mutexes;
    std::shared_ptr<AbstractTask> last_task = nullptr;

    virtual std::shared_ptr<AbstractTask> create_next(
        std::shared_ptr<AbstractTask> seed_task,
        const TaskProxy &task_proxy) = 0;

    bool test_mutexes(std::shared_ptr<AbstractTask> task) const;
    bool test_solvable(const TaskProxy &task_proxy) const;
    void dump_modifications(std::shared_ptr<AbstractTask> task) const;
    void update_alternative_task_mutexes(std::shared_ptr<AbstractTask> task);

    virtual void do_upgrade_parameters();

public:
    explicit SamplingTechnique(const options::Options &opts);
    SamplingTechnique(int count, std::string dump_directory, bool check_mutexes,
                      bool check_solvable, std::mt19937 &mt = utils::get_global_mt19937());
    virtual ~SamplingTechnique() = default;

    int get_count() const;
    int get_counter() const;
    bool empty() const;

    std::shared_ptr<AbstractTask> next(
        std::shared_ptr<AbstractTask> seed_task = tasks::g_root_task);
    std::shared_ptr<AbstractTask> next(
        std::shared_ptr<AbstractTask> seed_task,
        const TaskProxy &task_proxy);

    virtual void initialize() {
    }
    virtual const std::string &get_name() const = 0;

    static void add_options_to_parser(options::OptionParser &parser);
    static std::vector<int> extractInitialState(const State &state);
    static std::vector<FactPair> extractGoalFacts(const GoalsProxy &goals_proxy);
    static std::vector<FactPair> extractGoalFacts(const State &state);

    virtual bool has_upgradeable_parameters() const;
    virtual void upgrade_parameters();
    virtual void dump_upgradable_parameters(std::ostream &stream) const;

    static const std::string no_dump_directory;
};

class TechniqueNull : public SamplingTechnique {
protected:
    virtual std::shared_ptr<AbstractTask> create_next(
        std::shared_ptr<AbstractTask> seed_task,
        const TaskProxy &task_proxy) override;
public:
    TechniqueNull();
    virtual ~TechniqueNull() override = default;

    virtual const std::string &get_name() const override;
    const static std::string name;
};

class TechniqueNoneNone : public SamplingTechnique {
protected:
    virtual std::shared_ptr<AbstractTask> create_next(
        std::shared_ptr<AbstractTask> seed_task,
        const TaskProxy &task_proxy) override;
public:
    explicit TechniqueNoneNone(const options::Options &opts);
    TechniqueNoneNone(int count, std::string dump_directory, bool check_mutexes,
                      bool check_solvable);
    virtual ~TechniqueNoneNone() override = default;

    virtual const std::string &get_name() const override;
    const static std::string name;
};

class TechniqueIForwardNone : public SamplingTechnique {
protected:
    std::shared_ptr<utils::DiscreteDistribution> steps;
    const bool deprioritize_undoing_steps;
    const options::ParseTree bias_evaluator_tree;
    const bool bias_probabilistic;
    const double bias_adapt;
    utils::HashMap<PartialAssignment, int> cache;
    std::shared_ptr<Heuristic> bias = nullptr;
    const int bias_reload_frequency;
    int bias_reload_counter;
    std::shared_ptr<sampling::RandomWalkSampler> rws = nullptr;

    virtual std::shared_ptr<AbstractTask> create_next(
        std::shared_ptr<AbstractTask> seed_task,
        const TaskProxy &task_proxy) override;

public:
    explicit TechniqueIForwardNone(const options::Options &opts);
    virtual ~TechniqueIForwardNone() override = default;

    virtual const std::string &get_name() const override;
    const static std::string name;
};

class TechniqueGBackwardNone : public SamplingTechnique {
protected:
    std::shared_ptr<utils::DiscreteDistribution> steps;
    const bool wrap_partial_assignment;
    const bool deprioritize_undoing_steps;
    const bool is_valid_walk;
    const options::ParseTree bias_evaluator_tree;
    const bool bias_probabilistic;
    const double bias_adapt;
    utils::HashMap<PartialAssignment, int> cache;
    std::shared_ptr<Heuristic> bias = nullptr;
    const int bias_reload_frequency;
    int bias_reload_counter;
    std::shared_ptr<StateRegistry> state_registry = nullptr;
    std::shared_ptr<AbstractTask> last_partial_wrap_task = nullptr;
    std::shared_ptr<RegressionTaskProxy> regression_task_proxy = nullptr;
    std::shared_ptr<sampling::RandomRegressionWalkSampler> rrws = nullptr;

    virtual std::shared_ptr<AbstractTask> create_next(
        std::shared_ptr<AbstractTask> seed_task,
        const TaskProxy &task_proxy) override;

    virtual void do_upgrade_parameters() override ;

public:
    explicit TechniqueGBackwardNone(const options::Options &opts);
    virtual ~TechniqueGBackwardNone() override = default;

    PartialAssignment create_next_initial(
            std::shared_ptr<AbstractTask> seed_task,
            const TaskProxy &task_proxy);
    virtual void dump_upgradable_parameters(std::ostream &stream) const override;

    virtual const std::string &get_name() const override;
    const static std::string name;
};

class TechniqueUniformNone : public SamplingTechnique {
protected:
    virtual std::shared_ptr<AbstractTask> create_next(
        std::shared_ptr<AbstractTask> seed_task,
        const TaskProxy &task_proxy) override;

public:
    explicit TechniqueUniformNone(const options::Options &opts);
    virtual ~TechniqueUniformNone() override = default;

    virtual const std::string &get_name() const override;
    const static std::string name;
};

}

#endif
