#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdarg>
#include <chrono>
#include <iterator>
#include <string>
#include <regex>
#include <numeric>
#include <cmath>

#include "../gradinglib/gradinglib.hpp"
#include "td2.cpp"
#include <limits>

namespace tdgrading {

using namespace testlib;
using namespace std;

//-----------------------------------------------------------------------------

int test_max_parallel(std::ostream &out, const std::string test_name) {
    std::string fun_name = "MaxParallel";

    start_test_suite(out, test_name);

    std::vector<int> res;

    // Testing correctness
    for (size_t i = 0; i < 100; ++i) {
        size_t len = (rand() % 100) + 10;
        if (i < 2) {
            len = i;
        }
        double* test = new double[len];
        for (size_t j = 0; j < len; ++j) {
            test[j] = rand();
        }
        double correct = *std::max_element(test, test + len);

        size_t num_threads = 1 + (rand() % 5);
        double student_result = MaxParallel(test, len, num_threads);
        res.push_back(test_eq_approx(
            out, fun_name, student_result, correct, 0.1 
        ));
        delete[] test;
    }

    // Testing efficiency
    size_t N = 1000000;
    double* test = new double[N];
    for (size_t i = 0; i < N; ++i) {
        test[i] = rand();
    }
    auto start = std::chrono::steady_clock::now();
    double result = test[0];
    for (size_t i = 1; i < N; ++i) {
        result = std::max(test[i], result);
    }
    auto finish = std::chrono::steady_clock::now();
    auto elapsed_seq = std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count();
    delete[] test;

    test = new double[N];
    for (size_t i = 0; i < N; ++i) {
        test[i] = rand();
    }
    start = std::chrono::steady_clock::now();
    MaxParallel(test, N, 3);
    finish = std::chrono::steady_clock::now();
    auto elapsed_par = std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count();
    std::cout << "Testing if the parallel version is faster" << std::endl;
    std::cout << "Serial code runs in " << elapsed_seq << " microseconds while the parallel takes " << elapsed_par << std::endl;
    //res.push_back(test_le(out, fun_name, elapsed_par, (7 * elapsed_seq) / 10));
    delete[] test;

    return end_test_suite(out, test_name, accumulate(res.begin(), res.end(), 0), res.size());
}

//-----------------------------------------------------------------------------

void PrefixMaximumsSeq(double* start, int N, double* res) {
    res[0] = start[0];
    for (size_t i = 1; i < N; ++i) {
        res[i] = std::max(start[i], res[i - 1]);
    }
}

int test_prexif_maximums(std::ostream &out, const std::string test_name) {
    std::string fun_name = "PrefixMaximums";

    start_test_suite(out, test_name);

    std::vector<int> res;

    for (size_t i = 0; i < 150; ++i) {
        size_t len = (rand() % 100) + 10;
        double* test = new double[len];
        double* correct_result = new double[len];
        double* student_result = new double[len];
        for (size_t j = 0; j < len; ++j) {
            if (j < 10) {
                test[j] = -rand();
            } else {
                test[j] = rand() - RAND_MAX / 2;
            }
        }
        PrefixMaximumsSeq(test, len, correct_result);
        size_t num_threads = 1 + (rand() % 5);
        PrefixMaximums(test, len, num_threads, student_result);
        for (size_t i = 0; i < len; ++i) {
            res.push_back(test_eq_approx(
                out, fun_name, student_result[i], correct_result[i], 0.1
            ));
        }
        delete[] test;
        delete[] correct_result;
        delete[] student_result;
    }

    // Testing efficiency
    size_t N = 1000000;
    double* test = new double[N];
    for (size_t i = 0; i < N; ++i) {
        test[i] = rand();
    }
    double* result = new double[N];
    auto start = std::chrono::steady_clock::now();
    PrefixMaximumsSeq(test, N, result);
    auto finish = std::chrono::steady_clock::now();
    auto elapsed_seq = std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count();
    delete[] test;
    delete[] result;

    test = new double[N];
    result = new double[N];
    for (size_t i = 0; i < N; ++i) {
        test[i] = rand();
    }
    start = std::chrono::steady_clock::now();
    PrefixMaximums(test, N, 3, result);
    finish = std::chrono::steady_clock::now();
    auto elapsed_par = std::chrono::duration_cast<std::chrono::microseconds>(finish - start).count();
    std::cout << "Testing if the parallel version is faster" << std::endl;
    std::cout << "Serial code runs in " << elapsed_seq << " microseconds while the parallel takes " << elapsed_par << std::endl; 
    // res.push_back(test_le(out, fun_name, elapsed_par, (4 * elapsed_seq) / 5));
    delete[] test;
    delete[] result;


    return end_test_suite(out, test_name, accumulate(res.begin(), res.end(), 0), res.size());
}

//-----------------------------------------------------------------------------

int grading(std::ostream &out, const int test_case_number)
{
/**

Annotations used for the autograder.

[START-AUTOGRADER-ANNOTATION]
{
  "total" : 2,
  "names" : [
      "td2.cpp::MaxParallel_test",
      "td2.cpp::PrefixSums_test"
  ],
  "points" : [5, 5]
}
[END-AUTOGRADER-ANNOTATION]
*/

    int const total_test_cases = 2;
    std::string const test_names[total_test_cases] = {
        "MaxParallel_test",
        "PrefixMaximums_test"
    };
    int const points[total_test_cases] = {5, 5};
    int (*test_functions[total_test_cases]) (std::ostream &, const std::string) = {
        test_max_parallel,
        test_prexif_maximums
    };

    return run_grading(out, test_case_number, total_test_cases,
                       test_names, points,
                       test_functions);
}

} // End of namepsace tdgrading
