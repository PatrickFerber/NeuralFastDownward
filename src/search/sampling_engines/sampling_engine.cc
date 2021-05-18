#include "sampling_engine.h"

#include "../option_parser.h"

#include "../sampling_techniques/technique_null.h"

#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>
#include <numeric>
#include <set>
#include <cstdio>
#include <string>


using namespace std;

namespace sampling_engine {
const string SAMPLE_FILE_MAGIC_WORD = "# MAGIC FIRST LINE";
/* Global variable for sampling algorithms to store arbitrary paths (plans [
   operator ids] and trajectories [state ids]).
   (as everybody can access it, be certain who does and how) */


static vector<shared_ptr<sampling_technique::SamplingTechnique>>
prepare_sampling_techniques(
    vector<shared_ptr<sampling_technique::SamplingTechnique>> input) {
    if (input.empty()) {
        input.push_back(make_shared<sampling_technique::TechniqueNull>());
    }
    return input;
}

SamplingEngine::SamplingEngine(const options::Options &opts)
    : SearchEngine(opts),
      shuffle_sampling_techniques(opts.get<bool>("shuffle_techniques")),
      max_sample_cache_size(opts.get<int>("sample_cache_size")),
      iterate_sample_files(opts.get<bool>("iterate_sample_files")),
      index_sample_files(opts.get<int>("index_sample_files")),
      max_sample_files(opts.get<int>("max_sample_files")),
      count_sample_files(0),
      sampling_techniques(
        prepare_sampling_techniques(
            opts.get_list<shared_ptr<sampling_technique::SamplingTechnique>>(
                "techniques"))),
      current_technique(sampling_techniques.begin()),
      rng(utils::parse_rng_from_options(opts)) {
    if (max_sample_cache_size <= 0) {
        cerr << "sample_cache_size has to be positive: "
             << max_sample_cache_size << endl;
        utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
    }
    if (max_sample_files != -1 && max_sample_files <= 0) {
        cerr << "max_sample_files has to be positive or -1 for unlimited: "
             << max_sample_files << endl;
        utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
    }
}

void SamplingEngine::initialize() {
    cout << "Initializing Sampling Engine...";
    remove(plan_manager.get_plan_filename().c_str());
    cout << "done." << endl;
}

void SamplingEngine::update_current_technique() {
    if (shuffle_sampling_techniques) {
        current_technique = sampling_techniques.end();
        int total_remaining = accumulate(
                sampling_techniques.begin(),
                sampling_techniques.end(),
                0,
                [] (
                        int sum,
                        const shared_ptr<sampling_technique::SamplingTechnique> &st) {
                    assert (st->get_count() >= st->get_counter());
                    return sum + (st->get_count() - st->get_counter());});
        if (total_remaining == 0) {
            return;
        }
        int chosen = (*rng)(total_remaining);
        for (auto iter = sampling_techniques.begin();
                iter != sampling_techniques.end(); ++iter){
            int remaining = (*iter)->get_count() - (*iter)->get_counter();
            if (chosen < remaining) {
                current_technique = iter;
                break;
            } else {
                chosen -= remaining;
            }
        }
        assert (current_technique != sampling_techniques.end());
    } else {
        while(current_technique != sampling_techniques.end() &&
              (*current_technique)->empty()) {
            current_technique++;
        }
    }
}

SearchStatus SamplingEngine::step() {
    assert(!finalize);
    update_current_technique();
    if (current_technique == sampling_techniques.end()) {
        finalize = true;
        save_plan_if_necessary();
        return SOLVED;
    }

    const shared_ptr<AbstractTask> next_task = (*current_technique)->next(task);
    vector<string> new_samples =  sample(next_task);
    sample_cache.push_back(new_samples);
    sample_cache_size += new_samples.size();
    if (sample_cache_size > max_sample_cache_size) {
        save_plan_if_necessary();
    }
    return IN_PROGRESS;
}

void SamplingEngine::print_statistics() const {

    cout << "Generated Entries: " << (generated_samples + sample_cache_size)
         << endl;
    cout << "Sampling Techniques used:" << endl;
    for (auto &st : sampling_techniques) {
        cout << '\t' << st->get_name();
        cout << ":\t" << st->get_counter();
        cout << "/" << st->get_count() << '\n';
    }
}

void SamplingEngine::save_plan_if_necessary() {
    if (get_status() != SearchStatus::IN_PROGRESS) {
        finalize = true;
    }
    assert(finalize || sample_cache_size >= max_sample_cache_size);
    while ((finalize || sample_cache_size >= max_sample_cache_size) &&
           sample_cache_size > 0 &&
           (max_sample_files == -1 || count_sample_files < max_sample_files)) {
        ofstream outfile(
            plan_manager.get_plan_filename() + 
            ((iterate_sample_files) ? to_string(index_sample_files++) : ""),
            iterate_sample_files ? ios::trunc : ios::app);
        outfile << sample_file_header() << endl;
        size_t nb_samples = 0;
        size_t idx_vector = 0;
        size_t idx_sample = 0;
        for (const vector<string> & samples: sample_cache) {
            idx_sample = 0;
            for (const string & sample : samples){
                outfile << sample << endl;
                if (++nb_samples >= max_sample_cache_size) {
                    break;
                }
                idx_sample++;
            }
            if (nb_samples >= max_sample_cache_size){
                break;
            }
            idx_vector++;
        }
        outfile.close();
        assert(finalize || nb_samples == max_sample_cache_size);
        if (finalize && nb_samples < max_sample_cache_size) {
            idx_vector--;
            idx_sample--;
        }
        //Erase saved samples
        sample_cache[idx_vector].erase(
            sample_cache[idx_vector].begin(),
            sample_cache[idx_vector].begin() + idx_sample + 1);
        sample_cache.erase(
            sample_cache.begin(),
            sample_cache.begin() + idx_vector + 
                ((sample_cache[idx_vector].empty()) ? 1 : 0));
        sample_cache_size -= nb_samples;
        generated_samples += nb_samples;
        count_sample_files++;
    }
}

void SamplingEngine::add_sampling_options(options::OptionParser &parser) {
    parser.add_list_option<shared_ptr < 
        sampling_technique::SamplingTechnique >> (
        "techniques",
        "List of sampling technique definitions to use",
        "[]");
    parser.add_option<bool>("shuffle_techniques",
            "Instead of using one sampling technique after each other,"
            "for each task to generate a sampling technique is randomly chosen with"
            "their probability proportional to the remaining tasks of that technique.",
            "false");

    parser.add_option<int> (
        "sample_cache_size",
        "If more than sample_cache_size samples are cached, then the entries "
         "are written to disk and the cache is emptied. When sampling "
         "finishes, all remaining cached samples are written to disk. If "
         "running out of memory, the current cache is lost.", 
        "5000");
    parser.add_option<bool>(
        "iterate_sample_files",
        "Every time the cache is emptied, the output file name is the "
        "specified file name plus an appended increasing index. If not set, "
        "then every time the cache is full, the samples will be appended to "
        "the output file.",
        "false");
    parser.add_option<int>(
        "index_sample_files",
        "Initial index to append to the sample files written. Works only in "
        "combination with iterate_sample_files",
        "0");
    parser.add_option<int>(
        "max_sample_files",
        "Maximum number of sample files which will be written to disk."
        " After writing that many files, the search will terminate. Use -1 "
        "for unlimited.",
        "-1"
            );
    utils::add_rng_options(parser);
}
}
