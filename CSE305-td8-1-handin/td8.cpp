#include <atomic>
#include <climits>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

//--------- Slightly modified lazy version of SetList from the lecture --------

class Node {
public:
    std::mutex lock;
    std::string item;
    unsigned long key;
    std::shared_ptr<Node> next;
    bool marked;
    Node(const std::string& s, std::shared_ptr<Node> n = NULL) {
        item = s;
        key = std::hash<std::string>{}(s);
        next = n;
        marked = false;
    }
    Node(unsigned long k) {
        item = "";
        key = k;
        next = NULL; 
        marked = false;
    }
};

class SetList{
    static const unsigned long LOWEST_KEY;
    static const unsigned long LARGEST_KEY;
    std::shared_ptr<Node> head;

    static bool validate(const std::shared_ptr<const Node> pred, const std::shared_ptr<const Node> curr);
public:
    SetList() { // constructor
        head = std::make_shared<Node>(SetList::LOWEST_KEY);
        head->next = std::make_shared<Node>(SetList::LARGEST_KEY);
    }
    ~SetList() {}; // destructor
    bool add(const std::string& val);
    bool remove(const std::string& val);
    bool contains(const std::string& val) const;
};

const unsigned long SetList::LOWEST_KEY = 0;
const unsigned long SetList::LARGEST_KEY = ULONG_MAX;
 
bool SetList::validate(std::shared_ptr<const Node> pred, std::shared_ptr<const Node> curr) {
  return (!pred->marked) && (!curr->marked) && (pred->next == curr);
}

bool SetList::add(const std::string& val) {
    unsigned long key = std::hash<std::string>{}(val);
    while (true) {
        std::shared_ptr<Node> pred = this->head;
        std::shared_ptr<Node> curr = pred->next;
        while (curr->key < key) {
            pred = curr;
            curr = curr->next;
        }
        std::lock_guard<std::mutex> pred_lk(pred->lock);
        std::lock_guard<std::mutex> curr_lk(curr->lock);
        if (SetList::validate(pred, curr)) {
            if (key == curr->key) {
                return false;
            }
            pred->next = std::make_shared<Node>(val, curr);
            return true;
        }
    }
}
    
bool SetList::remove(const std::string& val) {
    unsigned long key = std::hash<std::string>{}(val);
    while (true) {
        std::shared_ptr<Node> pred = this->head;
        std::shared_ptr<Node> curr = pred->next;
        while (curr->key < key) {
            pred = curr;
            curr = curr->next;
        }
        std::lock_guard<std::mutex> pred_lk(pred->lock);
        std::lock_guard<std::mutex> curr_lk(curr->lock);
        if (SetList::validate(pred, curr)) {
            if (key == curr->key) {
                curr->marked = true;
            	pred->next = curr->next;
	            return true;
            }
	        return false;
        }
    }
}

bool SetList::contains(const std::string& val) const {
    unsigned long key = std::hash<std::string>{}(val);
    std::shared_ptr<Node> curr = this->head;
    while(curr->key < key)
        curr = curr->next;
    return (curr->key == key) && (!curr->marked);
}

//-----------------------------------------------------------------------------

class MultiNode {
public:
    std::mutex lock;
    std::string item;
    unsigned long key;
    std::shared_ptr<MultiNode> next;
    unsigned int multiplicity;

    MultiNode(const std::string& s, std::shared_ptr<MultiNode> n = NULL) {
        item = s;
        key = std::hash<std::string>{}(s);
        next = n;
        multiplicity = 1;
    }
    MultiNode(unsigned long k) {
        item = "";
        key = k;
        next = NULL; 
        multiplicity = 1;
    }
};

//a lock-free set based on a list with only add and contains methods.
class MultiSetList{
    static const unsigned long LOWEST_KEY;
    static const unsigned long LARGEST_KEY;
    std::shared_ptr<MultiNode> head;

    static bool validate(std::shared_ptr<const MultiNode> pred, std::shared_ptr<const MultiNode> curr);
public:
    MultiSetList() { // constructor
        head = std::make_shared<MultiNode>(MultiSetList::LOWEST_KEY);
        head->next = std::make_shared<MultiNode>(MultiSetList::LARGEST_KEY);
    }
    void add(const std::string& val);
    bool remove(const std::string& val);
    unsigned int contains(const std::string& val) const;
};

const unsigned long MultiSetList::LOWEST_KEY = 0;
const unsigned long MultiSetList::LARGEST_KEY = ULONG_MAX;
 
bool MultiSetList::validate(std::shared_ptr<const MultiNode> pred, std::shared_ptr<const MultiNode> curr) {
  return (pred->multiplicity > 0) && (curr->multiplicity > 0) && (pred->next == curr);
}


//checks if the element already exists by comparing the keys
// If key matches, the multiplicity is increased by one
// Otherwise, a new MultiNode is inserted into the list with a multiplicity of one
void MultiSetList::add(const std::string& val) {
    unsigned long key = std::hash<std::string>{}(val);
    while (true) {
        std::shared_ptr<MultiNode> pred = this->head;
        std::shared_ptr<MultiNode> curr = pred->next;
        while (curr->key < key) {
            pred = curr;
            curr = curr->next;
        }
        std::lock_guard<std::mutex> pred_lk(pred->lock);
        std::lock_guard<std::mutex> curr_lk(curr->lock);
        if (MultiSetList::validate(pred, curr)) {
            if (key == curr->key) {
                ++curr->multiplicity; // Increase multiplicity if element already exists
                return;
            }
            pred->next = std::make_shared<MultiNode>(val, curr);
            return;
        }
    }
}
    

//checks if the key matches the given value
// If it does and the multiplicity is greater than one, the multiplicity is decreased by one
// Otherwise, the node is removed from the list
bool MultiSetList::remove(const std::string& val) {
    unsigned long key = std::hash<std::string>{}(val);
    while (true) {
        std::shared_ptr<MultiNode> pred = this->head;
        std::shared_ptr<MultiNode> curr = pred->next;
        while (curr->key < key) {
            pred = curr;
            curr = curr->next;
        }
        std::lock_guard<std::mutex> pred_lk(pred->lock);
        std::lock_guard<std::mutex> curr_lk(curr->lock);
        if (MultiSetList::validate(pred, curr)) {
            if (key == curr->key) {
                if (curr->multiplicity > 1) {
                    --curr->multiplicity; // Decrease multiplicity if positive
                    return true;
                } else {
                    pred->next = curr->next;
                    return true;
                }
            }
            return false;
        }
    }
}

//checks if the key matches the given value and returns the corresponding multiplicity if found, 
// or zero otherwise
unsigned int MultiSetList::contains(const std::string& val) const {
    unsigned long key = std::hash<std::string>{}(val);
    std::shared_ptr<MultiNode> curr = this->head;
    while (curr->key < key)
        curr = curr->next;
    if (curr->key == key) {
        return curr->multiplicity;
    }
    return 0;
}

//-----------------------------------------------------------------------------

class AtomicNode {
public:
    std::string item;
    unsigned long key;
    std::atomic<AtomicNode*> next;
    
    AtomicNode(const std::string& s, AtomicNode* n = NULL) {
        item = s;
        key = std::hash<std::string>{}(s);
        std::atomic_init<AtomicNode*>(&next, n);
    }
    AtomicNode(unsigned long k) {
        item = "";
        key = k;
        next = NULL; 
    }
};


class MonotonicLockFreeSet {
    AtomicNode* head;

    static const unsigned long LOWEST_KEY;
    static const unsigned long LARGEST_KEY;
public:
    MonotonicLockFreeSet() {
        this->head = new AtomicNode(MonotonicLockFreeSet::LOWEST_KEY);
        this->head->next = new AtomicNode(MonotonicLockFreeSet::LARGEST_KEY);
    }
    ~MonotonicLockFreeSet();
    bool add(const std::string& val);
    bool contains(const std::string& val) const;
};

const unsigned long MonotonicLockFreeSet::LOWEST_KEY = 0;
const unsigned long MonotonicLockFreeSet::LARGEST_KEY = ULONG_MAX;

bool MonotonicLockFreeSet::add(const std::string& val) {
    unsigned long key = std::hash<std::string>{}(val);
    AtomicNode* newNode = new AtomicNode(val);

    while (true) {
        AtomicNode* pred = this->head;
        AtomicNode* curr = pred->next.load(std::memory_order_acquire);

        while (curr->key < key) {
            pred = curr;
            curr = pred->next.load(std::memory_order_acquire);
        }

        if (curr->key == key) {
            delete newNode;
            return false; // Element already exists
        }

        newNode->next = curr;

        if (pred->next.compare_exchange_weak(curr, newNode, std::memory_order_release, std::memory_order_relaxed)) {
            return true; // Successfully added the element
        }
    }
}

bool MonotonicLockFreeSet::contains(const std::string& val) const {
    unsigned long key = std::hash<std::string>{}(val);
    AtomicNode* curr = this->head->next.load(std::memory_order_acquire);
    while (curr->key < key) {
        curr = curr->next.load(std::memory_order_acquire);
    }
    return curr->key == key;
}

MonotonicLockFreeSet::~MonotonicLockFreeSet() {
    AtomicNode* pred = this->head;
    AtomicNode* curr = pred->next;
    while (curr != NULL) {
        delete pred;
        pred = curr;
        curr = curr->next;
    }
    delete pred;
}

//-----------------------------------------------------------------------------

template <typename T>
struct RefAndMark {
    T* ref;
    bool mark;
    RefAndMark(T* _ref, bool _mark) {
        this->ref = _ref;
        this->mark = _mark;
    }
};

template <typename T>
class AtomicMarkable {
    std::atomic<RefAndMark<T> > data;
public:
    AtomicMarkable(T* _ref = nullptr, bool mark = false): data(RefAndMark<T>(_ref, mark)) {}

    bool compare_and_set(T*& expected_ref, bool& expected_mark, T* desired_ref, bool desired_mark) {
        return false;
    }

    bool attempt_mark(T* expected_ref, bool new_mark) {
        return true;
    }

    T* get(bool& mark) const {
    }
};

template <typename T>
class AtomicMarkableReference {
    struct RefAndMark {
        T* ref;
        bool mark;
        RefAndMark(T* _ref = nullptr, bool _mark = false) : ref(_ref), mark(_mark) {}
    };

    std::atomic<RefAndMark> data;

public:
    AtomicMarkableReference(T* _ref = nullptr, bool mark = false) : data(RefAndMark(_ref, mark)) {}

    bool compare_and_set(T*& expected_ref, bool& expected_mark, T* desired_ref, bool desired_mark) {
        RefAndMark expected(expected_ref, expected_mark);
        RefAndMark desired(desired_ref, desired_mark);

        return data.compare_exchange_weak(expected, desired);
    }

    bool attempt_mark(T* expected_ref, bool new_mark) {
        RefAndMark expected(expected_ref, false);
        RefAndMark desired(expected_ref, new_mark);

        return data.compare_exchange_weak(expected, desired);
    }

    T* get(bool& mark) const {
        RefAndMark current = data.load();
        mark = current.mark;
        return current.ref;
    }
};

