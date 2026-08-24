#pragma once

#include <winsock2.h>
#include "event.h"

static const int kQueueSize = 1024;

struct EventQueue {
    InputEvent items[kQueueSize];
    int head;
    int tail;
    int count;
    bool stopping;
    CRITICAL_SECTION lock;
    HANDLE wakeup;

    bool Init();
    void Destroy();
    bool Push(const InputEvent& ev);
    bool Pop(InputEvent& out);
    void Stop();

    EventQueue() = default;
    EventQueue(const EventQueue&) = delete;
    EventQueue& operator=(const EventQueue&) = delete;
};