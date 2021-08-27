#include "mutex"
#include "memory"
#include "map"
#include "shared_mutex"

class SomeBigObject {
};

void swap(SomeBigObject &lhs, SomeBigObject &rhs);

class X {
private:
    SomeBigObject detail;
    std::mutex m;

public:
    X(const SomeBigObject &sd) : detail(sd) {}

    friend void Swap(X &lhs, X &rhs) {
        if (&lhs == &rhs) {
            return;
        }
        std::lock(lhs.m, rhs.m); // lock both mutexes simultaneously
        std::lock_guard lockA(lhs.m, std::adopt_lock); // adopt locked mutex to prevent locking in constructor
        std::lock_guard lockB(rhs.m, std::adopt_lock);
        swap(lhs.detail, rhs.detail);
    }

    friend void SwapScoped(X &lhs, X &rhs) {
        if (&lhs == &rhs) {
            return;
        }
        std::scoped_lock guard(lhs.m, rhs.m); // since C++ 17
        swap(lhs.detail, rhs.detail);
    }

    friend void SwapWithUniqueLock(X &lhs, X &rhs) {
        if (&lhs == &rhs) {
            return;
        }
        std::unique_lock lockA(lhs.m, std::defer_lock); // mutexes are not locked in constructor
        std::unique_lock lockB(rhs.m, std::defer_lock);
        std::lock(lhs.m, rhs.m); // lock both mutexes simultaneously

        swap(lhs.detail, rhs.detail);
    }
};


//<editor-fold desc="Protecting while initializing an object">
struct SomeResource {
    void doSomething() {}
};

std::shared_ptr<SomeResource> resourcePtr;
std::once_flag resourceFlag;
std::mutex resourceMutex;

void initResource() {
    resourcePtr.reset(new SomeResource);
}

void foo() {
    // this call ensures that resource is initialized only once
    // this is a lazy initialization pattern.
    std::call_once(resourceFlag, initResource);
    resourcePtr->doSomething();
}

/**
 * This double check idiom fails to prevent UB because e.g.
 *  thread A may reset the pointer but the object has not yet been created
 *  when thread B checks first check. So thread B proceeds to call
 *  resourcePtr->doSomething() but the object of SomeResource has not yet been created
 *  and this is UB.
 */
void undefinedBehaviourWithDoubleCheckLocking() {
    if (!resourcePtr) { // 1
        std::lock_guard lk(resourceMutex);
        if (!resourcePtr) {
            resourcePtr.reset(new SomeResource); // 2
        }
    }
    resourcePtr->doSomething();
}
//</editor-fold>

// Protecting a data structure with std::shared_mutex
class DnsEntry {
};

class DnsCache {
    std::map<std::string, DnsEntry> entries;
    mutable std::shared_mutex entryMutex;

public:
    DnsEntry findEntry(const std::string &domain) const {
        // reader lock. allows multiple threads. But if a thread holds exclusive lock
        // (lock_guard over shared mutex) then thread trying to acquire the shared lock
        // will have to wait
        std::shared_lock<std::shared_mutex> lk(entryMutex);
        const auto it = entries.find(domain);
        return (it == entries.end()) ? DnsEntry{} : it->second;
    }

    void updateOrAddEntry(const std::string &domain, const DnsEntry &dnsDetails) {
        // writer lock. Only one thread is operating on data. If any other thread holds shared lock
        // this thread waits for all of them to relinquish their locks.
        std::lock_guard<std::shared_mutex> lk(entryMutex);
        entries[domain] = dnsDetails;
    }
};






















