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
#include "td7.cpp"
#include <limits>

namespace tdgrading {

using namespace testlib;
using namespace std;

//-----------------------------------------------------------------------------

std::string foo(const std::string& s) {
    return "A" + s;
}

void inserter(SetListTrans& s) {
    for (size_t i = 0; i < 200000; ++i) {
        s.add("a");
    }
}

std::string bar(const std::string& s) {
    return "a";
}

int test_transform(std::ostream &out, const std::string test_name) {
    std::string fun_name = "SetListTrans";

    start_test_suite(out, test_name);

    std::vector<int> res;

    SetListTrans s;
    s.add("abc");
    s.add("def");
    s.add("egh");
    s.transform(&foo);

    if (
            s.contains("abc") or s.contains("def") or s.contains("egh") or
            !s.contains("Aabc") or !s.contains("Adef") or !s.contains("Aegh")
    ) {
        print(out, "Something wrong with a transform of {abc, def, egh}");
        res.push_back(0);
    } else {
        res.push_back(1);
    }

    std::thread t(&inserter, std::ref(s));
    for (size_t i = 0; i < 100; ++i) {
        s.add("b");
    }
    s.transform(&foo);
    t.join();

    if (!s.contains("Ab") or s.contains("b") or !s.contains("a")) {
        print(out, "Something wrong with concurrent insertion");
        res.push_back(0);
    } else {
        res.push_back(1);
    }

    s.transform(&bar);

    if (s.size() != 1) {
        print(out, "It seems that the case when the transfrom is not injective is not handled");
        res.push_back(0);
    } else {
        res.push_back(1);
    }


    return end_test_suite(out, test_name,
                          accumulate(res.begin(), res.end(), 0), res.size());
}

//-----------------------------------------------------------------------------


void simple_inserter(BoundedSetList& s, std::string& str) {
    s.add(str);
}

int test_bounded(std::ostream &out, const std::string test_name) {
    std::string fun_name = "BoundedSetList";

    start_test_suite(out, test_name);

    std::vector<int> res;

    BoundedSetList s(4);
    std::vector<std::string> items{"a", "b", "c", "d", "e", "f", "g"};
    std::vector<std::thread> inserters;
    for (auto it = items.begin(); it != items.end(); ++it) {
        inserters.emplace_back(std::thread(&simple_inserter, std::ref(s), std::ref(*it)));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::vector<size_t> sizes;
    while (s.get_count() > 0) {
        sizes.push_back(s.get_count());
        for (auto it = items.begin(); it != items.end(); ++it) {
            if (s.contains(*it)) {
                s.remove(*it);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                break;
            }
        }
    }
    std::for_each(inserters.begin(), inserters.end(), [](std::thread& t) {t.join();});
    if (sizes != std::vector<size_t>{4, 4, 4, 4, 3, 2, 1}) {
        print(out, "The sizes do not behave as expected, we are expecting size to behave like 4. 4, 4, 4, 3, 2, 1");
        res.push_back(0);
    } else {
        res.push_back(1);
    }


    return end_test_suite(out, test_name,
                          accumulate(res.begin(), res.end(), 0), res.size());
}

int grading(std::ostream &out, const int test_case_number)
{
/**

Annotations used for the autograder.

[START-AUTOGRADER-ANNOTATION]
{
  "total" : 2,
  "names" : ["td7.cpp::SetListTrans", "td7.cpp::BoundedSetList"],
  "points" : [6, 6]
}
[END-AUTOGRADER-ANNOTATION]
*/

    int const total_test_cases = 2;
    std::string const test_names[total_test_cases] = {"SetListTrans", "BoundedSetList"};
    int const points[total_test_cases] = {6, 6};
    int (*test_functions[total_test_cases]) (std::ostream &, const std::string) = {
        test_transform, test_bounded
    };

    return run_grading(out, test_case_number, total_test_cases,
                       test_names, points,
                       test_functions);
}

} // End of namepsace tdgrading
