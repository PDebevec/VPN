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

private:
    void resize();

private:
    std::unique_ptr<QData[]> buffer;
    size_t capacity;
    size_t mask;
    size_t head;
    size_t tail;
    size_t size;
    unsigned int bufferLen;

    mutable std::mutex mtx;
    mutable std::mutex resizeMtx;
    std::condition_variable cv;

    bool noWait;
};

CicrularBuffer::CicrularBuffer(size_t capacity, unsigned int bufferLen)
    : buffer(new QData[capacity]), head(0), tail(0), size(0), noWait(false), bufferLen(bufferLen)
{
    size_t powerOfTwoCapacity = 1;
    while (powerOfTwoCapacity < capacity) {
        powerOfTwoCapacity <<= 1;
    }
    capacity = powerOfTwoCapacity;
    mask = capacity - 1;

    this->capacity = capacity;

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
    {
        std::unique_lock<std::mutex> resizeLock(resizeMtx, std::defer_lock);
        std::lock_guard<std::mutex> lock(mtx);
        if (size == capacity) {
            resizeLock.lock();
            resize();
        }

        std::memcpy(buffer[tail].ucp, data, dataSize);
        buffer[tail].us = dataSize;

        tail = (tail + 1) & mask;
        ++size;
    }
    cv.notify_all();
}

bool CicrularBuffer::pop(unsigned char* data, unsigned int& dataLen) {
    std::unique_lock<std::mutex> lock(mtx);
    if (size == 0) {
        return false;
    }

    std::memcpy(data, buffer[head].ucp, buffer[head].us);
    dataLen = buffer[head].us;
    buffer[head].us = bufferLen;

    head = (head + 1) & mask;
    --size;
    return true;
}

void CicrularBuffer::resize()
{
    size_t newCapacity = capacity * 2;
    std::unique_ptr<QData[]> newBuffer(new QData[newCapacity]);

    for (size_t i = 0; i < size; ++i) {
        newBuffer[i] = std::move(buffer[(head + i) & mask]);
    }

    buffer = std::move(newBuffer);
    capacity = newCapacity;
    mask = capacity - 1;
    head = 0;
    tail = size;
}

bool CicrularBuffer::empty() const {
    std::lock_guard<std::mutex> lock(mtx);
    return size == 0;
}

CicrularBuffer::~CicrularBuffer()
{
    std::lock_guard<std::mutex> lock(mtx);
    buffer.reset();
}