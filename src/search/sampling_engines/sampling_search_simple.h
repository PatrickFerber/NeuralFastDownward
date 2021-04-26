#ifndef SEARCH_ENGINES_SAMPLING_SEARCH_SIMPLE_H
#define SEARCH_ENGINES_SAMPLING_SEARCH_SIMPLE_H

#include "sampling_search_base.h"

#include <vector>

namespace options {
class Options;
}


namespace sampling_engine {

class SamplingSearchSimple : public SamplingSearchBase {
protected:
    virtual std::vector<std::string> extract_samples() override;
    virtual std::string sample_file_header() const override;


public:
    explicit SamplingSearchSimple(const options::Options &opts);
    virtual ~SamplingSearchSimple() override = default;
};
}
#endif
