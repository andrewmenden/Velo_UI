#ifndef PACKING_H
#define PACKING_H

#define NOMINMAX

#include <imgui.h>

class Packing
{
	constexpr static float minWidth = 250;
	constexpr static size_t maxModules = 20;

	float gap;
	size_t xCount;
	size_t yCount;
	size_t indexX = 0;
	size_t indexY = 0;
	ImVec2 size;
	ImVec2 position;
	
public:
	inline Packing(ImVec2 screenSize, float gap = 10)
	{
		this->gap = gap;
		xCount = static_cast<size_t>(screenSize.x / minWidth);
		yCount = std::max<size_t>((maxModules + xCount - 1) / xCount, 3);
		position = ImVec2{ static_cast<float>(gap), static_cast<float>(gap) };
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