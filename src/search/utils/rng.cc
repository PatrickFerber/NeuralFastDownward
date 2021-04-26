#include "rng.h"

#include "system.h"

#include <chrono>

using namespace std;

namespace utils {
/*
  Ideally, one would use true randomness here from std::random_device. However,
  there exist platforms where this returns non-random data, which is condoned by
  the standard. On these platforms one would need to additionally seed with time
  and process ID (PID), and therefore generally seeding with time and PID only
  is probably good enough.
*/
RandomNumberGenerator::RandomNumberGenerator() {
    unsigned int secs = static_cast<unsigned int>(
        chrono::system_clock::now().time_since_epoch().count());
    seed(secs + get_process_id());
}

RandomNumberGenerator::RandomNumberGenerator(int seed_) {
    seed(seed_);
}

RandomNumberGenerator::RandomNumberGenerator(std::mt19937 &rng) : rng(rng) {
}

RandomNumberGenerator::~RandomNumberGenerator() { }

void RandomNumberGenerator::seed(int seed) {
    rng.seed(seed);
}

vector<int> RandomNumberGenerator::choose_n_of_N(int n, int N) {
    vector<int> result;
    result.reserve(n);
    for (int i = 0; i < n; ++i) {
        int r;
        do {
            r = (*this)(N);
        } while (find(std::begin(result), std::end(result), r) != std::end(result));
        result.push_back(r);
    }
    return result;
}
}
