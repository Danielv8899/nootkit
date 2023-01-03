#pragma once

#define ERROR "[!] nootkit: Failed: "
#define WARN "[*} nootkit: "
#define INFO "[-] nootkit: "

namespace Util {
	void handleError(NTSTATUS ntstatus, PCSTR fmt, ...);
}