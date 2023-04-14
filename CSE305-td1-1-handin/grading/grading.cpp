#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdarg>
#include <iterator>
#include <string>
#include <regex>
#include <numeric>
#include <cmath>

#include "../gradinglib/gradinglib.hpp"
#include "td1.cpp"
#include <limits>

namespace tdgrading {

using namespace testlib;
using namespace std;

//-----------------------------------------------------------------------------

int test_sum_parallel(std::ostream &out, const std::string test_name) {
    std::string fun_name = "SumParallel";

    start_test_suite(out, test_name);

    std::vector<int> res;

    for (size_t i = 0; i < 100; ++i) {
        size_t len = (rand() % 100) + 10;
        if (i < 2) {
            len = i;
        }
        std::vector<long double> test;
        for (size_t j = 0; j < len; ++j) {
            test.push_back(rand() % 100);
        }
        auto f = [](long double x) -> long double {return 5 * x * x + 3 * x + 28;};

        std::vector<long double> after_func(len);
        std::transform(test.begin(), test.end(), after_func.begin(), f);
        long double correct = std::accumulate(after_func.begin(), after_func.end(), 0.);

        size_t num_threads = 1 + (rand() % 5);
        long double student_result = SumParallel(test.begin(), test.end(), f, num_threads);
        res.push_back(test_eq_approx(
            out, fun_name, student_result, correct, (long double)0.1 
        ));
    }

    return end_test_suite(out, test_name, accumulate(res.begin(), res.end(), 0), res.size());
}

//-----------------------------------------------------------------------------

int test_mean_parallel(std::ostream &out, const std::string test_name) {
    std::string fun_name = "MeanParallel";

    start_test_suite(out, test_name);

    std::vector<int> res;

    for (size_t i = 0; i < 150; ++i) {
        size_t len = (rand() % 100) + 10;
        std::vector<long double> test;
        for (size_t j = 0; j < len; ++j) {
            test.push_back(rand() % 100);
        }
        long double correct = std::accumulate(test.begin(), test.end(), 0.) / len;
        size_t num_threads = 1 + (rand() % 5);
        long double student_result = MeanParallel(test.begin(), test.end(), num_threads);
        res.push_back(test_eq_approx(
            out, fun_name, student_result, correct, (long double)0.1
        ));
    }

    return end_test_suite(out, test_name, accumulate(res.begin(), res.end(), 0), res.size());
}

//-----------------------------------------------------------------------------

int test_variance_parallel(std::ostream &out, const std::string test_name) {
    std::string fun_name = "VarianceParallel";

    start_test_suite(out, test_name);

    std::vector<int> res;


    for (size_t i = 0; i < 150; ++i) {
        size_t len = (rand() % 100) + 10;
        std::vector<long double> test;
        for (size_t j = 0; j < len; ++j) {
            test.push_back(rand() % 100);
        }

        long double sum = 0;
        long double sum_sq = 0;
        for (auto it = test.begin(); it != test.end(); ++it) {
            sum += *it;
            sum_sq += (*it) * (*it);
        }
        long double correct = sum_sq / len - (sum / len) * (sum / len);

        size_t num_threads = 1 + (rand() % 5);
        long double student_result = VarianceParallel(test.begin(), test.end(), num_threads);
        res.push_back(test_eq_approx(
            out, fun_name, student_result, correct, (long double)0.1
        ));
    }

    return end_test_suite(out, test_name, accumulate(res.begin(), res.end(), 0), res.size());
}

//-----------------------------------------------------------------------------

int test_count_mins_parallel(std::ostream &out, const std::string test_name) {
    std::string fun_name = "CountMinsParallel";

    start_test_suite(out, test_name);

    std::vector<int> res;

    for (size_t i = 0; i < 300; ++i) {
        size_t len = (rand() % 100) * 100 + 1000;
        std::vector<int> test;
        for (size_t j = 0; j < len; ++j) {
            test.push_back(rand() % 30);
        }
        int min = *std::min_element(test.begin(), test.end());
        int count = std::count(test.begin(), test.end(), min);
        size_t num_threads = 1 + (rand() % 5);
        int student_result = CountMinsParallel(test.begin(), test.end(), num_threads);
        res.push_back(test_eq(out, fun_name, student_result, count));
    }

    return end_test_suite(out, test_name, accumulate(res.begin(), res.end(), 0), res.size());
}

//-----------------------------------------------------------------------------

int test_find_parallel(std::ostream &out, const std::string test_name) {
    std::string fun_name = "FindParallel";

    start_test_suite(out, test_name);

    std::vector<int> res;

    for (size_t i = 0; i < 300; ++i) {
        size_t len = (rand() % 100) + 10;
        std::vector<int> test;
        for (size_t j = 0; j < len; ++j) {
            test.push_back(rand() % 30);
        }
        int to_search = rand() % 30;
        bool correct = !(std::find(test.begin(), test.end(), to_search) == test.end());
        size_t num_threads = 1 + (rand() % 5);
        bool student_result = FindParallel(test.begin(), test.end(), to_search, num_threads);
        res.push_back(test_eq(out, fun_name, student_result, correct));
    }

    return end_test_suite(out, test_name, accumulate(res.begin(), res.end(), 0), res.size());
}

//-----------------------------------------------------------------------------

int f_hard(int a) {
    int b = 3;
    for (int i = 0; i < a; ++i) {
        b = b * b;
    }
    return b;
}

int test_run_with_timeout(std::ostream &out, const std::string test_name) {
    std::string fun_name = "RunWithTimeout";

    start_test_suite(out, test_name);

    std::vector<int> res;

    // hard calculation
    long int input = 100000000;
    std::optional<int> result_hard = RunWithTimeout<int, int>(f_hard, input, 100);
    if (result_hard.has_value()) {
        print(out, "Very hard calculation finished in 100 milliseconds; something is wrong");
        res.push_back(0);
    } else {
        res.push_back(1);
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));

    // simple evaluation
    auto f_simple = [](int a) {return a + 5;};
    std::optional<int> result_simple = RunWithTimeout<int, int>(f_simple, 15, 50);
    if (not result_simple.has_value()) {
        print(out, "For simple calculation, expected to get result without timeout but to timeout");
        res.push_back(0);
    } else if (result_simple.value() != 20) {
        print(out, "Function computing x + 5 for x = 15 returned ", result_simple.value(), " but 20 was expected");
        res.push_back(0);
    } else {
        res.push_back(1);
    }

    // timeouting
    auto f_sleep = [](int a) {std::this_thread::sleep_for(std::chrono::milliseconds(200)); return 3 * a;};
    std::optional<int> result_sleep = RunWithTimeout<int, int>(f_sleep, 5, 100);
    if (result_sleep.has_value()) {
        print(out, "Sleeping for 200 milliseconds did not timeout with the timeout being 100 milliseconds");
        res.push_back(0);
    } else {
        res.push_back(1);
    }

    // side effects from the timeout
    std::optional<int> result_sf = RunWithTimeout<int, int>(f_simple, 5, 100);
    if (result_sf.value() == 15) {
        print(out, "If the previous run has timeouted, you are getting its result. Seems that you use std::ref, not std::shared_ptr");
        res.push_back(0);
    } else {
        res.push_back(1);
    }

    return end_test_suite(out, test_name, accumulate(res.begin(), res.end(), 0), res.size());
}


//-----------------------------------------------------------------------------

int grading(std::ostream &out, const int test_case_number)
{
/**

Annotations used for the autograder.

[START-AUTOGRADER-ANNOTATION]
{
  "total" : 6,
  "names" : [
      "td1.cpp::SumParallel_test",
      "td1.cpp::MeanParallel_test",
      "td1.cpp::VarianceParallel_test",
      "td1.cpp::CountMinsParallel_test",
      "td1.cpp::FindParallel_test",
      "td1.cpp::RunWithTimeout_test"
  ],
  "points" : [3, 3, 3, 3, 4, 4]
}
[END-AUTOGRADER-ANNOTATION]
*/

    int const total_test_cases = 6;
    std::string const test_names[total_test_cases] = {
        "SumParallel_test",
        "MeanParallel_test",
        "VarianceParallel_test",
        "CountMinsParallel_test",
        "FindParallel_test",
        "RunWithTimeout_test"
    };
    int const points[total_test_cases] = {3, 3, 3, 3, 4, 4};
    int (*test_functions[total_test_cases]) (std::ostream &, const std::string) = {
        test_sum_parallel,
        test_mean_parallel,
        test_variance_parallel,
        test_count_mins_parallel,
        test_find_parallel,
        test_run_with_timeout
    };

    return run_grading(out, test_case_number, total_test_cases,
                       test_names, points,
                       test_functions);
}

} // End of namepsace tdgrading
