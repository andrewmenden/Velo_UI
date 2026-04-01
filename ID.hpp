#ifndef ID_H
#define ID_H

namespace ID
{
	inline int id = 0;

	inline int nextID()
	{
		return id++;
	}

	inline void resetID()
	{
		id = 0;
	}
}

#endif