#pragma once

#include <list>
#include "imgui.h"
#include "imgui_internal.h"
#include "DebugConsole.hpp"

namespace Packing
{
	std::list<ImGuiWindow*> windows;
	int totalWidth;
	int totalHeight;
	int minWidth = 250;
	int gap;
	int xCount;
	int maxSettings = 20;
	int yCount = 4;
	int indexX = 0;
	int indexY = 0;
	ImVec2 size;
	ImVec2 position;

	void init(int width, int height, int gap = 10)
	{
		totalWidth = width;
		totalHeight = height;
		Packing::gap = gap;
		xCount = width / minWidth;
		yCount = max(std::ceil(maxSettings / (float)xCount),3);
		indexX = 0;
		indexY = 0;
		position = ImVec2(gap, gap);
		size = ImVec2(
			(totalWidth - gap * (xCount + 1)) / xCount,
			(totalHeight - gap * (yCount + 1)) / yCount
		);
	}
	void init(ImVec2 Size, int gap = 10)
	{
		totalWidth = Size.x;
		totalHeight = Size.y;
		Packing::gap = gap;
		xCount = Size.x / minWidth;
		yCount = max(std::ceil(maxSettings / (float)xCount), 3);
		indexX = 0;
		indexY = 0;
		position = ImVec2(gap, gap);
		size = ImVec2(
			(totalWidth - gap * (xCount + 1)) / xCount,
			(totalHeight - gap * (yCount + 1)) / yCount
		);
	}

	ImVec4 placeWindow()
	{
		ImVec4 out(
			gap + indexX * (size.x + gap),
			(indexY + 1) * gap + (indexY * size.y),
			size.x,
			size.y
		);

		indexX++;
		if (indexX >= xCount)
		{
			indexX = 0;
			indexY++;
		}
		return out;
	}
}
