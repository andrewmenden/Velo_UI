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
	inline void UpdateSearch(std::string_view, bool) override {}
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

		bool anyMatches = 
			std::ranges::any_of(settings, [](const auto& s) { return s->GetMatchesSearch(); }) ||
			(std::ranges::empty(settings) && !cycle.search); // can occur with favorites
		PushSearchStyle(anyMatches, cycle);
		if (ImGui::Begin(name.c_str(), &enabled))
		{
			PopSearchStyle(anyMatches, cycle);
			RenderSettings(cycle);
		}
		ImGui::End();
	}

	inline void RenderSettings(Cycle& cycle)
	{
		for (const auto& setting : settings)
		{
			PreRender(&*setting, cycle);
			
			RenderChildSetting(&*setting, cycle);

			PostRender(&*setting, cycle);
		}
	}

	inline void UpdateSearch(std::string_view pattern, bool matchTooltips) override
	{
		if constexpr (Owning)
		{
			for (const auto& s : settings)
				s->UpdateSearch(pattern, matchTooltips);
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
	std::array<char, 100> searchBuffer{};

public:
	using SettingsModule::SettingsModule;

	inline void Init(Cycle& cycle) override
	{
		RestoreLayoutFromIni(cycle);

		UpdateStyleColorsPreset(GetSettingsUIStyleTheme(), GetSettingsUIStyleHue());
		UpdateStyleRounding(GetSettingsUIStyleRounding());
		UpdateStyleBorder(GetSettingsUIStyleBorder());
		UpdateStyleCenteredTitle(GetSettingsUIStyleCenteredTitle());
	}

	inline void RenderImGui(Cycle& cycle) override
	{
		UpdateBounds();

		cycle.inputWidth = GetSettingsUIStyleInputWidth()->GetValue();
		cycle.theme = GetSettingsUIStyleTheme()->GetValue();
		cycle.highlightMatches = GetSearchHighlightMatches()->GetValue();
		cycle.search = strnlen_s(searchBuffer.data(), searchBuffer.size()) != 0;
		cycle.hideNonMatches = GetSearchHideNonMatches()->GetValue();
		cycle.enableUiHotkey = GetEnabled()->GetValue().hotkey;

		bool resetLayoutMatches = LowerContains("reset layout", searchBuffer.data());

		bool anyMatches = 
			std::ranges::any_of(settings, [](const auto& s) { return s->GetMatchesSearch(); }) ||
			resetLayoutMatches;
		PushSearchStyle(anyMatches, cycle);
		if (ImGui::Begin(name.c_str()))
		{
			PopSearchStyle(anyMatches, cycle);
			ImGui::PushID(cycle.NextID());
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::InputTextWithHint("", "Search...", searchBuffer.data(), searchBuffer.size()))
				searchChanged = true;
			ImGui::PopID();

			RenderSettings(cycle);

			if (resetLayoutMatches || !cycle.hideNonMatches)
			{
				PushSearchStyle(resetLayoutMatches, cycle);

				ImGui::PushID(cycle.NextID());
				if (ImGui::Button("reset layout"))
					requestResetLayout = true;
				ImGui::PopID();

				PopSearchStyle(resetLayoutMatches, cycle);
			}
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

		if (searchChanged || GetSearchMatchTooltips()->HasChanged())
		{
			for (const auto& m : cycle.modules)
				m->UpdateSearch(searchBuffer.data(), GetSearchMatchTooltips()->GetValue());
			searchChanged = false;
		}

		EnumeratorSetting* preset = GetSettingsUIStyleTheme();
		FloatSetting* hue = GetSettingsUIStyleHue();
		FloatSetting* rounding = GetSettingsUIStyleRounding();
		BoolSetting* border = GetSettingsUIStyleBorder();
		BoolSetting* centeredTitle = GetSettingsUIStyleCenteredTitle();

		if (preset->HasChanged())
			UpdateStyleColorsPreset(preset, hue);
		if (hue->HasChanged())
			UpdateStyleColorsHue(hue);
		if (rounding->HasChanged())
			UpdateStyleRounding(rounding);
		if (border->HasChanged())
			UpdateStyleBorder(border);
		if (centeredTitle->HasChanged())
			UpdateStyleCenteredTitle(centeredTitle);

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
				m->ChangeBounds(packing.NextWindow());
		}
	}

	inline void RestoreLayoutFromIni(Cycle& cycle)
	{
		if (HasIni())
			ImGui::LoadIniSettingsFromDisk(Global::iniPath);
		else
			ResetLayout(cycle);
	}

	inline void UpdateStyleRounding(FloatSetting* rounding) const
	{
		if (!rounding)
			return;
		ImGuiStyle& style = ImGui::GetStyle();
		float roundingValue = rounding->GetValue();
		style.WindowRounding = roundingValue;
		style.ChildRounding = roundingValue;
		style.FrameRounding = roundingValue;
		style.PopupRounding = roundingValue;
		style.GrabRounding = roundingValue;
	}

	inline void UpdateStyleBorder(BoolSetting* border) const
	{
		if (!border)
			return;
		ImGuiStyle& style = ImGui::GetStyle();
		float borderSize = border->GetValue() ? 1.0f : 0.0f;
		style.WindowBorderSize = borderSize;
		style.ChildBorderSize = borderSize;
		style.PopupBorderSize = borderSize;
	}

	inline void UpdateStyleCenteredTitle(BoolSetting* centeredTitle) const
	{
		if (!centeredTitle)
			return;
		ImGuiStyle& style = ImGui::GetStyle();
		style.WindowTitleAlign.x = centeredTitle->GetValue() ? 0.5f : 0.0f;
	}

	inline void UpdateStyleColorsPreset(EnumeratorSetting* preset, FloatSetting* hue) const
	{
		if (!preset || !hue)
			return;
		switch (preset->GetValue())
		{
		case 0:
			ImGui::StyleColorsDark();
			break;
		case 1:
			ImGui::StyleColorsLight();
			break;
		case 2:
			ImGui::StyleColorsClassic();
			break;
		}
		UpdateStyleColorsHue(hue);
	}

	inline void UpdateStyleColorsHue(FloatSetting* hue) const
	{
		if (!hue)
			return;

		ImGuiStyle& style = ImGui::GetStyle();
		float hueValue = hue->GetValue();

		for (int i = 0; i < ImGuiCol_COUNT; i++)
		{
			ImVec4& col = style.Colors[i];
			float h,s,v;
			ImGui::ColorConvertRGBtoHSV(col.x, col.y, col.z, h, s, v);
			h = hueValue;
			ImGui::ColorConvertHSVtoRGB(h, s, v, col.x, col.y, col.z);
		}
	}

	inline BoolListSetting* GetWindows() const
	{
		return GetSetting<BoolListSetting>("windows");
	}

	inline ToggleSetting* GetEnabled() const
	{
		return GetSetting<ToggleSetting>("enabled");
	}

	inline CategorySetting* GetSettingsUIStyle() const
	{
		return GetSetting<CategorySetting>("settings UI style");
	}

	inline EnumeratorSetting* GetSettingsUIStyleTheme() const
	{
		return GetSetting<EnumeratorSetting>("preset");
	}

	inline FloatSetting* GetSettingsUIStyleHue() const
	{
		return GetSetting<FloatSetting>("hue");
	}

	inline FloatSetting* GetSettingsUIStyleRounding() const
	{
		return GetSettingsUIStyle()->GetSetting<FloatSetting>("rounding");
	}

	inline BoolSetting* GetSettingsUIStyleBorder() const
	{
		return GetSettingsUIStyle()->GetSetting<BoolSetting>("border");
	}

	inline BoolSetting* GetSettingsUIStyleCenteredTitle() const
	{
		return GetSettingsUIStyle()->GetSetting<BoolSetting>("center title");
	}

	inline FloatSetting* GetSettingsUIStyleInputWidth() const
	{
		return GetSettingsUIStyle()->GetSetting<FloatSetting>("input width");
	}

	inline CategorySetting* GetSearch() const
	{
		return GetSetting<CategorySetting>("search");
	}

	inline BoolSetting* GetSearchHighlightMatches() const
	{
		return GetSearch()->GetSetting<BoolSetting>("highlight matches");
	}

	inline BoolSetting* GetSearchHideNonMatches() const
	{
		return GetSearch()->GetSetting<BoolSetting>("hide non matches");
	}

	inline BoolSetting* GetSearchMatchTooltips() const
	{
		return GetSearch()->GetSetting<BoolSetting>("match tooltips");
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