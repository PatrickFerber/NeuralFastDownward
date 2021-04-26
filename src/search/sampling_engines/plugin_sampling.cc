//#include "overlap_search.h"
#include "sampling_engine.h"
#include "sampling_v.h"
#include "sampling_search.h"
#include "sampling_tasks.h"
#include "../search_engines/search_common.h"

#include "../option_parser.h"
#include "../plugin.h"

using namespace std;

namespace plugin_sampling {
static shared_ptr<SearchEngine> _parse_sampling_search(OptionParser &parser) {
    parser.document_synopsis("Sampling Search Manager", "");

    sampling_engine::SamplingSearchBase::add_sampling_search_base_options(parser);
    sampling_engine::SamplingSearch::add_sampling_search_options(parser);
    sampling_engine::SamplingEngine::add_sampling_options(parser);
    sampling_engine::SamplingStateEngine::add_sampling_state_options(
            parser, "fields", "pddl", ";", "\t");
    SearchEngine::add_options_to_parser(parser);

    Options opts = parser.parse();
    shared_ptr<sampling_engine::SamplingSearch> engine;
    if (!parser.dry_run()) {
        engine = make_shared<sampling_engine::SamplingSearch>(opts);
    }

    return engine;
}

static Plugin<SearchEngine> _plugin_search1("sampling", _parse_sampling_search);
static Plugin<SearchEngine> _plugin_search2("sampling_search", _parse_sampling_search);


static shared_ptr<SearchEngine> _parse_sampling_v(OptionParser &parser) {
    parser.document_synopsis("Sampling Search Manager", "");


    sampling_engine::SamplingV::add_sampling_v_options(parser);
    sampling_engine::SamplingEngine::add_sampling_options(parser);
    sampling_engine::SamplingStateEngine::add_sampling_state_options(
            parser, "csv", "fdr", ";", ";");
    SearchEngine::add_options_to_parser(parser);



    Options opts = parser.parse();

    shared_ptr<sampling_engine::SamplingV> engine;
    if (!parser.dry_run()) {
        engine = make_shared<sampling_engine::SamplingV>(opts);
    }

    return engine;
}

static Plugin<SearchEngine> _plugin_q("sampling_v", _parse_sampling_v);


static shared_ptr<SearchEngine> _parse_sampling_tasks(OptionParser &parser) {
    parser.document_synopsis("Sampling Task Manager", "");

    sampling_engine::SamplingEngine::add_sampling_options(parser);
    SearchEngine::add_options_to_parser(parser);
    // Overwrite default values of the following two options
    parser.add_option<int> (
            "sample_cache_size",
            "If more than sample_cache_size samples are cached, then the entries "
            "are written to disk and the cache is emptied. When sampling "
            "finishes, all remaining cached samples are written to disk. If "
            "running out of memory, the current cache is lost.",
            "1");
    parser.add_option<bool>(
            "iterate_sample_files",
            "Every time the cache is emptied, the output file name is the "
            "specified file name plus an appended increasing index. If not set, "
            "then every time the cache is full, the samples will be appended to "
            "the output file.",
            "true");

    Options opts = parser.parse();



    shared_ptr<sampling_engine::SamplingTasks> engine;
    if (!parser.dry_run()) {
        engine = make_shared<sampling_engine::SamplingTasks>(opts);
    }

    return engine;
}

static Plugin<SearchEngine> _plugin_tasks1("generator", _parse_sampling_tasks);
static Plugin<SearchEngine> _plugin_tasks2("sampling_tasks", _parse_sampling_tasks);


//static shared_ptr<SearchEngine> _parse_sampling_overlap(OptionParser &parser) {
//    parser.document_synopsis("Sampling Overlap Manager", "");
//
//
//    overlap_search::OverlapSearch::add_overlap_options(parser);
//    SearchEngine::add_options_to_parser(parser);
//
//    Options opts = parser.parse();
//    shared_ptr<overlap_search::OverlapSearch> engine;
//    if (!parser.dry_run()) {
//        engine = make_shared<overlap_search::OverlapSearch>(opts);
//    }
//    return engine;
//}

//static Plugin<SearchEngine> _plugin_overlap("overlap_search", _parse_sampling_overlap);
}

