#pragma once

VOID IocUnload(PDRIVER_OBJECT driverObject);
NTSTATUS IocCreateClose(PDEVICE_OBJECT devObj, PIRP irp);
NTSTATUS IoctlHandler(PDEVICE_OBJECT devObj, PIRP irp);
