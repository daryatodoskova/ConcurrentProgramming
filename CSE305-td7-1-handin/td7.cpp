#include <algorithm>
#include <atomic>
#include <climits>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "CoarseSetList.cpp"
#include "SetList.cpp"

//-----------------------------------------------------------------------------

class SetListTrans: public SetList {
public:
    template <typename F>
    void transform(F f); 
};

//replaces each value s by f(s)
template <typename F>
void SetListTrans::transform(F f) {
    Node *pred, *curr;
    pred = this->head;
    curr = pred->next;
    // lock all the mutexes in the list
    pred->lock.lock();
    curr->lock.lock();
    std::vector<std::string> values;
    int i = 0;
    while (curr->key != this->LARGEST_KEY) {
        values.push_back(curr->item);
        i++;
        curr->lock.unlock();
        pred->next = curr->next;
        curr = pred->next;
        curr->lock.lock();
    }
    pred->lock.unlock();
    curr->lock.unlock();
    for (auto &v : values) {
        this->add(f(v));
    }
}

//-----------------------------------------------------------------------------

//thread inserting an element into a set that has reached the capacity
//and does not contain the element gets blocked.

class BoundedSetList: public SetList {
    std::condition_variable not_full;
    size_t capacity;  //fixed capacity
    size_t count;
    std::mutex count_lock;  //only taken at the moment of actual insertion.
public:
    BoundedSetList(size_t capacity) {
        this->capacity = capacity;
        this->count = 0;
    }

    size_t get_capacity() const {return this->capacity;}
    size_t get_count() const {return this->count;}

    bool add(const std::string& val);
    bool remove(const std::string& val);
};

bool BoundedSetList::add(const std::string& val) {
    Node* pred = this->search(val);
    Node* curr = pred->next;
    bool exists = (curr->key == std::hash<std::string>{}(val));
    if (!exists) {
        if (this->count < this->capacity) {
            Node *new_node = new Node(val);
            pred->next = new_node;
            new_node->next = curr;
            this->count++;
        } 
        else {
            pred->lock.unlock();
            curr->lock.unlock();
            std::unique_lock<std::mutex> lk(this->count_lock);
            while (this->capacity <= this->count) {
                this->not_full.wait(lk);
            }
            exists = this->add(val);
        }
    }
    pred->lock.unlock();
    curr->lock.unlock();
    return !exists;
}

bool BoundedSetList::remove(const std::string& val) {
    bool exists = SetList::remove(val);
    if (exists) {
        std::lock_guard<std::mutex> lock(this->count_lock);
        --this->count;
        if (this->count < this->capacity) {
            this->not_full.notify_one();
        }
    }
    return exists;
}

//-----------------------------------------------------------------------------
