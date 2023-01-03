#include "Header.h"
#include "Util.h"

void Util::handleError(NTSTATUS ntstatus, PCSTR fmt, ...) {

    va_list args;
    va_start(args, fmt);

    vDbgPrintExWithPrefix(ERROR, DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, fmt, args);
    DbgPrint("%d\n", ntstatus);
    va_end(args);
}