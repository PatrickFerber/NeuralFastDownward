#ifndef SEARCH_ENGINES_SAMPLING_STATE_ENGINE_H
#define SEARCH_ENGINES_SAMPLING_STATE_ENGINE_H

#include "sampling_engine.h"

#include <functional>
#include <memory>
#include <ostream>
#include <sstream>
#include <tree.hh>
#include <vector>


namespace utils {
    class RandomNumberGenerator;
}

namespace sampling_engine {
using Trajectory = std::vector<StateID>;
using Plan = std::vector<OperatorID>;

class Path {
protected:
    Trajectory trajectory;
    Plan plan;
public:
    explicit Path(const StateID &start);
    Path(const Path &) = delete;
    ~Path() = default;

    void add(const OperatorID &op, const StateID &next);

    const Trajectory &get_trajectory() const;
    const Plan &get_plan() const;
};
extern std::vector<Path> paths;


enum SampleFormat {
    CSV,
    FIELDS
};

enum StateFormat {
    PDDL,
    FDR,
};


class SamplingStateEngine : public SamplingEngine {
protected:
    const bool skip_undefined_facts;

    const SampleFormat sample_format;
    const StateFormat state_format;
    const std::string field_separator;
    const std::string state_separator;


    virtual std::string sample_file_header() const override;

    void convert_and_push_state(
            std::ostringstream &oss,
            const State &state) const;
    void convert_and_push_goal(
            std::ostringstream &oss, const AbstractTask &task) const;

public:
    explicit SamplingStateEngine(const options::Options &opts);
    virtual ~SamplingStateEngine() override  = default;


    static void add_sampling_state_options(
            options::OptionParser &parser,
            const std::string &default_sample_format,
            const std::string &default_state_format,
            const std::string &default_field_separator,
            const std::string &default_state_separator);
};
}
#endif
