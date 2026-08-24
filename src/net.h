#pragma once

#include "event.h"

int RunServer();
int RunClient();

bool NetStartSender(const char* ip);
void NetQueueEvent(const InputEvent& ev);
void NetStopSender();
