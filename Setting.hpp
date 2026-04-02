#ifndef SETTING_H
#define SETTING_H

#include <string>

#include <json.hpp>

#include "ID.hpp"
#include "Keys.hpp"
#include "Packing.hpp"
#include "Global.hpp"

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

constexpr float divideRange = 1000.0f;

inline std::array<char, MAX_PATH> stringBuffer{};

class Setting;

std::unique_ptr<Setting> JsonToSetting(const nlohmann::json&);
	
class Setting
{
private:
	bool changed = false;

protected:
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

	inline bool hasChanged() const
	{
		return changed;
	}

	inline void setChanged()
	{
		changed = true;
		Global::settingChanged = true;
	}

	inline void resetChanged()
	{
		changed = false;
	}

	inline const std::string& getName() const
	{
		return name;
	}

	virtual inline void visit(const std::function<void(Setting*)>& f)
	{
		f(this);
	}

	virtual void renderImGui() = 0;

protected:
	// right clicking setting labels allows you to copy/paste settings
	// with shift is copy and without shift is paste
	inline void checkCopyPaste()
	{
		if (ImGui::IsItemClicked(1))
		{
			static std::unique_ptr<Setting> clipboard;
			if (ImGui::IsKeyDown(ImGuiMod_Shift))
			{
				clipboard = copy();
			}
			else if (clipboard)
			{
				paste(clipboard.get());
			}
		}
	}

	inline void renderLabel()
	{
		ImGui::SameLine();
		ImGui::Text(name.c_str());
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip(tooltip.c_str());
		checkCopyPaste();
	}

public:
	virtual void reloadDefault() = 0;

	virtual void changesAsJson(nlohmann::json& json) = 0;
	virtual void updateFromJson(const nlohmann::json& data) = 0;

	virtual SettingType getType() = 0;

	virtual std::unique_ptr<Setting> copy() const = 0;

	virtual void paste(Setting* setting) = 0;
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

	inline const T& getDefault() const
	{
		return defaultValue;
	}

	inline T& getDefault()
	{
		return defaultValue;
	}

	inline void setDefault(const T& value)
	{
		defaultValue = value;
	}

	inline const T& getValue() const
	{
		return value;
	}

	inline T& getValue()
	{
		return value;
	}

	inline void setValue(const T& value)
	{
		this->value = value;
	}

	inline virtual void reloadDefault() override
	{
		value = defaultValue;
		setChanged();
	}

	inline virtual void changesAsJson(nlohmann::json& json) override
	{
		if (hasChanged())
		{
			nlohmann::json temp;
			temp["ID"] = id;
			temp["Value"] = value;
			json.push_back(temp);
		}
	}

	inline virtual void updateFromJson(const nlohmann::json& json) override
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

#define MAKE_COPY_PASTE(T) \
inline std::unique_ptr<Setting> copy() const override \
{ \
	return std::unique_ptr<Setting>(new T(*this)); \
} \
inline void paste(Setting* setting) override \
{ \
	setChanged(); \
	if (T* s = dynamic_cast<T*>(setting)) \
	{ \
		value = s->value; \
	} \
}

class IntSetting : public SettingOfType<int>
{
	int min;
	int max;

public:
	inline IntSetting(const nlohmann::json& json) :
		SettingOfType{ json },
		min{ json["Min"] },
		max{ json["Max"] } {}

	inline void renderImGui() override
	{
		ImGui::SetNextItemWidth(Global::inputWidth);
		ImGui::PushID(ID::nextID());
		if (ImGui::DragInt("", &value, (max - min) / divideRange, min, max, "%d", ImGuiSliderFlags_None))
			setChanged();
		ImGui::PopID();
		renderLabel();
	}

	inline SettingType getType() override
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

	inline void renderImGui() override
	{
		ImGui::SetNextItemWidth(Global::inputWidth);
		ImGui::PushID(ID::nextID());
		if (ImGui::DragFloat("", &value, stepSize, min, max, "%.2f"))
			setChanged();
		ImGui::PopID();
		renderLabel();
	}

	inline SettingType getType() override
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

	inline void renderImGui() override
	{
		ImGui::PushID(ID::nextID());
		if (ImGui::Checkbox("", &value))
			setChanged();
		ImGui::PopID();
		renderLabel();
	}

	inline SettingType getType() override
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
	std::array<char, 100> keyName;

public:
	inline ToggleSetting(const nlohmann::json& json) :
		SettingOfType{ json } {}

	inline void renderImGui() override
	{
		if (value.hotkey != previousHotkey)
		{
			strcpy(keyName.data(), keyCodeToString(value.hotkey).c_str());
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
				setChanged();
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
					setChanged();
					pressed = true;
				}
			}
		}

		wasPressed = pressed;
		ImGui::SameLine();
		ImGui::PushID(ID::nextID());
		if (ImGui::Checkbox("", &value.state))
			setChanged();
		ImGui::PopID();
		renderLabel();
	}

	inline SettingType getType() override
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
	std::array<char, 100> keyName;

public:
	inline HotkeySetting(const nlohmann::json& json) :
		SettingOfType{ json } {}

	inline void renderImGui() override
	{
		if (value != previousValue)
		{
			strcpy(keyName.data(), keyCodeToString(value).c_str());
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
				setChanged();
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
					setChanged();
					pressed = true;
				}
			}
		}
		wasPressed = pressed;
		renderLabel();
	}

	inline SettingType getType() override
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

	inline void renderImGui() override
	{
		ImGui::SetNextItemWidth(Global::inputWidth);
		ImGui::PushID(ID::nextID());
		if (ImGui::DragFloat("##value_x", &value.x, stepSize.x, min.x, max.x, "%.2f"))
			setChanged();
		ImGui::PopID();
		ImGui::SetNextItemWidth(Global::inputWidth);
		ImGui::PushID(ID::nextID());
		ImGui::SameLine();
		if (ImGui::DragFloat("##value_y", &value.y, stepSize.y, min.y, max.y, "%.2f"))
			setChanged();
		ImGui::PopID();
		renderLabel();
	}

	inline SettingType getType() override
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

	inline void renderImGui() override
	{
		ImGui::SetNextItemWidth(Global::inputWidth);
		ImGui::PushID(ID::nextID());
		strcpy_s(stringBuffer.data(), stringBuffer.size(), value.c_str());
		if (ImGui::InputText("", stringBuffer.data(), stringBuffer.size()))
		{
			value = stringBuffer.data();
			setChanged();
		}
		ImGui::PopID();
		renderLabel();
	}

	inline SettingType getType() override
	{
		return stString;
	}

	MAKE_COPY_PASTE(StringSetting)
};

class RoundingMultiplierSetting : public SettingOfType<std::string>
{
	inline static bool isValid(std::string_view value)
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

	inline void renderImGui() override
	{
		ImGui::SetNextItemWidth(Global::inputWidth);
		ImGui::PushID(ID::nextID());
		strcpy_s(stringBuffer.data(), stringBuffer.size(), value.c_str());
		if (ImGui::InputText("", stringBuffer.data(), stringBuffer.size()))
		{
			if (isValid(stringBuffer.data()))
			{
				value = stringBuffer.data();
				setChanged();
			}
		}
		ImGui::PopID();
		renderLabel();
	}

	inline SettingType getType() override
	{
		return stRoundingMultiplier;
	}

	MAKE_COPY_PASTE(RoundingMultiplierSetting)
};

// std::vector<bool> is annoying
class BoolListSetting : public SettingOfType<std::vector<bool>>
{
	std::vector<std::string> valueIdentifiers;
	bool collapsing = false;

public:
	inline BoolListSetting(const nlohmann::json& json) :
		SettingOfType{ json },
		valueIdentifiers{ json["Identifiers"].get<std::vector<std::string>>()},
		collapsing{ json["Collapsing"] } {}

	inline void renderImGui() override
	{
		if (!collapsing)
		{
			if (ImGui::BeginMenu(name.c_str(), true))
			{
				size_t i = 0;
				for (auto it = value.begin(); it != value.end(); ++it)
				{
					ImGui::PushID(ID::nextID());
					bool open = *it;
					if (ImGui::Checkbox(valueIdentifiers[i++].c_str(), &open))
					{
						*it = open;
						setChanged();
					}
					ImGui::PopID();
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
					for (auto it = value.begin(); it != value.end(); ++it)
						*it = true;
					setChanged();
				}
				ImGui::SameLine();
				if (ImGui::Button("hide all"))
				{
					for (auto it = value.begin(); it != value.end(); ++it)
						*it = false;
					setChanged();
				}
				size_t i = 0;
				for (auto it = value.begin(); it != value.end(); ++it)
				{
					ImGui::PushID(ID::nextID());
					if (ImGui::MenuItem(valueIdentifiers[i++].c_str(), nullptr, *it))
					{
						*it = !*it;
						setChanged();
					}
					ImGui::PopID();
				}
			}
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip(tooltip.c_str());
	}

	inline SettingType getType() override
	{
		return stBoolList;
	}

	MAKE_COPY_PASTE(BoolListSetting)
};

class StringListSetting : public SettingOfType<std::vector<std::string>>
{
	std::array<char, MAX_PATH> buffer{};

public:
	inline StringListSetting(const nlohmann::json& json) :
		SettingOfType{ json } {}

	inline void renderImGui() override
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
				setChanged();
			}
			ImGui::EndCombo();
		}
		ImGui::PopID();
		if (ImGui::IsItemHovered())
			ImGui::SetItemTooltip(tooltip.c_str());
	}

	inline SettingType getType() override
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

	inline void renderImGui() override
	{
		ImGui::SetNextItemWidth(Global::inputWidth);
		if (ImGui::BeginCombo(name.c_str(), identifiers[value].c_str()))
		{
			size_t i = 0;
			for (const auto& a : identifiers)
			{
				ImGui::PushID(ID::nextID());
				if (ImGui::RadioButton(a.c_str(), &value, i++))
					setChanged();
				ImGui::PopID();
			}
			ImGui::EndCombo();
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip(tooltip.c_str());
	}

	inline SettingType getType() override
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

	inline auto asInt() const
	{
		return IM_COL32(255 * arr[0], 255 * arr[1], 255 * arr[2], 255);
	}
};

class ColorSetting : public SettingOfType<Color>
{
public:
	inline ColorSetting(const nlohmann::json& json) :
		SettingOfType{ json } {}

	inline void renderImGui() override
	{
		ImGui::PushID(ID::nextID());
		ImGui::SetNextItemWidth(9.0f / 5.0f * Global::inputWidth);
		if (ImGui::ColorEdit3("", value.arr.data(), ImGuiColorEditFlags_NoSmallPreview))
			setChanged();
		ImGui::PopID();
		ImGui::SameLine();
		ImGui::PushID(ID::nextID());
		if (ImGui::ColorEdit4("", value.arr.data(), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar))
			setChanged();
		ImGui::PopID();
		renderLabel();
	}

	inline SettingType getType() override
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
	inline void drawGradient(float x1, float x2, float y1, float y2, Color Lcolor, Color Rcolor)
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilledMultiColor(ImVec2{ x1, y1 }, ImVec2{ x2, y2 },
			Lcolor.asInt(), Rcolor.asInt(), Rcolor.asInt(), Lcolor.asInt());
	}
	inline void drawRect(float x1, float x2, float y1, float y2, Color color)
	{
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(ImVec2{ x1, y1 }, ImVec2{ x2, y2 }, color.asInt());
	}
	inline void drawFullGradient(float width, float height)
	{
		ImVec2 p = ImGui::GetCursorScreenPos();
		if (value.colors.size() == 1)
		{
			drawGradient(p.x, p.x + width, p.y, p.y + height, value.colors.front(), value.colors.front());
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
			drawGradient(p.x + currX, p.x + nextX, p.y, p.y + height, prevColor, currColor);
			currX = nextX;
			nextX += stepSize;
			prevColor = currColor;
		}
	}

	inline void drawFullDiscrete(float width, float height)
	{
		ImVec2 p = ImGui::GetCursorScreenPos();
		if (value.colors.size() == 1)
		{
			drawRect(p.x, p.x + width, p.y, p.y + height, value.colors.front());
			return;
		}
		float stepSize = width / value.colors.size();
		float xOff = 0.0f;
		for (auto& c : value.colors)
		{
			drawRect(p.x + xOff, p.x + xOff + stepSize, p.y, p.y + height, c);
			xOff += stepSize;
		}
	}

public:
	inline ColorTransitionSetting(const nlohmann::json& json) :
		SettingOfType{ json }
	{}

	inline void renderImGui() override
	{
		//enableAdvanced - RGB - Color - Label
		ImGui::PushID(ID::nextID());
		if (ImGui::Checkbox("", &value.advanced))
			setChanged();
		ImGui::PopID();
		ImGui::SetItemTooltip("enable advanced color edit");
		ImGui::SameLine();
		if (!value.advanced)
		{
			ImGui::PushID(ID::nextID());
			ImGui::SetNextItemWidth(9.0f / 5.0f * Global::inputWidth);
			if (ImGui::ColorEdit3("", value.colors.front().arr.data(), ImGuiColorEditFlags_NoSmallPreview))
				setChanged();
			ImGui::PopID();
			ImGui::SameLine();
			ImGui::PushID(ID::nextID());
			if (ImGui::ColorEdit4("", value.colors.front().arr.data(), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar))
				setChanged();
			ImGui::PopID();
		}
		else //advanced mode :(
		{
			float lineHeight = ImGui::GetTextLineHeight();
			float padding = ImGui::GetStyle().FramePadding.y;
			float height = lineHeight + 2 * padding;

			if (value.discrete)
				drawFullDiscrete(Global::inputWidth, height);
			else
				drawFullGradient(Global::inputWidth, height);
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
					setChanged();
				ImGui::PopID();
				ImGui::SetNextItemWidth(Global::inputWidth);
				ImGui::PushID(ID::nextID());
				if (ImGui::DragInt("period", &value.period, 5000 / divideRange, 0, 5000))
					setChanged();
				ImGui::PopID();
				ImGui::SetNextItemWidth(Global::inputWidth);
				ImGui::PushID(ID::nextID());
				if (ImGui::DragInt("offset", &value.offset, 5000 / divideRange, 0, 5000))
					setChanged();
				ImGui::PopID();
				ImGui::PushID(ID::nextID());
				if (ImGui::Button("add color"))
				{
					value.colors.push_back(Color{ 1.0, 1.0, 1.0, 1.0 });
					setChanged();
				}
				ImGui::PopID();

				auto remove = value.colors.end();
				for (auto it = value.colors.begin(); it != value.colors.end(); ++it)
				{
					ImGui::PushID(ID::nextID());
					if (ImGui::ColorEdit4("", it->arr.data(), ImGuiColorEditFlags_NoInputs))
						setChanged();
					if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && value.colors.size() > 1)
						remove = it;
					ImGui::PopID();
					ImGui::SameLine();
				}
				if (remove != value.colors.end())
				{
					value.colors.erase(remove);
					setChanged();
				}

				ImGui::EndCombo();
			}
			else
				ImGui::PopStyleColor(2);
			ImGui::PopID();
		}
		renderLabel();
	}

	inline SettingType getType() override
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

	inline void renderImGui() override
	{
		ImGui::NewLine();
		renderLabel();
		ImGui::SetNextItemWidth(Global::inputWidth);
		ImGui::PushID(ID::nextID());
		strcpy_s(stringBuffer.data(), stringBuffer.size(), value.label.c_str());
		if (ImGui::InputText("label", stringBuffer.data(), stringBuffer.size()))
		{
			value.label = stringBuffer.data();
			setChanged();
		}
		ImGui::PopID();

		ImGui::SetNextItemWidth(Global::inputWidth);
		ImGui::PushID(ID::nextID());
		if (ImGui::DragInt2("position", value.position.data()))
			setChanged();
		ImGui::SetNextItemWidth(Global::inputWidth);
		if (ImGui::DragInt2("size", value.size.data(), 1, 0, 1 << 31 - 1))
			setChanged();
		ImGui::PopID();
	}

	inline SettingType getType() override
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
			subSettings.push_back(s->copy());
	}

	virtual void visit(const std::function<void(Setting*)>& f) override
	{
		f(this);
		for (const auto& s : subSettings)
			s->visit(f);
	}

private:
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
			reloadDefault();
		ImGui::PopID();
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("restore defaults");
		}
		ImGui::SetWindowFontScale(1.0f);
		ImGui::PopStyleColor(3);
	}

public:
	inline void renderImGui() override
	{
		ImGui::SetWindowFontScale(1.0f);
		if (ImGui::TreeNode(name.c_str()))
		{
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip(tooltip.c_str());
			checkCopyPaste();
			RenderReset();
			for (const auto& a : subSettings)
				a->renderImGui();
			ImGui::TreePop();
		}
		else
		{
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip(tooltip.c_str());
			checkCopyPaste();
			RenderReset();
		}
	}

	inline void reloadDefault() override
	{
		for (const auto& s : subSettings)
			s->reloadDefault();
	}

	inline void changesAsJson(nlohmann::json& json) override
	{
		for (const auto& s : subSettings)
			s->changesAsJson(json);
	}

	void updateFromJson(const nlohmann::json& json) override
	{
		for (const auto& s : subSettings)
			s->updateFromJson(json);
	}

	inline SettingType getType() override
	{
		return stCategory;
	}

	inline std::unique_ptr<Setting> copy() const override
	{
		return std::unique_ptr<Setting>{ new CategorySetting(*this) };
	}

	inline bool pasteCompatible(CategorySetting* setting)
	{
		if (subSettings.size() != setting->subSettings.size())
			return false;
		for (auto it1 = subSettings.begin(), it2 = setting->subSettings.begin(); it1 != subSettings.end(); ++it1, ++it2)
		{
			if ((*it1)->getType() != (*it2)->getType())
				return false;
			if ((*it1)->getType() == stCategory)
				return dynamic_cast<CategorySetting*>(it1->get())->pasteCompatible(dynamic_cast<CategorySetting*>(it2->get()));
		}
		return true;
	}

	inline void paste(Setting* setting) override
	{
		setChanged();
		if (CategorySetting* s = dynamic_cast<CategorySetting*>(setting))
		{
			if (pasteCompatible(s))
			{
				for (size_t i = 0; i < subSettings.size(); i++)
				{
					subSettings[i]->paste(s->subSettings[i].get());
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
	inline void updateBounds()
	{
		if (changeBounds.w != -1.0f)
		{
			ImGui::SetNextWindowPos(ImVec2{ changeBounds.x, changeBounds.y });
			ImGui::SetNextWindowSize(ImVec2{ changeBounds.z, changeBounds.w });
			changeBounds = { -1.0f, -1.0f, -1.0f, -1.0f };
		}
	}

public:
	inline virtual void RenderImGui(bool& open)
	{
		updateBounds();

		if (ImGui::Begin(this->name.c_str(), &open))
		{
			for (const auto& s : settings)
				s->renderImGui();
			ImGui::End();
		}
	}

	inline void ResetDefaults()
	{
		for (const auto& s : settings)
		{
			s->reloadDefault();
		}
	}

	inline std::string getName() const
	{
		return name;
	}

	inline void pushSetting(std::unique_ptr<Setting>&& setting)
	{
		settings.push_back(std::move(setting));
	}

	inline void changesAsJson(nlohmann::json& json) const
	{
		for (const auto& s : settings)
			s->changesAsJson(json);
	}

	inline void updateFromJson(nlohmann::json json)
	{
		for (const auto& s : settings)
			s->updateFromJson(json);
	}

	inline void visit(const std::function<void(Setting*)>& f)
	{
		for (const auto& s : settings)
			s->visit(f);
	}
};

class UiModule : public Module
{
	bool requestResetLayout = false;

public:
	using Module::Module;

	inline virtual void RenderImGui(bool&) override
	{
		updateBounds();

		if (ImGui::Begin(name.c_str()))
		{
			for (const auto& setting : settings)
				setting->renderImGui();

			ImGui::PushID(ID::nextID());
			if (ImGui::Button("reset layout"))
				requestResetLayout = true;
			ImGui::PopID();

			ImGui::End();
		}
	}

	inline bool isRequestingResetLayout()
	{
		return std::exchange(requestResetLayout, false);
	}

	inline FloatSetting& getInputWidth() const
	{
		for (const auto& s : settings)
		{
			if (s->getName() == "input width")
				return dynamic_cast<FloatSetting&>(*s);
		}
		throw;
	}

	inline BoolListSetting& getWindows() const
	{
		for (auto& s : settings)
		{
			if (s->getName() == "windows")
				return dynamic_cast<BoolListSetting&>(*s);
		}
		throw;
	}

	inline ToggleSetting& getEnabled() const
	{
		for (const auto& s : settings)
		{
			if (s->getName() == "enabled")
				return dynamic_cast<ToggleSetting&>(*s);
		}
		throw;
	}
};

#endif