#ifndef SEARCH_ENGINES_SAMPLE_CACHE_H
#define SEARCH_ENGINES_SAMPLE_CACHE_H

#include "../utils/hash.h"

#include <vector>
#include <string>

namespace sampling_engine {
class SamplingEngine;

class SampleCache {
public:
    struct Iterator {
        using iterator_category = std::forward_iterator_tag;
        using difference_type   = std::ptrdiff_t;
        using value_type        = const std::string;
        using pointer           = const std::string*;
        using reference         = const std::string&;

        Iterator(
            bool unique,
            std::vector<std::string>::iterator iter_vector,
            std::unordered_set<std::string>::iterator iter_set);
        reference operator*() const ;
        pointer operator->();
        Iterator& operator++();
        Iterator operator++(int);

        bool operator== (const Iterator& other);
        bool operator!= (const Iterator& other);

        std::vector<std::string>::iterator get_iter_vector();
        std::unordered_set<std::string>::iterator get_iter_set();

    private:
        const bool unique;
        std::vector<std::string>::iterator iter_vector;
        std::unordered_set<std::string>::iterator iter_set;
    };

private:
    const bool unique_samples;
    std::vector<std::string> redundant_cache;
    std::unordered_set<std::string> unique_cache;

public:
    SampleCache(bool unique_samples);

    template<class AnyIterator>
    void insert(AnyIterator first, AnyIterator last) {
        if (unique_samples) {
            unique_cache.insert(first, last);
        } else {
            redundant_cache.insert(redundant_cache.end(), first, last);
        }
    }
    void erase(Iterator first, Iterator last);
    Iterator begin();
    Iterator end();
    std::size_t size() const;
};

class SampleCacheManager {
protected:
    SampleCache sample_cache;
    const size_t max_size;

    const bool iterate_sample_files;
    const int max_sample_files;

    int index_sample_files;
    sampling_engine::SamplingEngine *engine;
    int nb_files_written = 0;
    int nb_samples_written = 0;
    bool is_finalized = false;

    void write_to_disk();
public:
    SampleCacheManager(
            SampleCache sample_cache,
            int max_size, bool iterate_sample_files,
            int max_sample_files, int index_sample_files,
            SamplingEngine *engine);
    template<class AnyIterator>
    void insert(AnyIterator first, AnyIterator last) {
        assert (!is_finalized);
        sample_cache.insert(first, last);
        if (sample_cache.size() >= max_size) {
            write_to_disk();
        }
    }
    void finalize();
    size_t size() const;

};
}
#endif
