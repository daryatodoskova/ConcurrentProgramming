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
#include "td6.cpp"
#include <limits>

namespace tdgrading {

using namespace testlib;

int test_ordered_vec(std::ostream &out, const std::string test_name) {
    std::string fun_name = "OrderedVec";

    start_test_suite(out, test_name);

    std::vector<int> res;
   
    for (size_t i = 0; i < 1000; ++i) {
        OrderedVec v;
        for (size_t j = 0; j < 1000; ++j) {
            v.insert(rand() % 100);
        }
        std::vector<int> data = v.get_data();
        if (!std::is_sorted(data.begin(), data.end())) {
            print(out, "The data is not maintained in the sorted order");
            res.push_back(0);
        } else {
           res.push_back(1);
        }

        if (v.search(101)) {
            print(out, "Finds nonexisting element");
            res.push_back(0);
        } else {
           res.push_back(1);
        }

        if (!v.search(data[0])) {
            print(out, "Does not find an element which is present");
            res.push_back(0);
        } else {
           res.push_back(1);
        }
    }

    return end_test_suite(out, test_name,
                          accumulate(res.begin(), res.end(), 0), res.size());
}

int test_divide_once_even(std::ostream& out, const std::string test_name) {
    std::string fun_name = "DivideOnceEven";
    
    start_test_suite(out, test_name);    
    std::vector<int> res;

    std::mutex m;
    std::condition_variable iseven;
    int n = 5;
    std::thread t(&DivideOnceEven, std::ref(iseven), std::ref(m), std::ref(n));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    iseven.notify_all();
    if (n == 5) {
        res.push_back(1);
    } else {
        res.push_back(0);
    }

    n = 6;
    iseven.notify_all();
    t.join();
    if (n == 3) {
        res.push_back(1);
    } else {
        res.push_back(0);
    }

    std::thread t1(&DivideOnceEven, std::ref(iseven), std::ref(m), std::ref(n));
    std::thread t2(&DivideOnceEven, std::ref(iseven), std::ref(m), std::ref(n));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    n = 20;
    iseven.notify_all();
    t1.join();
    t2.join();
    if (n == 5) {
        res.push_back(1);
    } else {
        res.push_back(0);
    }

    return end_test_suite(out, test_name,
                          accumulate(res.begin(), res.end(), 0), res.size());
}

int test_safe_unbounded_queue(std::ostream &out, const std::string test_name) {
    std::string fun_name = "SafeUnboundedQueue";

    start_test_suite(out, test_name);

    std::vector<int> res;

    // one-thread test
    SafeUnboundedQueue<int> Q;
    auto start = std::chrono::steady_clock::now();
    for (size_t i = 0; i < 100000; ++i) {
        Q.push(i);
    }
    bool preserved = true;
    for (size_t i = 0; i < 100000; ++i) {
        int res = Q.pop();    
        if (res != i) {
            print(out, "Order is not preserved with single-thread pushing");
            preserved = false;
        }
    }
    auto end = std::chrono::steady_clock::now();
    if (preserved) {
        res.push_back(1);
    } else {
        res.push_back(0);
    }

    int duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    if (duration > 2000) {
        print(out, "Your queue is pretty slow. Are you using std::queue?");
        res.push_back(0);
    } else {
        res.push_back(1);
    }

    // multiple threads pushing
    SafeUnboundedQueue<int> push_parallel;
    std::thread even([](SafeUnboundedQueue<int>& q) -> void {
            for (size_t i = 0; i < 100000; ++i) {
                q.push(2 * i);
            }
        }, std::ref(push_parallel)        
    );
    std::thread odd([](SafeUnboundedQueue<int>& q) -> void {
            for (size_t i = 0; i < 100000; ++i) {
                q.push(2 * i + 1);
            }
        }, std::ref(push_parallel)
    );
    even.join();
    odd.join();
    int max_odd = -1;
    int max_even = -2;
    int total = 0;
    bool reversed = false;
    while (!push_parallel.is_empty()) {
        int next = push_parallel.pop();
        if (next % 2 == 0) {
            if (next <= max_even) {
                reversed = true;
                print(out, "Order is reversed while pushing");
            }
            max_even = std::max(max_even, next);
        } else {
            if (next <= max_odd) {
                reversed = true;
                print(out, "Order is reversed while pushing");
            }
            max_odd = std::max(max_odd, next);
        }
        ++total;
    }
    if (reversed) {
        res.push_back(0);
    } else {
        res.push_back(1);
    }
    if (total < 200000) {
        print(out, "Elements lost while pushing");
        res.push_back(0);
    } else {
        res.push_back(1);
    }

    // multiple threads reading
    SafeUnboundedQueue<int> pop_parallel;
    for (size_t i = 0; i < 1000000; ++i) {
        pop_parallel.push(i);
    }

    auto popper = [](SafeUnboundedQueue<int>& q, std::vector<int>& dest) -> void {
        while (!q.is_empty()) {
            int a = q.pop();
            dest.push_back(a);
            if ((a == 999999) || (a == 999998)) {
                break;
            }
        }
    };
    std::vector<int> A;
    std::vector<int> B;
    std::thread ta(popper, std::ref(pop_parallel), std::ref(A));
    std::thread tb(popper, std::ref(pop_parallel), std::ref(B));
    ta.join();
    tb.join();
    std::vector<int> merged(A.size() + B.size());
    std::merge(A.begin(), A.end(), B.begin(), B.end(), merged.begin());
    if (merged.size() != 1000000) {
        print(out, "Wrong number of popped elements ", merged.size(), " instead of 1000000");
        res.push_back(0);
    } else {
        res.push_back(1);
    }

    bool not_sequence = false;
    for (size_t i = 0; i < merged.size(); ++i) {
        if (merged[i] != i) {
            print(out, "Popped numbers are not the same as pushed");
            not_sequence = true;
        }
    }
    if (not_sequence) {
        res.push_back(0);
    } else {
        res.push_back(1);
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
  "total" : 3,
  "names" : ["td6.cpp::OrderedVec", "td6.cpp::DivideOnceEven", "td6.cpp::SafeUnboundedQueue"],
  "points" : [7, 6, 7]
}
[END-AUTOGRADER-ANNOTATION]
*/

    int const total_test_cases = 3;
    std::string const test_names[total_test_cases] = {"OrderedVec", "DivideOnceEven", "SafeUnboundedQueue"};
    int const points[total_test_cases] = {7, 6, 7};
    int (*test_functions[total_test_cases]) (std::ostream &, const std::string) = {
      test_ordered_vec, test_divide_once_even, test_safe_unbounded_queue
    };

    return run_grading(out, test_case_number, total_test_cases,
                       test_names, points,
                       test_functions);
}

} // End of namepsace tdgrading
