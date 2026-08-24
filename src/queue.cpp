#include "queue.h"

bool EventQueue::Init()
{
    head = 0;
    tail = 0;
    count = 0;
    stopping = false;

    InitializeCriticalSection(&lock);
    wakeup = CreateSemaphore(nullptr, 0, kQueueSize, nullptr);

    if (wakeup == nullptr)
    {
        DeleteCriticalSection(&lock);
        return false;
    }

    return true;
}

void EventQueue::Destroy()
{
    if (wakeup != nullptr)
    {
        CloseHandle(wakeup);
        wakeup = nullptr;
    }

    DeleteCriticalSection(&lock);
}

bool EventQueue::Push(const InputEvent& ev)
{
    bool queued = false;

    EnterCriticalSection(&lock);

    if (count < kQueueSize)
    {
        items[head] = ev;
        head = (head + 1) % kQueueSize;
        count++;
        queued = true;
    }

    LeaveCriticalSection(&lock);

    if (queued)
    {
        ReleaseSemaphore(wakeup, 1, nullptr);
    }

    return queued;
}

bool EventQueue::Pop(InputEvent& out)
{
    for (;;)
    {
        WaitForSingleObject(wakeup, INFINITE);

        EnterCriticalSection(&lock);

        if (count > 0)
        {
            out = items[tail];
            tail = (tail + 1) % kQueueSize;
            count--;

            LeaveCriticalSection(&lock);
            return true;
        }

        const bool done = stopping;

        LeaveCriticalSection(&lock);

        if (done)
        {
            return false;
        }
    }
}

void EventQueue::Stop()
{
    EnterCriticalSection(&lock);
    stopping = true;
    LeaveCriticalSection(&lock);

    ReleaseSemaphore(wakeup, 1, nullptr);
}
