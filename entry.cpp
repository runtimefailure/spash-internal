/*  copyright (c) 2069 - all rights resolved 
    diego paste services ud.
*/

#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "render/render.hpp"
#include "core/communication/coms.hpp"
#include "rbx/update/offsets.hpp"
#include "rbx/taskscheduler/scheduler.hpp"

void bipass()
{
    static int seconds = 1;
    uintptr_t baseAddress = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
    using printfunc = void(__cdecl*)(int, const char*, ...);
    auto print = reinterpret_cast<printfunc>(baseAddress + Functions::print);

    print(1, "datamodel: 		", 			(taskscheduler::get_datamodel());
	print(1, "scriptcontext: 	", 			(taskscheduler::get_scriptcontext(taskscheduler::get_datamodel()));
	print(1, "gameloaded:		",			(taskscheduler::get_gameloaded(taskscheduler::get_datamodel()));

    seconds++;
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
		std::thread(bipass).detach();
		//MessageBoxA(NULL, "injected",  "diegosploit", MB_OK | MB_ICONERROR | MB_TOPMOST);
    }
    return TRUE;
}