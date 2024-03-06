#pragma once

#include <queue>
#include <condition_variable>

struct QData {
    unsigned char* ucp;
    unsigned short us;
};

class SafeQueue {
public:
    SafeQueue();
    ~SafeQueue();

    void push(unsigned char* value, unsigned short size);
    unsigned char* pop(int& len);
    bool empty() const;
    size_t size() const;
    void clear();

private:
    std::queue<QData> q;
    mutable std::mutex mtx;
};

SafeQueue::SafeQueue() {}

SafeQueue::~SafeQueue() {
    clear();
}

void SafeQueue::push(unsigned char* value, unsigned short size) {
    std::lock_guard<std::mutex> lock(mtx);
    q.push({ value, size });
}

unsigned char* SafeQueue::pop(int& len) {
    std::unique_lock<std::mutex> lock(mtx);
    if (q.empty()) {
        len = NULL;
        return nullptr;
    }

    QData value = q.front();
    q.pop();
    len = (int)value.us;
    return value.ucp;
}

bool SafeQueue::empty() const {
    std::lock_guard<std::mutex> lock(mtx);
    return q.empty();
}

size_t SafeQueue::size() const {
    std::lock_guard<std::mutex> lock(mtx);
    return q.size();
}

void SafeQueue::clear() {
    std::lock_guard<std::mutex> lock(mtx);
    while (!q.empty()) {
        delete[] q.front().ucp;
        q.pop();
    }
}
