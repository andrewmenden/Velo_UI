#ifndef KEYS_H
#define KEYS_H

#include <string>
#include <unordered_map>

#include "Global.hpp"

constexpr uint16_t unmappedKey = 0x97;
constexpr uint16_t modifierShift = 0x100;
constexpr uint16_t modifierCtrl = 0x200;
constexpr uint16_t modifierGamePad = 0x400;
constexpr uint16_t modifierAny = 0x800;

inline const std::unordered_map<uint16_t, const char*> keyStringMap =
{
	{ VK_LBUTTON,     "LBUTTON" },
	{ VK_RBUTTON,     "RBUTTON" },
	{ VK_MBUTTON,     "MBUTTON" },
	{ VK_XBUTTON1,    "XBUTTON1" },
	{ VK_XBUTTON2,    "XBUTTON2" },
	{ VK_BACK,        "BACKSPACE" },
	{ VK_TAB,        "TAB" },
	{ VK_CLEAR,        "CLEAR" },
	{ VK_RETURN,    "ENTER" },
	{ VK_SHIFT,        "SHIFT" },
	{ VK_CONTROL,    "CTRL" },
	{ VK_MENU,        "ALT" },
	{ VK_PAUSE,        "PAUSE" },
	{ VK_CAPITAL,    "CAPS LOCK" },
	{ VK_ESCAPE,    "ESC" },
	{ VK_SPACE,        "SPACE" },
	{ VK_PRIOR,        "PAGE UP" },
	{ VK_NEXT,        "PAGE DOWN" },
	{ VK_END,        "END" },
	{ VK_HOME,        "HOME" },
	{ VK_LEFT,        "LEFT" },
	{ VK_UP,        "UP" },
	{ VK_RIGHT,        "RIGHT" },
	{ VK_DOWN,        "DOWN" },
	{ VK_SELECT,    "SELECT" },
	{ VK_PRINT,        "PRINT" },
	{ VK_EXECUTE,    "EXECUTE" },
	{ VK_SNAPSHOT,    "PRINT SCREEN" },
	{ VK_INSERT,    "INS" },
	{ VK_DELETE,    "DEL" },
	{ VK_HELP,        "HELP" },
	{ VK_LWIN,        "LEFT WIN" },
	{ VK_RWIN,        "RIGHT WIN" },
	{ VK_SLEEP,        "SLEEP" },
	{ VK_NUMPAD0,    "0 (NUM)" },
	{ VK_NUMPAD1,    "1 (NUM)" },
	{ VK_NUMPAD2,    "2 (NUM)" },
	{ VK_NUMPAD3,    "3 (NUM)" },
	{ VK_NUMPAD4,    "4 (NUM)" },
	{ VK_NUMPAD5,    "5 (NUM)" },
	{ VK_NUMPAD6,    "6 (NUM)" },
	{ VK_NUMPAD7,    "7 (NUM)" },
	{ VK_NUMPAD8,    "8 (NUM)" },
	{ VK_NUMPAD9,    "9 (NUM)" },
	{ VK_MULTIPLY,    "* (NUM)" },
	{ VK_ADD,        "+ (NUM)" },
	{ VK_SEPARATOR, "?" },
	{ VK_SUBTRACT,    "- (NUM)" },
	{ VK_DECIMAL,    ". (NUM)" },
	{ VK_DIVIDE,    "/ (NUM)" },
	{ VK_F1,        "F1" },
	{ VK_F2,        "F2" },
	{ VK_F3,        "F3" },
	{ VK_F4,        "F4" },
	{ VK_F5,        "F5" },
	{ VK_F6,        "F6" },
	{ VK_F7,        "F7" },
	{ VK_F8,        "F8" },
	{ VK_F9,        "F9" },
	{ VK_F10,        "F10" },
	{ VK_F11,        "F11" },
	{ VK_F12,        "F12" },
	{ VK_F13,        "F13" },
	{ VK_F14,        "F14" },
	{ VK_F15,        "F15" },
	{ VK_F16,        "F16" },
	{ VK_F17,        "F17" },
	{ VK_F18,        "F18" },
	{ VK_F19,        "F19" },
	{ VK_F20,        "F20" },
	{ VK_F21,        "F21" },
	{ VK_F22,        "F22" },
	{ VK_F23,        "F23" },
	{ VK_F24,        "F24" },
	{ VK_NUMLOCK,    "NUM LOCK" },
	{ VK_SCROLL,    "SCROLL LOCK" },
	{ VK_LSHIFT,    "LEFT SHIFT" },
	{ VK_RSHIFT,    "RIGHT SHIFT" },
	{ VK_LCONTROL,    "LEFT CTRL" },
	{ VK_RCONTROL,    "RIGHT CTRL" },
	{ VK_LMENU,        "LEFT ALT" },
	{ VK_HOME,        "RIGHT ALT" },
	{ unmappedKey,            "N/A" }
};

inline const std::unordered_map<uint16_t, const char*> gamePadButtonStringMap =
{
	{ 0, "A" },
	{ 1, "B" },
	{ 2, "X" },
	{ 3, "Y" },
	{ 4, "LEFT SHOULDER" },
	{ 5, "RIGHT SHOULDER" },
	{ 6, "LEFT STICK" },
	{ 7, "RIGHT STICK" },
	{ 8, "START" },
	{ 9, "BACK" },
	{ 10, "BIG BUTTON" },
	{ 11, "TRIGGER LEFT" },
	{ 12, "TRIGGER RIGHT" },
	{ 13, "DPAD LEFT" },
	{ 14, "DPAD RIGHT" },
	{ 15, "DPAD UP" },
	{ 16, "DPAD DOWN" }
};

inline std::string keyCodeToString(uint16_t keyCode)
{
	if (keyCode & modifierGamePad)
	{
		auto it = gamePadButtonStringMap.find(keyCode & 0xff);
		if (it == gamePadButtonStringMap.end())
			return "N/A";
		return it->second;
	}

	bool shift = (keyCode & modifierShift) != 0;
	bool ctrl = (keyCode & modifierCtrl) != 0;
	bool invariant = (keyCode & modifierAny) != 0;
	keyCode &= 0xff;

	auto it = keyStringMap.find(keyCode);

	std::string name;

	if (it != keyStringMap.end())
		name = it->second;
	else
	{
		std::array<wchar_t, 100> bufferw{};
		size_t length = GetKeyNameTextW(MapVirtualKeyW(keyCode, MAPVK_VK_TO_VSC) << 16, bufferw.data(), bufferw.size());

		std::array<char, 100> buffer{};
		length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, bufferw.data(), length, buffer.data(), buffer.size(), NULL, NULL);

		if (length == 0)
			name = "N/A";
		else
			name = std::string(buffer.data(), length);
	}
	if (invariant)
	{
		name = "ANY+" + name;
	}
	else
	{
		if (shift)
		{
			name = "SHIFT+" + name;
		}
		if (ctrl)
		{
			name = "CTRL+" + name;
		}
	}
	return name;
}

inline uint16_t getPressedKey(uint16_t ignore)
{
	for (uint16_t b = 0; b < static_cast<uint16_t>(Global::gamePadButtonsDown.size()); b++)
	{
		if (Global::gamePadButtonsDown[b] && (b | modifierGamePad) != ignore)
			return b | modifierGamePad;
	}

	uint16_t shiftMod =
		GetAsyncKeyState(VK_LSHIFT) & 0x8000 ||
		GetAsyncKeyState(VK_RSHIFT) & 0x8000 || 
		GetAsyncKeyState(VK_SHIFT) & 0x8000 ? modifierShift : 0;
	uint16_t controlMod =
		GetAsyncKeyState(VK_LCONTROL) & 0x8000 ||
		GetAsyncKeyState(VK_RCONTROL) & 0x8000 ||
		GetAsyncKeyState(VK_CONTROL) & 0x8000 ? modifierCtrl : 0;

	uint16_t key = 0;

	for (uint16_t k = 8; k < 256; k++)
	{
		if (k == VK_LSHIFT || k == VK_RSHIFT || k == VK_SHIFT || k == VK_LCONTROL || k == VK_RCONTROL || k == VK_CONTROL)
			continue;

		if ((k | shiftMod | controlMod) != ignore && GetAsyncKeyState(k) & 0x8000)
		{
			key = k;
			break;
		}
	}

	if (key == 0)
		return 0;

	return key | shiftMod | controlMod;
}

#endif