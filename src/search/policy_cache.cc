#include "policy_cache.h"

using namespace std;


PolicyResult &PolicyCache::operator[](Policy *policy) {
    return policy_results[policy];
}
