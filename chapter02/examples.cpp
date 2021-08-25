#include <numeric>
#include "thread"
#include "utility"
#include "iostream"
#include "vector"
#include "algorithm"

void doSomething(unsigned i) {
    std::cout << "Doing something in thread with ID = " << std::this_thread::get_id()
              << " with i = " << i << '\n';
}

struct Func {
    int &i;

    Func(int &i_) : i(i_) {}

    void operator()() const {
        for (unsigned j = 0; j < 1000; ++j) {
            doSomething(i);
        }
    }
};

/**
 * RAII wrapper over std::thread object that allows to capture
 * local state by reference because it joins the thread it owns in
 * its destructor.
 */
class ScopedThread {
    std::thread t;

public:
    explicit ScopedThread(std::thread t_) : t(std::move(t_)) {
        if (!t.joinable()) {
            throw std::logic_error("No thread");
        }
    }

    ~ScopedThread() {
        t.join();
    }

    ScopedThread(const ScopedThread &) = delete;

    ScopedThread &operator=(const ScopedThread &) = delete;
};

void f() {
    int someLocalState = 10;
    ScopedThread t{std::thread(Func(someLocalState))};
}

/**
 * Wrapper class over std::thread that supports standard thread operations
 * but joins the thread it owns in its destructor. It also supports move semantics.
 */
class JoiningThread {
    std::thread t;
public:
    JoiningThread() noexcept = default;

    template<class Callable, typename ... Args>
    explicit JoiningThread(Callable &&func, Args &&... args) :
            t(std::forward<Callable>(func), std::forward<Args>(args)...) {}

    explicit JoiningThread(std::thread t_) noexcept: t(std::move(t_)) {}

    JoiningThread(JoiningThread &&other) noexcept: t(std::move(other.t)) {}

    JoiningThread &operator=(JoiningThread &&other) noexcept {
        if (joinable()) {
            join();
        }
        t = std::move(other.t);
        return *this;
    }

    JoiningThread &operator=(std::thread other) noexcept {
        if (joinable()) {
            join();
        }
        t = std::move(other);
        return *this;
    }

    ~JoiningThread() noexcept {
        if (joinable()) {
            join();
        }
    }

    void swap(JoiningThread &other) noexcept {
        t.swap(other.t);
    }

    std::thread::id get_id() const noexcept {
        return t.get_id();
    }

    bool joinable() {
        return t.joinable();
    }

    void join() {
        t.join();
    }

    void detach() {
        t.detach();
    }

    std::thread &asThread() noexcept {
        return t;
    }

    const std::thread &asThread() const noexcept {
        return t;
    }
};

/**
 * Example function that shows that threads can be placed in a container that
 * supports move semantics. Here it is a std::vector.
 */
void g() {
    std::vector<std::thread> threads;
    for (unsigned i = 0; i < 20; ++i) {
        threads.emplace_back(doSomething, i); // spawn threads
    }
    for (auto &t : threads) {
        t.join();
    }
}

template<typename Iterator, typename T>
struct AccumulateBlock {
    void operator()(Iterator first, Iterator last, T &result) {
        result = std::accumulate(first, last, result);
    }
};

/**
 * Naive implementation of a parallel accumulate algorithm.
 * @tparam Iterator
 * @tparam T
 * @param first
 * @param last
 * @param init
 * @return
 */
template<typename Iterator, typename T>
T parallelAccumulate(Iterator first, Iterator last, T init) {
    unsigned long const length = std::distance(first, last);

    if (!length) {
        return init;
    }
    // select necessary number of threads to handle the parallel execution
    unsigned long const minPerThread = 25;
    unsigned long const maxThreads = (length + minPerThread - 1) / minPerThread;

    unsigned long const hardwareThreads = std::thread::hardware_concurrency();
    unsigned long const numThreads = std::min(
            hardwareThreads != 0 ? hardwareThreads : 2, maxThreads
    );

    unsigned long const blockSize = length / numThreads;

    std::vector<T> results(numThreads);
    std::vector<std::thread> threads(numThreads - 1); // need one less because of the main thread that will
    // handle the remaining elements

    Iterator blockStart = first;
    for (unsigned long i = 0; i < (numThreads - 1); ++i) {
        Iterator blockEnd = blockStart;
        std::advance(blockEnd, blockSize);
        threads[i] = std::thread(
                AccumulateBlock<Iterator, T>(),
                blockStart, blockEnd, std::ref(results[i]) // !!pass the last parameter to the functor as reference to avoid copying!
        );
        blockStart = blockEnd;
    }

    AccumulateBlock<Iterator, T>()(blockStart, last, results[numThreads - 1]);

    for (auto &t : threads) {
        t.join();
    }

    return std::accumulate(results.begin(), results.end(), init);
}


int main() {
    f();
    g();
}


























