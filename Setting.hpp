#ifndef SETTING_H
#define SETTING_H

#include <string>

#include <json.hpp>

#include "BoolVector.hpp"
#include "ID.hpp"
#include "Keys.hpp"
#include "Packing.hpp"
#include "Global.hpp"

#define MAKE_COPY_PASTE(T) \
inline std::unique_ptr<Setting> Copy() const override \
{ \
	return std::unique_ptr<Setting>(new T(*this)); \
} \
inline void Paste(Setting* setting) override \
{ \
	SetChanged(); \
	if (T* s = dynamic_cast<T*>(setting)) \
	{ \
		value = s->value; \
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
	constexpr auto toLower = [](char c) { return std::tolower(c); };

	return 
		pattern.empty() || 
		std::ranges::search(haystack, pattern, {}, toLower, toLower).begin() != haystack.end();
}

constexpr float divideRange = 1000.0f;

inline std::array<char, MAX_PATH> stringBuffer{};

class Setting;

std::unique_ptr<Setting> JsonToSetting(const nlohmann::json&);
	
class Setting
{
private:
	bool changed = false;
	
protected:
	bool matchesSearch = true;
	std::string name;
	int id;
	std::string tooltip;

	Setting() = default;

public:
	inline Setting(const nlohmann::json& json) :
		name{ json["Name"] },
		id{ json["ID"] },
		tooltip{ json["Tooltip"] } {}

	virtual ~Setting() = default;

	inline bool HasChanged() const
	{
		return changed;
	}

	inline void SetChanged()
	{
		changed = true;
		Global::settingChanged = true;
	}

	inline void ResetChanged()
	{
		changed = false;
	}

	inline bool GetMatchesSearch() const
	{
		return matchesSearch;
	}

	virtual inline void UpdateSearch(std::string_view pattern)
	{
		matchesSearch = LowerContains(name, pattern);
	}

	inline const std::string& GetName() const
	{
		return name;
	}

	virtual inline void Visit(const std::function<void(Setting*)>& f)
	{
		f(this);
	}

	virtual void RenderImGui() = 0;

protected:
	// right clicking setting labels allows you to copy/paste settings
	// with shift is copy and without shift is paste
	inline void CheckCopyPaste()
	{
		if (ImGui::IsItemClicked(1))
		{
			static std::unique_ptr<Setting> clipboard;
			if (ImGui::IsKeyDown(ImGuiMod_Shift))
			{
				clipboard = Copy();
			}
			else if (clipboard)
			{
				Paste(clipboard.get());
			}
		}
	}

	inline void RenderLabel()
	{
		ImGui::SameLine();
		ImGui::Text(name.c_str());
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip(tooltip.c_str());
		CheckCopyPaste();
	}

public:
	virtual void ReloadDefault() = 0;

	virtual void ChangesAsJson(nlohmann::json& json) = 0;
	virtual void UpdateFromJson(const nlohmann::json& data) = 0;

	virtual SettingType GetType() = 0;

	virtual std::unique_ptr<Setting> Copy() const = 0;

	virtual void Paste(Setting* setting) = 0;
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

	inline virtual void ReloadDefault() override
	{
		value = defaultValue;
		SetChanged();
	}

	inline virtual void ChangesAsJson(nlohmann::json& json) override
	{
		if (HasChanged())
		{
			nlohmann::json temp;
			temp["ID"] = id;
			temp["Value"] = value;
			json.push_back(temp);
		}
	}

	inline virtual void UpdateFromJson(const nlohmann::json& json) override
	{
		for (const auto& data : json["Changes"])
		{
			if (data["ID"] == id)
			{
				if constexpr (constructFromGet)
					value = data["Value"].get<T>();
				else
					value = data["Value"];
				return;
			}
		}
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

	inline void RenderImGui() override
	{
		ImGui::SetNextItemWidth(Global::inputWidth);
		ImGui::PushID(ID::nextID());
		if (ImGui::DragInt("", &value, (max - min) / divideRange, min, max, "%d", ImGuiSliderFlags_None))
			SetChanged();
		ImGui::PopID();
		RenderLabel();
	}

	inline SettingType GetType() override
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

	inline void RenderImGui() override
	{
		ImGui::SetNextItemWidth(Global::inputWidth);
		ImGui::PushID(ID::nextID());
		if (ImGui::DragFloat("", &value, stepSize, min, max, "%.2f"))
			SetChanged();
		ImGui::PopID();
		RenderLabel();
	}

	inline SettingType GetType() override
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

	inline void RenderImGui() override
	{
		ImGui::PushID(ID::nextID());
		if (ImGui::Checkbox("", &value))
			SetChanged();
		ImGui::PopID();
		RenderLabel();
	}

	inline SettingType GetType() override
	{
		return stBool;
	}

	MAKE_COPY_PASTE(BoolSetting)
};

struct Toggle
{
	bool state;
	int hotkey;

	inline Toggle(bool state, int hotkey) :
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

class ToggleSetting : public SettingOfType<Toggle>
{
private:
	int previousHotkey = -1;
	bool wasPressed = false;
	std::chrono::high_resolution_clock::time_point pressTime;
	std::array<char, 100> keyName{};

public:
	inline ToggleSetting(const nlohmann::json& json) :
		SettingOfType{ json } {}

	inline void RenderImGui() override
	{
		if (value.hotkey != previousHotkey)
		{
			strcpy_s(keyName.data(), keyName.size(), keyCodeToString(value.hotkey).c_str());
			previousHotkey = value.hotkey;
		}

		ImGui::SetNextItemWidth(Global::inputWidth);
		ImGui::PushID(ID::nextID());
		ImGui::InputText("", keyName.data(), keyName.size(), ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoUndoRedo);
		ImGui::PopID();
		bool pressed = false;
		if (ImGui::IsItemActive())
		{
			if (ImGui::IsKeyDown(ImGuiKey_MouseRight))
			{
				value.hotkey = unmappedKey;
				SetChanged();
			}
			else
			{
				int k = getPressedKey(Global::enableUiKey);
				if (k != 0)
				{
					if (!wasPressed)
						pressTime = std::chrono::high_resolution_clock::now();
					if (std::chrono::high_resolution_clock::now() - pressTime >= std::chrono::milliseconds(500))
						k |= modifierAny;
					value.hotkey = k;
					SetChanged();
					pressed = true;
				}
			}
		}

		wasPressed = pressed;
		ImGui::SameLine();
		ImGui::PushID(ID::nextID());
		if (ImGui::Checkbox("", &value.state))
			SetChanged();
		ImGui::PopID();
		RenderLabel();
	}

	inline SettingType GetType() override
	{
		return stToggle;
	}

	MAKE_COPY_PASTE(ToggleSetting)
};

class HotkeySetting : public SettingOfType<int>
{
	int previousValue = -1;
	bool wasPressed = false;
	std::chrono::high_resolution_clock::time_point pressTime;
	std::array<char, 100> keyName{};

public:
	inline HotkeySetting(const nlohmann::json& json) :
		SettingOfType{ json } {}

	inline void RenderImGui() override
	{
		if (value != previousValue)
		{
			strcpy_s(keyName.data(), keyName.size(), keyCodeToString(value).c_str());
			previousValue = value;
		}

		ImGui::SetNextItemWidth(Global::inputWidth);
		ImGui::PushID(ID::nextID());
		ImGui::InputText("", keyName.data(), keyName.size(), ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoUndoRedo);
		ImGui::PopID();
		bool pressed = false;
		if (ImGui::IsItemActive())
		{
			if (ImGui::IsKeyDown(ImGuiKey_MouseRight))
			{
				value = unmappedKey;
				SetChanged();
			}
			else
			{
				int k = getPressedKey(Global::enableUiKey);
				if (k != 0)
				{
					if (!wasPressed)
						pressTime = std::chrono::high_resolution_clock::now();
					if (std::chrono::high_resolution_clock::now() - pressTime >= std::chrono::milliseconds(500))
						k |= modifierAny;
					value = k;
					SetChanged();
					pressed = true;
				}
			}
		}
		wasPressed = pressed;
		RenderLabel();
	}

	inline SettingType GetType() override
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

	inline void RenderImGui() override
	{
		ImGui::SetNextItemWidth(Global::inputWidth);
		ImGui::PushID(ID::nextID());
		if (ImGui::DragFloat("##value_x", &value.x, stepSize.x, min.x, max.x, "%.2f"))
			SetChanged();
		ImGui::PopID();
		ImGui::SetNextItemWidth(Global::inputWidth);
		ImGui::PushID(ID::nextID());
		ImGui::SameLine();
		if (ImGui::DragFloat("##value_y", &value.y, stepSize.y, min.y, max.y, "%.2f"))
			SetChanged();
		ImGui::PopID();
		RenderLabel();
	}

	inline SettingType GetType() override
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

	inline void RenderImGui() override
	{
		ImGui::SetNextItemWidth(Global::inputWidth);
		ImGui::PushID(ID::nextID());
		strcpy_s(stringBuffer.data(), stringBuffer.size(), value.c_str());
		if (ImGui::InputText("", stringBuffer.data(), stringBuffer.size()))
		{
			value = stringBuffer.data();
			SetChanged();
		}
		ImGui::PopID();
		RenderLabel();
	}

	inline SettingType GetType() override
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

	inline void RenderImGui() override
	{
		ImGui::SetNextItemWidth(Global::inputWidth);
		ImGui::PushID(ID::nextID());
		strcpy_s(stringBuffer.data(), stringBuffer.size(), value.c_str());
		if (ImGui::InputText("", stringBuffer.data(), stringBuffer.size()))
		{
			if (IsValid(stringBuffer.data()))
			{
				value = stringBuffer.data();
				SetChanged();
			}
		}
		ImGui::PopID();
		RenderLabel();
	}

	inline SettingType GetType() override
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

	inline void RenderImGui() override
	{
		if (!collapsing)
		{
			if (ImGui::BeginMenu(name.c_str(), true))
			{
				size_t i = 0;
				for (bool& v : value)
				{
					ImGui::PushID(ID::nextID());
					if (!identifiersMatchesSearch[i] && matchesSearch) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
					if (ImGui::Checkbox(identifiers[i].c_str(), &v))
						SetChanged();
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
				if (ImGui::Button("show all"))
				{
					std::ranges::fill(value, true);
					SetChanged();
				}
				ImGui::SameLine();
				if (ImGui::Button("hide all"))
				{
					std::ranges::fill(value, false);
					SetChanged();
				}
				size_t i = 0;
				for (bool& v : value)
				{
					ImGui::PushID(ID::nextID());
					if (!identifiersMatchesSearch[i] && matchesSearch) ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
					if (ImGui::MenuItem(identifiers[i].c_str(), nullptr, v))
					{
						v = !v;
						SetChanged();
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

	inline SettingType GetType() override
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

	inline void RenderImGui() override
	{
		ImGui::SetNextItemWidth(Global::inputWidth);
		ImGui::PushID(ID::nextID());
		if (ImGui::BeginCombo("", name.c_str()))
		{
			ImGui::PushID(ID::nextID());
			ImGui::InputText("", buffer.data(), buffer.size());
			ImGui::PopID();
			ImGui::PushID(ID::nextID());
			if (ImGui::Button("+", ImVec2{ -1, 0 }))
			{
				std::string_view entered = buffer.data();

				if (!entered.empty() && std::ranges::find(value, entered) == value.end())
					value.emplace_back(entered);
			}
			ImGui::PopID();
			auto remove = value.end();
			for (auto it = value.begin(); it != value.end(); ++it)
			{
				ImGui::PushID(ID::nextID());
				if (ImGui::Button((std::string("- ") + *it).c_str()))
					remove = it;
				ImGui::PopID();
			}
			if (remove != value.end())
			{
				value.erase(remove);
				SetChanged();
			}
			ImGui::EndCombo();
		}
		ImGui::PopID();
		if (ImGui::IsItemHovered())
			ImGui::SetItemTooltip(tooltip.c_str());
	}

	inline SettingType GetType() override
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

	inline void RenderImGui() override
	{
		ImGui::SetNextItemWidth(Global::inputWidth);
		if (ImGui::BeginCombo(name.c_str(), identifiers[value].c_str()))
		{
			size_t i = 0;
			for (const auto& a : identifiers)
			{
				ImGui::PushID(ID::nextID());
				if (ImGui::RadioButton(a.c_str(), &value, i++))
					SetChanged();
				ImGui::PopID();
			}
			ImGui::EndCombo();
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip(tooltip.c_str());
	}

	inline SettingType GetType() override
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

	inline void RenderImGui() override
	{
		ImGui::PushID(ID::nextID());
		ImGui::SetNextItemWidth(9.0f / 5.0f * Global::inputWidth);
		if (ImGui::ColorEdit3("", value.arr.data(), ImGuiColorEditFlags_NoSmallPreview))
			SetChanged();
		ImGui::PopID();
		ImGui::SameLine();
		ImGui::PushID(ID::nextID());
		if (ImGui::ColorEdit4("", value.arr.data(), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar))
			SetChanged();
		ImGui::PopID();
		RenderLabel();
	}

	inline SettingType GetType() override
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

	inline void RenderImGui() override
	{
		//enableAdvanced - RGB - Color - Label
		ImGui::PushID(ID::nextID());
		if (ImGui::Checkbox("", &value.advanced))
			SetChanged();
		ImGui::PopID();
		ImGui::SetItemTooltip("enable advanced color edit");
		ImGui::SameLine();
		if (!value.advanced)
		{
			ImGui::PushID(ID::nextID());
			ImGui::SetNextItemWidth(9.0f / 5.0f * Global::inputWidth);
			if (ImGui::ColorEdit3("", value.colors.front().arr.data(), ImGuiColorEditFlags_NoSmallPreview))
				SetChanged();
			ImGui::PopID();
			ImGui::SameLine();
			ImGui::PushID(ID::nextID());
			if (ImGui::ColorEdit4("", value.colors.front().arr.data(), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar))
				SetChanged();
			ImGui::PopID();
		}
		else //advanced mode :(
		{
			float lineHeight = ImGui::GetTextLineHeight();
			float padding = ImGui::GetStyle().FramePadding.y;
			float height = lineHeight + 2 * padding;

			if (value.discrete)
				DrawFullDiscrete(Global::inputWidth, height);
			else
				DrawFullGradient(Global::inputWidth, height);
			ImGui::PushID(ID::nextID());
			ImGui::PushItemWidth(Global::inputWidth);
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4{ 0.0f, 0.0f, 0.0f, 0.0f });
			if (ImGui::BeginCombo("", "", ImGuiComboFlags_NoArrowButton))
			{
				ImGui::PopStyleColor(2);
				ImGui::SetNextItemWidth(Global::inputWidth);
				ImGui::PushID(ID::nextID());
				if (ImGui::Checkbox("discrete", &value.discrete))
					SetChanged();
				ImGui::PopID();
				ImGui::SetNextItemWidth(Global::inputWidth);
				ImGui::PushID(ID::nextID());
				if (ImGui::DragInt("period", &value.period, 5000 / divideRange, 0, 5000))
					SetChanged();
				ImGui::PopID();
				ImGui::SetNextItemWidth(Global::inputWidth);
				ImGui::PushID(ID::nextID());
				if (ImGui::DragInt("offset", &value.offset, 5000 / divideRange, 0, 5000))
					SetChanged();
				ImGui::PopID();
				ImGui::PushID(ID::nextID());
				if (ImGui::Button("add color"))
				{
					value.colors.push_back(Color{ 1.0, 1.0, 1.0, 1.0 });
					SetChanged();
				}
				ImGui::PopID();

				auto remove = value.colors.end();
				for (auto it = value.colors.begin(); it != value.colors.end(); ++it)
				{
					ImGui::PushID(ID::nextID());
					if (ImGui::ColorEdit4("", it->arr.data(), ImGuiColorEditFlags_NoInputs))
						SetChanged();
					if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && value.colors.size() > 1)
						remove = it;
					ImGui::PopID();
					ImGui::SameLine();
				}
				if (remove != value.colors.end())
				{
					value.colors.erase(remove);
					SetChanged();
				}

				ImGui::EndCombo();
			}
			else
				ImGui::PopStyleColor(2);
			ImGui::PopID();
		}
		RenderLabel();
	}

	inline SettingType GetType() override
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

	inline void RenderImGui() override
	{
		ImGui::NewLine();
		RenderLabel();
		ImGui::SetNextItemWidth(Global::inputWidth);
		ImGui::PushID(ID::nextID());
		strcpy_s(stringBuffer.data(), stringBuffer.size(), value.label.c_str());
		if (ImGui::InputText("label", stringBuffer.data(), stringBuffer.size()))
		{
			value.label = stringBuffer.data();
			SetChanged();
		}
		ImGui::PopID();

		ImGui::SetNextItemWidth(Global::inputWidth);
		ImGui::PushID(ID::nextID());
		if (ImGui::DragInt2("position", value.position.data()))
			SetChanged();
		ImGui::SetNextItemWidth(Global::inputWidth);
		if (ImGui::DragInt2("size", value.size.data(), 1, 0, 1 << (31 - 1)))
			SetChanged();
		ImGui::PopID();
	}

	inline SettingType GetType() override
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

	virtual void Visit(const std::function<void(Setting*)>& f) override
	{
		f(this);
		for (const auto& s : subSettings)
			s->Visit(f);
	}


private:
#pragma warning(push)
#pragma warning(disable : 4068)
#pragma diag_suppress 27
#pragma warning(pop)
	inline void RenderReset()
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
		ImGui::PushID(ID::nextID());
		if (ImGui::Button((const char*)u8"\xe800", ImVec2(temp, temp)))
			ReloadDefault();
		ImGui::PopID();
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("restore defaults");
		}
		ImGui::SetWindowFontScale(1.0f);
		ImGui::PopStyleColor(3);
	}

public:
	inline void RenderImGui() override
	{
		u8"\xe800";
		ImGui::SetWindowFontScale(1.0f);
		if (ImGui::TreeNode(name.c_str()))
		{
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip(tooltip.c_str());
			CheckCopyPaste();
			RenderReset();
			for (const auto& a : subSettings)
			{
				if (!a->GetMatchesSearch() && matchesSearch)
					ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
				a->RenderImGui();
				if (!a->GetMatchesSearch() && matchesSearch)
					ImGui::PopStyleVar();
			}
			ImGui::TreePop();
		}
		else
		{
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip(tooltip.c_str());
			CheckCopyPaste();
			RenderReset();
		}
	}

	inline void ReloadDefault() override
	{
		for (const auto& s : subSettings)
			s->ReloadDefault();
	}

	inline void ChangesAsJson(nlohmann::json& json) override
	{
		for (const auto& s : subSettings)
			s->ChangesAsJson(json);
	}

	void UpdateFromJson(const nlohmann::json& json) override
	{
		for (const auto& s : subSettings)
			s->UpdateFromJson(json);
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

	inline SettingType GetType() override
	{
		return stCategory;
	}

	inline std::unique_ptr<Setting> Copy() const override
	{
		return std::unique_ptr<Setting>{ new CategorySetting(*this) };
	}

	inline bool pasteCompatible(CategorySetting* setting)
	{
		if (subSettings.size() != setting->subSettings.size())
			return false;
		for (auto it1 = subSettings.begin(), it2 = setting->subSettings.begin(); it1 != subSettings.end(); ++it1, ++it2)
		{
			if ((*it1)->GetType() != (*it2)->GetType())
				return false;
			if ((*it1)->GetType() == stCategory)
				return dynamic_cast<CategorySetting*>(it1->get())->pasteCompatible(dynamic_cast<CategorySetting*>(it2->get()));
		}
		return true;
	}

	inline void Paste(Setting* setting) override
	{
		SetChanged();
		if (CategorySetting* s = dynamic_cast<CategorySetting*>(setting))
		{
			if (pasteCompatible(s))
			{
				for (size_t i = 0; i < subSettings.size(); i++)
				{
					subSettings[i]->Paste(s->subSettings[i].get());
				}
			}
		}
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

class Module
{
protected:
	std::string name;
	std::vector<std::unique_ptr<Setting>> settings;
	ImVec4 changeBounds{ -1.0f, -1.0f, -1.0f, -1.0f };

public:
	inline Module(const nlohmann::json& data) :
		name{ data["Name"] }
	{
		for (const auto& s : data["Settings"])
		{
			if (s["ID"] >= 0)
				settings.push_back(JsonToSetting(s));
		}
	}

	virtual ~Module() = default;

	inline void ChangeBounds(ImVec4 bounds)
	{
		changeBounds = bounds;
	}

protected:
	inline void UpdateBounds()
	{
		if (changeBounds.w != -1.0f)
		{
			ImGui::SetNextWindowPos(ImVec2{ changeBounds.x, changeBounds.y });
			ImGui::SetNextWindowSize(ImVec2{ changeBounds.z, changeBounds.w });
			changeBounds = { -1.0f, -1.0f, -1.0f, -1.0f };
		}
	}

	inline void RenderSettings()
	{
		for (const auto& s : settings)
		{
			if (!s->GetMatchesSearch())
				ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
			s->RenderImGui();
			if (!s->GetMatchesSearch())
				ImGui::PopStyleVar();
		}
	}

public:
	inline virtual void RenderImGui(bool& open)
	{
		UpdateBounds();

		if (ImGui::Begin(name.c_str(), &open))
		{
			RenderSettings();
		}
		ImGui::End();
	}

	inline void ResetDefaults()
	{
		for (const auto& s : settings)
			s->ReloadDefault();
	}

	inline std::string GetName() const
	{
		return name;
	}

	inline void UpdateSearch(std::string_view search)
	{
		for (const auto& s : settings)
			s->UpdateSearch(search);
	}

	inline void PushSetting(std::unique_ptr<Setting>&& setting)
	{
		settings.push_back(std::move(setting));
	}

	inline void ChangesAsJson(nlohmann::json& json) const
	{
		for (const auto& s : settings)
			s->ChangesAsJson(json);
	}

	inline void UpdateFromJson(const nlohmann::json& json)
	{
		for (const auto& s : settings)
			s->UpdateFromJson(json);
	}

	inline void Visit(const std::function<void(Setting*)>& f)
	{
		for (const auto& s : settings)
			s->Visit(f);
	}

	template <std::derived_from<Setting> T>
	inline T& GetSetting(std::string_view name) const
	{
		for (const auto& s : settings)
		{
			if (s->GetName() == name)
				return dynamic_cast<T&>(*s);
		}
		throw;
	}
};

class UiModule : public Module
{
	bool requestResetLayout = false;
	bool searchChanged = false;
	std::array<char, 100> searchBuffer{};

public:
	using Module::Module;

	inline virtual void RenderImGui(bool&) override
	{
		UpdateBounds();

		if (ImGui::Begin(name.c_str()))
		{
			ImGui::PushID(ID::nextID());
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::InputTextWithHint("", "Search...", searchBuffer.data(), searchBuffer.size()))
				searchChanged = true;
			ImGui::PopID();

			RenderSettings();

			ImGui::PushID(ID::nextID());
			if (ImGui::Button("reset layout"))
				requestResetLayout = true;
			ImGui::PopID();

		}
		ImGui::End();
	}

	inline bool IsRequestingResetLayout()
	{
		return std::exchange(requestResetLayout, false);
	}

	inline bool GetSearchChanged()
	{
		return std::exchange(searchChanged, false);
	}

	inline std::string_view GetSearch() const
	{
		return searchBuffer.data();
	}

	inline FloatSetting& GetInputWidth() const {
		return GetSetting<FloatSetting>("input width");
	}

	inline BoolListSetting& GetWindows() const
	{
		return GetSetting<BoolListSetting>("windows");
	}

	inline ToggleSetting& GetEnabled() const
	{
		return GetSetting<ToggleSetting>("enabled");
	}
};

#endif