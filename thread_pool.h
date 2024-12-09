#pragma once

#include <vector>
#include <queue>
#include <windows.h>
#include <future>
#include <memory>


using std::packaged_task, std::future, std::vector, std::queue, std::move, std::forward, std::bind, std::function;

//
// Interface
//

template<typename T>
class ThreadPool {
public:

    explicit ThreadPool(int maxThreadCount);
    ~ThreadPool();

    template<typename F, typename... Args>
    future<T> scheduleTask(F&& function, Args&&... args);

    void shutdown();

private:
    static DWORD _threadLoop(LPVOID param);
    bool _fetchTask(packaged_task<T()>& task);

    int _maxThreadCount;
    volatile bool _isActive = true;
    vector<HANDLE> _threads;
    queue<packaged_task<T()>> _tasks;
    CRITICAL_SECTION _criticalSection;
    CONDITION_VARIABLE _conditionVariable;
};


//
// Implementation
//

template<typename T>
ThreadPool<T>::ThreadPool(int maxThreadCount) : _maxThreadCount(maxThreadCount)
{
    InitializeCriticalSection(&_criticalSection);
    InitializeConditionVariable(&_conditionVariable);

    _threads.reserve(_maxThreadCount);

    for (int i = 0; i < _maxThreadCount; ++i) {
        HANDLE thread = CreateThread(nullptr, 0, _threadLoop, this, 0, nullptr);

        if (thread) _threads.push_back(thread);
    }
}

template<typename T>
ThreadPool<T>::~ThreadPool() {
    shutdown();

    for (HANDLE thread : _threads) {
        WaitForSingleObject(thread, INFINITE);
        CloseHandle(thread);
    }

    DeleteCriticalSection(&_criticalSection);
}

template<typename T>
template<typename F, typename... Args>
future<T> ThreadPool<T>::scheduleTask(F&& func, Args&&... args) {
    packaged_task<T()> task(bind(forward<F>(func), forward<Args>(args)...));

    future<T> future = task.get_future();

    EnterCriticalSection(&_criticalSection);

    _tasks.push(move(task));

    LeaveCriticalSection(&_criticalSection);
    WakeConditionVariable(&_conditionVariable);

    return future;
}

template<typename T>
void ThreadPool<T>::shutdown() {
    EnterCriticalSection(&_criticalSection);

    _isActive = false;

    LeaveCriticalSection(&_criticalSection);
    WakeAllConditionVariable(&_conditionVariable);
}

template<typename T>
DWORD ThreadPool<T>::_threadLoop(LPVOID param) {
    ThreadPool* pool = static_cast<ThreadPool*>(param);

    while (true) {
        packaged_task<T()> task;

        if (!pool->_fetchTask(task)) return 0;

        task();
    }
}

template<typename T>
bool ThreadPool<T>::_fetchTask(packaged_task<T()>& task) {
    EnterCriticalSection(&_criticalSection);

    while (_isActive && _tasks.empty()) {
        SleepConditionVariableCS(&_conditionVariable, &_criticalSection, INFINITE);
    }

    if (!_isActive) {
        LeaveCriticalSection(&_criticalSection);
        return false;
    }

    task = move(_tasks.front());
    _tasks.pop();

    LeaveCriticalSection(&_criticalSection);

    return true;
}
