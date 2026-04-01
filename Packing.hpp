#ifndef PACKING_H
#define PACKING_H

#include <imgui.h>

class Packing
{
	constexpr static int minWidth = 250;
	constexpr static int maxModules = 20;

	int gap;
	int xCount;
	int yCount;
	int indexX = 0;
	int indexY = 0;
	ImVec2 size;
	ImVec2 position;
	
public:
	inline Packing(ImVec2 screenSize, int gap = 10)
	{
		this->gap = gap;
		xCount = screenSize.x / minWidth;
		yCount = max(std::ceil(maxModules / (float)xCount), 3);
		position = ImVec2{ (float)gap, (float)gap };
		size = ImVec2{
			(screenSize.x - gap * (xCount + 1)) / xCount,
			(screenSize.y - gap * (yCount + 1)) / yCount
		};
	}

	inline ImVec4 nextWindow()
	{
		ImVec4 out{
			gap + indexX * (size.x + gap),
			(indexY + 1) * gap + (indexY * size.y),
			size.x,
			size.y
		};

		indexX++;
		if (indexX >= xCount)
		{
			indexX = 0;
			indexY++;
		}
		return out;
	}
};

#endif