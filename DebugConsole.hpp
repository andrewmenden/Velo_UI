#pragma once

#include "imgui.h"
#include <string>

class DebugConsole
{
private:
	static std::string contents;
public:
	static void Write(std::string content)
	{
		contents += content;
	}

	static void WriteLine(std::string content)
	{
		contents += content + '\n';
	}

	static void Clear()
	{
		contents.clear();
	}

	static void RenderImGui()
	{
		ImGui::Begin("Debug Console");
		ImGui::TextWrapped(contents.c_str());
		ImGui::End();
	}
};

std::string DebugConsole::contents = "";