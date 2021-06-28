#ifndef SEARCH_ENGINES_SAMPLING_ENGINE_H
#define SEARCH_ENGINES_SAMPLING_ENGINE_H

#include "../search_engine.h"

#include "../sampling_techniques/sampling_technique.h"
#include "../utils/hash.h"

#include "sample_cache.h"

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
    const std::vector<std::shared_ptr<
        sampling_technique::SamplingTechnique>>sampling_techniques;
    std::vector<std::shared_ptr<
        sampling_technique::SamplingTechnique>>::const_iterator current_technique;
    SampleCacheManager sample_cache_manager;
    std::shared_ptr<utils::RandomNumberGenerator> rng;


    virtual void initialize() override;
    virtual void update_current_technique();
    virtual SearchStatus step() override;
    virtual std::vector<std::string> sample(
        std::shared_ptr<AbstractTask> task) = 0;
public:
    explicit SamplingEngine(const options::Options &opts);
    virtual ~SamplingEngine() = default;

    virtual std::string sample_file_header() const = 0;
    virtual void print_statistics() const override;
    virtual void save_plan_if_necessary() override;

    static void add_sampling_options(options::OptionParser &parser);
};
}
#endif
