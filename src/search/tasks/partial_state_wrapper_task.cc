#include "partial_state_wrapper_task.h"

using namespace std;

namespace extra_tasks {
PartialStateWrapperTask::PartialStateWrapperTask(
    const shared_ptr<AbstractTask> &parent)
    : DelegatingTask(parent) { }

bool PartialStateWrapperTask::is_undefined(const FactPair& fact) const {
    return fact.value == parent->get_variable_domain_size(fact.var);
}

int PartialStateWrapperTask::get_variable_domain_size(int var) const {
    return parent->get_variable_domain_size(var) + 1;
}

std::string PartialStateWrapperTask::get_fact_name(const FactPair& fact) const {
    if (is_undefined(fact)) {
        return "<undefined>";
    } else {
        return parent->get_fact_name(fact);
    }
}

bool PartialStateWrapperTask::are_facts_mutex(const FactPair& fact1, const FactPair& fact2) const {
    /* 
     To be correct, we would need to check that if fact1 is undefined there
     at one of its variable values is not mutex with fact2 and vice versa and
     for the case that both are undefined.
     Assuming that the preprocessor was used(!), there is at least one value for
     an undefined variable s.t. it is not mutex to the other fact (otherwise
     they would have been merged together)
     */
    if (is_undefined(fact1) || is_undefined(fact2)) {
        return false;
    } else {
        return parent->are_facts_mutex(fact1, fact2);
    }
}
}
