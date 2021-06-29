#include "sample_cache.h"

#include "sampling_engine.h"

#include "../plan_manager.h"

#include "../utils/system.h"

#include <fstream>
#include <iostream>

using namespace std;

namespace sampling_engine {

SampleCache::Iterator::Iterator(
        bool unique,
        std::vector<std::string>::iterator iter_vector,
        utils::HashSet<std::string>::iterator iter_set)
        : unique(unique),
          iter_vector(iter_vector),
          iter_set(iter_set) {}

SampleCache::Iterator::reference SampleCache::Iterator::operator*() const {
    return unique ? *iter_set : *iter_vector;
}
SampleCache::Iterator::pointer SampleCache::Iterator::operator->() {
    return &operator*();
}
SampleCache::Iterator& SampleCache::Iterator::operator++() {
    if (unique) {iter_set++;} else {iter_vector++;}
    return *this;
}
SampleCache::Iterator SampleCache::Iterator::operator++(int) {
    Iterator tmp = *this;
    ++(*this);
    return tmp;}

bool SampleCache::Iterator::operator==(const Iterator& other) {
    return unique == other.unique &&
           iter_vector == other.iter_vector &&
           iter_set == other.iter_set;
}

bool SampleCache::Iterator::operator!=(const Iterator& other) {
    return ! (*this == other);
}

std::vector<std::string>::iterator SampleCache::Iterator::get_iter_vector() {
    return iter_vector;
}

utils::HashSet<std::string>::iterator SampleCache::Iterator::get_iter_set() {
    return iter_set;
}

SampleCache::SampleCache(bool unique_samples)
    : unique_samples(unique_samples) {}

void SampleCache::erase(Iterator first, Iterator last) {
        if(unique_samples){
            unique_cache.erase(first.get_iter_set(), last.get_iter_set());
        } else {
            redundant_cache.erase(first.get_iter_vector(), last.get_iter_vector());
        }
}

size_t SampleCache::size() const {
    return unique_samples ? unique_cache.size() : redundant_cache.size();
}

SampleCache::Iterator SampleCache::begin() {
    return Iterator(unique_samples, redundant_cache.begin(), unique_cache.begin());
}

SampleCache::Iterator SampleCache::end() {
    return Iterator(unique_samples, redundant_cache.end(), unique_cache.end());
}


SampleCacheManager::SampleCacheManager(
        SampleCache sample_cache, int max_size, bool iterate_sample_files,
        int max_sample_files, int index_sample_files,
        SamplingEngine *engine)
        : sample_cache(sample_cache),
          max_size(max_size),
          iterate_sample_files(iterate_sample_files),
          max_sample_files(max_sample_files),
          index_sample_files(index_sample_files),
          engine(engine){
    if (max_size <= 0) {
        cerr << "The maximum cache size has be positive: "
             << max_size << endl;
        utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
    }
    if (max_sample_files != -1 && max_sample_files <= 0) {
        cerr << "The maximum number of sample files to write has to be "
                "positive or unlimited (-1): "
             << max_sample_files << endl;
        utils::exit_with(utils::ExitCode::SEARCH_INPUT_ERROR);
    }
}

void SampleCacheManager::write_to_disk() {
    assert(is_finalized || sample_cache.size() >= max_size);
    int newly_written = 0;
    SampleCache::Iterator iter = sample_cache.begin();
    while (((is_finalized && iter != sample_cache.end()) ||
             sample_cache.size() - newly_written >= max_size) &&
           (max_sample_files == -1 || nb_files_written < max_sample_files)) {

        ofstream outfile(
                engine->get_plan_manager().get_plan_filename() +
                ((iterate_sample_files) ? to_string(index_sample_files++) : ""),
                iterate_sample_files ? ios::trunc : ios::app);
        outfile << engine->sample_file_header() << "\n";

        size_t nb_samples = 0;
        for (; iter != sample_cache.end() && nb_samples < max_size;
               ++iter, ++nb_samples) {
            outfile << *iter << "\n";
        }

        outfile.close();
        assert(is_finalized || nb_samples == max_size);
        newly_written += nb_samples;
        ++nb_files_written;
    }
    nb_samples_written += newly_written;
    sample_cache.erase(sample_cache.begin(), iter);
}

void SampleCacheManager::finalize() {
    assert (!is_finalized);
    is_finalized = true;
    write_to_disk();
}

size_t SampleCacheManager::size() const {
    return nb_samples_written + sample_cache.size();
}
}
