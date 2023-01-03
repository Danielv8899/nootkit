#pragma once

typedef struct _ELEVATOR {
    int pid;
    int key;
}ELEVATOR, * PELEVATOR;

typedef struct _ORIG_TOKEN {
    PTRLEN peproc;
    PTRLEN ptoken_efr;
}ORIG_TOKEN, * PORIG_TOKEN;

namespace Elevator {
	PTRLEN FindProcessEPROC(int terminate_PID);
	VOID SeDeleteAccessState(PACCESS_STATE AccessState);
	NTSTATUS Escalate(int pid);
};