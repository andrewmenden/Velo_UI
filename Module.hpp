#ifndef MODULE_H
#define MODULE_H

#include "ModuleManager.hpp"
#include "Setting.hpp"
#include "Packing.hpp"

class HiddenModule : public Module
{
public:
	inline HiddenModule(std::string_view name) :
		Module{ name }
	{
		enabled = false;
	}

	inline bool Visible() override { return false; }
	inline void RenderImGui(Cycle&) override {}
	inline void UpdateSearch(std::string_view) override {}
	inline void Visit(const std::function<void(Identifyable&)>&) override {}
};

template <bool Owning> // meh but allows us to reuse this class for FavoritesModule
class BasicSettingsModule : public Module
{
	using SettingT = std::conditional_t<Owning, std::unique_ptr<Setting>, Setting*>;

protected:
	std::vector<SettingT> settings;

public:
	using Module::Module;

	inline virtual void RenderImGui(Cycle& cycle) override
	{
		UpdateBounds();

		if (ImGui::Begin(name.c_str(), &enabled))
		{
			RenderSettings(cycle);
		}
		ImGui::End();
	}

	inline void RenderSettings(Cycle& cycle)
	{
		for (const auto& setting : settings)
		{
			PreRender(&*setting, cycle);
			
			if (!setting->GetMatchesSearch())
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
			setting->RenderImGui(cycle);
			if (!setting->GetMatchesSearch())
				ImGui::PopStyleVar();

			PostRender(&*setting, cycle);
		}
	}

	inline void UpdateSearch(std::string_view pattern) override
	{
		if constexpr (Owning)
		{
			for (const auto& s : settings)
				s->UpdateSearch(pattern);
		}
	}

	template <std::derived_from<Setting> T>
	inline T* GetSetting(std::string_view name) const
	{
		for (const auto& s : settings)
		{
			if (s->GetName() == name)
				return dynamic_cast<T*>(s.get());
		}
		return nullptr;
	}

	inline virtual void Visit(const std::function<void(Identifyable&)>& f) override
	{
		if constexpr (Owning)
		{
			for (const auto& s : settings)
				s->Visit(f);
		}
	}

	inline virtual void PreRender(Setting* setting, Cycle& cycle) {};
	inline virtual void PostRender(Setting* setting, Cycle& cycle) {};
};

class SettingsModule : public BasicSettingsModule<true>
{
public:
	inline SettingsModule(const nlohmann::json& data) :
		BasicSettingsModule{ data["Name"] }
	{
		for (const auto& s : data["Settings"])
		{
			if (s["ID"] >= 0)
				settings.push_back(JsonToSetting(s));
		}
	}

	inline void PushSetting(std::unique_ptr<Setting>&& setting)
	{
		settings.push_back(std::move(setting));
	}
};

class UiModule : public SettingsModule
{
	bool requestResetLayout = false;
	bool searchChanged = false;
	bool firstRender = true;
	std::array<char, 100> searchBuffer{};

public:
	using SettingsModule::SettingsModule;

	inline void RenderImGui(Cycle& cycle) override
	{
		if (firstRender)
		{
			RestoreLayoutFromIni(cycle);
			firstRender = false;
		}

		UpdateBounds();

		cycle.inputWidth = GetInputWidth()->GetValue();
		cycle.enableUiHotkey = GetEnabled()->GetValue().hotkey;

		if (ImGui::Begin(name.c_str()))
		{
			ImGui::PushID(cycle.NextID());
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::InputTextWithHint("", "Search...", searchBuffer.data(), searchBuffer.size()))
				searchChanged = true;
			ImGui::PopID();

			RenderSettings(cycle);

			ImGui::PushID(cycle.NextID());
			if (ImGui::Button("reset layout"))
				requestResetLayout = true;
			ImGui::PopID();

		}
		ImGui::End();

		BoolListSetting* windows = GetWindows();
		auto enabledIt = windows->GetValue().begin();
		for (auto& m : cycle.modules.VisibleModules())
		{
			if (m.get() == this) continue;
			m->enabled = *(enabledIt++);
		}
	}

	inline virtual void Update(Cycle& cycle) override
	{
		BoolListSetting* windows = GetWindows();
		auto enabledIt = windows->GetValue().begin();
		bool changed = false;
		for (auto& m : cycle.modules.VisibleModules())
		{
			if (m.get() == this) continue;
			if (*enabledIt != m->enabled)
			{
				*enabledIt = m->enabled;
				changed = true;
			}
			++enabledIt;
		}
		if (changed)
			windows->AddChange(cycle);

		if (searchChanged)
		{
			for (const auto& m : cycle.modules)
				m->UpdateSearch(searchBuffer.data());
			searchChanged = false;
		}

		if (requestResetLayout)
		{
			ResetLayout(cycle);
			requestResetLayout = false;
		}
	}

	inline static bool HasIni()
	{
		return std::filesystem::exists(Global::iniPath);
	}

	inline void ResetLayout(Cycle& cycle)
	{
		Packing packing{ Global::windowSize };

		for (auto& m : cycle.modules)
		{
			if (m->enabled)
				m->ChangeBounds(packing.nextWindow());
		}
	}

	inline void RestoreLayoutFromIni(Cycle& cycle)
	{
		if (HasIni())
			ImGui::LoadIniSettingsFromDisk(Global::iniPath);
		else
			ResetLayout(cycle);
	}

	inline FloatSetting* GetInputWidth() const {
		return GetSetting<FloatSetting>("input width");
	}

	inline BoolListSetting* GetWindows() const
	{
		return GetSetting<BoolListSetting>("windows");
	}

	inline ToggleSetting* GetEnabled() const
	{
		return GetSetting<ToggleSetting>("enabled");
	}
};

class FavoritesModule : public BasicSettingsModule<false>, public Identifyable
{
	std::vector<int> temp;
	Setting* currentlyRendered = nullptr;

public:
	inline FavoritesModule(const nlohmann::json& json) :
		BasicSettingsModule{ "Favorites" },
		Identifyable{ json["ID"] }
	{
		for (int id : json["Favorites"])
			temp.push_back(id);
	}

	inline void RenderImGui(Cycle& cycle) override
	{
		for (int id : temp)
		{
			Setting* setting = dynamic_cast<Setting*>(cycle.modules.GetIdent(id));
			if (setting != nullptr)
				settings.push_back(setting);
		}
		temp.clear();

		BasicSettingsModule::RenderImGui(cycle);
	}

	inline void PreRender(Setting* setting, Cycle& cycle) override
	{
		currentlyRendered = setting;
	}

	inline void ContextMenu(Identifyable& ident, Cycle& cycle) override
	{
		bool changed = false;

		if (Setting* s = dynamic_cast<Setting*>(&ident))
		{
			ImGui::Separator();

			auto it = std::ranges::find_if(settings, [&](const auto& test)
				{
					return test->GetID() == s->GetID();
				});
			bool isFavorite = it != settings.end();
			if (!isFavorite)
			{
				if (ImGui::MenuItem("add favorite"))
				{
					settings.push_back(s);
					ImGui::CloseCurrentPopup();
					changed = true;
				}
			}
			else
			{
				if (ImGui::MenuItem("remove favorite"))
				{
					settings.erase(it);
					ImGui::CloseCurrentPopup();
					changed = true;
				}
				else if (s == currentlyRendered) // important to write else
				{
					if (it != settings.begin() && ImGui::MenuItem("move up"))
					{
						std::swap(*(it - 1), *it);
						ImGui::CloseCurrentPopup();
						changed = true;
					}
					if (it != settings.end() - 1 && ImGui::MenuItem("move down"))
					{
						std::swap(*it, *(it + 1));
						ImGui::CloseCurrentPopup();
						changed = true;
					}
				}
			}
		}

		if (changed)
		{
			nlohmann::json json;
			json["ID"] = id;

			for (const auto& s : settings)
				json["Favorites"].push_back(s->GetID());

			cycle.changes.push_back(std::move(json));
		}
	}

	inline void UpdateFromJson(const nlohmann::json& json) override
	{
		
	}

	inline void Visit(const std::function<void(Identifyable&)>& f) override
	{
		f(*this);
	}
};

inline std::unique_ptr<Module> JsonToModule(const nlohmann::json& json)
{
	return std::unique_ptr<Module>{ [&]() -> Module*
		{
			std::string name = json["Name"];
			if (name == "UI")
				return new UiModule{ json };
			if (name == "Favorites")
				return new FavoritesModule{ json };
			return new SettingsModule{ json };
		}() };
}

class ClipboardModule : public HiddenModule
{
	std::unique_ptr<Setting> clipboard;

public:
	inline ClipboardModule() :
		HiddenModule{ "Clipboard" } {}

	inline void ContextMenu(Identifyable& ident, Cycle& cycle) override
	{
		if (Setting* s = dynamic_cast<Setting*>(&ident))
		{
			ImGui::Separator();

			if (ImGui::MenuItem("copy"))
			{
				clipboard = s->Copy();
				ImGui::CloseCurrentPopup();
			}
			if (clipboard && s->PasteCompatible(clipboard.get()) && ImGui::MenuItem("paste"))
			{
				s->Paste(clipboard.get(), cycle);
				ImGui::CloseCurrentPopup();
			}
		}
	}
};

#endif