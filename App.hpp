#pragma once
#include <vector>
#include "imgui.h"
#include "imgui_internal.h"
#include "ID.hpp"

#include "Setting.hpp"
#include <fstream>
#include <filesystem>
#include <Windows.h>
#include <d3d11.h>
#include <d3d9.h>
#include <dxgi.h>
#include <SDL.h>
#include <stdio.h>
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_opengl3.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_sdl2.h"

using namespace Settings;
namespace fs = std::filesystem;

enum GraphicsDeviceType
{
	gdtNONE,
	gdtD3D9,
	gdtD3D11,
	gdtOPENGL3
};

namespace Global
{
	HWND hwnd;
	HINSTANCE hInstance;
	ID3D11Device* deviced3d11 = nullptr;
	IDirect3DDevice9* deviced3d9 = nullptr;
	ID3D11DeviceContext* deviceContext = nullptr;
	IDXGISwapChain* swapChain = nullptr;
	GraphicsDeviceType gdt;
	ImVec2 windowSize{};

	static const ImWchar iconRanges[5] =
	{
		0xe800,0xe823,
		0xf104, 0xf13e,
		0
	};

	ImVec2 getWindowSize()
	{
		return windowSize;
	}
}

class App
{
private:
	std::vector<SettingsModule*> modules;
	SettingsModule* UIModule;
	//std::vector<std::string> moduleNames;
	std::string jsonString;

	ImGuiIO* io;
	ImFont* font = nullptr;

	//ui module settings
	bool hasChanged;
	std::vector<bool*> modulesOpen;
	bool onLoad = true;
public:
	inline static Setting* JsonToPSetting(nlohmann::json data)
	{
		Setting* currS = nullptr;
		int type = data["Type"];
		switch (type)
		{
		case 0: //int
			currS = new IntSetting();
			break;
		case 1: //flaot
			currS = new FloatSetting();
			break;
		case 2: //bool
			currS = new BoolSetting();
			break;
		case 3: //toggle
			currS = new ToggleSetting();
			break;
		case 4: //hotkey
			currS = new HotkeySetting();
			break;
		case 5: //vector
			currS = new VectorSetting();
			break;
		case 6: //string
			currS = new StringSetting();
			break;
		case 7: //roundingMultiplier
			currS = new RoundingMultiplierSetting();
			break;
		case 8: //bool list
			currS = new BoolListSetting();
			break;
		case 9: //string list
			currS = new StringListSetting();
			break;
		case 10: //enumerator
			currS = new EnumeratorSetting();
			break;
		case 11: //color
			currS = new ColorSetting();
			break;
		case 12: //color transition
			currS = new ColorTransitionSetting();
			break;
		case 13: //input box
			currS = new InputBoxSetting();
			break;
		case 14: //category setting
			currS = new CategorySetting();
		}
		if (currS)
			currS->LoadFromJson(data);
		return currS;
	}

	inline void setIo()
	{
		io = &ImGui::GetIO();
	}

	inline void loadFonts(float fontSize)
	{
		io->Fonts->Clear();
		font = io->Fonts->AddFontFromFileTTF("Velo/fonts/UiFont.ttf", fontSize);

		ImFontConfig config;
		config.MergeMode = true;
		//config.GlyphMinAdvanceX = fontSize; // Use if you want to make the icon monospaced
		io->Fonts->AddFontFromFileTTF("Velo/fonts/Icons.ttf", fontSize, &config, Global::iconRanges);
		io->Fonts->Build();
	}
	
	inline void LoadUiSave()
	{
		int index = 0;
		if (!fs::exists(fs::path("Velo/Ui Save Data.json")))
			return;

		std::ifstream file("Velo/Ui Save Data.json");
		nlohmann::json data;
		file >> data;
		for (const auto& a : data)
		{
			switch ((int)a["ID"])
			{
			case -1: //Open Windows
				for (const auto& b : a["Value"])
				{
					modules[index++]->setOpen(b);
				}
				break;
			case -2: //Input Width
				Setting::width = a["Value"];
				break;
			case -3: //Open UI hotkey

				break;
			}
		}
		file.close();
		loadFonts(16.0f);
	}

	inline bool hasIni()
	{
		return fs::exists(fs::current_path() / fs::path("Velo\\imgui.ini"));
	}

	inline bool ResetLayout(bool useIni=true)
	{
		if (hasIni() && useIni)
		{
			ImGui::LoadIniSettingsFromDisk("Velo/imgui.ini");
			return false;
		}
		else
		{
			Packing::init(Global::getWindowSize());
			for (auto& m : modules)
				m->ResetLayout();
			return true;
		}
	}

	inline void LoadFromJson(nlohmann::json data)
	{
		EventBool::AnyChange = false;
		nlohmann::json temp;
		for (const auto& a : data["Modules"])
		{
			SettingsModule* currM = new SettingsModule();
			currM->setName(std::string(a["Name"]));
			//moduleNames.push_back(std::string(a["Name"]));
			for (const auto& b : a["Settings"])
			{
				if (b["ID"] >= 0)
					currM->pushSetting(JsonToPSetting(b));
			}
			if (currM->getName() == "UI")
			{
				UIModule = currM;
				continue;
			}
			modules.push_back(currM);
		}
		io = &ImGui::GetIO();
		io->IniFilename = "Velo/imgui.ini";
		LoadUiSave();
		//ResetLayout();
		//ImVec4 place = Packing::placeWindow();
		//ImGui::SetNextWindowPos(ImVec2(place.x, place.y));
		//ImGui::SetNextWindowSize(ImVec2(place.z, place.w));
	}

	inline void RenderImGui()
	{
		ID::resetID();

		if (onLoad)
		{
			if (ResetLayout(true))
			{
				ImVec4 place = Packing::placeWindow();
				ImGui::SetNextWindowPos(ImVec2(place.x, place.y));
				ImGui::SetNextWindowSize(ImVec2(place.z, place.w));
			}
			onLoad = false;
		}

		ImGui::Begin("UI");
		if (ImGui::CollapsingHeader("windows"))
		{
			if (ImGui::Button("show all"))
			{
				for (auto& a : modules)
					a->setOpen(true);
			}
			ImGui::SameLine();
			if (ImGui::Button("hide all"))
			{
				for (auto& a : modules)
					a->setOpen(false);
			}
			int i = 0;
			for (const auto& a : modules)
			{
				ImGui::PushID(ID::nextID());
				if (ImGui::MenuItem(a->getName().c_str(), NULL, &a->getOpen()))
					hasChanged = true;
				ImGui::PopID();
			}
		}

		ImGui::PushID(ID::nextID());
		ImGui::SetNextItemWidth(Setting::width);
		if (ImGui::DragFloat("input widths", &Setting::width, 1.0f, 1.0f, 1000.0f))
			hasChanged = true;
		ImGui::PopID();
		UIModule->RenderImGui();

		ImGui::PushID(ID::nextID());
		if (ImGui::Button("reset layout"))
		{
			ResetLayout(false);
			ImVec4 place = Packing::placeWindow();
			ImGui::GetCurrentWindow()->Pos =ImVec2(place.x, place.y);
			ImGui::SetWindowSize(ImVec2(place.z, place.w));
		}
		ImGui::PopID();
		ImGui::End();

		//iterate through modules
		for (int i = 0; i < modules.size(); i++)
		{
			if (modules[i]->getOpen())
			{
				modules[i]->RenderImGui();
			}
		}
		if (hasChanged)
		{
			hasChanged = false;
			nlohmann::json json;
			nlohmann::json temp;
			temp["ID"] = -1; //windows open
			for (const auto& a : modules)
				temp["Value"].push_back(a->getOpen());
			json.push_back(temp);
			temp["ID"] = -2; //width
			temp["Value"] = Setting::width;
			json.push_back(temp);
			std::ofstream file("Velo/Ui Save Data.json");
			file << std::setw(4) << json;
			file.close();
			hasChanged = false;
		}
	}

	inline void GetJson(nlohmann::json& json, bool all)
	{
		for (const auto& a : modules)
			a->GetJson(json, all);
		UIModule->GetJson(json, all);
	}

	inline void UpdateJson(nlohmann::json json)
	{
		for (const auto& a : modules)
			a->UpdateJson(json);
		UIModule->UpdateJson(json);
	}

	inline int GetChangeSize()
	{
		if (EventBool::AnyChange)
		{
			EventBool::AnyChange = false;
			nlohmann::json temp;
			GetJson(temp, false);
			nlohmann::json json;
			json["Changes"] = temp;
			jsonString = json.dump(2);
			return jsonString.size() + 1;//idk about null terminator
		}
		return 0;
	}

	inline std::string GetJsonString()
	{
		return jsonString;
	}

	inline ~App()
	{
		for (const auto& a : modules)
			delete a;
		for (const auto& a : modulesOpen)
			delete a;
		delete UIModule;
	}
};

namespace Global
{
	App app;
}