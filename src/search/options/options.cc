#include "options.h"

using namespace std;

namespace options {
Options::Options(bool help_mode)
    : unparsed_config("<missing>"),
      help_mode(help_mode) {
}

bool Options::contains(const string &key) const {
    return storage.find(key) != storage.end();
}

const string &Options::get_unparsed_config() const {
    return unparsed_config;
}

void Options::set_unparsed_config(const string &config) {
    unparsed_config = config;
}

void Options::set_registry(Registry *registry_) {
    registry = registry_;
}

Registry *Options::get_registry() const {
    if (registry == nullptr) {
        cerr << "Attempt to extract missing registry from options" << endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
    return registry;
}

void Options::set_predefinitions(const Predefinitions *predefinitions_) {
    predefinitions = predefinitions_;
}

const Predefinitions *Options::get_predefinitions() const {
    if (predefinitions == nullptr) {
        cerr << "Attempt to extract missing predefinition from options" << endl;
        utils::exit_with(utils::ExitCode::SEARCH_CRITICAL_ERROR);
    }
    return predefinitions;
}
}
