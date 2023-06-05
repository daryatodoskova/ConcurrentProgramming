#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <cmath>
#include <mutex>
#include <numeric>
#include <thread>
#include <queue>

//----------------------------------------------------------------------------

class OrderedVec {
        std::vector<int> data;
        std::atomic<int> num_searchers; // atomic counter for the number of ongoing searches
        std::mutex insert_mutex; // mutex that protects data during insertion
    public:
        OrderedVec() : num_searchers(0) { //an atomic integer that keeps track of the number of ongoing searches.
        }
        //inserts val into the vector in a way that keeps the vector sorted in ascending order
        void insert(int val) {
            std::lock_guard<std::mutex> lock(insert_mutex);
            data.insert(std::upper_bound(data.begin(), data.end(), val), val);
        }
        //returns a boolean indicating whether the value is present in the vector or not
        bool search(int val) {
            std::lock_guard<std::mutex> lock(insert_mutex);
            num_searchers++; // increment the number of ongoing searches while holding the lock
            auto iter = std::lower_bound(data.begin(), data.end(), val);
            bool result = (iter != data.end() && *iter == val);
            num_searchers--; // decrement the number of ongoing searches after the search is complete
            return result;
        }

        // used for testing
        std::vector<int> get_data() {
            return data;
        }
};

//----------------------------------------------------------------------------
void DivideOnceEven(std::condition_variable& iseven, std::mutex& m, int& n) {
    std::unique_lock<std::mutex> lockk(m);
    // locks the mutex and uses lock to wait on the condition variable until n becomes even
    while (!(n % 2 == 0)) {
        iseven.wait(lockk);
    }
    //once n becomes even, divides it by 2 and terminates
    n /= 2;
}

//-----------------------------------------------------------------------------

template <class E> 
class SafeUnboundedQueue {
        std::queue<E> elements; //thread-safe unbounded queue by using std::queue
        std::mutex lock;
        std::condition_variable not_empty; // Condition variable to signal when the queue is not empty
    public: 
        SafeUnboundedQueue<E>(){}
        void push(const E& element);
        // Pops an element from the queue and returns it
        E pop ();
        // Returns true if the queue is empty, false otherwise
        bool is_empty() const {return this->elements.empty();}
};

// Pushes an element to the queue
template <class E>
void SafeUnboundedQueue<E>::push(const E& element) {
    std::unique_lock<std::mutex> lockk(this->lock);
    bool was_empty = this->elements.empty();
    this->elements.push(element);
    if (was_empty) {
        this->not_empty.notify_all(); // Signal to all waiting threads that the queue is not empty
    }
}

// Pops an element from the queue and returns it
template <class E> 
E SafeUnboundedQueue<E>::pop() {
    std::unique_lock<std::mutex> lockk(this->lock);
    // Wait until the queue is not empty
    while (this->elements.empty()) {
        this->not_empty.wait(lockk);
    }
    // Get the front element and remove it from the queue
    E element = this->elements.front();
    this->elements.pop();
    return element;
}

//-----------------------------------------------------------------------------

