#ifndef UTILS_RNG_H
#define UTILS_RNG_H

#include <algorithm>
#include <cassert>
#include <random>
#include <vector>

namespace utils {
class RandomNumberGenerator {
    // Mersenne Twister random number generator.
    std::mt19937 rng;

public:
    RandomNumberGenerator(); // Seed with a value depending on time and process ID.
    explicit RandomNumberGenerator(int seed);
    explicit RandomNumberGenerator(std::mt19937 &rng);
    RandomNumberGenerator(const RandomNumberGenerator &) = delete;
    RandomNumberGenerator &operator=(const RandomNumberGenerator &) = delete;
    ~RandomNumberGenerator();

    void seed(int seed);

    // Return random double in [0..1).
    double operator()() {
        std::uniform_real_distribution<double> distribution(0.0, 1.0);
        return distribution(rng);
    }

    // Return random integer in [0..bound).
    int operator()(int bound) {
        assert(bound > 0);
        std::uniform_int_distribution<int> distribution(0, bound - 1);
        return distribution(rng);
    }

    template<typename T>
    typename std::vector<T>::const_iterator choose(const std::vector<T> &vec) {
        return vec.begin() + operator()(vec.size());
    }

    template<typename T>
    typename std::vector<T>::iterator choose(std::vector<T> &vec) {
        return vec.begin() + operator()(vec.size());
    }

    template<typename W>
    size_t weighted_choose_index(const std::vector<W> &weights) {
        assert(all_of(weights.begin(), weights.end(), [](W i) {return i >= 0.0;}));
        double sum = std::accumulate(weights.begin(), weights.end(), 0.0);
        assert(sum > 0.0);
        double choice = operator()() * sum;
        for (size_t i = 0; i < weights.size(); ++i) {
            choice -= weights[i];
            if (choice < 0) {
                return i;
            }
        }
        assert(false);
        return 0;
    }


    template<typename T, typename W>
    typename std::vector<T>::const_iterator weighted_choose(
            const std::vector<T> &vec,
            const std::vector<W> &weights) {
        assert(vec.size() == weights.size());
        return vec.begin() + weighted_choose_index(weights);
    }

    template<typename T>
    void shuffle(std::vector<T> &vec) {
        std::shuffle(vec.begin(), vec.end(), rng);
    }
};
}

#endif
