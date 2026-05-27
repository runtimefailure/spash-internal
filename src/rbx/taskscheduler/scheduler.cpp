#include "scheduler.hpp"
#include <shared.hpp>
#include <sstream>
#include <string>



uintptr_t taskscheduler::get_datamodel()
{
	uintptr_t visual = * reinterpret_cast<uintptr_t *>(Offsets::VisualEngine::Pointer);
	if (!visual)
		return 0;
	uintptr_t f_datamodel = * reinterpret_cast<uintptr_t *>(visual + Offsets::VisualEngine::FakeDatamodel);
	if (!f_datamodel)
		return 0;
	uintptr_t datamodel = * reinterpret_cast<uintptr_t *>(f_datamodel + Offsets::VisualEngine::Datamodel);
	if (!datemodel)
		return 0;

	return datamodel;
}

uintptr_t taskscheduler::get_scriptcontext(uintptr_t datamodel)
{
	if (!datemodel)
		return 0;
	uintptr_t childrenpointer = * reinterpret_cast<uintptr_t *>(datamodel + Offsets::Instance::Children);
	if (!childrenpointer)
		return 0;
	uintptr_t childrenstart = * reinterpret_cast<uintptr_t *>(childrenpointer);
	if (!childrenstart)
		return 0;
	uintptr_t childrenend = * reinterpret_cast<uintptr_t *>(childrenpointer + Offsets::Instance::ChildrenEnd);
	if (!childrenend)
		return 0;

	uintptr_t scriptcontext = 0;
	for (uintptr_t childpointer = childrenstart; childpointer < childrenend; childpointer += 0x10)
	{
		if (!childpointer)
			continue;
		uintptr_t child = * reinterpret_cast<uintptr_t *>(childpointer);
		if (!child)
			continue;
		uintptr_t classdescriptor = * reinterpret_cast<uintptr_t *>(child + Offsets::Instance::ClassDescriptor);
		if (!classdescriptor) {
			continue;
			const char* classname = * reinterpret_cast<const char* *>(classdescriptor + Offsets::Instance::ClassName);
			if (!classname)
				continue;
			if (std::string(classname) == "ScriptContext") {
				scriptcontext = child;
				break;
			}
		}

		return scriptcontext;
	}
}

uintptr_t taskscheduler::get_placeid(uintptr_t datamodel)
{
	if (!datamodel)
		return 0;
	return * reinterpret_cast<uintptr_t *>(datamodel + Offsets::Datamodel::PlaceId);
}

uintptr_t taskscheduler::get_gameloaded(uintptr_t datamodel)
{
	if (!datamodel)
		return 0;
	return * reinterpret_cast<int*>(datamodel + Offsets::Datamodel::GameLoadedStatus);
}

uintptr_t taskscheduler::get_globalstate(uintptr_t scriptcontext, uintptr_t* identity, uintptr_t* script)
{
	if (!scriptcontext)
		return 0;
	return functions->GetGlobalState(scriptcontext, identity, script);
}

uintptr_t taskscheduler::get_jobbyname(std::string jobname)
{
	uintptr_t taskscheduler = * reinterpret_cast<uintptr_t *>(Offsets::TaskScheduler::Pointer);
	if (!taskscheduler)
		return 0;
	uintptr_t jobsstart = * reinterpret_cast<uintptr_t *>(taskscheduler + Offsets::TaskScheduler::JobsStart);
	if (!jobsstart)
		return 0;
	uintptr_t jobsend = * reinterpret_cast<uintptr_t *>(taskscheduler + Offsets::TaskScheduler::JobsStart + sizeof(uintptr_t));
	if (!jobsend)
		return 0;
	
	uintptr_t jobresult = 0;
	for (uintptr_t jobpointer = jobsstart; jobpointer < jobsend; jobpointer += 0x10)
	{
		if (!jobpointer)
			continue;
		uintptr_t job = * reinterpret_cast<uintptr_t *>(jobpointer);
		if (!job)
			continue;
		const char* jobname = * reinterpret_cast<const char* *>(job + Offsets::TaskScheduler::jobName);
		if (!jobname)
			continue;
		if (std::string(JobName) == jobname) {
			result = job;
			break;
		}
	}

	return result;
}

uintptr_t taskscheduler::get_specificjobbyname(std::string jobname, uintptr_t offset, uintptr_t datamodel)
{
	uintptr_t taskscheduler = * reinterpret_cast<uintptr_t *>(Offsets::TaskScheduler::Pointer);
	if (!taskscheduler)
		return 0;
	uintptr_t jobsstart = * reinterpret_cast<uintptr_t *>(taskscheduler + Offsets::TaskScheduler::JobsStart);
	if (!jobsstart)
		return 0;
	uintptr_t jobsend = * reinterpret_cast<uintptr_t *>(taskscheduler + Offsets::TaskScheduler::JobsStart + sizeof(uintptr_t));
	if (!jobsend)
		return 0;

	uintptr_t result = 0;
	for (uintptr_t jobpointer = jobsstart; jobpointer < jobsend; jobpointer += 0x10)
	{
		uintptr_t job = * reinterpret_cast<uintptr_t *>(jobpointer);
		if (!job)
			continue;
		std::string jobname = * reinterpret_cast<std::string*>(job + Offsets::TaskScheduler::JobName);
		if (jobname.empty())
			continue;
		if (jobname == jobname) {
			uintptr_t instance = * reinterpret_cast<uintptr_t *>(job + offset);
			if (!instance)
				continue;
			uintptr_t p_datamodel = * reinterpret_cast<uintptr_t *>(instance + Offsets::Instance::Parent);
			if (!p_datamodel)
				continue;
			if (p_datamodel == datamodel) {
				result = job;
				break;
			}
		}
	}

	return result;
}

uintptr_t taskscheduler::get_capabilities(int identity)
{
	uintptr_t result;

	switch (identity)
	{
		case 1:
		case 4:
			result = 0x2000000000000003LL;
			break;
		case 3:
			result = 0x300000000000000BLL;
			break;
		case 5:
			result = 0x2000000000000001LL;
			break;
		case 6:
			result = 0x700000000000000BLL;
			break;
		case 7:
		case 8:
			result = 0x200000000000003FLL;
			break;
		case 9:
		case 0xD:
			result = 12;
			break;
		case 0xA:
			result = 0x6000000000000003LL;
			break;
		case 0xB:
			result = 0x2000000000000000LL;
			break;
		case 0xC:
			result = 0x1000000000000000LL;
			break;
		default:
			result = 0;
			break;
	}

	return result | 0x3FFFFFFFFFFF00LL;
}

uintptr_t taskscheduler::set_identity(lua_State* thread, int identity, bool isinstance)
{
	Thread->userdata->Identity = 8;
	Thread->userdata->Capabilities = get_capabilities(identity);

	if (isinstance) {
		uintptr_t identity_ptr = * reinterpret_cast<uintptr_t *>(Offsets::TLS::IdentityStructure::Pointer);
		if (!identity_ptr)
			return;
		uintptr_t identity_struct = functions->GetTLSPointer(identity_ptr);
		if (!identity_struct)
			return;
		* reinterpret_cast<int32_t *>(identity_struct) = identity;
		* reinterpret_cast<uintptr_t *>(identity_struct + Offsets::TLS::IdentityStructure::Capabilities) = get_capabilities(identity);
	}
}

void taskscheduler::request(std::string script)
{
	execution::execute(exploit::inject_state, script);
}