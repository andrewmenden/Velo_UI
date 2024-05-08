#pragma once


class EventBool
{
private:
	bool hasChanged = false;
public:
	static bool AnyChange;

	inline EventBool()
	{
		hasChanged = false;
	}

	inline bool isChanged()
	{
		return hasChanged;
	}

	inline void operator=(const bool& rhs)
	{
		if (rhs)
		{
			EventBool::AnyChange = true;
		}
		hasChanged = rhs;
	}
	inline void operator=(const int& rhs)
	{
		if (rhs)
		{
			EventBool::AnyChange = true;
		}
		hasChanged = rhs;
	}
};

bool EventBool::AnyChange = false;