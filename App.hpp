#ifndef APP_H
#define APP_H

#include <vector>
#include <array>
#include <ranges>

#include "Global.hpp"
#include "Setting.hpp"

constexpr float fontSize = 16.0f;

constexpr const char* iniPath = "Velo\\imgui.ini";
constexpr const char* fontPath = "Velo\\fonts\\UiFont.ttf";
constexpr const char* iconsPath = "Velo\\fonts\\Icons.ttf";

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
	std::vector<std::unique_ptr<Module>> modules; // modules[0] is always the UI module
	UiModule* uiModule = nullptr;
	Cycle cycle;

	bool firstRender = true;

public:
	inline void Init()
	{
		ImGuiIO& io = ImGui::GetIO();

		io.Fonts->Clear();
		io.Fonts->AddFontFromFileTTF(fontPath, fontSize);

		ImFontConfig config;
		config.MergeMode = true;
		//config.GlyphMinAdvanceX = fontSize; // Use if you want to make the icon monospaced

		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		io.Fonts->AddFontFromFileTTF(iconsPath, fontSize, &config, iconRanges.data());
		io.Fonts->Build();
	}

	inline bool HasIni()
	{
		return std::filesystem::exists(iniPath);
	}

	inline void ResetLayout()
	{
		Packing packing{ Global::windowSize };
		uiModule->ChangeBounds(packing.nextWindow());

		auto enabledIt = uiModule->GetWindows().GetValue().begin();
		for (auto& m : modules | std::views::drop(1))
		{
			if (*(enabledIt++))
				m->ChangeBounds(packing.nextWindow());
		}
	}

	inline void RestoreLayoutFromIni()
	{
		if (HasIni())
			ImGui::LoadIniSettingsFromDisk(iniPath);
		else
			ResetLayout();
	}

	inline void LoadFromJson(const nlohmann::json& data)
	{
		for (const auto& m : data["Modules"])
		{
			if (m["Name"] == "UI")
			{ // the UI module always comes first
				uiModule = new UiModule{ m };
				modules.insert(modules.begin(), std::unique_ptr<Module>{ uiModule });
			}
			else
				modules.push_back(std::make_unique<Module>(m));
		}
		ImGui::GetIO().IniFilename = iniPath;
	}

	inline void RenderImGui()
	{
		cycle.Reset();

		if (firstRender)
		{
			RestoreLayoutFromIni();
			firstRender = false;
		}

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

		//ImGui::Begin("test");
		//ImGui::ShowStyleEditor();
		//ImGui::End();

		bool dummy;
		uiModule->RenderImGui(dummy, cycle);

		if (uiModule->GetSearchChanged())
		{
			for (const auto& m : modules)
				m->UpdateSearch(uiModule->GetSearch());
		}

		//iterate through the remaining modules
		auto enabledIt = uiModule->GetWindows().GetValue().begin();
		for (const auto& m : modules | std::views::drop(1))
		{
			bool wasEnabled = *enabledIt;
			if (*enabledIt)
				m->RenderImGui(*enabledIt, cycle);
			if (*enabledIt != wasEnabled)
				uiModule->GetWindows().AddChange(cycle);

			++enabledIt;
		}

		if (uiModule->IsRequestingResetLayout())
			ResetLayout();
	}

	inline void UpdateFromJson(const nlohmann::json& json)
	{
		for (auto& m : modules)
			m->UpdateFromJson(json);
	}

	inline std::string ChangesAsJsonString()
	{
		if (!cycle.changes.empty())
		{
			nlohmann::json json;
			json["Changes"] = std::move(cycle.changes);
			cycle.changes.clear();
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