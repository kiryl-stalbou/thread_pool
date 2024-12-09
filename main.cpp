#include <iostream>
#include <chrono>

#include "thread_pool.h"


using std::future, std::chrono::high_resolution_clock, std::chrono::milliseconds, std::endl, std::cout;

unsigned long long fib(int n) {
    if (n <= 1) return n;

    return fib(n - 1) + fib(n - 2);
}

int main() {
    const int fibStartNum = 40;
    const int fibEndNum = 43;



    //
    // Single thread execution
    //

    auto singleThreadStartTime = high_resolution_clock::now();


    for (int i = fibStartNum; i <= fibEndNum; ++i) 
    {
        cout << "Fib(" << i << ") = " << fib(i) << endl;
    }


    auto singleThreadEndTime = high_resolution_clock::now();
    auto singleThreadDuration = duration_cast<milliseconds>(singleThreadEndTime - singleThreadStartTime);
    cout << "\nSingle thread execution time: " << singleThreadDuration.count() << "ms\n\n" << endl;



    //
    // Thread pool execution
    //

    const int maxThreadCount = 4;
    ThreadPool<unsigned long long> threadPool(maxThreadCount);
    vector<future<unsigned long long>> fibFutures;
    auto threadPoolStartTime = high_resolution_clock::now();


    for (int i = fibStartNum; i <= fibEndNum; ++i) 
    {
        fibFutures.push_back(threadPool.scheduleTask(fib, i));
    }

    for (int i = 0; i < fibFutures.size(); ++i) 
    {
        cout << "Fib(" << fibStartNum + i << ") = " << fibFutures[i].get() << endl;
    }


    auto threadPoolEndTime = high_resolution_clock::now();
    auto threadPoolDuration = duration_cast<milliseconds>(threadPoolEndTime - threadPoolStartTime);
    cout << "\nThread pool execution time: " << threadPoolDuration.count() << "ms\n\n" << endl;
    threadPool.shutdown();



    return 0;
}
