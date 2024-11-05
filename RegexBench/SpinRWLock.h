#pragma once

#include <atomic>
#include <thread>

class SpinRWLock
{
private:
    std::atomic<int> reader_count;
    std::atomic<bool> writer;

public:
    SpinRWLock() : reader_count(0), writer(false) {}

    void lock_read()
    {
        while (true)
        {
            while (writer.load(std::memory_order_acquire))
            {
                std::this_thread::yield();
            }

            reader_count.fetch_add(1, std::memory_order_acquire);

            if (!writer.load(std::memory_order_acquire))
            {
                break;
            }

            reader_count.fetch_sub(1, std::memory_order_release);
        }
    }

    void unlock_read()
    {
        reader_count.fetch_sub(1, std::memory_order_release);
    }

    void lock_write()
    {
        while (writer.exchange(true, std::memory_order_acquire))
        {
            std::this_thread::yield();
        }

        while (reader_count.load(std::memory_order_acquire) > 0)
        {
            std::this_thread::yield();
        }
    }

    void unlock_write()
    {
        writer.store(false, std::memory_order_release);
    }
};

// ReadLockGuard 클래스
class ReadLockGuard
{
private:
    SpinRWLock& lock;

public:
    explicit ReadLockGuard(SpinRWLock& rwLock) : lock(rwLock)
    {
        lock.lock_read();
    }

    ~ReadLockGuard()
    {
        lock.unlock_read();
    }
};

// WriteLockGuard 클래스
class WriteLockGuard
{
private:
    SpinRWLock& lock;

public:
    explicit WriteLockGuard(SpinRWLock& rwLock) : lock(rwLock)
    {
        lock.lock_write();
    }

    ~WriteLockGuard()
    {
        lock.unlock_write();
    }
};
