#pragma once

#include <Windows.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_dx11.h>
#include <imgui/imgui_impl_win32.h>

namespace c_gui {
	namespace components
	{
		void editor();
		void explorer();
	}

	void render();
	void styles();
}