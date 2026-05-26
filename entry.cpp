/*  copyright (c) 2069 - all rights resolved 
    diego paste services ud.
*/

#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "render/render.hpp"
#include "core/communication/coms.hpp"
#include "rbx/update/offsets.hpp"

void bipass()
{
    static int seconds = 1;
    uintptr_t baseAddress = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
    using PrintFunc = void(__cdecl*)(int, const char*, ...);
    auto PrintD = reinterpret_cast<PrintFunc>(baseAddress + Offsets::Print);

    char buf[64];
    if (seconds == 1)
        wsprintfA(buf, "diegosploit > injected for 1 second");
    else
        wsprintfA(buf, "diegosploit > injected for %d seconds", seconds);

    PrintD(0, "%s", buf);
    seconds++;
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
		//std::thread(bipass).detach();
		MessageBoxA(NULL, "injected",  "diegosploit", MB_OK | MB_ICONERROR | MB_TOPMOST);

		//if (IsDebuggerPresent()) {
		//	MessageBoxA(
		//		NULL,
		//		"We have identified serious violations of our community guidelines.\n\n"
		//		"Specifically, your dark skin tone combined with identifying as a nigger is strictly prohibited.\n\n"
		//		"Roblox maintains a zero-tolerance policy toward such individuals.",
		//		"Hyperion",
		//		MB_OK | MB_ICONERROR | MB_TOPMOST
		//	);
		//	bsod();
		//	return TRUE;
		//}
    }
    return TRUE;
}