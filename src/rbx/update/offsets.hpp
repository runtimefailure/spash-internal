//version-9377ee10133e4be3

#pragma once
#include <Windows.h>
#include <cstdint>

#ifndef REBASE
#define REBASE(x) (x)
#endif


namespace Offsets {
	const uintptr_t TaskSchedulerPointer = REBASE(0x7BFE988);
	const uintptr_t Print = REBASE(0x1DD04E0);
	const uintptr_t ScriptContextResume = REBASE(0x1D5F0B0);
	const uintptr_t get_capabilities = REBASE(0x48086F0);
	const uintptr_t KTable = REBASE(0x7C07590);
	const uintptr_t GetFFlag = REBASE(0x4B2A010);
	const uintptr_t WebSocketServiceEnableClientCreation = REBASE(0x5C8FB88);
	const uintptr_t FlogDataBank = REBASE(0x4B2CC10);
	const uintptr_t RobloxLogCrash = REBASE(0x4AF4970);
	const uintptr_t RawScheduler = REBASE(0x185A3E0);
	const uintptr_t GetProperty = REBASE(0x4AF4970);
	const uintptr_t OpcodeLookupTable = REBASE(0x62F1970);
	const uintptr_t IsLegalSendEvent = REBASE(0xA95458);
	const uintptr_t AttachRobloxExtraSpace = REBASE(0x1C93300);
	const uintptr_t loadSafe = REBASE(0x4750D80);
	namespace VisualEngine {
		const uintptr_t Pointer								= REBASE(0x7B79A08);
		const uintptr_t DeviceD3D11							= REBASE(0x8);		
		const uintptr_t SwapChain							= REBASE(0xC8);		
	}
}
namespace Hyperion {
	const uintptr_t ControlFlowGuardian = REBASE(0x75CFC0);
	const uintptr_t BitMap = REBASE(0x1D26B8);
}
namespace Raknet {
	const uintptr_t ProcessNetworkPacket = REBASE(0x1314090);
	const uintptr_t ReportNetworkError = REBASE(0x9CF4F0);
	const uintptr_t Send = REBASE(0x311DED0);
	const uintptr_t HandleConnectionState = REBASE(0x9CA8D0);
}
namespace Signals {
	const uintptr_t FireTouchInterest = REBASE(0x2A2C450);
	const uintptr_t FireMouseClick = REBASE(0x244F610);
	const uintptr_t FireRightMouseClick = REBASE(0x244F830);
	const uintptr_t FireMouseHoverEnter = REBASE(0x2450C40);
	const uintptr_t FireMouseHoverLeave = REBASE(0x2450E60);
	const uintptr_t FireClickDetector = REBASE(0x2863460);
	const uintptr_t FireProximityPrompt = REBASE(0x24CD410);
}
namespace Instance {
	const uintptr_t NewInstance = REBASE(0x62C240);
	const uintptr_t PushInstance = REBASE(0x1D22700);
}
namespace Thread {
	const uintptr_t GetCurrentThreadId = REBASE(0x9AD0);
	const uintptr_t IdentityPtr = REBASE(0x7A643C8);
	const uintptr_t GetIdentityStruct = REBASE(0x6C70);
	const uintptr_t Impersonator = REBASE(0x1C5FF60);
}
namespace Flags {
	const uintptr_t LockViolationScriptCrash = REBASE(0x474F4B0);
	const uintptr_t EnableLoadModule = REBASE(0x7618918);
}
namespace Lua {
	const uintptr_t Luau_Execute = REBASE(0x475C5B0);
	const uintptr_t luaD_throw = REBASE(0x474AFB0);
	const uintptr_t luaVM_load = REBASE(0x1C7B670);
	const uintptr_t luaH_dummynode = REBASE(0x62F49E4);
	const uintptr_t luaO_nilobject = REBASE(0x62F44E0);
	const uintptr_t luaL_newmetatable = REBASE(0x4EF1710);
	const uintptr_t luaC_step = REBASE(0x474F4B0);
	const uintptr_t luaT_eventnames = REBASE(0x62F4280);
	const uintptr_t luaT_typenames = REBASE(0x5ABA340);
	const uintptr_t luaD_rawrunprotected = REBASE(0x4B0DA30);
	const uintptr_t luaG_runerrorL = REBASE(0x474DE60);
	const uintptr_t lua_pushfstringL = REBASE(0x4746720);
	const uintptr_t luaA_toobject = REBASE(0x47452A0);
	const uintptr_t luaL_errorL = REBASE(0x474C310);
	const uintptr_t luaO_chunkid = REBASE(0x477AFC0);
	const uintptr_t luaF_newproto = REBASE(0x47750B0);
	const uintptr_t luaF_freeproto = REBASE(0x4774D20);
}
namespace TaskRblx {
	const uintptr_t Synchronize = REBASE(0x1DAB520);
	const uintptr_t Desynchronize = REBASE(0x1DAAAF0);
	const uintptr_t Defer = REBASE(0x1DAA450);
	const uintptr_t Spawn = REBASE(0x1DAB3A0);
	const uintptr_t Delay = REBASE(0x1DAA8B0);
	const uintptr_t Wait = REBASE(0x1DAB730);
	const uintptr_t Cancel = REBASE(0x1DAA140);
}
namespace DataModel {
	const uintptr_t FakeDataModelPointer = REBASE(0x74F6758);
	const uintptr_t FakeDataModelToDataModel = 0x1D0;
	const uintptr_t Children = 0x78;
	const uintptr_t GameLoaded = 0x638;
	const uintptr_t ScriptContext = 0x400;
	const uintptr_t GameId = 0x198;
}
namespace ExtraSpace {
	const uintptr_t ScriptContextToResume = 0x953;
	const uintptr_t IsCoreScript = 0x180;
	const uintptr_t RequireBypass = 0x828;
	const uintptr_t GetLuaStateForInstance = REBASE(0x1D57950);
	const uintptr_t GetLuaState = REBASE(0x1D56EF0);
}
namespace InstanceBase {
	const uintptr_t Primitive = 0x148;
	const uintptr_t Overlap = 0x1E8;
	const uintptr_t PropertyDescriptorBitFlags = 0x88;
	const uintptr_t ScriptableMask = 0x10;
	const uintptr_t ClassDescriptor = 0x18;
	const uintptr_t ClassName = 0x8;
	const uintptr_t PropDescriptor = 0x28;
}