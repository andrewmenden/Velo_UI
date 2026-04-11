#ifndef APP_H
#define APP_H

#include <vector>
#include <array>
#include <ranges>

#include "Global.hpp"
#include "Module.hpp"

constexpr float fontSize = 16.0f;

// corresponds to Velo/fonts/Icons.ttf
constexpr std::array<ImWchar, 5> iconRanges
{
	0xe800, 0xe823,
	0xf104, 0xf13e,
	0
};

class App
{
private:
	ModuleManager modules;
	nlohmann::json changes;

public:
	inline void Init()
	{
		ImGuiIO& io = ImGui::GetIO();

		io.Fonts->Clear();
		io.Fonts->AddFontFromFileTTF(Global::fontPath, fontSize);

		ImFontConfig config;
		config.MergeMode = true;
		//config.GlyphMinAdvanceX = fontSize; // Use if you want to make the icon monospaced

		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable | ImGuiConfigFlags_NavEnableGamepad | ImGuiConfigFlags_NavEnableKeyboard;

		io.Fonts->AddFontFromFileTTF(Global::iconsPath, fontSize, &config, iconRanges.data());
		io.Fonts->Build();
	}

	inline void UpdateFromJson(const nlohmann::json& json)
	{
		modules.UpdateFromJson(json);
	}

	inline void LoadFromJson(const nlohmann::json& data)
	{
		modules.Push(std::make_unique<ClipboardModule>());
		modules.LoadFromJson(data);
		ImGui::GetIO().IniFilename = Global::iniPath;
	}

	inline void RenderImGui()
	{
		Cycle cycle{ .modules = modules, .changes = changes };

		// ImGui::DockSpaceOverViewport();
		ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
		const ImGuiViewport *viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);
		window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground;
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::Begin("DockSpace", nullptr, window_flags);
		ImGui::PopStyleVar(2);

		ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspace_id, ImVec2{ 0.0f, 0.0f }, ImGuiDockNodeFlags_PassthruCentralNode);
		ImGui::End();

		modules.DoCycle(cycle);
	}

	inline std::string ChangesAsJsonString()
	{
		if (!changes.empty())
		{
			nlohmann::json json;
			json["Changes"] = std::move(changes);
			changes.clear();
			return json.dump(0);
		}
		return "";
	}
};

namespace Global
{
	App app;
}

#endif