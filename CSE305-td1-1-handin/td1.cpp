#pragma once
#include <climits>
#include <thread>
#include <numeric>
#include <iterator>
#include <optional>
#include <vector>
#include <iostream>

typedef long double Num;
typedef std::vector<long double>::const_iterator NumIter;

//-----------------------------------------------------------------------------

void SumMapThread(NumIter begin, NumIter end, Num f(Num), long double& result) {
    Num local_result = 0;
    while (begin != end) {
        local_result += f(*begin);
        ++begin;
    }
    result = local_result;
}

/**
 * @brief Sums f(x) for x in [begin, end)
 * @param begin Start iterator
 * @param end End iterator
 * @param f Function to apply
 * @param result Reference to the result variable
 */
Num SumParallel(NumIter begin, NumIter end, Num f(Num), size_t num_threads) {
    size_t length = end - begin;
    if (length == 0) {
        return 0.;
    }
    size_t block_size = length / num_threads;
    std::vector<Num> results(num_threads, 0.);
    std::vector<std::thread> workers(num_threads - 1);
    NumIter start_block = begin;
    for (size_t i = 0; i < num_threads - 1; ++i) {
        NumIter end_block = start_block + block_size;
        workers[i] = std::thread(&SumMapThread, start_block, end_block, f, std::ref(results[i]));
        start_block = end_block;
    }
    SumMapThread(start_block, end, f, results[num_threads - 1]);

    for (size_t i = 0; i < num_threads - 1; ++i) {
        workers[i].join();
    }

    Num total_result = 0.;
    for (size_t i = 0; i < results.size(); ++i) {
        total_result += results[i];
    }
    return total_result;
}

//-----------------------------------------------------------------------------

/**
 * @brief Computes the mean of the numbers in [begin, end)
 * @param begin Start iterator
 * @param end End iterator
 * @param num_threads The number of threads to use
 * @return The mean in the range
*/
Num MeanParallel(NumIter begin, NumIter end, size_t num_threads) {
    size_t length = end - begin;
    Num sum = SumParallel(begin, end, [](Num x) -> Num {return x;}, num_threads);
    Num mean = sum / length;
    return mean;
}

//-----------------------------------------------------------------------------

/**
 * @brief Computes the variance of the numbers in [begin, end)
 * @param begin Start iterator
 * @param end End iterator
 * @param num_threads The number of threads to use
 * @return The variance in the range
*/
Num VarianceParallel(NumIter begin, NumIter end, size_t num_threads) { //Var(X)=E(X^2)−(EX)^2
    size_t length = end - begin;
    Num average = MeanParallel(begin, end, num_threads);
    Num squares_mean = SumParallel(begin, end, [](Num x) {return x * x;}, num_threads) / length;
    Num var = squares_mean - average * average;
    return var;
}

//-----------------------------------------------------------------------------

/**
 * @brief Computes the occurences of the minimal value in [begin, end)
 * @param begin Start iterator
 * @param end End iterator
 * @param num_threads The number of threads to use
 * @return the number of occurences of the minimal value in [begin, end)
*/


void FindCountMins(std::vector<int>::const_iterator begin, std::vector<int>::const_iterator end, int& min, int& cnt) {
    cnt = 0;
    min = INT_MAX;
    for (std::vector<int>::const_iterator iter = begin; iter != end; ++iter) {
        if (*iter == min) {
            ++cnt;
            //std::cout<<min;
        }
        if (*iter < min) {
            min = *iter;
            cnt = 1;
        }
    }
}

// returns the number of occurences of the minimal value in [begin, end)
int CountMinsParallel(std::vector<int>::const_iterator begin, std::vector<int>::const_iterator end, size_t num_threads) {
    size_t length = end - begin;
    if (length == 0) {
        return 0.;
    }
    size_t block_size = length / num_threads;
    std::vector<int> mins(num_threads, 0);
    std::vector<int> cnts(num_threads, 0);
    std::vector<std::thread> workers(num_threads - 1);
    std::vector<int>::const_iterator start_block = begin;
    for (size_t i = 0; i < num_threads - 1; ++i) {
        std::vector<int>::const_iterator end_block = start_block + block_size;
        workers[i] = std::thread(&FindCountMins, start_block, end_block, std::ref(mins[i]), std::ref(cnts[i]));
        start_block = end_block;
    }
    FindCountMins(start_block, end, mins[num_threads - 1], cnts[num_threads - 1]);
    for (size_t i = 0; i < num_threads - 1; ++i) {
        workers[i].join();
    }
    int min = INT_MAX;
    int cnt = 0;
    for (size_t i = 0; i < mins.size(); ++i) {
        if (mins[i] == min) {
            cnt += cnts[i];
        }
        if (mins[i] < min) {
            cnt = cnts[i];
            min = mins[i];
        }
    }
    return cnt;
}
    

//-----------------------------------------------------------------------------

/**
 * @brief Finds target in [begin, end)
 * @param begin Start iterator
 * @param end End iterator
 * @param target The target to search for
 * @param num_threads The number of threads to use
 * @return The sum in the range
*/
template <typename Iter, typename T>
bool FindParallel(Iter begin, Iter end, T target, size_t num_threads) {
    // takes the start and end iterators of a container 
    // searches for the target value in parallel using num_threads threads
    size_t length = std::distance(begin, end);
    if (length == 0) {
        return false;
    }
    size_t block_size = length / num_threads;
    std::vector<std::thread> workers(num_threads - 1);
    
    bool found = false;
    Iter start_block = begin;
    
    for (size_t i = 0; i < num_threads - 1; ++i) {
        Iter end_block = start_block;
        std::advance(end_block, block_size);
        workers[i] = std::thread([&]{
            while (begin != end) {
                if ((*begin == target) or found) {
                    found = true;
                    return;
                }
                ++begin;
            }
        });
        start_block = end_block;
    }
    while (start_block != end && !found) {
        if (*start_block == target) {
            found = true;
            break;
        }
        ++start_block;
    }


    for (size_t i = 0; i < num_threads - 1; ++i) {
        workers[i].join();
    }

    return found;
}

//-----------------------------------------------------------------------------


/**
 * @brief Runs a function and checks whether it finishes within a timeout
 * @param f Function to run
 * @param arg Arguments to pass to the function 
 * @param timeout The timeout
 * @return std::optional with result (if the function finishes) and empty (if timeout)
 */
template <typename ArgType, typename ReturnType>
std::optional<ReturnType> RunWithTimeout(ReturnType f(ArgType), ArgType arg, size_t timeout) {
    // YOUR CODE HERE (AND MAYBE AN AUXILIARY FUNCTION OUTSIDE)
    return {};
}

//-----------------------------------------------------------------------------
