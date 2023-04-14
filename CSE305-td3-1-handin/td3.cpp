#pragma once
#include <cfloat>
#include <climits>
#include <thread>
#include <numeric>
#include <iterator>
#include <vector>

//-----------------------------------------------------------------------------

template <typename T>
// Searches for occurrences of a target in a block of an array
void FindThread(T* arr, size_t block_size, T target, unsigned int count, std::atomic<unsigned int>& occurences) {
    // Pointer to the end of the block
    T* end = arr + block_size;
    while (arr != end) {
        if (occurences >= count){
            return;
        }
        if (*arr == target){
            occurences += 1;
        }
        ++arr;
    }
}

/**
 * @brief Checks if there are at least `count` occurences of targert in the array
 * @param arr - pointer to the first element of the array
 * @param N - the length of the array
 * @param target - the target to search for
 * @param count - the number of occurences to stop after
 * @param num_threads - the number of threads to use
 * @return if there are at least `count` occurences
*/
template <typename T>
bool FindParallel(T* arr, size_t N, T target, size_t count, size_t num_threads) {
    // Atomic variable to store the number of occurrences found so far
    std::atomic<unsigned int> occurences = 0;
    if (N == 0) {
        return (count == 0);
    }
    
    size_t block_size = N / num_threads;
    std::vector<std::thread> workers(num_threads - 1);
    // Pointer to the start of the block to be searched by the first thread
    T* start_block = arr;
    for (size_t i = 0; i < num_threads - 1; ++i) {
        workers[i] = std::thread(&FindThread<T>, start_block, block_size, target, count, std::ref(occurences));
        start_block += block_size;
    }
    FindThread(start_block, arr + N - start_block, target, count, std::ref(occurences));

    for (size_t i = 0; i < num_threads - 1; ++i) {
        workers[i].join();
    }

    return (occurences >= count);
}

//-----------------------------------------------------------------------------
//methods should transfer money from one account to another in a thread-safe manner
//not allowed to use std::lock
//-----------------------------------------------------------------------------
class Account {
        unsigned int money;
        unsigned int account_id;
        std::mutex lock;

        static std::atomic<unsigned int> max_account_id;

    public:
        
        Account() {
            money = 0;
            account_id = max_account_id.fetch_add(1);
        }

        Account(unsigned int init_money) {
            money = init_money;
            account_id = max_account_id.fetch_add(1);
        }

        Account(const Account& other) = delete;

        Account& operator = (const Account& other) = delete;
        
        unsigned int get_amount() const {
            return this->money;
        }

        unsigned int get_id() const {
            return this->account_id;
        }

        // withdrwas deduction if the current amount is at least deduction
        // returns whether the withdrawal took place
        bool withdraw(unsigned int deduction) {
            std::lock_guard<std::mutex> guard(lock);
            if (money >= deduction) {
                money -= deduction;
                return true;
            }
            return false;
        }

        // adds the prescribed amount of money to the account
        void add(unsigned int to_add) {
            std::lock_guard<std::mutex> guard(lock);
            money += to_add;
        }

        // transfers amount from from to to if there are enough money on from
        // returns whether the transfer happened
        static bool transfer(unsigned int amount, Account& from, Account& to) {
            std::unique_lock<std::mutex>  guard_to(to.lock, std::defer_lock);
            std::unique_lock<std::mutex>  guard_from(from.lock, std::defer_lock);
            if (from.get_id() > to.get_id()) {
                guard_to.lock();
                guard_from.lock();
            } else {
                guard_from.lock();
                guard_to.lock();
            }
            if (amount <= from.money) {
                from.money -= amount;
                to.money += amount;
                return true;
            }
            return false;
        }
};

std::atomic<unsigned int> Account::max_account_id(0);

//-----------------------------------------------------------------------------
