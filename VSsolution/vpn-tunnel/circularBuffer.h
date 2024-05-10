#pragma once

#include <queue>
#include <condition_variable>

struct QData {
	unsigned char* ucp;
	unsigned int us;

    ~QData() {
        delete[] ucp;
    }
};

class CicrularBuffer {
public:
    CicrularBuffer(size_t capacity, unsigned int bufferLen);

    void wait();
    void stopWait();
    void push(unsigned char* data, unsigned int dataSize);
    bool pop(unsigned char* data, unsigned int& dataLen);
    bool empty() const;

    ~CicrularBuffer();

    size_t size;
private:
    void resize();

private:
    std::unique_ptr<QData[]> buffer;
    size_t capacity;
    size_t head;
    size_t tail;
    unsigned int bufferLen;

    mutable std::mutex mtx;
    std::condition_variable cv;

    bool noWait;
};

CicrularBuffer::CicrularBuffer(size_t capacity, unsigned int bufferLen)
    : buffer(new QData[capacity]), capacity(capacity), head(0), tail(0), size(0), noWait(false), bufferLen(bufferLen)
{
    for (size_t i = 0; i < capacity; ++i) {
        buffer[i].ucp = new unsigned char[bufferLen];
        buffer[i].us = bufferLen;
    }
}

void CicrularBuffer::wait() {
    std::unique_lock<std::mutex> lock(mtx);
    cv.wait(lock, [this] { return size > 0 || noWait; });
}

void CicrularBuffer::stopWait() {
    {
        std::lock_guard<std::mutex> lock(mtx);
        noWait = true;
    }
    cv.notify_all();
}

void CicrularBuffer::push(unsigned char* data, unsigned int dataSize) {
    std::lock_guard<std::mutex> lock(mtx);
    if (size == capacity) {
        resize();
    }

    std::memcpy(buffer[tail].ucp, data, dataSize);
    buffer[tail].us = dataSize;

    tail = (tail + 1) % capacity;
    ++size;
    cv.notify_all();
}

void CicrularBuffer::resize()
{
    size_t newCapacity = capacity * 2;
    std::unique_ptr<QData[]> newBuffer(new QData[newCapacity]);

    for (size_t i = 0; i < size; ++i) {
        newBuffer[i] = std::move(buffer[(head + i) % capacity]);
    }

    buffer = std::move(newBuffer);
    capacity = newCapacity;
    head = 0;
    tail = size;
}

bool CicrularBuffer::pop(unsigned char* data, unsigned int& dataLen) {
    std::unique_lock<std::mutex> lock(mtx);
    if (size == 0) {
        return false;
    }

    std::memcpy(data, buffer[head].ucp, buffer[head].us);
    dataLen = buffer[head].us;
    buffer[head].us = bufferLen;

    head = (head + 1) % capacity;
    --size;
    return true;
}

bool CicrularBuffer::empty() const {
    std::lock_guard<std::mutex> lock(mtx);
    return size == 0;
}

CicrularBuffer::~CicrularBuffer()
{
    std::lock_guard<std::mutex> lock(mtx);
    head = tail = size = 0;
    buffer.reset();
}