#pragma once

namespace ID
{
	int id = 0;
	int nextID()
	{
		return id++;
	}
	void resetID()
	{
		id = 0;
	}
}