#ifndef SEARCH_ENGINES_SAMPLING_ENGINE_H
#define SEARCH_ENGINES_SAMPLING_ENGINE_H

#include "../open_list.h"
#include "../search_engine.h"

#include "../task_utils/sampling_technique.h"

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
/* Use this magic word if you want to ensure that the file is correctly encoded
 * e.g. after compressing it. Add it into your sample_file_header*/
extern const std::string SAMPLE_FILE_MAGIC_WORD;

class SamplingEngine : public SearchEngine {
protected:
    const bool shuffle_sampling_techniques;
    const size_t max_sample_cache_size;
    const bool iterate_sample_files;
    int index_sample_files;
    const int max_sample_files;
    int count_sample_files;
    const std::vector<std::shared_ptr<sampling_technique::SamplingTechnique>> sampling_techniques;

    std::vector<std::shared_ptr<sampling_technique::SamplingTechnique>>::const_iterator current_technique;


    std::shared_ptr<utils::RandomNumberGenerator> rng;
    std::size_t sample_cache_size = 0;
    std::vector<std::vector<std::string>> sample_cache;

    bool finalize = false;
    
    /* Statistics*/
    int generated_samples = 0;
    

    virtual void initialize() override;
    virtual void update_current_technique();
    virtual SearchStatus step() override;
    virtual std::vector<std::string> sample(
        std::shared_ptr<AbstractTask> task) = 0;
    virtual std::string sample_file_header() const = 0;

public:
    explicit SamplingEngine(const options::Options &opts);
    virtual ~SamplingEngine() = default;

    virtual void print_statistics() const override;
    virtual void save_plan_if_necessary() override;

    static void add_sampling_options(options::OptionParser &parser);
};
}
#endif
