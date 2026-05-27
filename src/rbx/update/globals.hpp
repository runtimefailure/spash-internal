#pragma once
#include <Windows.h>
#include <ShlObj.h>
#include <filesystem>
#include <string>

#include <lualib.h>
#include <lobject.h>
#include <lstate.h>
#include <lapi.h>

namespace exploit {
	inline std::filesystem::path get_base_path() {
		wchar_t path[MAX_PATH];
		if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path))) {
			return std::filesystem::path(path);
		}
		return std::filesystem::path(L"C:\\Windows\\Temp");
	}

	inline std::filesystem::path localappdata 	= get_base_path();
	inline std::filesystem::path exploit		= localappdata / "diegosploit";
	inline std::filesystem::path workspace		= exploit / "workspace";
	inline std::filesystem::path autoexec		= exploit / "autoexec";
	inline std::filesystem::path logs			= exploit / "logs";
	inline std::string build 					= "2b1721d47abf49aa";
	inline std::string name 					= "diegosploit";
	inline lua_State* inject_state;

	namespace internal {
		namespace ui {
			inline static bool enabled = true;
			inline static float dpi = 1.0f;
		}
		namespace functions {
			inline static bool raknet = true;
		}
	}
}