#pragma once
#pragma execution_character_set("utf-8")
#define SPDLOG_SKIP_AUTOMATIC_COMPILE_OPTIONS_CHECK
#define FMT_UNICODE 0

#include <Windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "rbx/update/offsets.hpp"
#include "rbx/update/globals.hpp"

namespace fs = std::filesystem;

inline std::shared_ptr<spdlog::logger> logger;
inline std::shared_ptr<spdlog::logger> crash_logger;
inline PVOID veh_handle = nullptr;

inline void init_log_paths()
{
	try
	{
		std::error_code ec;
		if (!fs::exists(exploit::logs, ec)) {
			fs::create_directories(exploit::logs, ec);
		}

		if (ec) {
			std::string errMsg = "failed to create exploit filesystem: " + ec.message();
			MessageBoxA(NULL, errMsg.c_str(), "init error", MB_OK | MB_ICONERROR);
			return;
		}

		logger = spdlog::basic_logger_mt("logger", (exploit::logs / "debug.log").string(), true);
		crash_logger = spdlog::basic_logger_mt("crash_logger", (exploit::logs / "crash.log").string(), true);

		if (logger && crash_logger) {
			logger->set_level(spdlog::level::debug);
			crash_logger->set_level(spdlog::level::err);
			spdlog::flush_on(spdlog::level::debug);
		}
	}
	catch (const std::exception& e) {
		std::string errMsg = "spdlog runtime crash: ";
		errMsg += e.what();
		MessageBoxA(NULL, errMsg.c_str(), "Spdlog Catch Block", MB_OK | MB_ICONERROR);
	}
	catch (...) {
		MessageBoxA(NULL, "unknown exception inside log setup.", "fallback", MB_OK | MB_ICONERROR);
	}
}

inline bool safe_read_bytes(uintptr_t src, unsigned char* dest)
{
	__try
	{
		unsigned char* p = reinterpret_cast<unsigned char*>(src);
		for (int i = 0; i < 16; ++i) {
			dest[i] = p[i];
		}
		return true;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		return false;
	}
}

inline void dump_memory(uintptr_t address, size_t range, std::shared_ptr<spdlog::logger>& logger)
{
	if (!logger) return;
	logger->error("memory dump around 0x{:X}:", address);
	uintptr_t start = address - (range / 2);

	for (size_t i = 0; i < range; i += 16)
	{
		uintptr_t current_addr = start + i;
		unsigned char bytes[16] = { 0 };
		char buf[128];

		if (safe_read_bytes(current_addr, bytes))
		{
			sprintf_s(buf, "0x%016llX: %02X %02X %02X %02X %02X %02X %02X %02X - %02X %02X %02X %02X %02X %02X %02X %02X",
				current_addr, bytes[0], bytes[1], bytes[2], bytes[3], bytes[4], bytes[5], bytes[6], bytes[7],
				bytes[8], bytes[9], bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);
		}
		else
		{
			sprintf_s(buf, "0x%016llX: ?? ?? ?? ?? ?? ?? ?? ?? - ?? ?? ?? ?? ?? ?? ?? ??", current_addr);
		}

		logger->error("  {}", buf);
	}
}

inline LONG WINAPI VectoredExceptionHandler(PEXCEPTION_POINTERS exceptionInfo)
{
	PEXCEPTION_RECORD record = exceptionInfo->ExceptionRecord;
	PCONTEXT context = exceptionInfo->ContextRecord;
	DWORD code = record->ExceptionCode;

	if (code == EXCEPTION_ACCESS_VIOLATION || code == EXCEPTION_ILLEGAL_INSTRUCTION || code == 0xC0000409)
	{
		if (crash_logger)
		{
			crash_logger->error("================ CRASH DETECTED ================");
			crash_logger->error("Exception Code: 0x{:X}", code);
			crash_logger->error("Exception Address: 0x{:X} (Base Offset: 0x{:X})",
				reinterpret_cast<uintptr_t>(record->ExceptionAddress),
				reinterpret_cast<uintptr_t>(record->ExceptionAddress) - reinterpret_cast<uintptr_t>(GetModuleHandle(NULL)));

			if (code == EXCEPTION_ACCESS_VIOLATION)
			{
				crash_logger->error("Inaccessible Memory Operation: {}", record->ExceptionInformation[0] == 0 ? "READ" : "WRITE");
				crash_logger->error("Inaccessible Target Address: 0x{:X}", record->ExceptionInformation[1]);
			}

			crash_logger->error("Registers Snapshot:");
			crash_logger->error("  RAX: 0x{:X} | RBX: 0x{:X}", context->Rax, context->Rbx);
			crash_logger->error("  RCX: 0x{:X} | RDX: 0x{:X}", context->Rcx, context->Rdx);
			crash_logger->error("  RSI: 0x{:X} | RDI: 0x{:X}", context->Rsi, context->Rdi);
			crash_logger->error("  RBP: 0x{:X} | RSP: 0x{:X}", context->Rbp, context->Rsp);
			crash_logger->error("  R8:  0x{:X} | R9:  0x{:X}", context->R8, context->R9);
			crash_logger->error("  R10: 0x{:X} | R11: 0x{:X}", context->R10, context->R11);
			crash_logger->error("  R12: 0x{:X} | R13: 0x{:X}", context->R12, context->R13);
			crash_logger->error("  R14: 0x{:X} | R15: 0x{:X}", context->R14, context->R15);
			crash_logger->error("  RIP: 0x{:X}", context->Rip);

			dump_memory(context->Rip, 64, crash_logger);

			if (code == EXCEPTION_ACCESS_VIOLATION && record->ExceptionInformation[1] > 0x1000)
			{
				dump_memory(record->ExceptionInformation[1], 64, crash_logger);
			}

			crash_logger->flush();
		}
		else {
			char panic_buf[256];
			sprintf_s(panic_buf, "crash handler tripped before logger initialized.\nCode: 0x%X\nRIP: 0x%llX", code, context->Rip);
			MessageBoxA(NULL, panic_buf, "earily crash handler", MB_OK | MB_ICONERROR);
		}

		if (logger)
		{
			logger->flush();
		}

		SuspendThread(GetCurrentThread());
		while (true) { Sleep(1000); }
	}

	return EXCEPTION_CONTINUE_SEARCH;
}

namespace crashhandler
{
	inline bool setup()
	{
		init_log_paths();

		if (logger)
			logger->info("initializing crash handler...");

		veh_handle = AddVectoredExceptionHandler(1, VectoredExceptionHandler);
		return true;
	}
}