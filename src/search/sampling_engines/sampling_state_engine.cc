#include "sampling_state_engine.h"

#include "../option_parser.h"

#include "../task_utils/task_properties.h"

#include <iostream>
#include <string>

using namespace std;

namespace sampling_engine {
/* Global variable for sampling algorithms to store arbitrary paths (plans [
   operator ids] and trajectories [state ids]).
   (as everybody can access it, be certain who does and how) */
std::vector<Path> paths;

Path::Path(const StateID &start) {
    trajectory.push_back(start);
}

void Path::add(const OperatorID &op, const StateID &next) {
    plan.push_back(op);
    trajectory.push_back(next);
}

const Plan &Path::get_plan() const {
    return plan;
}

const Trajectory &Path::get_trajectory() const {
    return trajectory;
}

SampleFormat select_sample_format(const string &sample_format) {
    if (sample_format == "csv") {
        return SampleFormat::CSV;
    } else if (sample_format == "fields") {
        return SampleFormat::FIELDS;
    }
    cerr << "Invalid sample format:" << sample_format << endl;
    utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
}

StateFormat select_state_format(const string &state_format) {
    if (state_format == "pddl") {
        return StateFormat::PDDL;
    } else if (state_format == "fdr") {
        return StateFormat::FDR;
    }
    cerr << "Invalid state format:" << state_format << endl;
    utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
}


SamplingStateEngine::SamplingStateEngine(const options::Options &opts)
    : SamplingEngine(opts),
      skip_undefined_facts(opts.get<bool>("skip_undefined_facts")),
      sample_format(select_sample_format(
              opts.get<string>("sample_format"))),
      state_format(select_state_format(
              opts.get<string>("state_format"))),
      field_separator(opts.get<string>("field_separator")),
      state_separator(opts.get<string>("state_separator")) {
}

string SamplingStateEngine::sample_file_header() const {
    ostringstream oss;
    oss << SAMPLE_FILE_MAGIC_WORD << "\n"
        << "# SampleFormat: " << sample_format << "\n";
    if (sample_format == SampleFormat::FIELDS) {
        oss << "# Starts with json format describing the fields, followed by \n"
               "# whitespace followed by all fields separated by "
            << field_separator << ".\n";
    } else if (sample_format == SampleFormat::CSV) {
        oss << "# All fields one after another concatenated by "
            << field_separator << ".\n";
    } else {
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
    oss << "# StateFormat: " << state_format << "\n"
        << "# Element in state entry are separated by " << state_separator
        << "(this might have been converted between writing the samples and "
           "and long time storage of the samples.";
    return oss.str();
}

void SamplingStateEngine::convert_and_push_state(
        ostringstream &oss,
        const State &state) const {

    if (state_format == StateFormat::FDR) {
        const AbstractTask *task = state.get_task().get_task();
        bool first_round = true;
        for (int var = 0; var < task->get_num_variables(); ++var) {
            for (int val = 0; val < task->get_variable_domain_size(var); ++val) {
                if (skip_undefined_facts &&
                        task->is_undefined(FactPair(var, val))) {
                    continue;
                }
                oss << (first_round ? "" : state_separator)
                    << (state.get_values()[var] == val);
                first_round = false;
            }
        }
    } else if (state_format == StateFormat::PDDL) {
        task_properties::dump_pddl(state,oss,
                state_separator, skip_undefined_facts);
    } else {
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
}

void SamplingStateEngine::convert_and_push_goal(
        ostringstream &oss, const AbstractTask &task) const {
    if (state_format == StateFormat::FDR) {
        vector<int> shift;
        shift.push_back(0);
        shift.reserve(task.get_num_variables());
        for (int var = 0; var < task.get_num_variables(); ++var) {
            int nb_undefined = 0;
            if (skip_undefined_facts) {
                for (int val = 0; val < task.get_variable_domain_size(var); ++val) {
                    if (task.is_undefined(FactPair(var, val))) {
                        nb_undefined++;
                    }
                }
            }
            shift.push_back(shift[var] + task.get_variable_domain_size(var) -
                nb_undefined);
        }
        vector<int> goal(shift.back(), 0);
        for (int i = 0; i < task.get_num_goals(); ++i) {
            const FactPair &fp = task.get_goal_fact(i);
            if (skip_undefined_facts && task.is_undefined(fp)) {
                continue;
            }
            int nb_undefined = 0;
            if (skip_undefined_facts) {
                for (int val = 0; val < fp.value; ++val) {
                    if (task.is_undefined(FactPair(fp.var, val))) {
                        nb_undefined++;
                    }
                }
            }
            goal[shift[fp.var] + fp.value - nb_undefined] = 1;
        }
        bool first_round = true;
        for (int i : goal) {
            oss << (first_round ? "" : state_separator) << i;
            first_round = false;
        }
    } else {
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
}

void SamplingStateEngine::add_sampling_state_options(
        options::OptionParser &parser,
        const string &default_sample_format,
        const string &default_state_format,
        const std::string &default_field_separator,
        const std::string &default_state_separator) {

    parser.add_option<string> (
        "sample_format",
        "Format in which to write down the samples. The field order is"
        "V - value, current state, optionally goal condition. Choose from:"
        "csv: writes the data fields separated by 'separator'.",
        default_sample_format
    );
    parser.add_option<string> (
        "state_format",
        "Format in which to write down states. Choose from:"
        "fdr: write states as fdr representation of Fast Downward."
        "All values are separated by the separator.",
        default_state_format
    );
    parser.add_option<string> (
        "field_separator",
        "String sequence used to separate the fields in a sample",
        default_field_separator
    );
    parser.add_option<string> (
        "state_separator",
        "String sequence used to separate the items in a state field",
        default_state_separator
    );
    parser.add_option<bool> (
        "skip_undefined_facts",
        "Does not write down facts representing the variable is undefined",
        "false"
    );
}
}
