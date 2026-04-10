#ifndef SETTING_H
#define SETTING_H

#include <string>
#include <fstream>

#include <json.hpp>

#include "BoolVector.hpp"
#include "Keys.hpp"
#include "ModuleManager.hpp"

#define MAKE_COPY_PASTE(T) \
inline std::unique_ptr<Setting> Copy() const override \
{ \
	return std::unique_ptr<Setting>(new T(*this)); \
} \
inline bool PasteCompatible(Setting* setting) const override \
{ \
	return dynamic_cast<T*>(setting); \
} \
inline void Paste(Setting* setting, Cycle& cycle) override \
{ \
	if (T* s = dynamic_cast<T*>(setting)) \
	{ \
		value = s->value; \
		AddChange(cycle); \
	} \
}

enum SettingType : int
{
	stInt = 0,
	stFloat = 1,
	stBool = 2,
	stToggle = 3,
	stHotkey = 4,
	stVector = 5,
	stString = 6,
	stRoundingMultiplier = 7,
	stBoolList = 8,
	stStringList = 9,
	stEnumerator = 10,
	stColor = 11,
	stColorTransition = 12,
	stInputBox = 13,
	stCategory = 14
};

constexpr bool LowerContains(std::string_view haystack, std::string_view pattern)
{
	constexpr auto toLower = [](char c) { return std::tolower(static_cast<unsigned char>(c)); };

	return 
		pattern.empty() || 
		std::ranges::search(haystack, pattern, {}, toLower, toLower).begin() != haystack.end();
}

constexpr float divideRange = 1000.0f;

inline std::array<char, MAX_PATH> stringBuffer{};

class Setting;

std::unique_ptr<Setting> JsonToSetting(const nlohmann::json&);

class Setting : public Identifyable
{
protected:
	std::string name;
	bool matchesSearch = true;
	std::string tooltip;

	Setting() = default;

public:
	inline Setting(const nlohmann::json& json) :
		Identifyable{ json["ID"] },
		name{ json["Name"] },
		tooltip{ json["Tooltip"] } {}

	virtual ~Setting() = default;

	inline const std::string& GetName() const
	{
		return name;
	}

	inline bool GetMatchesSearch() const
	{
		return matchesSearch;
	}

	virtual inline void UpdateSearch(std::string_view pattern)
	{
		matchesSearch = LowerContains(name, pattern);
	}

	virtual void RenderImGui(Cycle&) = 0;

	virtual void AdditionalContextMenuItems(Cycle&) {}

protected:
	inline void RenderLabel(Cycle& cycle)
	{
		std::array<char, 16> id{};
		_itoa_s(this->id, id.data(), id.size(), 10);
		ImGui::PushID(id.data());
		ImGui::SameLine();
		ImGui::PopID();
		ImGui::Text(name.c_str());
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip(tooltip.c_str());
		RenderContextMenu(id.data(), cycle);
	}

	inline void RenderContextMenu(const char* id, Cycle& cycle)
	{
		if (ImGui::BeginPopupContextItem(id))
		{
			if (ImGui::MenuItem("restore default"))
			{
				RestoreDefault(cycle);
				ImGui::CloseCurrentPopup();
			}

			cycle.modules.ContextMenu(*this, cycle);

			AdditionalContextMenuItems(cycle);
			ImGui::EndPopup();
		}
	}

public:
	virtual void RestoreDefault(Cycle&) = 0;

	virtual SettingType GetType() const = 0;

	virtual std::unique_ptr<Setting> Copy() const = 0;
	virtual bool PasteCompatible(Setting*) const = 0;
	virtual void Paste(Setting*, Cycle&) = 0;
	virtual void Visit(const std::function<void(Identifyable&)>&) = 0;
};

template <typename T> requires
	(std::is_convertible_v<T, nlohmann::json> &&
	std::is_convertible_v<nlohmann::json, T>)
class SettingOfType : public Setting
{
	// not sure why but it crashes if we don't differentiate
	constexpr static bool constructFromGet = requires (nlohmann::json j) { j.get<T>(); };

protected:
	T defaultValue;
	T value;

public:
	inline SettingOfType(const nlohmann::json& json) requires constructFromGet :
		Setting{ json },
		defaultValue{ json["Default"].get<T>() },
		value{ json["Value"].get<T>() } {}

	inline SettingOfType(const nlohmann::json& json) requires (!constructFromGet) :
		Setting{ json },
		defaultValue{ json["Default"] },
		value{ json["Value"] } {}

	inline const T& GetDefault() const
	{
		return defaultValue;
	}

	inline T& GetDefault()
	{
		return defaultValue;
	}

	inline void SetDefault(const T& value)
	{
		defaultValue = value;
	}

	inline const T& GetValue() const
	{
		return value;
	}

	inline T& GetValue()
	{
		return value;
	}

	inline void SetValue(const T& value)
	{
		this->value = value;
	}

	inline void AddChange(Cycle& cycle) const
	{
		nlohmann::json json;
		json["ID"] = id;
		json["Value"] = value;
		cycle.changes.push_back(std::move(json));
	}

	inline virtual void RestoreDefault(Cycle& cycle) override
	{
		value = defaultValue;
		AddChange(cycle);
	}

	inline virtual void UpdateFromJson(const nlohmann::json& json) override
	{
		if constexpr (constructFromGet)
			value = json["Value"].get<T>();
		else
			value = json["Value"];
	}

	inline virtual void Visit(const std::function<void(Identifyable&)>& f) override
	{
		f(*this);
	}
};

class IntSetting : public SettingOfType<int>
{
	int min;
	int max;

public:
	inline IntSetting(const nlohmann::json& json) :
		SettingOfType{ json },
		min{ json["Min"] },
		max{ json["Max"] } {}

	inline void RenderImGui(Cycle& cycle) override
	{
		ImGui::SetNextItemWidth(cycle.inputWidth);
		ImGui::PushID(cycle.NextID());
		if (ImGui::DragInt("", &value, (max - min) / divideRange, min, max, "%d", ImGuiSliderFlags_None))
			AddChange(cycle);
		ImGui::PopID();
		RenderLabel(cycle);
	}

	inline SettingType GetType() const override
	{
		return stInt;
	}

	MAKE_COPY_PASTE(IntSetting)
};

class FloatSetting : public SettingOfType<float>
{
	float min;
	float max;
	float stepSize;

public:
	inline FloatSetting(const nlohmann::json& json) :
		SettingOfType{ json },
		min{ json["Min"] },
		max{ json["Max"] } 
	{
		stepSize = (max - min) / divideRange;
	}

	inline void RenderImGui(Cycle& cycle) override
	{
		ImGui::SetNextItemWidth(cycle.inputWidth);
		ImGui::PushID(cycle.NextID());
		if (ImGui::DragFloat("", &value, stepSize, min, max, "%.2f"))
			AddChange(cycle);
		ImGui::PopID();
		RenderLabel(cycle);
	}

	inline SettingType GetType() const override
	{
		return stFloat;
	}

	MAKE_COPY_PASTE(FloatSetting)
};

class BoolSetting : public SettingOfType<bool>
{
public:
	inline BoolSetting(const nlohmann::json& json) :
		SettingOfType{ json } {}

	inline void RenderImGui(Cycle& cycle) override
	{
		ImGui::PushID(cycle.NextID());
		if (ImGui::Checkbox("", &value))
			AddChange(cycle);
		ImGui::PopID();
		RenderLabel(cycle);
	}

	inline SettingType GetType() const override
	{
		return stBool;
	}

	MAKE_COPY_PASTE(BoolSetting)
};

struct Toggle
{
	bool state;
	uint16_t hotkey;

	inline Toggle(bool state, uint16_t hotkey) :
		state{ state },
		hotkey{ hotkey } {}

	inline Toggle(const nlohmann::json& json) :
		state{ json["State"] },
		hotkey{ json["Hotkey"] } {}

	inline operator nlohmann::json() const
	{
		nlohmann::json json;
		json["State"] = state;
		json["Hotkey"] = hotkey;
		return json;
	}
};

inline bool HotkeyContextMenuItems(uint16_t& hotkey)
{
	bool canAddShift = !(hotkey & (modifierShift | modifierGamePad)) && hotkey != unmappedKey;
	if (canAddShift && ImGui::MenuItem("SHIFT+ modifier"))
	{
		hotkey |= modifierShift;
		hotkey &= ~modifierAny;
		ImGui::CloseCurrentPopup();
		return true;
	}
	bool canAddCtrl = !(hotkey & (modifierCtrl | modifierGamePad)) && hotkey != unmappedKey;
	if (canAddCtrl && ImGui::MenuItem("CTRL+ modifier"))
	{
		hotkey |= modifierCtrl;
		hotkey &= ~modifierAny;
		ImGui::CloseCurrentPopup();
		return true;
	}
	bool canAddAny = !(hotkey & modifierAny) && hotkey != unmappedKey && !(hotkey & modifierGamePad);
	if (canAddAny && ImGui::MenuItem("ANY+ modifier"))
	{
		hotkey |= modifierAny;
		hotkey &= ~(modifierShift | modifierCtrl);
		ImGui::CloseCurrentPopup();
		return true;
	}

	if (canAddShift || canAddCtrl || canAddAny)
		ImGui::Separator();

	if (ImGui::MenuItem("LBUTTON"))
	{
		hotkey = VK_LBUTTON;
		ImGui::CloseCurrentPopup();
		return true;
	}
	if (ImGui::MenuItem("RBUTTON"))
	{
		hotkey = VK_RBUTTON;
		ImGui::CloseCurrentPopup();
		return true;
	}
	if (ImGui::MenuItem("MBUTTON"))
	{
		hotkey = VK_MBUTTON;
		ImGui::CloseCurrentPopup();
		return true;
	}
	if (ImGui::MenuItem("XBUTTON1"))
	{
		hotkey = VK_XBUTTON1;
		ImGui::CloseCurrentPopup();
		return true;
	}
	if (ImGui::MenuItem("XBUTTON2"))
	{
		hotkey = VK_XBUTTON2;
		ImGui::CloseCurrentPopup();
		return true;
	}
	return false;
}

class ToggleSetting : public SettingOfType<Toggle>
{
private:
	uint16_t previousHotkey = -1;
	std::array<char, 100> keyName{};

public:
	inline ToggleSetting(const nlohmann::json& json) :
		SettingOfType{ json } {}

	inline void RenderImGui(Cycle& cycle) override
	{
		if (value.hotkey != previousHotkey)
		{
			strcpy_s(keyName.data(), keyName.size(), keyCodeToString(value.hotkey).c_str());
			previousHotkey = value.hotkey;
		}

		ImGui::SetNextItemWidth(cycle.inputWidth);
		ImGui::PushID(cycle.NextID());
		ImGui::InputText("", keyName.data(), keyName.size(), ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoUndoRedo);
		ImGui::PopID();
		if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
		{
			value.hotkey = unmappedKey;
			AddChange(cycle);
		}
		if (ImGui::IsItemActive())
		{
			uint16_t k = getPressedKey(cycle.enableUiHotkey);
			if (k != 0)
			{
				value.hotkey = k;
				AddChange(cycle);
			}
		}

		ImGui::SameLine();
		ImGui::PushID(cycle.NextID());
		if (ImGui::Checkbox("", &value.state))
			AddChange(cycle);
		ImGui::PopID();
		RenderLabel(cycle);
	}

	inline void AdditionalContextMenuItems(Cycle& cycle) override
	{
		ImGui::Separator();
		if (HotkeyContextMenuItems(value.hotkey))
			AddChange(cycle);
	}

	inline SettingType GetType() const override
	{
		return stToggle;
	}

	MAKE_COPY_PASTE(ToggleSetting)
};

class HotkeySetting : public SettingOfType<uint16_t>
{
	uint16_t previousValue = -1;
	std::array<char, 100> keyName{};

public:
	inline HotkeySetting(const nlohmann::json& json) :
		SettingOfType{ json } {}

	inline void RenderImGui(Cycle& cycle) override
	{
		if (value != previousValue)
		{
			strcpy_s(keyName.data(), keyName.size(), keyCodeToString(value).c_str());
			previousValue = value;
		}

		ImGui::SetNextItemWidth(cycle.inputWidth);
		ImGui::PushID(cycle.NextID());
		ImGui::InputText("", keyName.data(), keyName.size(), ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoUndoRedo);
		ImGui::PopID();
		if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
		{
			value = unmappedKey;
			AddChange(cycle);
		}
		if (ImGui::IsItemActive())
		{
			uint16_t k = getPressedKey(cycle.enableUiHotkey);
			if (k != 0)
			{
				value = k;
				AddChange(cycle);
			}
		}
		RenderLabel(cycle);
	}

	inline void AdditionalContextMenuItems(Cycle& cycle) override
	{
		ImGui::Separator();
		if (HotkeyContextMenuItems(value))
			AddChange(cycle);
	}

	inline SettingType GetType() const override
	{
		return stHotkey;
	}

	MAKE_COPY_PASTE(HotkeySetting)
};

struct Vector
{
	float x;
	float y;

	inline Vector() = default;

	inline Vector(float x, float y) :
		x{ x }, y{ y } {}

	inline Vector(const nlohmann::json& json) :
		x{ json["X"] },
		y{ json["Y"] } {}

	inline operator nlohmann::json() const
	{
		nlohmann::json json;
		json["X"] = x;
		json["Y"] = y;
		return json;
	}
};

class VectorSetting : public SettingOfType<Vector>
{
	Vector min;
	Vector max;
	Vector stepSize;

public:
	inline VectorSetting(const nlohmann::json& json) :
		SettingOfType{ json },
		min{ json["Min"] },
		max{ json["Max"] }
	{
		stepSize.x = (max.x - min.x) / divideRange;
		stepSize.y = (max.y - min.y) / divideRange;
	}

	inline void RenderImGui(Cycle& cycle) override
	{
		ImGui::SetNextItemWidth(cycle.inputWidth);
		ImGui::PushID(cycle.NextID());
		if (ImGui::DragFloat("##value_x", &value.x, stepSize.x, min.x, max.x, "%.2f"))
			AddChange(cycle);
		ImGui::PopID();
		ImGui::SetNextItemWidth(cycle.inputWidth);
		ImGui::PushID(cycle.NextID());
		ImGui::SameLine();
		if (ImGui::DragFloat("##value_y", &value.y, stepSize.y, min.y, max.y, "%.2f"))
			AddChange(cycle);
		ImGui::PopID();
		RenderLabel(cycle);
	}

	inline SettingType GetType() const override
	{
		return stVector;
	}

	MAKE_COPY_PASTE(VectorSetting)
};

class StringSetting : public SettingOfType<std::string>
{
public:
	inline StringSetting(const nlohmann::json& json) :
		SettingOfType{ json } {}

	inline void RenderImGui(Cycle& cycle) override
	{
		ImGui::SetNextItemWidth(cycle.inputWidth);
		ImGui::PushID(cycle.NextID());
		strcpy_s(stringBuffer.data(), stringBuffer.size(), value.c_str());
		if (ImGui::InputText("", stringBuffer.data(), stringBuffer.size()))
		{
			value = stringBuffer.data();
			AddChange(cycle);
		}
		ImGui::PopID();
		RenderLabel(cycle);
	}

	inline SettingType GetType() const override
	{
		return stString;
	}

	MAKE_COPY_PASTE(StringSetting)
};

class RoundingMultiplierSetting : public SettingOfType<std::string>
{
	inline static bool IsValid(std::string_view value)
	{
		bool decimalFound = false;
		for (char c : value)
		{
			if (c == '.')
			{
				if (decimalFound)
					return false;
				decimalFound = true;
			}
			else if (c < '0' || c > '9')
				return false;
		}
		return true;
	}

public:
	inline RoundingMultiplierSetting(const nlohmann::json& json) :
		SettingOfType{ json } {}

	inline void RenderImGui(Cycle& cycle) override
	{
		ImGui::SetNextItemWidth(cycle.inputWidth);
		ImGui::PushID(cycle.NextID());
		strcpy_s(stringBuffer.data(), stringBuffer.size(), value.c_str());
		if (ImGui::InputText("", stringBuffer.data(), stringBuffer.size()))
		{
			if (IsValid(stringBuffer.data()))
			{
				value = stringBuffer.data();
				AddChange(cycle);
			}
		}
		ImGui::PopID();
		RenderLabel(cycle);
	}

	inline SettingType GetType() const override
	{
		return stRoundingMultiplier;
	}

	MAKE_COPY_PASTE(RoundingMultiplierSetting)
};

// std::vector<bool> is annoying
class BoolListSetting : public SettingOfType<bool_vector>
{
	std::vector<std::string> identifiers;
	bool_vector identifiersMatchesSearch;
	bool collapsing = false;

public:
	inline BoolListSetting(const nlohmann::json& json) :
		SettingOfType{ json },
		identifiers{ json["Identifiers"].get<std::vector<std::string>>()},
		identifiersMatchesSearch(value.size(), true),
		collapsing{ json["Collapsing"] } {}

	inline void RenderImGui(Cycle& cycle) override
	{
		if (!collapsing)
		{
			if (ImGui::BeginMenu(name.c_str(), true))
			{
				RenderContextMenu(nullptr, cycle);

				size_t i = 0;
				for (bool& v : value)
				{
					ImGui::PushID(cycle.NextID());
					if (!identifiersMatchesSearch[i] && matchesSearch) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
					if (ImGui::Checkbox(identifiers[i].c_str(), &v))
						AddChange(cycle);
					if (!identifiersMatchesSearch[i] && matchesSearch) ImGui::PopStyleVar();
					ImGui::PopID();

					i++;
				}
				ImGui::EndMenu();
			}
		}
		else
		{
			if (ImGui::CollapsingHeader(name.c_str()))
			{
				RenderContextMenu(nullptr, cycle);

				if (ImGui::Button("show all"))
				{
					std::ranges::fill(value, true);
					AddChange(cycle);
				}
				ImGui::SameLine();
				if (ImGui::Button("hide all"))
				{
					std::ranges::fill(value, false);
					AddChange(cycle);
				}
				size_t i = 0;
				for (bool& v : value)
				{
					ImGui::PushID(cycle.NextID());
					if (!identifiersMatchesSearch[i] && matchesSearch) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
					if (ImGui::MenuItem(identifiers[i].c_str(), nullptr, v))
					{
						v = !v;
						AddChange(cycle);
					}
					if (!identifiersMatchesSearch[i] && matchesSearch) ImGui::PopStyleVar();
					ImGui::PopID();

					i++;
				}
			}
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip(tooltip.c_str());
	}

	inline SettingType GetType() const override
	{
		return stBoolList;
	}

	virtual inline void UpdateSearch(std::string_view pattern) override
	{
		auto containsPattern = [&](std::string_view h) { return LowerContains(h, pattern); };
		std::ranges::transform(identifiers, identifiersMatchesSearch.begin(), containsPattern);
		matchesSearch = containsPattern(name) || std::ranges::any_of(identifiersMatchesSearch, std::identity{});
	}

	MAKE_COPY_PASTE(BoolListSetting)
};

class StringListSetting : public SettingOfType<std::vector<std::string>>
{
	std::array<char, MAX_PATH> buffer{};

public:
	inline StringListSetting(const nlohmann::json& json) :
		SettingOfType{ json } {}

	inline void RenderImGui(Cycle& cycle) override
	{
		ImGui::SetNextItemWidth(cycle.inputWidth);
		ImGui::PushID(cycle.NextID());
		if (ImGui::BeginCombo("", "..."))
		{
			ImGui::PushID(cycle.NextID());
			ImGui::InputText("", buffer.data(), buffer.size());
			ImGui::PopID();
			ImGui::PushID(cycle.NextID());
			if (ImGui::Button("+", ImVec2{ -1, 0 }))
			{
				std::string_view entered = buffer.data();

				if (!entered.empty() && std::ranges::find(value, entered) == value.end())
				{
					value.emplace_back(entered);
					AddChange(cycle);
				}
			}
			ImGui::PopID();
			auto remove = value.end();
			for (auto it = value.begin(); it != value.end(); ++it)
			{
				ImGui::PushID(cycle.NextID());
				if (ImGui::Button((std::string("- ") + *it).c_str()))
					remove = it;
				ImGui::PopID();
			}
			if (remove != value.end())
			{
				value.erase(remove);
				AddChange(cycle);
			}
			ImGui::EndCombo();
		}
		ImGui::PopID();
		RenderLabel(cycle);
	}

	inline SettingType GetType() const override
	{
		return stStringList;
	}

	MAKE_COPY_PASTE(StringListSetting)
};

class EnumeratorSetting : public SettingOfType<int>
{
	std::vector<std::string> identifiers;

public:
	inline EnumeratorSetting(const nlohmann::json& json) :
		SettingOfType{ json },
		identifiers{ json["Identifiers"].get<std::vector<std::string>>() } {}

	inline void RenderImGui(Cycle& cycle) override
	{
		ImGui::SetNextItemWidth(cycle.inputWidth);
		ImGui::PushID(cycle.NextID());
		if (ImGui::BeginCombo("", identifiers[value].c_str()))
		{
			size_t i = 0;
			for (const auto& a : identifiers)
			{
				ImGui::PushID(cycle.NextID());
				if (ImGui::RadioButton(a.c_str(), &value, i++))
					AddChange(cycle);
				ImGui::PopID();
			}
			ImGui::EndCombo();
		}
		ImGui::PopID();
		RenderLabel(cycle);
	}

	inline SettingType GetType() const override
	{
		return stEnumerator;
	}

	MAKE_COPY_PASTE(EnumeratorSetting)
};

struct Color
{
	std::array<float, 4> arr;

	inline Color() = default;

	inline Color(float r, float g, float b, float a) :
		arr{ r, g, b, a } {}

	inline Color(const nlohmann::json& json) :
		arr{ json["R"] / 255.0f, json["G"] / 255.0f, json["B"] / 255.0f, json["A"] / 255.0f } {}

	inline operator nlohmann::json() const
	{
		nlohmann::json json;
		json["R"] = static_cast<int>(arr[0] * 255.5f);
		json["G"] = static_cast<int>(arr[1] * 255.5f);
		json["B"] = static_cast<int>(arr[2] * 255.5f);
		json["A"] = static_cast<int>(arr[3] * 255.5f);
		return json;
	}

	inline auto AsInt() const
	{
		return IM_COL32(255 * arr[0], 255 * arr[1], 255 * arr[2], 255);
	}
};

class ColorSetting : public SettingOfType<Color>
{
public:
	inline ColorSetting(const nlohmann::json& json) :
		SettingOfType{ json } {}

	inline void RenderImGui(Cycle& cycle) override
	{
		ImGui::PushID(cycle.NextID());
		ImGui::SetNextItemWidth(9.0f / 5.0f * cycle.inputWidth);
		if (ImGui::ColorEdit3("", value.arr.data(), ImGuiColorEditFlags_NoSmallPreview))
			AddChange(cycle);
		ImGui::PopID();
		ImGui::SameLine();
		ImGui::PushID(cycle.NextID());
		if (ImGui::ColorEdit4("", value.arr.data(), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar))
			AddChange(cycle);
		ImGui::PopID();
		RenderLabel(cycle);
	}

	inline SettingType GetType() const override
	{
		return stColor;
	}

	MAKE_COPY_PASTE(ColorSetting)
};

struct ColorTransition
{
	int period;
	int offset;
	bool discrete;
	bool advanced;
	std::vector<Color> colors;

	inline ColorTransition(const nlohmann::json& json) :
		period{ json["Period"] },
		offset{ json["Offset"] },
		discrete{ json["Discrete"] },
		advanced{ json["Advanced"] },
		colors{ json["Colors"].begin(), json["Colors"].end() } {}

	inline operator nlohmann::json() const
	{
		nlohmann::json json;
		json["Period"] = period;
		json["Offset"] = offset;
		json["Discrete"] = discrete;
		json["Advanced"] = advanced;
		json["Colors"] = colors;
		return json;
	}
};

class ColorTransitionSetting : public SettingOfType<ColorTransition>
{
	inline void DrawGradient(float x1, float x2, float y1, float y2, Color Lcolor, Color Rcolor)
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilledMultiColor(ImVec2{ x1, y1 }, ImVec2{ x2, y2 },
			Lcolor.AsInt(), Rcolor.AsInt(), Rcolor.AsInt(), Lcolor.AsInt());
	}

	inline void DrawRect(float x1, float x2, float y1, float y2, Color color)
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(ImVec2{ x1, y1 }, ImVec2{ x2, y2 }, color.AsInt());
	}

	inline void DrawFullGradient(float width, float height)
	{
		ImVec2 p = ImGui::GetCursorScreenPos();
		if (value.colors.size() == 1)
		{
			DrawGradient(p.x, p.x + width, p.y, p.y + height, value.colors.front(), value.colors.front());
			return;
		}
		float stepSize = width / (value.colors.size() - 1);
		float currX = 0.0f;
		float nextX = stepSize;
		Color prevColor;
		bool first = true;
		for (auto& currColor : value.colors)
		{
			if (first)
			{
				first = false;
				prevColor = currColor;
				continue;
			}
			DrawGradient(p.x + currX, p.x + nextX, p.y, p.y + height, prevColor, currColor);
			currX = nextX;
			nextX += stepSize;
			prevColor = currColor;
		}
	}

	inline void DrawFullDiscrete(float width, float height)
	{
		ImVec2 p = ImGui::GetCursorScreenPos();
		if (value.colors.size() == 1)
		{
			DrawRect(p.x, p.x + width, p.y, p.y + height, value.colors.front());
			return;
		}
		float stepSize = width / value.colors.size();
		float xOff = 0.0f;
		for (auto& c : value.colors)
		{
			DrawRect(p.x + xOff, p.x + xOff + stepSize, p.y, p.y + height, c);
			xOff += stepSize;
		}
	}

public:
	inline ColorTransitionSetting(const nlohmann::json& json) :
		SettingOfType{ json }
	{}

	inline void RenderImGui(Cycle& cycle) override
	{
		//enableAdvanced - RGB - Color - Label
		ImGui::PushID(cycle.NextID());
		if (ImGui::Checkbox("", &value.advanced))
			AddChange(cycle);
		ImGui::PopID();
		ImGui::SetItemTooltip("enable advanced color edit");
		ImGui::SameLine();
		if (!value.advanced)
		{
			ImGui::PushID(cycle.NextID());
			ImGui::SetNextItemWidth(9.0f / 5.0f * cycle.inputWidth);
			if (ImGui::ColorEdit3("", value.colors.front().arr.data(), ImGuiColorEditFlags_NoSmallPreview))
				AddChange(cycle);
			ImGui::PopID();
			ImGui::SameLine();
			ImGui::PushID(cycle.NextID());
			if (ImGui::ColorEdit4("", value.colors.front().arr.data(), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar))
				AddChange(cycle);
			ImGui::PopID();
		}
		else //advanced mode :(
		{
			float lineHeight = ImGui::GetTextLineHeight();
			float padding = ImGui::GetStyle().FramePadding.y;
			float height = lineHeight + 2 * padding;

			if (value.discrete)
				DrawFullDiscrete(cycle.inputWidth, height);
			else
				DrawFullGradient(cycle.inputWidth, height);
			ImGui::PushID(cycle.NextID());
			ImGui::PushItemWidth(cycle.inputWidth);
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });
			if (ImGui::BeginCombo("", "", ImGuiComboFlags_NoArrowButton))
			{
				ImGui::PopStyleColor(2);
				ImGui::SetNextItemWidth(cycle.inputWidth);
				ImGui::PushID(cycle.NextID());
				if (ImGui::Checkbox("discrete", &value.discrete))
					AddChange(cycle);
				ImGui::PopID();
				ImGui::SetNextItemWidth(cycle.inputWidth);
				ImGui::PushID(cycle.NextID());
				if (ImGui::DragInt("period", &value.period, 5000 / divideRange, 0, 5000))
					AddChange(cycle);
				ImGui::PopID();
				ImGui::SetNextItemWidth(cycle.inputWidth);
				ImGui::PushID(cycle.NextID());
				if (ImGui::DragInt("offset", &value.offset, 5000 / divideRange, 0, 5000))
					AddChange(cycle);
				ImGui::PopID();
				ImGui::PushID(cycle.NextID());
				if (ImGui::Button("add color"))
				{
					value.colors.push_back(Color{ 1.0, 1.0, 1.0, 1.0 });
					AddChange(cycle);
				}
				ImGui::PopID();

				auto remove = value.colors.end();
				for (auto it = value.colors.begin(); it != value.colors.end(); ++it)
				{
					ImGui::PushID(cycle.NextID());
					if (ImGui::ColorEdit4("", it->arr.data(), ImGuiColorEditFlags_NoInputs))
						AddChange(cycle);
					if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && value.colors.size() > 1)
						remove = it;
					ImGui::PopID();
					ImGui::SameLine();
				}
				if (remove != value.colors.end())
				{
					value.colors.erase(remove);
					AddChange(cycle);
				}

				ImGui::EndCombo();
			}
			else
				ImGui::PopStyleColor(2);
			ImGui::PopID();
		}
		RenderLabel(cycle);
	}

	inline SettingType GetType() const override
	{
		return stColorTransition;
	}

	MAKE_COPY_PASTE(ColorTransitionSetting)
};

struct InputBox
{
	std::string label;
	std::array<int, 2> position;
	std::array<int, 2> size;

	inline InputBox(const nlohmann::json& json) :
		label{ json["Label"] },
		position{ json["X"], json["Y"] },
		size{ json["W"], json["H"] } {}

	inline operator nlohmann::json() const
	{
		nlohmann::json json;
		json["Label"] = label;
		json["X"] = position[0];
		json["Y"] = position[1];
		json["W"] = size[0];
		json["H"] = size[1];
		return json;
	}
};

class InputBoxSetting : public SettingOfType<InputBox>
{	
public:
	inline InputBoxSetting(const nlohmann::json& json) :
		SettingOfType{ json } {}

	inline void RenderImGui(Cycle& cycle) override
	{
		ImGui::NewLine();
		RenderLabel(cycle);
		ImGui::SetNextItemWidth(cycle.inputWidth);
		ImGui::PushID(cycle.NextID());
		strcpy_s(stringBuffer.data(), stringBuffer.size(), value.label.c_str());
		if (ImGui::InputText("label", stringBuffer.data(), stringBuffer.size()))
		{
			value.label = stringBuffer.data();
			AddChange(cycle);
		}
		ImGui::PopID();

		ImGui::SetNextItemWidth(cycle.inputWidth);
		ImGui::PushID(cycle.NextID());
		if (ImGui::DragInt2("position", value.position.data()))
			AddChange(cycle);
		ImGui::SetNextItemWidth(cycle.inputWidth);
		if (ImGui::DragInt2("size", value.size.data(), 1, 0, 1 << (31 - 1)))
			AddChange(cycle);
		ImGui::PopID();
	}

	inline SettingType GetType() const override
	{
		return stInputBox;
	}

	MAKE_COPY_PASTE(InputBoxSetting)
};

class CategorySetting : public Setting
{
private:
	std::vector<std::unique_ptr<Setting>> subSettings;
	bool enabled = false;

public:
	inline CategorySetting(const nlohmann::json& json) :
		Setting{ json }
	{
		for (const auto& a : json["Value"])
			subSettings.push_back(JsonToSetting(a));
	}

	inline CategorySetting(const CategorySetting& right) :
		Setting{ right },
		enabled{ right.enabled }
	{
		for (const auto& s : right.subSettings)
			subSettings.push_back(s->Copy());
	}

private:
#pragma warning(push)
#pragma warning(disable : 4068)
#pragma diag_suppress 27
#pragma warning(pop)
	inline void RenderReset(Cycle& cycle)
	{
		bool hasScroll = ImGui::GetCurrentWindow()->ScrollbarY;
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 1, 1, 1, 0.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 1, 1, 1, 0.0f });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 1, 1, 1, 0.0f });
		if (hasScroll)
			ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::GetTextLineHeight() - 2 * ImGui::CalcTextSize((const char*)u8"\xe800").x); //2x for possible scroll bar
		else
			ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::GetTextLineHeight() - ImGui::CalcTextSize((const char*)u8"\xe800").x); //2x for possible scroll bar
		float temp = ImGui::GetTextLineHeight();
		ImGui::SetWindowFontScale(0.75f);
		ImGui::PushID(cycle.NextID());
		if (ImGui::Button((const char*)u8"\xe800", ImVec2(temp, temp)))
			RestoreDefault(cycle);
		ImGui::PopID();
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("restore default");
		}
		ImGui::SetWindowFontScale(1.0f);
		ImGui::PopStyleColor(3);
	}

public:
	inline void RenderImGui(Cycle& cycle) override
	{
		ImGui::SetWindowFontScale(1.0f);

		if (ImGui::TreeNode(name.c_str()))
		{
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip(tooltip.c_str());
			RenderContextMenu(nullptr, cycle);
			RenderReset(cycle);
			for (const auto& s : subSettings)
			{
				if (!s->GetMatchesSearch() && matchesSearch)
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
				s->RenderImGui(cycle);
				if (!s->GetMatchesSearch() && matchesSearch)
					ImGui::PopStyleVar();
			}
			ImGui::TreePop();
		}
		else
		{
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip(tooltip.c_str());
			RenderContextMenu(nullptr, cycle);
			RenderReset(cycle);
		}
	}

	inline void RestoreDefault(Cycle& cycle) override
	{
		for (const auto& s : subSettings)
			s->RestoreDefault(cycle);
	}

	void UpdateFromJson(const nlohmann::json& json) override
	{
		
	}

	virtual inline void UpdateSearch(std::string_view pattern) override
	{
		matchesSearch = LowerContains(name, pattern);
		for (const auto& s : subSettings)
		{
			s->UpdateSearch(pattern);
			matchesSearch |= s->GetMatchesSearch();
		}
	}

	inline SettingType GetType() const override
	{
		return stCategory;
	}

	inline std::unique_ptr<Setting> Copy() const override
	{
		return std::unique_ptr<Setting>{ new CategorySetting(*this) };
	}

	inline bool PasteCompatible(Setting* setting) const override
	{
		if (CategorySetting* c = dynamic_cast<CategorySetting*>(setting))
		{
			if (subSettings.size() != c->subSettings.size())
				return false;
			for (auto it1 = subSettings.cbegin(), it2 = c->subSettings.cbegin(); it1 != subSettings.end(); ++it1, ++it2)
			{
				if ((*it1)->GetType() != (*it2)->GetType())
					return false;
				if ((*it1)->GetType() == stCategory)
					return dynamic_cast<CategorySetting*>(it1->get())->PasteCompatible(dynamic_cast<CategorySetting*>(it2->get()));
			}
			return true;
		}
		return false;
	}

	inline void Paste(Setting* setting, Cycle& cycle) override
	{
		if (CategorySetting* s = dynamic_cast<CategorySetting*>(setting))
		{
			if (PasteCompatible(s))
			{
				for (size_t i = 0; i < subSettings.size(); i++)
				{
					subSettings[i]->Paste(s->subSettings[i].get(), cycle);
				}
			}
		}
	}

	inline void Visit(const std::function<void(Identifyable&)>& f) override
	{
		f(*this);
		for (const auto& s : subSettings)
			s->Visit(f);
	}
};

inline std::unique_ptr<Setting> JsonToSetting(const nlohmann::json& json)
{
	return std::unique_ptr<Setting>{ [&]() -> Setting*
		{
			switch (static_cast<int>(json["Type"]))
			{
			case stInt: return new IntSetting{ json };
			case stFloat: return new FloatSetting{ json };
			case stBool: return new BoolSetting{ json };
			case stToggle: return new ToggleSetting{ json };
			case stHotkey: return new HotkeySetting{ json };
			case stVector: return new VectorSetting{ json };
			case stString: return new StringSetting{ json };
			case stRoundingMultiplier: return new RoundingMultiplierSetting{ json };
			case stBoolList: return new BoolListSetting{ json };
			case stStringList: return new StringListSetting{ json };
			case stEnumerator: return new EnumeratorSetting{ json };
			case stColor: return new ColorSetting{ json };
			case stColorTransition: return new ColorTransitionSetting{ json };
			case stInputBox: return new InputBoxSetting{ json };
			case stCategory: return new CategorySetting{ json };
			default: return nullptr;
			}
		}() };
}

#endif