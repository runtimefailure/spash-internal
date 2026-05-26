#pragma once
#include <iostream>
#include <Windows.h>
#include <thread>
#include <string>
#include <functional>

namespace tphandler {
	void reset();
	void initenv(uintptr_t Datamodel);
	void initialize();
}