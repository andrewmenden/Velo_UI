#pragma once
#include <string>
#include "imgui.h"
#include "imgui_internal.h"
#include "json.hpp"
#include "EventBool.hpp"
#include "ID.hpp"
#include "Keys.hpp"
#include "Packing.hpp"

#define FLOAT_COLOR(f) IM_COL32(255*f[0],255*f[1],255*f[2],255)

namespace Settings
{
	const float divideRange = 1000.0f;
	uint8_t EnableUI = -1;
	
	class Setting
	{
	private:
	protected:
		std::string name;
		int ID;
		EventBool hasChanged;
		std::string tooltip;
	public:
		static float width;
		virtual void RenderImGui() = 0;
		inline void RenderLabel()
		{
			ImGui::SameLine();
			ImGui::Text(this->name.c_str());
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip(this->tooltip.c_str());
		}
		virtual void ReloadDefault() = 0;
		virtual void LoadFromJson(nlohmann::json data) = 0;
		inline bool HasChanged()
		{
			return hasChanged.isChanged();
		}
		virtual void GetJson(nlohmann::json& json, bool all = false) = 0;
		virtual void UpdateJson(nlohmann::json json) = 0;
		inline void printName(std::ostream& os)
		{
			os << name << '\n';
		}
	};
	float Setting::width = 70.0f;

	class IntSetting : public Setting
	{
	private:
		int defaultValue;
		int value;
		int min;
		int max;
	public:
		inline void RenderImGui() override
		{
			ImGui::SetNextItemWidth(width);
			ImGui::PushID(ID::nextID());
			if (ImGui::DragInt("", &value, 1.0f, min, max, "%d", ImGuiSliderFlags_None))
				this->hasChanged = true;
			ImGui::PopID();
			RenderLabel();
		}
		inline void ReloadDefault() override
		{
			value = defaultValue;
			this->hasChanged = true;
		}
		inline void LoadFromJson(nlohmann::json data) override
		{
			this->name = data["Name"];
			this->ID = data["ID"];
			this->tooltip = data["Tooltip"];
			defaultValue = data["Default"];
			value = data["Value"];
			min = data["Min"];
			max = data["Max"];
		}
		inline void GetJson(nlohmann::json& json, bool all) override
		{
			if (hasChanged.isChanged() || all)
			{
				nlohmann::json temp;
				temp["ID"] = this->ID;
				temp["Value"] = value;
				//placeholder for replace in GetJson
				json.push_back(temp);
			}
			hasChanged = false;
		}
		void UpdateJson(nlohmann::json json) override
		{
			for (const auto& data : json["Changes"])
			{
				if (data["ID"] == this->ID)
				{
					value = data["Value"];
					break;
				}
			}
		}
	};

	class FloatSetting : public Setting
	{
	private:
		float defaultValue;
		float value;
		float min;
		float max;
		float stepSize;
	public:
		inline void RenderImGui() override
		{
			ImGui::SetNextItemWidth(width);
			ImGui::PushID(ID::nextID());
			if (ImGui::DragFloat("", &value, stepSize, min, max, "%.2f"))
				hasChanged = true;
			ImGui::PopID();
			RenderLabel();
		}
		inline void ReloadDefault() override
		{
			value = defaultValue;
			hasChanged = true;
		}
		inline void LoadFromJson(nlohmann::json data) override
		{
			this->name = data["Name"];
			this->ID = data["ID"];
			this->tooltip = data["Tooltip"];
			defaultValue = data["Default"];
			value = data["Value"];
			min = data["Min"];
			max = data["Max"];
			stepSize = (max - min) / divideRange;
		}
		inline void GetJson(nlohmann::json& json, bool all) override
		{
			if (hasChanged.isChanged() || all)
			{
				nlohmann::json temp;
				temp["ID"] = this->ID;
				temp["Value"] = value;
				//placeholder for replace in GetJson
				json.push_back(temp);
			}
			hasChanged = false;
		}
		void UpdateJson(nlohmann::json json) override
		{
			for (const auto& data : json["Changes"])
			{
				if (data["ID"] == this->ID)
				{
					value = data["Value"];
					break;
				}
			}
		}
	};

	class BoolSetting : public Setting
	{
	private:
		bool defaultValue;
		bool value;
	public:
		inline void RenderImGui() override
		{
			ImGui::PushID(ID::nextID());
			if (ImGui::Checkbox("", &value))
				hasChanged = true;
			ImGui::PopID();
			RenderLabel();
		}
		inline void ReloadDefault() override
		{
			value = defaultValue;
			hasChanged = true;
		}
		inline void LoadFromJson(nlohmann::json data) override
		{
			this->ID = data["ID"];
			this->name = data["Name"];
			this->tooltip = data["Tooltip"];
			defaultValue = data["Default"];
			value = data["Value"];
		}
		inline void GetJson(nlohmann::json& json, bool all) override
		{
			if (hasChanged.isChanged() || all)
			{
				nlohmann::json temp;
				temp["ID"] = this->ID;
				temp["Value"] = value;
				//placeholder for replace in GetJson
				json.push_back(temp);
			}
			hasChanged = false;
		}
		void UpdateJson(nlohmann::json json) override
		{
			for (const auto& data : json["Changes"])
			{
				if (data["ID"] == this->ID)
				{
					value = data["Value"];
					break;
				}
			}
		}
	};

	class ToggleSetting : public Setting
	{
	private:
		bool defaultValue;
		int defaultHotkey;
		bool value;
		int hotkey;
		char* keyName = new char[100]; //doubt this will be exceeded
	public:
		inline void RenderImGui() override
		{
			ImGui::SetNextItemWidth(width);
			ImGui::PushID(ID::nextID());
			ImGui::InputText("", keyName, 100, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoUndoRedo);
			ImGui::PopID();
			if (ImGui::IsItemActive())
			{
				if (ImGui::IsKeyDown(ImGuiKey_MouseRight))
				{
					hotkey = 151;
					strcpy(keyName, "N/A");
					hasChanged = true;
				}
				for (auto& k : Keys::KeyCodesEx)
				{
					if (GetAsyncKeyState(k) & 0x8000)
					{
						hotkey = k;
						if (hotkey == EnableUI)
							continue;
						strcpy(keyName, Keys::KeyCodeToString(k).c_str());
						hasChanged = true;
						break;
					}
				}
			}
			ImGui::SameLine();
			ImGui::PushID(ID::nextID());
			if (ImGui::Checkbox("", &value))
				hasChanged = true;
			ImGui::PopID();
			RenderLabel();
		}
		inline void ReloadDefault() override
		{
			value = defaultValue;
			hotkey = defaultHotkey;
			hasChanged = true;
		}
		inline void LoadFromJson(nlohmann::json data) override
		{
			this->name = data["Name"];
			this->ID = data["ID"];
			this->tooltip = data["Tooltip"];
			defaultValue = data["Default"]["State"];
			defaultHotkey = data["Default"]["Hotkey"];
			value = data["Value"]["State"];
			hotkey = data["Value"]["Hotkey"];
			strcpy(keyName, Keys::KeyCodeToString(hotkey).c_str());
		}
		inline void GetJson(nlohmann::json& json, bool all) override
		{
			if (hasChanged.isChanged() || all)
			{
				nlohmann::json temp;
				temp["ID"] = this->ID;
				temp["Value"]["State"] = value;
				temp["Value"]["Hotkey"] = hotkey;
				//placeholder for replace in GetJson
				json.push_back(temp);
			}
			hasChanged = false;
		}
		void UpdateJson(nlohmann::json json) override
		{
			for (const auto& data : json["Changes"])
			{
				if (data["ID"] == this->ID)
				{
					value = data["Value"]["State"];
					hotkey = data["Value"]["Hotkey"];
					strcpy(keyName, Keys::KeyCodeToString(hotkey).c_str());
					break;
				}
			}
		}
	};

	class HotkeySetting : public Setting
	{
	private:
		int defaultValue;
		int value;
		char* keyName = new char[100]; //doubt this will be exceeded
	public:
		inline void RenderImGui() override
		{
			ImGui::SetNextItemWidth(width);
			ImGui::PushID(ID::nextID());
			ImGui::InputText("", keyName, 100, ImGuiInputTextFlags_ReadOnly | ImGuiInputTextFlags_NoUndoRedo);
			ImGui::PopID();
			if (ImGui::IsItemActive())
			{
				if (ImGui::IsKeyDown(ImGuiKey_MouseRight))
				{
					value = 151;
					strcpy(keyName, "N/A");
					hasChanged = true;
				}
				for (auto& k : Keys::KeyCodesEx)
				{
					if (GetAsyncKeyState(k) & 0x8000)
					{
						value = k;
						if (value == EnableUI)
							continue;
						strcpy(keyName, Keys::KeyCodeToString(k).c_str());
						hasChanged = true;
						break;
					}
				}
			}
			RenderLabel();
		}
		inline void ReloadDefault() override
		{
			value = defaultValue;
		}
		inline void LoadFromJson(nlohmann::json data) override
		{
			this->name = data["Name"];
			this->ID = data["ID"];
			this->tooltip = data["Tooltip"];
			defaultValue = data["Default"];
			value = data["Value"];
			strcpy(keyName, Keys::KeyCodeToString(value).c_str());
		}
		inline void GetJson(nlohmann::json& json, bool all) override
		{
			if (hasChanged.isChanged() || all)
			{
				nlohmann::json temp;
				temp["ID"] = this->ID;
				temp["Value"] = value;
				//placeholder for replace in GetJson
				json.push_back(temp);
			}
			hasChanged = false;
		}
		void UpdateJson(nlohmann::json json) override
		{
			for (const auto& data : json["Changes"])
			{
				if (data["ID"] == this->ID)
				{
					value = data["Value"];
					strcpy(keyName, Keys::KeyCodeToString(value).c_str());
					break;
				}
			}
		}
	};

	class VectorSetting : public Setting
	{
	private:
		float defaultValueX;
		float defaultValueY;
		float valueX;
		float valueY;
		float minX;
		float minY;
		float maxX;
		float maxY;
		float stepSizeX;
		float stepSizeY;
	public:
		inline void RenderImGui() override
		{
			//float dragWidth = (ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize(this->name.c_str()).x - ImGui::CalcTextSize(" to ").x) / 2;
			ImGui::SetNextItemWidth(width);
			ImGui::PushID(ID::nextID());
			//ImGui::PushItemWidth(dragWidth);
			if (ImGui::DragFloat("##value_x", &valueX, stepSizeX, minX, maxX, "%.2f"))
				hasChanged = true;
			ImGui::PopID();
			ImGui::SetNextItemWidth(width);
			ImGui::PushID(ID::nextID());
			ImGui::SameLine();
			if (ImGui::DragFloat("##value_y", &valueY, stepSizeY, minY, maxY, "%.2f"))
				hasChanged = true;
			ImGui::PopID();
			RenderLabel();
		}
		inline void ReloadDefault() override
		{
			valueX = defaultValueX;
			valueY = defaultValueY;
			hasChanged = true;
		}
		inline void LoadFromJson(nlohmann::json data) override
		{
			this->name = data["Name"];
			this->ID = data["ID"];
			this->tooltip = data["Tooltip"];
			float temp;
			temp = data["Default"]["X"];
			defaultValueX = temp;
			temp = data["Default"]["Y"];
			defaultValueY = temp;
			temp = data["Value"]["X"];
			valueX = temp;
			temp = data["Value"]["Y"];
			valueY = temp;
			minX = data["Min"]["X"];
			minY = data["Min"]["Y"];
			maxX = data["Max"]["X"];
			maxY = data["Max"]["Y"];
			stepSizeX = (maxX - minX) / divideRange;
			stepSizeY = (maxY - minY) / divideRange;
		}
		inline void GetJson(nlohmann::json& json, bool all) override
		{
			if (hasChanged.isChanged() || all)
			{
				nlohmann::json temp;
				temp["ID"] = this->ID;
				temp["Value"]["X"] = valueX;
				temp["Value"]["Y"] = valueY;
				//placeholder for replace in GetJson
				json.push_back(temp);
			}
			hasChanged = false;
		}
		void UpdateJson(nlohmann::json json) override
		{
			for (const auto& data : json["Changes"])
			{
				if (data["ID"] == this->ID)
				{
					float temp;
					temp = data["Value"]["X"];
					valueX = temp;
					temp = data["Value"]["Y"];
					valueY = temp;
					break;
				}
			}
		}
	};

	class StringSetting : public Setting
	{
	private:
		std::string defaultValue;
		char* value = new char[MAX_PATH] {};
	public:
		inline void RenderImGui() override
		{
			ImGui::SetNextItemWidth(width);
			ImGui::PushID(ID::nextID());
			if (ImGui::InputText("", value, MAX_PATH))
				hasChanged = true;
			ImGui::PopID();
			RenderLabel();
		}
		inline void ReloadDefault() override
		{
			strcpy(value, defaultValue.c_str());
		}
		inline void LoadFromJson(nlohmann::json data) override
		{
			this->name = data["Name"];
			this->ID = data["ID"];
			this->tooltip = data["Tooltip"];
			defaultValue = data["Default"];
			strcpy(value, std::string(data["Value"]).c_str());
		}
		inline void GetJson(nlohmann::json& json, bool all) override
		{
			if (hasChanged.isChanged() || all)
			{
				nlohmann::json temp;
				temp["ID"] = this->ID;
				temp["Value"] = std::string(value);
				//placeholder for replace in GetJson
				json.push_back(temp);
			}
			hasChanged = false;
		}
		void UpdateJson(nlohmann::json json) override
		{
			for (const auto& data : json["Changes"])
			{
				if (data["ID"] == this->ID)
				{
					strcpy(value, std::string(data["Value"]).c_str());
					break;
				}
			}
		}
	};

	class RoundingMultiplierSetting : public Setting
	{
	private:
		std::string defaultValue;
		char* value = new char[MAX_PATH];
		std::string previous;

		bool isValid(char* value)
		{
			int i = 0;
			char c = value[0];
			bool decimalFound = false;
			while (c != '\0')
			{
				if (c == '.')
				{
					if (decimalFound)
						return false;
					decimalFound = true;
				}
				else if (c < '0' || c>'9')
					return false;
				c = value[++i];
			}
			return true;
		}
	public:
		inline void RenderImGui() override
		{
			ImGui::SetNextItemWidth(width);
			ImGui::PushID(ID::nextID());
			if (ImGui::InputText("", value, MAX_PATH))
			{
				if (!isValid(value))
					strcpy(value, previous.c_str());
				else
				{
					previous = std::string(value);
					hasChanged = true;
				}
			}
			ImGui::PopID();
			RenderLabel();
		}
		inline void ReloadDefault() override
		{
			strcpy(value, defaultValue.c_str());
			hasChanged = true;
		}
		inline void LoadFromJson(nlohmann::json data) override
		{
			this->name = data["Name"];
			this->ID = data["ID"];
			this->tooltip = data["Tooltip"];
			defaultValue = data["Default"];
			previous = data["Value"];
			strcpy(value, previous.c_str());
		}
		inline void GetJson(nlohmann::json& json, bool all) override
		{
			if (hasChanged.isChanged() || all)
			{
				nlohmann::json temp;
				temp["ID"] = this->ID;
				temp["Value"] = std::string(value);
				//placeholder for replace in GetJson
				json.push_back(temp);
			}
			hasChanged = false;
		}
		void UpdateJson(nlohmann::json json) override
		{
			for (const auto& data : json["Changes"])
			{
				if (data["ID"] == this->ID)
				{
					previous = data["Value"];
					strcpy(value, previous.c_str());
					break;
				}
			}
		}
		inline ~RoundingMultiplierSetting()
		{
			delete[] value;
		}
	};

	class BoolListSetting : public Setting
	{
	private:
		std::vector<uint8_t> defaultValue;
		std::list<uint8_t> value;
		std::vector<std::string> valueIdentifiers;
	public:
		inline void RenderImGui() override
		{
			if (ImGui::BeginMenu(this->name.c_str(), true))
			{
				int i = 0;
				for (auto& a : value)
				{
					ImGui::PushID(ID::nextID());
					if (ImGui::Checkbox(valueIdentifiers[i++].c_str(), (bool*)&a))
						hasChanged = true;
					ImGui::PopID();
				}
				ImGui::EndMenu();
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip(this->tooltip.c_str());
		}
		inline void ReloadDefault() override
		{
			int i = 0;
			for (auto& a : value)
				a = defaultValue[i++];
			hasChanged = true;
		}
		inline void LoadFromJson(nlohmann::json data) override
		{
			this->name = data["Name"];
			this->ID = data["ID"];
			this->tooltip = data["Tooltip"];
			auto& temp = data["Default"];
			for (const auto& a : temp)
				defaultValue.push_back(a);
			temp = data["Value"];
			for (auto& a : temp)
				value.push_back((uint8_t)a);
			temp = data["Identifiers"];
			for (const auto& a : temp)
				valueIdentifiers.push_back(a);
		}
		inline void GetJson(nlohmann::json& json, bool all) override
		{
			if (hasChanged.isChanged() || all)
			{
				nlohmann::json temp;
				temp["ID"] = this->ID;
				for (auto& a : value)
					temp["Value"].push_back((bool)a);
				//placeholder for replace in GetJson
				json.push_back(temp);
			}
			hasChanged = false;
		}
		void UpdateJson(nlohmann::json json) override
		{
			for (const auto& data : json["Changes"])
			{
				if (data["ID"] == this->ID)
				{
					value.clear();
					auto& temp = data["Value"];
					for (auto& a : temp)
						value.push_back((uint8_t)a);
					break;
				}
			}
		}
		inline ~BoolListSetting()
		{
		}
	};

	class StringListSetting : public Setting
	{
	private:
		std::vector<std::string> defaultValue;
		std::list<char*> value;
		char* buffer = new char[MAX_PATH] {};
		std::string remove = "";
	public:
		inline void RenderImGui() override
		{
			ImGui::SetNextItemWidth(width);
			ImGui::PushID(ID::nextID());
			if (ImGui::BeginCombo("", this->name.c_str()))
			{
				//ImGui::SetNextItemWidth(width);
				ImGui::PushID(ID::nextID());
				ImGui::InputText("", buffer, MAX_PATH);
				ImGui::PopID();
				ImGui::PushID(ID::nextID());
				if (ImGui::Button("+", ImVec2(-1, 0)))
				{
					char* temp = new char[MAX_PATH] {};
					strcpy(temp, buffer);
					value.push_back(temp);
					delete[] temp; //IF THERE IS A PROBLEM IT IS HERE
					hasChanged = true;
				}
				ImGui::PopID();
				for (auto& a : value)
				{
					ImGui::PushID(ID::nextID());
					if (ImGui::Button((std::string("- ") + std::string(a)).c_str()))
						remove = std::string(a);
					ImGui::PopID();
				}
				if (remove != "")
				{
					char* temp = new char[MAX_PATH];
					strcpy(temp, remove.c_str());
					value.remove_if([temp](const char* str) {
						return std::strcmp(str, temp) == 0;
						});
					delete[] temp;
					remove = "";
					hasChanged = true;
				}
				ImGui::EndCombo();
			}
			ImGui::PopID();
			if (ImGui::IsItemHovered())
				ImGui::SetItemTooltip(this->tooltip.c_str());
		}
		inline void ReloadDefault() override
		{
			for (auto& a : value)
				delete[] a;
			value.clear();
			for (auto& a : defaultValue)
			{
				char* temp = new char[MAX_PATH];
				strcpy(temp, a.c_str());
				value.push_back(temp);
			}
			hasChanged = true;
		}
		inline void LoadFromJson(nlohmann::json data) override
		{
			this->name = data["Name"];
			this->ID = data["ID"];
			this->tooltip = data["Tooltip"];
			auto& temp = data["Default"];
			for (const auto& a : temp)
				defaultValue.push_back(a);
			temp = data["Value"];
			int i = 0;
			for (const auto& a : temp)
			{
				char* temp = new char[MAX_PATH] {};
				strcpy(temp, std::string(a).c_str());
				value.push_back(temp);
			}
		}
		inline void GetJson(nlohmann::json& json, bool all) override
		{
			if (hasChanged.isChanged() || all)
			{
				nlohmann::json temp;
				temp["ID"] = this->ID;
				for (const auto& a : value)
					temp["Value"].push_back(std::string(a));
				//placeholder for replace in GetJson
				json.push_back(temp);
			}
			hasChanged = false;
		}
		void UpdateJson(nlohmann::json json) override
		{
			for (const auto& data : json["Changes"])
			{
				if (data["ID"] == this->ID)
				{
					for (const auto& a : value)
						delete[] a;
					auto& temp = data["Value"];
					int i = 0;
					for (const auto& a : temp)
					{
						char* temp = new char[MAX_PATH] {};
						strcpy(temp, std::string(a).c_str());
						value.push_back(temp);
					}
					break;
				}
			}
		}
		inline ~StringListSetting()
		{
			for (const auto& a : value)
				delete[] a;
			delete[] buffer;
		}
	};

	class EnumeratorSetting : public Setting
	{
	private:
		int defaultValue;
		int value;
		std::vector<std::string> identifiers;
	public:
		inline void RenderImGui() override
		{
			ImGui::SetNextItemWidth(width);
			if (ImGui::BeginCombo(this->name.c_str(), identifiers[value].c_str()))
			{
				int i = 0;
				for (const auto& a : identifiers)
				{
					ImGui::PushID(ID::nextID());
					if (ImGui::RadioButton(a.c_str(), &value, i++))
						hasChanged = true;
					ImGui::PopID();
				}
				ImGui::EndCombo();
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip(this->tooltip.c_str());
		}
		inline void ReloadDefault() override
		{
			value = defaultValue;
			hasChanged = true;
		}
		inline void LoadFromJson(nlohmann::json data) override
		{
			this->name = data["Name"];
			this->ID = data["ID"];
			this->tooltip = data["Tooltip"];
			defaultValue = data["Default"];
			value = data["Value"];
			for (const auto& a : data["Identifiers"])
				identifiers.push_back(a);
		}
		inline void GetJson(nlohmann::json& json, bool all) override
		{
			if (hasChanged.isChanged() || all)
			{
				nlohmann::json temp;
				temp["ID"] = this->ID;
				temp["Value"] = value;
				//placeholder for replace in GetJson
				json.push_back(temp);
			}
			hasChanged = false;
		}
		void UpdateJson(nlohmann::json json) override
		{
			for (const auto& data : json["Changes"])
			{
				if (data["ID"] == this->ID)
				{
					value = data["Value"];
					break;
				}
			}
		}
	};

	class ColorSetting : public Setting
	{
	private:
		float defaultValue[4]{};
		float value[4]{};
	public:
		inline void RenderImGui() override
		{
			ImGui::PushID(ID::nextID());
			ImGui::SetNextItemWidth(9.0f / 5.0f * width);
			if (ImGui::ColorEdit3("", value, ImGuiColorEditFlags_NoSmallPreview))
				hasChanged = true;
			ImGui::PopID();
			ImGui::SameLine();
			ImGui::PushID(ID::nextID());
			if (ImGui::ColorEdit4("", value, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar))
				hasChanged = true;
			ImGui::PopID();
			RenderLabel();
		}
		inline void ReloadDefault() override
		{
			value[0] = defaultValue[0];
			value[1] = defaultValue[1];
			value[2] = defaultValue[2];
			value[3] = defaultValue[3];
			hasChanged = true;
		}
		inline void LoadFromJson(nlohmann::json data) override
		{
			this->name = data["Name"];
			this->ID = data["ID"];
			this->tooltip = data["Tooltip"];
			int i = 3;
			for (const auto& a : data["Default"])
				defaultValue[i--] = a / 255.0f;
			i = 3;
			for (const auto& a : data["Value"])
				value[i--] = a / 255.0f;
			//enableAlpha = data["EnableAlpha"];
		}
		inline void GetJson(nlohmann::json& json, bool all) override
		{
			if (hasChanged.isChanged() || all)
			{
				nlohmann::json temp;
				temp["ID"] = this->ID;
				temp["Value"]["R"] = (int)(255 * value[0]);
				temp["Value"]["G"] = (int)(255 * value[1]);
				temp["Value"]["B"] = (int)(255 * value[2]);
				temp["Value"]["A"] = (int)(255 * value[3]);
				//placeholder for replace in GetJson
				json.push_back(temp);
			}
			hasChanged = false;
		}
		void UpdateJson(nlohmann::json json) override
		{
			for (const auto& data : json["Changes"])
			{
				if (data["ID"] == this->ID)
				{
					int i = 3;
					for (const auto& a : data["Value"])
						value[i--] = a / 255.0f;
					break;
				}
			}
		}
	};

	//kill me (double check)
	class ColorTransitionSetting : public Setting
	{
	private:
		int defaultValuePeriod;
		int defaultValueOffset;
		bool defaultValueDiscrete;
		std::vector<std::array<float, 4>> defaultValueColors;
		int valuePeriod;
		int valueOffset;
		bool valueDiscrete;
		std::list<std::array<float, 4>> valueColors;
		float* remove = nullptr;
		float simpleColor[3]{};
		bool enableAdvanced = false;
		int colorIdx = 0;

		inline void drawGradient(float x1, float x2, float y1, float y2, std::array<float, 4> Lcolor, std::array<float, 4> Rcolor)
		{
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			drawList->AddRectFilledMultiColor(ImVec2(x1, y1), ImVec2(x2, y2),
				FLOAT_COLOR(Lcolor), FLOAT_COLOR(Rcolor), FLOAT_COLOR(Rcolor), FLOAT_COLOR(Lcolor));
		}
		inline void drawRect(float x1, float x2, float y1, float y2, std::array<float, 4> color)
		{
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			drawList->AddRectFilled(ImVec2(x1, y1), ImVec2(x2, y2), FLOAT_COLOR(color));
		}
		inline void drawFullGradient(float width, float height)
		{
			ImVec2 p = ImGui::GetCursorScreenPos();
			if (valueColors.size() == 1)
			{
				drawGradient(p.x, p.x + width, p.y, p.y + height, valueColors.front(), valueColors.front());
				return;
			}
			float stepSize = width / (valueColors.size() - 1);
			float currX = 0.0f;
			float nextX = stepSize;
			std::array<float, 4> prevColor;
			bool first = true;
			for (auto& currColor : valueColors)
			{
				if (first)
				{
					first = false;
					prevColor[0] = currColor[0];
					prevColor[1] = currColor[1];
					prevColor[2] = currColor[2];
					prevColor[3] = currColor[3];
					continue;
				}
				drawGradient(p.x + currX, p.x + nextX, p.y, p.y + height, prevColor, currColor);
				currX = nextX;
				nextX += stepSize;
				prevColor[0] = currColor[0];
				prevColor[1] = currColor[1];
				prevColor[2] = currColor[2];
				prevColor[3] = currColor[3];
			}
			//ImGui::Dummy(ImVec2(width,height));
			//ImGui::SetCursorScreenPos(ImVec2(width, 0));
		}
		inline void drawFullDiscrete(float width, float height)
		{
			ImVec2 p = ImGui::GetCursorScreenPos();
			if (valueColors.size() == 1)
			{
				drawRect(p.x, p.x + width, p.y, p.y + height, valueColors.front());
				return;
			}
			float stepSize = width / valueColors.size();
			float xOff = 0.0f;
			for (auto& c : valueColors)
			{
				drawRect(p.x + xOff, p.x + xOff + stepSize, p.y, p.y + height, c);
				xOff += stepSize;
			}
		}
	public:
		inline void RenderImGui() override
		{
			//enableAdvanced - RGB - Color - Label
			ImGui::PushID(ID::nextID());
			if (ImGui::Checkbox("", &enableAdvanced))
				hasChanged = true;
			ImGui::PopID();
			ImGui::SetItemTooltip("enable advanced color edit");
			ImGui::SameLine();
			if (!enableAdvanced)
			{
				ImGui::PushID(ID::nextID());
				ImGui::SetNextItemWidth(9.0f / 5.0f * width);
				if (ImGui::ColorEdit3("", valueColors.front().data(), ImGuiColorEditFlags_NoSmallPreview))
					hasChanged = true;
				ImGui::PopID();
				ImGui::SameLine();
				ImGui::PushID(ID::nextID());
				if (ImGui::ColorEdit4("", valueColors.front().data(), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar))
					hasChanged = true;
				ImGui::PopID();
			}
			else //advanced mode :(
			{
				float lineHeight = ImGui::GetTextLineHeight();
				float padding = ImGui::GetStyle().FramePadding.y;
				float height = lineHeight + 2 * padding;

				if (valueDiscrete)
					drawFullDiscrete(width, height);
				else
					drawFullGradient(width, height);
				ImGui::PushID(ID::nextID());
				ImGui::PushItemWidth(width);
				ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
				ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
				if (ImGui::BeginCombo("", "", ImGuiComboFlags_NoArrowButton))
				{
					ImGui::PopStyleColor(2);
					ImGui::SetNextItemWidth(width);
					ImGui::PushID(ID::nextID());
					if (ImGui::Checkbox("discrete", &valueDiscrete))
						hasChanged = true;
					ImGui::PopID();
					ImGui::SetNextItemWidth(width);
					ImGui::PushID(ID::nextID());
					if (ImGui::DragInt("period", &valuePeriod, 5000 / divideRange, 0, 5000))
						hasChanged = true;
					ImGui::PopID();
					ImGui::SetNextItemWidth(width);
					ImGui::PushID(ID::nextID());
					if (ImGui::DragInt("offset", &valueOffset, 5000 / divideRange, 0, 5000))
						hasChanged = true;
					ImGui::PopID();
					ImGui::PushID(ID::nextID());
					if (ImGui::Button("add color"))
					{
						valueColors.push_back(std::array<float, 4>{1.0, 1.0, 1.0, 1.0});
						hasChanged = true;
					}
					ImGui::PopID();

					for (auto& a : valueColors)
					{
						ImGui::PushID(ID::nextID());
						if (ImGui::ColorEdit4("", a.data(), ImGuiColorEditFlags_NoInputs))
							hasChanged = true;
						if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && valueColors.size() > 1)
							remove = a.data();
						ImGui::PopID();
						ImGui::SameLine();
					}
					if (remove != nullptr)
					{
						float* temp = remove;
						valueColors.remove_if([temp](std::array<float, 4>& a) {
							return (temp == a.data());
							});
						remove = nullptr;
						hasChanged = true;
					}

					ImGui::EndCombo();
				}
				else
					ImGui::PopStyleColor(2);
				ImGui::PopID();
			}
			RenderLabel();
		}
		inline void ReloadDefault() override
		{
			valuePeriod = defaultValuePeriod;
			valueOffset = defaultValueOffset;
			valueDiscrete = defaultValueDiscrete;
			int i = 0;
			valueColors.clear();
			for (auto& a : defaultValueColors)
			{
				std::array<float, 4> temp;
				temp[0] = defaultValueColors[i][0];
				temp[1] = defaultValueColors[i][1];
				temp[2] = defaultValueColors[i][2];
				temp[3] = defaultValueColors[i++][3];
				valueColors.push_back(temp);
			}
			hasChanged = true;
			if (valueColors.size() > 1)
				enableAdvanced = true;
			else
				enableAdvanced = false;
		}
		inline void LoadFromJson(nlohmann::json data) override
		{
			this->name = data["Name"];
			this->ID = data["ID"];
			this->tooltip = data["Tooltip"];
			defaultValuePeriod = data["Default"]["Period"];
			defaultValueOffset = data["Default"]["Offset"];
			defaultValueDiscrete = data["Default"]["Discrete"];
			for (const auto& a : data["Default"]["Colors"])
			{
				//a = [r,g,b,a]
				int i = 3;
				std::array<float, 4> temp{};
				for (const auto& b : a)
					temp[i--] = b / 255.0f;
				defaultValueColors.push_back(temp);
			}
			valuePeriod = data["Value"]["Period"];
			valueOffset = data["Value"]["Offset"];
			valueDiscrete = data["Value"]["Discrete"];
			for (const auto& a : data["Value"]["Colors"])
			{
				//a = [r,g,b,a]
				int i = 3;
				std::array<float, 4> temp{};
				for (const auto& b : a)
					temp[i--] = b / 255.0f;
				valueColors.push_back(temp);
			}
			if (valueColors.size() > 1)
				enableAdvanced = true;
		}
		inline void GetJson(nlohmann::json& json, bool all) override
		{
			//FIX THIS
			if (hasChanged.isChanged() || all)
			{
				nlohmann::json temp;
				temp["ID"] = this->ID;
				temp["Value"]["Period"] = valuePeriod;
				temp["Value"]["Offset"] = valueOffset;
				temp["Value"]["Discrete"] = valueDiscrete;
				for (const auto& a : valueColors)
				{
					nlohmann::json currColor;
					currColor["R"] = (int)(255 * a[0]);
					currColor["G"] = (int)(255 * a[1]);
					currColor["B"] = (int)(255 * a[2]);
					currColor["A"] = (int)(255 * a[3]);
					temp["Value"]["Colors"].push_back(currColor);
					if (!enableAdvanced)
						break;
				}
				//placeholder for replace in GetJson
				json.push_back(temp);
			}
			hasChanged = false;
		}
		void UpdateJson(nlohmann::json json) override
		{
			for (const auto& data : json["Changes"])
			{
				if (data["ID"] == this->ID)
				{
					valueColors.clear();
					valuePeriod = data["Value"]["Period"];
					valueOffset = data["Value"]["Offset"];
					valueDiscrete = data["Value"]["Discrete"];
					for (const auto& a : data["Value"]["Colors"])
					{
						//a = [r,g,b,a]
						int i = 3;
						std::array<float, 4> temp{};
						for (const auto& b : a)
							temp[i--] = b / 255.0f;
						valueColors.push_back(temp);
					}
					break;
				}
			}
		}
	};

	class InputBoxSetting : public Setting
	{
	private:
		std::string defaultValueLabel;
		int defaultValue[4]{};
		char* valueLabel = new char[10];
		int value[4]{};
	public:
		inline void RenderImGui() override
		{
			ImGui::NewLine();
			RenderLabel();
			ImGui::SetNextItemWidth(width);
			ImGui::PushID(ID::nextID());
			if (ImGui::InputText("label", valueLabel, 10))
				hasChanged = true;
			ImGui::PopID();

			ImGui::SetNextItemWidth(width);
			ImGui::PushID(ID::nextID());
			if (ImGui::DragInt2("position", value))
				hasChanged = true;
			ImGui::SetNextItemWidth(width);
			if (ImGui::DragInt2("size", &value[2], 1, 0, 1 << 31 - 1))
				hasChanged = true;
			ImGui::PopID();
			//RenderLabel();
		}
		inline void ReloadDefault() override
		{
			strcpy(valueLabel, defaultValueLabel.c_str());
			value[0] = defaultValue[0];
			value[1] = defaultValue[1];
			value[2] = defaultValue[2];
			value[3] = defaultValue[3];
			hasChanged = true;
		}
		inline void LoadFromJson(nlohmann::json data) override
		{
			this->name = data["Name"];
			this->ID = data["ID"];
			this->tooltip = data["Tooltip"];
			defaultValueLabel = data["Default"]["Label"];
			defaultValue[0] = data["Default"]["X"];
			defaultValue[1] = data["Default"]["Y"];
			defaultValue[2] = data["Default"]["W"];
			defaultValue[3] = data["Default"]["H"];
			strcpy(valueLabel, std::string(data["Value"]["Label"]).c_str());
			value[0] = data["Value"]["X"];
			value[1] = data["Value"]["Y"];
			value[2] = data["Value"]["W"];
			value[3] = data["Value"]["H"];
		}
		inline void GetJson(nlohmann::json& json, bool all) override
		{
			if (hasChanged.isChanged() || all)
			{
				nlohmann::json temp;
				temp["ID"] = this->ID;
				nlohmann::json v;
				v["Label"] = valueLabel;
				v["X"] = value[0];
				v["Y"] = value[1];
				v["W"] = value[2];
				v["H"] = value[3];
				temp["Value"] = v;
				//placeholder for replace in GetJson
				json.push_back(temp);
			}
			hasChanged = false;
		}
		void UpdateJson(nlohmann::json json) override
		{
			for (const auto& data : json["Changes"])
			{
				if (data["ID"] == this->ID)
				{
					strcpy(valueLabel, std::string(data["Value"]["Label"]).c_str());
					value[0] = data["Value"]["X"];
					value[1] = data["Value"]["Y"];
					value[2] = data["Value"]["W"];
					value[3] = data["Value"]["H"];
					break;
				}
			}
		}
	};

	class CategorySetting : public Setting
	{
	private:
		std::vector<Setting*> subSettings;
		bool enabled = false;
		inline void RenderReset()
		{
			bool hasScroll = ImGui::GetCurrentWindow()->ScrollbarY;
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1, 1, 1, 0.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 0.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(1, 1, 1, 0.0f));
			if (hasScroll)
				ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::GetTextLineHeight() - 2 * ImGui::CalcTextSize(u8"\xe800").x); //2x for possible scroll bar
			else
				ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::GetTextLineHeight() - ImGui::CalcTextSize(u8"\xe800").x); //2x for possible scroll bar
			float temp = ImGui::GetTextLineHeight();
			ImGui::SetWindowFontScale(0.75f);
			ImGui::PushID(ID::nextID());
			if (ImGui::Button(u8"\xe800", ImVec2(temp, temp)))
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
			ImGui::SetWindowFontScale(1.0f);
			if (ImGui::TreeNode(this->name.c_str()))
			{
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip(this->tooltip.c_str());
				RenderReset();
				for (const auto& a : subSettings)
					a->RenderImGui();
				ImGui::TreePop();
			}
			else
			{
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip(this->tooltip.c_str());
				RenderReset();
			}
		}
		inline void ReloadDefault() override
		{
			for (const auto& a : subSettings)
			{
				a->ReloadDefault();
			}
		}
		inline void LoadFromJson(nlohmann::json data) override
		{
			this->name = data["Name"];
			this->tooltip = data["Tooltip"];
			for (const auto a : data["Value"])
			{
				Setting* currS = nullptr;
				int type = a["Type"];
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
					currS->LoadFromJson(a);
				subSettings.push_back(currS);
			}
		}
		inline void GetJson(nlohmann::json& json, bool all) override
		{
			for (const auto& a : subSettings)
				a->GetJson(json, all);
		}
		void UpdateJson(nlohmann::json json) override
		{
			for (const auto& a : subSettings)
				a->UpdateJson(json);
		}
		inline ~CategorySetting()
		{
			for (const auto& a : subSettings)
				delete a;
		}
	};

	class SettingsModule
	{
	private:
		std::string name;
		std::vector<Setting*> settings;
		bool open = true;
		bool positionNext = false;
	public:
		inline void ResetLayout()
		{
			positionNext = true;
		}

		inline void RenderImGui()
		{
			if (positionNext)
			{
				ImVec4 place = Packing::placeWindow();
				ImGui::SetNextWindowPos(ImVec2(place.x, place.y));
				ImGui::SetNextWindowSize(ImVec2(place.z, place.w));
				positionNext = false;
			}
			if (ImGui::Begin(this->name.c_str(), &open))
			{
				int i = 0;
				for (const auto& setting : settings)
				{
					setting->RenderImGui();
				}
				ImGui::End();
			}
		}
		inline void ResetDefaults()
		{
			for (const auto& setting : settings)
			{
				setting->ReloadDefault();
			}
		}
		inline void setName(std::string name)
		{
			this->name = name;
		}
		inline void setOpen(bool open)
		{
			this->open = open;
		}
		inline bool& getOpen()
		{
			return open;
		}
		inline std::string getName()
		{
			return name;
		}
		inline void pushSetting(Setting* setting)
		{
			settings.push_back(setting);
		}
		inline void GetJson(nlohmann::json& json, bool all)
		{
			for (const auto& a : settings)
			{
				//if (!a->HasChanged())
				//	continue;
				a->GetJson(json, all);
			}
		}
		inline void UpdateJson(nlohmann::json json)
		{
			for (const auto& a : settings)
				a->UpdateJson(json);
		}
	};
}