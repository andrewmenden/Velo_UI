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
	inline void init()
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

	inline bool hasIni()
	{
		return std::filesystem::exists(iniPath);
	}

	inline void resetLayout()
	{
		Packing packing{ Global::windowSize };
		uiModule->ChangeBounds(packing.nextWindow());

		auto enabledIt = uiModule->getWindows().getValue().begin();
		for (auto& m : modules | std::views::drop(1))
		{
			if (*(enabledIt++))
				m->ChangeBounds(packing.nextWindow());
		}
	}

	inline void restoreLayoutFromIni()
	{
		if (hasIni())
			ImGui::LoadIniSettingsFromDisk(iniPath);
		else
			resetLayout();
	}

	inline void loadFromJson(const nlohmann::json& data)
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

	inline void renderImGui()
	{
		ID::resetID();

		if (firstRender)
		{
			restoreLayoutFromIni();
			firstRender = false;
		}

		bool dummy;
		uiModule->RenderImGui(dummy);

		Global::inputWidth = uiModule->getInputWidth().getValue();
		Global::enableUiKey = uiModule->getEnabled().getValue().hotkey;

		//iterate through the remaining modules
		auto enabledIt = uiModule->getWindows().getValue().begin();
		for (const auto& m : modules | std::views::drop(1))
		{
			bool open = *enabledIt;
			if (open)
				m->RenderImGui(open);
			if (open != *enabledIt)
				uiModule->getWindows().setChanged();
			*enabledIt = open;

			++enabledIt;
		}

		if (uiModule->isRequestingResetLayout())
			resetLayout();
	}

	inline void changesAsJson(nlohmann::json& json)
	{
		for (const auto& m : modules)
			m->changesAsJson(json);
	}

	inline void updateFromJson(const nlohmann::json& json)
	{
		for (auto& m : modules)
			m->updateFromJson(json);
	}

	inline std::string changesAsJsonString()
	{
		if (Global::settingChanged)
		{
			Global::settingChanged = false;
			nlohmann::json temp;

			changesAsJson(temp);
			for (auto& m : modules)
				m->visit([](Setting* s) { s->resetChanged(); });

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