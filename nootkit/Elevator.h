#pragma once

typedef struct _ELEVATOR {
    int pid;
    int key;
}ELEVATOR, * PELEVATOR;

namespace Elevator {
	NTSTATUS Escalate(int pid);
};