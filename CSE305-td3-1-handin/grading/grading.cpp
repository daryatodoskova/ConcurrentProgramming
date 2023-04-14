#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdarg>
#include <chrono>
#include <future>
#include <iterator>
#include <string>
#include <regex>
#include <numeric>
#include <cmath>

#include "../gradinglib/gradinglib.hpp"
#include "td3.cpp"
#include <limits>

namespace tdgrading {

using namespace testlib;
using namespace std;

//-----------------------------------------------------------------------------

int test_find_parallel(std::ostream &out, const std::string test_name) {
    std::string fun_name = "FindParallel";

    start_test_suite(out, test_name);

    std::vector<int> res;
   
    for (size_t i = 0; i < 2000; ++i) {
        size_t len = (rand() % 30000) + 10;
        if (i < 2) {
            len = i + 1;
        }
        int* test = new int[len];
        for (size_t j = 0; j < len; ++j) {
            test[j] = rand() % (len / (i % 2 == 0 ? 9 : 200) + 1);
        }
        int count = std::count(test, test + len, test[0]);
        bool correct = (count >= 10);
        bool student_result = FindParallel<int>(test, len, test[0], 10, (rand() % 5) + 1);
        delete[] test;
        res.push_back(test_eq(
           out, fun_name, student_result, correct
        ));
    }

    size_t N = 10000000;
    int* long_vec = new int[N];
    for (size_t i = 0; i < N; ++i) {
        long_vec[i] = rand() % 100;
    }
    for (size_t i = 0; i < 5; ++i) {
        long_vec[i] = 101;
        long_vec[5000000 + i] = 101;
    }
    auto start = std::chrono::steady_clock::now();
    FindParallel<int>(long_vec, N, 101, 10, 2);
    auto end = std::chrono::steady_clock::now();
    auto rt = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    if (rt > 10) {
        print(out, "It seems that you do not terminate after finding the necessary number of occurences, too slow");
        res.push_back(0);
    } else {
        res.push_back(1);
    }

    for (size_t i = 0; i < N; ++i) {
        long_vec[i] = rand() % 100;
    }
    for (size_t i = 0; i < 5; ++i) {
        long_vec[i] = 101;
        long_vec[3333333 + i] = 101;
        long_vec[6666666 + i] = 101;
    }
    start = std::chrono::steady_clock::now();
    FindParallel<int>(long_vec, N, 101, 10, 3);
    end = std::chrono::steady_clock::now();
    rt = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    if (rt > 10) {
        print(out, "It seems that you do not terminate after finding the necessary number of occurences, too slow");
        res.push_back(0);
    } else {
        res.push_back(1);
    }
    delete[] long_vec;

    return end_test_suite(out, test_name,
                          accumulate(res.begin(), res.end(), 0), res.size());
}

//-----------------------------------------------------------------------------

// Auxiliary functions for testing the Account class
void withdraw(Account& a) {
    for (size_t i = 0; i < 1000000; ++i) {
        a.withdraw(1);
    }
}

void add(Account& a) {
    for (size_t i = 0; i < 1000000; ++i) {
        a.add(1);
    }
}

void transfer(Account& a, Account& b) {
    for (size_t i = 0; i < 1000; ++i) {
        Account::transfer(1, a, b);
    }
}

void generate_accounts1(std::vector<unsigned int>& result) {
    for (size_t i = 0; i < 100000; ++i) {
        Account a;
        result.push_back(a.get_id());
    }
}

void generate_accounts2(std::vector<unsigned int>& result) {
    for (size_t i = 0; i < 100000; ++i) {
        Account a(0);
        result.push_back(a.get_id());
    }
}

int check_deadlock() {
    Account B(1000);
    Account C(1000);
    std::thread t1(&transfer, std::ref(B), std::ref(C));
    std::thread t2(&transfer, std::ref(C), std::ref(B));
    t1.join();
    t2.join();
    return 1;
}

int test_account(std::ostream &out, const std::string test_name) {
    std::string fun_name = "Account";

    start_test_suite(out, test_name);
    std::vector<int> res;
  
    Account A(2000000);
    std::thread t1(&withdraw, std::ref(A));
    std::thread t2(&withdraw, std::ref(A));
    t1.join();
    t2.join();
    if (A.get_amount() != 0 ) {
        print(out, "Parallel withdrawals from an account interleave, consider using lock");
        res.push_back(0);
    } else {
        res.push_back(1);
    }
 
    std::thread t3(&add, std::ref(A));
    std::thread t4(&add, std::ref(A));
    t3.join();
    t4.join();
    if (A.get_amount() != 2000000 ) {
        print(out, "Parallel additions from an account interleave, consider using lock");
        res.push_back(0);
    } else {
        res.push_back(1);
    }
    
    std::future<int> f = std::async(std::launch::async, &check_deadlock);
    std::future_status status = f.wait_for(std::chrono::seconds(5));
    if (status != std::future_status::ready) {
        print(out, "Dealocks in parallel transfer");
        res.push_back(0);
    } else {
        res.push_back(1);
    }

    std::vector<unsigned int> ids1;
    std::vector<unsigned int> ids2;
    std::thread t5(&generate_accounts1, std::ref(ids1));
    std::thread t6(&generate_accounts2, std::ref(ids2));
    t5.join();
    t6.join();
    ids1.insert(ids1.end(), ids2.begin(), ids2.end());
    std::sort(ids1.begin(), ids1.end());
    for (auto it = ids1.begin(); it != ids1.end() - 1; ++it) {
        if (*it == *(it + 1)) {
            print(out, "Parallel creation of accounts yields equal ids");
            res.push_back(0);
        } else {
            res.push_back(1);
        }
    }

    return end_test_suite(out, test_name,
                          accumulate(res.begin(), res.end(), 0), res.size());
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
      "td3.cpp::FindParallel_test",
      "td3.cpp::Account_test"
  ],
  "points" : [5, 5]
}
[END-AUTOGRADER-ANNOTATION]
*/

    int const total_test_cases = 2;
    std::string const test_names[total_test_cases] = {
        "MaxParallel_test",
        "Account_test"
    };
    int const points[total_test_cases] = {5, 5};
    int (*test_functions[total_test_cases]) (std::ostream &, const std::string) = {
        test_find_parallel, test_account
    };

    return run_grading(out, test_case_number, total_test_cases,
                       test_names, points,
                       test_functions);
}

} // End of namepsace tdgrading
