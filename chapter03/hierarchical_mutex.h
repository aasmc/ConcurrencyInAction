#pragma once

#include "mutex"
#include "thread"
#include "memory"

/**
 * Hierarchical mutex that allows ordering of locks according
 * to some their position in the hierarchy. If a thead holds a lock
 * on a HierarchicalMutex then it can only acquire a lock on another
 * HierarchicalMutex with a lower hierarchy number.
 */
class HierarchicalMutex {
    std::mutex internalMutex;
    unsigned long const hierarchyValue;
    unsigned long previousHierarchyValue;
    /**
     * State of the variable is local to every thread and is completely independent
     * of all other threads of execution.
     */
    static thread_local unsigned long thisThreadHierarchyValue;

    void CheckForHierarchyViolation() {
        if (thisThreadHierarchyValue <= hierarchyValue) {
            throw std::logic_error("Mutex hierarchy violated");
        }
    }

    void UpdateHierarchyValue() {
        previousHierarchyValue = thisThreadHierarchyValue;
        thisThreadHierarchyValue = hierarchyValue;
    }

public:

    explicit HierarchicalMutex(unsigned long value) :
            hierarchyValue(value),
            previousHierarchyValue(0) {};

    /**
     * Locks internal mutex only if this thread's hierarchy value is greater
     * than the hierarchy of this mutex.
     */
    void lock() {
        CheckForHierarchyViolation();
        internalMutex.lock();
        UpdateHierarchyValue();
    }

    void unlock() {
        if (thisThreadHierarchyValue != hierarchyValue) {
            throw std::logic_error("Mutex hierarchy violated");
        }
        thisThreadHierarchyValue = previousHierarchyValue;
        internalMutex.unlock();
    }

    bool try_lock() {
        CheckForHierarchyViolation();
        if (!internalMutex.try_lock()) {
            return false;
        }
        UpdateHierarchyValue();
        return true;
    }
};

// initialize the thread local hierarchical value to max possible value to allow
// for any mutex to be locked initially
thread_local unsigned long HierarchicalMutex::thisThreadHierarchyValue(ULONG_MAX);

HierarchicalMutex highLevelMutex(10000);
HierarchicalMutex lowLevelMutex(5000);
HierarchicalMutex otherMutex(6000);

int doLowLevelStuff();

int lowLevelFunc() {
    std::lock_guard<HierarchicalMutex> lk(lowLevelMutex);
    return doLowLevelStuff();
}

void highLevelStuff(int someParam);

void highLevelFunc() {
    std::lock_guard<HierarchicalMutex> lk(highLevelMutex);
    highLevelFunc(lowLevelFunc());
}

void threadA() {
    highLevelFunc();
}

void doOtherStuff();

void otherStuff() {
    highLevelFunc();
    doOtherStuff();
}

void threadB() {
    // fails to lock because otherMutex has 6000 value
    // while highLevelFunc() locks mutex of 10000 value and is called from doOtherStuff();
    std::lock_guard<HierarchicalMutex> lk(otherMutex);
    otherStuff();
}

void ExampleUsage() {


}


























