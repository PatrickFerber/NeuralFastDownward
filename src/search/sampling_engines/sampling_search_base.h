#ifndef SEARCH_ENGINES_SAMPLING_SEARCH_BASE_H
#define SEARCH_ENGINES_SAMPLING_SEARCH_BASE_H

#include "sampling_state_engine.h"

#include "../options/predefinitions.h"
#include "../options/registries.h"

#include <deque>
#include <functional>
#include <memory>
#include <ostream>
#include <sstream>
#include <tree.hh>
#include <vector>

class Evaluator;
class Heuristic;
class PruningMethod;

namespace options {
class Options;
struct ParseNode;
using ParseTree = tree<ParseNode>;
}


namespace sampling_engine {

class SamplingSearchBase : public SamplingStateEngine {
protected:
    // Internal
    const options::ParseTree search_parse_tree;
    options::Registry registry;
    options::Predefinitions predefinitions;
    std::vector<std::string> tmp_ignore_repredefinitions;
    std::unordered_set<std::string> ignore_repredefinitions;

    std::shared_ptr<SearchEngine> engine;

    virtual void post_search(std::vector<std::string> &samples);
    virtual std::vector<std::string> extract_samples() = 0;

    virtual std::vector<std::string> sample(
            std::shared_ptr<AbstractTask> task) override;
    virtual void next_engine();

    

public:
    explicit SamplingSearchBase(const options::Options &opts);
    virtual ~SamplingSearchBase() override = default;

    static void add_sampling_search_base_options(options::OptionParser &parser);
};
}
#endif
