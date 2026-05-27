/*  copyright (c) 2069 - all rights resolved 
    diego paste services ud.

	see credits.md ;-;
*/

#pragma once
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "render/render.hpp"
#include "core/communication/coms.hpp"
#include "rbx/update/offsets.hpp"
#include "rbx/update/globals.hpp"
#include "rbx/taskscheduler/scheduler.hpp"

void bipass()
{
    uintptr_t baseAddress = reinterpret_cast<uintptr_t>(GetModuleHandleA(nullptr));
    using printfunc = void(__cdecl*)(int, const char*, ...);
    auto print = reinterpret_cast<printfunc>(baseAddress + Functions::print);
	auto datamodel = taskscheduler::get_datamodel();

    print(1, "datamodel: %s", 			taskscheduler::get_datamodel()); // cracka technologies
	print(1, "scriptcontext: %s", 		taskscheduler::get_scriptcontext(datamodel));
	print(1, "gameloaded: %s",			taskscheduler::get_gameloaded(datamodel));
	print(0, "%s loaded", 				exploit::name);
}


BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
		std::thread(bipass).detach();
		//MessageBoxA(NULL, "injected",  exploit::name, MB_OK | MB_ICONERROR | MB_TOPMOST);
    }
    return TRUE;
}