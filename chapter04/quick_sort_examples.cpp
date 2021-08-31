#include "list"
#include "algorithm"
#include "utility"
#include "future"

using namespace std;

/**
 * Sequential implementation of the quick sort algorithm, that
 * sorts a given std::list of
 * @tparam T
 * @param input
 * @return
 */
template<typename T>
list<T> sequentialQuickSort(list<T> input) {
    if (input.empty()) {
        return input;
    }

    list<T> result;
    // Transfers the element pointed to by input.begin() from input into result.
    // The element is inserted before the element pointed to by result.begin().
    result.splice(result.begin(), input, input.begin());
    // this may lead to suboptimal sorting, but operations on list to
    // select random pivot are costly, so here we stick with this version
    const T &pivot = *result.begin();

    auto dividePoint = partition(input.begin(), input.end(),
                                 [&pivot](const T &t) { return t < pivot; });

    list<T> lowerPart;
    // Transfers the elements in the range [input.begin(), dividePoint) from input into *lowerPart.
    // The elements are inserted before the element pointed to by lowerPart.end().
    // The behavior is undefined if lowerPart.end() is an iterator in the range [input.begin(), dividePoint).
    lowerPart.splice(lowerPart.end(), input, input.begin(), dividePoint);

    auto newLower(sequentialQuickSort(move(lowerPart)));
    auto newHigher(sequentialQuickSort(move(input)));

    result.splice(result.end(), newHigher);
    result.splice(result.begin(), newLower);
    return result;
}


template<typename T>
list<T> parallelQuickSort(list<T> input) {
    if (input.empty()) {
        return input;
    }

    list<T> result;
    // Transfers the element pointed to by input.begin() from input into result.
    // The element is inserted before the element pointed to by result.begin().
    result.splice(result.begin(), input, input.begin());
    // this may lead to suboptimal sorting, but operations on list to
    // select random pivot are costly, so here we stick with this version
    const T &pivot = *result.begin();

    auto dividePoint = partition(input.begin(), input.end(),
                                 [&pivot](const T &t) { return t < pivot; });

    list<T> lowerPart;
    // Transfers the elements in the range [input.begin(), dividePoint) from input into *lowerPart.
    // The elements are inserted before the element pointed to by lowerPart.end().
    // The behavior is undefined if lowerPart.end() is an iterator in the range [input.begin(), dividePoint).
    lowerPart.splice(lowerPart.end(), input, input.begin(), dividePoint);

    // sort the lower part of the list on another thread if hardware concurrency allows it
    // otherwise it is sorted synchronously
    future<list<T>> newLower(async(&parallelQuickSort<T>, move(lowerPart)));

    auto newHigher(sequentialQuickSort(move(input)));

    result.splice(result.end(), newHigher);
    result.splice(result.begin(), newLower.get());
    return result;
}
































