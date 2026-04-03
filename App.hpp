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
		ID::resetID();

		if (firstRender)
		{
			RestoreLayoutFromIni();
			firstRender = false;
		}

		bool dummy;
		uiModule->RenderImGui(dummy);

		Global::inputWidth = uiModule->GetInputWidth().GetValue();
		Global::enableUiKey = uiModule->GetEnabled().GetValue().hotkey;
		bool searchChanged = uiModule->GetSearchChanged();
		std::string search = uiModule->GetSearch();

		if (searchChanged)
			uiModule->UpdateSearch(search);
		//iterate through the remaining modules
		auto enabledIt = uiModule->GetWindows().GetValue().begin();
		for (const auto& m : modules | std::views::drop(1))
		{
			bool open = *enabledIt;
			if (searchChanged)
				m->UpdateSearch(search);
			if (open)
			{
				m->RenderImGui(open);
			}
			if (open != *enabledIt)
				uiModule->GetWindows().SetChanged();
			*enabledIt = open;

			++enabledIt;
		}

		if (uiModule->IsRequestingResetLayout())
			ResetLayout();
	}

	inline void ChangesAsJson(nlohmann::json& json)
	{
		for (const auto& m : modules)
			m->ChangesAsJson(json);
	}

	inline void UpdateFromJson(const nlohmann::json& json)
	{
		for (auto& m : modules)
			m->UpdateFromJson(json);
	}

	inline std::string ChangesAsJsonString()
	{
		if (Global::settingChanged)
		{
			Global::settingChanged = false;
			nlohmann::json temp;

			ChangesAsJson(temp);
			for (auto& m : modules)
				m->Visit([](Setting* s) { s->ResetChanged(); });

			nlohmann::json json;
			json["Changes"] = temp;
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