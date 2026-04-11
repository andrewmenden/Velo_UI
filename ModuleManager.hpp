#ifndef MODULE_MANAGER_H
#define MODULE_MANAGER_H

#include <string>
#include <json.hpp>

#include "Global.hpp"

class Identifyable
{
protected:
	int id;

public:
	inline Identifyable(int id) : id{ id } {}

	virtual ~Identifyable() = default;

	inline const int& GetID() const
	{
		return id;
	}

	virtual void UpdateFromJson(const nlohmann::json&) = 0;
};

class ModuleManager;
class Setting;

struct Cycle
{
	ModuleManager& modules;
	nlohmann::json& changes;
	std::vector<Setting*> changedSettings;

	float inputWidth;
	uint16_t enableUiHotkey;
	int currentID = -1;

	inline int NextID()
	{
		return ++currentID;
	}

	inline int CurrentId() const
	{
		return currentID;
	}

	inline bool RemoveIfChanged(Setting* setting)
	{
		auto it = std::find(changedSettings.begin(), changedSettings.end(), setting);
		if (it != changedSettings.end())
		{
			changedSettings.erase(it);
			return true;
		}
		return false;
	}
};

class Module // abstract base
{
protected:
	std::string name;

public:
	bool enabled;

private:
	ImVec4 changeBounds{ -1.0f, -1.0f, -1.0f, -1.0f };

public:
	inline Module(std::string_view name) :
		name{ name },
		enabled{ true } {}

	inline const std::string& GetName() const
	{
		return name;
	}

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

public:
	inline virtual bool Visible() { return true; }
	inline virtual void RenderImGui(Cycle&) = 0;
	inline virtual void Update(Cycle&) {}; // called after RenderImGui
	inline virtual void UpdateSearch(std::string_view) = 0;
	inline virtual void ContextMenu(Identifyable&, Cycle&) {}
	inline virtual void Visit(const std::function<void(Identifyable&)>&) = 0;
};

std::unique_ptr<Module> JsonToModule(const nlohmann::json&);

class ModuleManager
{
	std::vector<std::unique_ptr<Module>> modules;
	std::unordered_map<int, Identifyable*> idToIdent;

public:
	inline decltype(auto) begin()
	{
		return modules.begin();
	}

	inline decltype(auto) end()
	{
		return modules.end();
	}

	inline decltype(auto) begin() const
	{
		return modules.begin();
	}

	inline decltype(auto) end() const
	{
		return modules.end();
	}

	inline decltype(auto) VisibleModules() const
	{
		return modules | std::views::filter([](const auto& m) { return m->Visible(); });
	}

	inline void Push(std::unique_ptr<Module>&& module)
	{
		modules.push_back(std::move(module));
	}

	inline Identifyable* GetIdent(int id) const
	{
		auto it = idToIdent.find(id);
		if (it != idToIdent.end())
			return it->second;
		return nullptr;
	}

	inline void UpdateFromJson(const nlohmann::json& json)
	{
		for (const auto& data : json["Changes"])
		{
			int id = data["ID"];
			auto it = idToIdent.find(id);
			if (it != idToIdent.end())
				it->second->UpdateFromJson(data);
		}
	}

	inline void LoadFromJson(const nlohmann::json& data)
	{
		for (const auto& m : data["Modules"])
		{
			modules.push_back(JsonToModule(m));
			modules.back()->Visit([&](Identifyable& ident)
				{
					idToIdent.emplace(ident.GetID(), &ident);
				});
		}
	}

	inline void ContextMenu(Identifyable& ident, Cycle& cycle)
	{
		for (const auto& m : modules)
			m->ContextMenu(ident, cycle);
	}
};

#endif