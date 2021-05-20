#include "sampling_search_base.h"

#include "../evaluator.h"
#include "../evaluation_context.h"
#include "../heuristic.h"
#include "../plugin.h"

#include "../task_utils/successor_generator.h"
#include "../task_utils/task_properties.h"
#include "../utils/countdown_timer.h"
#include "../utils/logging.h"

#include <algorithm>
#include <iostream>
#include <memory>
#include <set>
#include <string>

using namespace std;

namespace sampling_engine {

options::ParseTree prepare_search_parse_tree(
        const std::string &unparsed_config) {
    options::ParseTree pt = options::generate_parse_tree(unparsed_config);
    return subtree(pt, options::first_child_of_root(pt));
}

SamplingSearchBase::SamplingSearchBase(const options::Options &opts)
    : SamplingStateEngine(opts),
      search_parse_tree(prepare_search_parse_tree(opts.get_unparsed_config())),
      registry(*opts.get_registry()),
      predefinitions(*opts.get_predefinitions()),
      tmp_ignore_repredefinitions(opts.get_list<string>("ignore_repredefinitions")){
    ignore_repredefinitions = std::unordered_set<string>(
            tmp_ignore_repredefinitions.begin(),
            tmp_ignore_repredefinitions.end());
}


void SamplingSearchBase::next_engine() {
    utils::g_log.silence = true;
    sampling_engine::paths.clear();
    registry.handle_all_repredefinition(predefinitions, ignore_repredefinitions);

    options::OptionParser engine_parser(
        search_parse_tree, registry, predefinitions, false);
    engine = engine_parser.start_parsing<shared_ptr < SearchEngine >> ();
    if (engine->get_max_time() > timer->get_remaining_time()) {
        engine->reduce_max_time(timer->get_remaining_time());
    }
    utils::g_log.silence = false;
}

std::vector<std::string> SamplingSearchBase::sample(std::shared_ptr<AbstractTask> task) {
    utils::g_log << "." << flush;
    sampling_technique::modified_task = task;
    next_engine();
    utils::g_log.silence = true;
    engine->search();
    utils::g_log.silence = false;
    vector<string> samples;
    if (engine->found_solution()) {
        samples = extract_samples();
        post_search(samples);
    }
    return samples;
}


void SamplingSearchBase::post_search(vector<string> &/*samples*/) {}


void SamplingSearchBase::add_sampling_search_base_options(options::OptionParser &parser) {
    parser.add_option<shared_ptr < SearchEngine >> (
            "search",
            "Search engine to use for sampling");
    parser.add_list_option<string>(
            "ignore_repredefinitions",
            "List of predefined object types NOT to redefine for every "
            "search", "[]");
}
}
