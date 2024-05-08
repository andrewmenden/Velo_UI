#pragma once

#include "Setting.hpp"
#include "ID.hpp"

namespace Program
{
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
}