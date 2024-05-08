#pragma once

#include <queue>
#include <condition_variable>

struct QData {
	unsigned char* ucp;
	unsigned int us;
};

class SafeTwoQueue
{
public:
	SafeTwoQueue(size_t, unsigned int);

	void wait();
	void stopWait();

	void restart();

	void push(unsigned char*, unsigned int);
	void pop(unsigned char*, unsigned int&);

	bool empty() const;
	size_t size() const;

	void clear();

	~SafeTwoQueue();

private:
	std::queue<QData*> q1;
	std::queue<QData*> q2;

	mutable std::mutex mtx;
	std::condition_variable cv;

	QData* tempPush;
	QData* tempPop;

	unsigned int bufferLen;
	bool noWait;
};

SafeTwoQueue::SafeTwoQueue(size_t q1Size, unsigned int bufferSize)
	:bufferLen(bufferSize)
{
	tempPush = nullptr;
	tempPop = nullptr;

	noWait = false;

	for (size_t i = 0; i < q1Size; i++)
	{
		q1.push(new QData{new unsigned char[bufferSize], bufferSize});
	}
}

inline void SafeTwoQueue::wait()
{
	std::unique_lock<std::mutex> lock(mtx);
	cv.wait(lock, [this] { return !q2.empty() || noWait; });
}

inline void SafeTwoQueue::stopWait()
{
	{
		std::lock_guard<std::mutex> lock(mtx);
		noWait = true;
	}
	cv.notify_all();
}

inline void SafeTwoQueue::restart()
{
	noWait = false;
	while (!q2.empty())
	{
		delete[] q2.front();
		q2.pop();
	}
}

void SafeTwoQueue::push(unsigned char* data, unsigned int dataSize)
{
	std::lock_guard<std::mutex> lock(mtx);
	if (q1.empty())
	{
		q1.push(new QData{ new unsigned char[bufferLen], bufferLen});
	}

	tempPush = q1.front();
	q1.pop();

	std::memcpy(tempPush->ucp, data, dataSize);
	tempPush->us = dataSize;

	q2.push(tempPush);

	cv.notify_all();
}

void SafeTwoQueue::pop(unsigned char* data, unsigned int& dataLen)
{
	std::unique_lock<std::mutex> lock(mtx);
	if (q2.empty())
	{
		cv.wait(lock, [this] { return !q2.empty() || noWait; });
		if (noWait)
		{
			return;
		}
	}

	tempPop = q2.front();
	q2.pop();

	std::memcpy(data, tempPop->ucp, tempPop->us);
	dataLen = tempPop->us;

	tempPop->us = bufferLen;
	q1.push(tempPop);
}

inline bool SafeTwoQueue::empty() const
{
	std::lock_guard<std::mutex> lock(mtx);
	return q2.empty();
}

inline size_t SafeTwoQueue::size() const
{
	std::lock_guard<std::mutex> lock(mtx);
	return q2.size();
}

void SafeTwoQueue::clear()
{
	std::lock_guard<std::mutex> lock(mtx);
	while (!q1.empty())
	{
		delete[] q1.front();
		q1.pop();
	}
	while (!q2.empty())
	{
		delete[] q2.front();
		q2.pop();
	}
}

SafeTwoQueue::~SafeTwoQueue()
{
	clear();
}