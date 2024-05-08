#pragma onces

#include "imgui.h"
#include <Windows.h>
#include <string>
#include <unordered_map>

#define IM_VK_KEYPAD_ENTER (VK_RETURN + 256)

namespace Keys
{
	static ImGuiKey VirtualKeyToImGuiKey(uint8_t key)
	{
		switch (key)
		{
		case VK_TAB: return ImGuiKey_Tab;
		case VK_LEFT: return ImGuiKey_LeftArrow;
		case VK_RIGHT: return ImGuiKey_RightArrow;
		case VK_UP: return ImGuiKey_UpArrow;
		case VK_DOWN: return ImGuiKey_DownArrow;
		case VK_PRIOR: return ImGuiKey_PageUp;
		case VK_NEXT: return ImGuiKey_PageDown;
		case VK_HOME: return ImGuiKey_Home;
		case VK_END: return ImGuiKey_End;
		case VK_INSERT: return ImGuiKey_Insert;
		case VK_DELETE: return ImGuiKey_Delete;
		case VK_BACK: return ImGuiKey_Backspace;
		case VK_SPACE: return ImGuiKey_Space;
		case VK_RETURN: return ImGuiKey_Enter;
		case VK_ESCAPE: return ImGuiKey_Escape;
		case VK_OEM_7: return ImGuiKey_Apostrophe;
		case VK_OEM_COMMA: return ImGuiKey_Comma;
		case VK_OEM_MINUS: return ImGuiKey_Minus;
		case VK_OEM_PERIOD: return ImGuiKey_Period;
		case VK_OEM_2: return ImGuiKey_Slash;
		case VK_OEM_1: return ImGuiKey_Semicolon;
		case VK_OEM_PLUS: return ImGuiKey_Equal;
		case VK_OEM_4: return ImGuiKey_LeftBracket;
		case VK_OEM_5: return ImGuiKey_Backslash;
		case VK_OEM_6: return ImGuiKey_RightBracket;
		case VK_OEM_3: return ImGuiKey_GraveAccent;
		case VK_CAPITAL: return ImGuiKey_CapsLock;
		case VK_SCROLL: return ImGuiKey_ScrollLock;
		case VK_NUMLOCK: return ImGuiKey_NumLock;
		case VK_SNAPSHOT: return ImGuiKey_PrintScreen;
		case VK_PAUSE: return ImGuiKey_Pause;
		case VK_NUMPAD0: return ImGuiKey_Keypad0;
		case VK_NUMPAD1: return ImGuiKey_Keypad1;
		case VK_NUMPAD2: return ImGuiKey_Keypad2;
		case VK_NUMPAD3: return ImGuiKey_Keypad3;
		case VK_NUMPAD4: return ImGuiKey_Keypad4;
		case VK_NUMPAD5: return ImGuiKey_Keypad5;
		case VK_NUMPAD6: return ImGuiKey_Keypad6;
		case VK_NUMPAD7: return ImGuiKey_Keypad7;
		case VK_NUMPAD8: return ImGuiKey_Keypad8;
		case VK_NUMPAD9: return ImGuiKey_Keypad9;
		case VK_DECIMAL: return ImGuiKey_KeypadDecimal;
		case VK_DIVIDE: return ImGuiKey_KeypadDivide;
		case VK_MULTIPLY: return ImGuiKey_KeypadMultiply;
		case VK_SUBTRACT: return ImGuiKey_KeypadSubtract;
		case VK_ADD: return ImGuiKey_KeypadAdd;
		case IM_VK_KEYPAD_ENTER: return ImGuiKey_KeypadEnter;
		case VK_LSHIFT: return ImGuiKey_LeftShift;
		case VK_LCONTROL: return ImGuiKey_LeftCtrl;
		case VK_LMENU: return ImGuiKey_LeftAlt;
		case VK_LWIN: return ImGuiKey_LeftSuper;
		case VK_RSHIFT: return ImGuiKey_RightShift;
		case VK_RCONTROL: return ImGuiKey_RightCtrl;
		case VK_RMENU: return ImGuiKey_RightAlt;
		case VK_RWIN: return ImGuiKey_RightSuper;
		case VK_APPS: return ImGuiKey_Menu;
		case '0': return ImGuiKey_0;
		case '1': return ImGuiKey_1;
		case '2': return ImGuiKey_2;
		case '3': return ImGuiKey_3;
		case '4': return ImGuiKey_4;
		case '5': return ImGuiKey_5;
		case '6': return ImGuiKey_6;
		case '7': return ImGuiKey_7;
		case '8': return ImGuiKey_8;
		case '9': return ImGuiKey_9;
		case 'A': return ImGuiKey_A;
		case 'B': return ImGuiKey_B;
		case 'C': return ImGuiKey_C;
		case 'D': return ImGuiKey_D;
		case 'E': return ImGuiKey_E;
		case 'F': return ImGuiKey_F;
		case 'G': return ImGuiKey_G;
		case 'H': return ImGuiKey_H;
		case 'I': return ImGuiKey_I;
		case 'J': return ImGuiKey_J;
		case 'K': return ImGuiKey_K;
		case 'L': return ImGuiKey_L;
		case 'M': return ImGuiKey_M;
		case 'N': return ImGuiKey_N;
		case 'O': return ImGuiKey_O;
		case 'P': return ImGuiKey_P;
		case 'Q': return ImGuiKey_Q;
		case 'R': return ImGuiKey_R;
		case 'S': return ImGuiKey_S;
		case 'T': return ImGuiKey_T;
		case 'U': return ImGuiKey_U;
		case 'V': return ImGuiKey_V;
		case 'W': return ImGuiKey_W;
		case 'X': return ImGuiKey_X;
		case 'Y': return ImGuiKey_Y;
		case 'Z': return ImGuiKey_Z;
		case VK_F1: return ImGuiKey_F1;
		case VK_F2: return ImGuiKey_F2;
		case VK_F3: return ImGuiKey_F3;
		case VK_F4: return ImGuiKey_F4;
		case VK_F5: return ImGuiKey_F5;
		case VK_F6: return ImGuiKey_F6;
		case VK_F7: return ImGuiKey_F7;
		case VK_F8: return ImGuiKey_F8;
		case VK_F9: return ImGuiKey_F9;
		case VK_F10: return ImGuiKey_F10;
		case VK_F11: return ImGuiKey_F11;
		case VK_F12: return ImGuiKey_F12;
		case VK_F13: return ImGuiKey_F13;
		case VK_F14: return ImGuiKey_F14;
		case VK_F15: return ImGuiKey_F15;
		case VK_F16: return ImGuiKey_F16;
		case VK_F17: return ImGuiKey_F17;
		case VK_F18: return ImGuiKey_F18;
		case VK_F19: return ImGuiKey_F19;
		case VK_F20: return ImGuiKey_F20;
		case VK_F21: return ImGuiKey_F21;
		case VK_F22: return ImGuiKey_F22;
		case VK_F23: return ImGuiKey_F23;
		case VK_F24: return ImGuiKey_F24;
		case VK_BROWSER_BACK: return ImGuiKey_AppBack;
		case VK_BROWSER_FORWARD: return ImGuiKey_AppForward;
		default: return ImGuiKey_None;
		}
	}
	static uint8_t ImGuiKeyToVirtualKey(int key)
	{
		switch (key)
		{
		case ImGuiKey_Tab: return VK_TAB;
		case ImGuiKey_LeftArrow: return VK_LEFT;
		case ImGuiKey_RightArrow: return VK_RIGHT;
		case ImGuiKey_UpArrow: return VK_UP;
		case ImGuiKey_DownArrow: return VK_DOWN;
		case ImGuiKey_PageUp: return VK_PRIOR;
		case ImGuiKey_PageDown: return VK_NEXT;
		case ImGuiKey_Home: return VK_HOME;
		case ImGuiKey_End: return VK_END;
		case ImGuiKey_Insert: return VK_INSERT;
		case ImGuiKey_Delete: return VK_DELETE;
		case ImGuiKey_Backspace: return VK_BACK;
		case ImGuiKey_Space: return VK_SPACE;
		case ImGuiKey_Enter: return VK_RETURN;
		case ImGuiKey_Escape: return VK_ESCAPE;
		case ImGuiKey_Apostrophe: return VK_OEM_7;
		case ImGuiKey_Comma: return VK_OEM_COMMA;
		case ImGuiKey_Minus: return VK_OEM_MINUS;
		case ImGuiKey_Period: return VK_OEM_PERIOD;
		case ImGuiKey_Slash: return VK_OEM_2;
		case ImGuiKey_Semicolon: return VK_OEM_1;
		case ImGuiKey_Equal: return VK_OEM_PLUS;
		case ImGuiKey_LeftBracket: return VK_OEM_4;
		case ImGuiKey_Backslash: return VK_OEM_5;
		case ImGuiKey_RightBracket: return VK_OEM_6;
		case ImGuiKey_GraveAccent: return VK_OEM_3;
		case ImGuiKey_CapsLock: return VK_CAPITAL;
		case ImGuiKey_ScrollLock: return VK_SCROLL;
		case ImGuiKey_NumLock: return VK_NUMLOCK;
		case ImGuiKey_PrintScreen: return VK_SNAPSHOT;
		case ImGuiKey_Pause: return VK_PAUSE;
		case ImGuiKey_Keypad0: return VK_NUMPAD0;
		case ImGuiKey_Keypad1: return VK_NUMPAD1;
		case ImGuiKey_Keypad2: return VK_NUMPAD2;
		case ImGuiKey_Keypad3: return VK_NUMPAD3;
		case ImGuiKey_Keypad4: return VK_NUMPAD4;
		case ImGuiKey_Keypad5: return VK_NUMPAD5;
		case ImGuiKey_Keypad6: return VK_NUMPAD6;
		case ImGuiKey_Keypad7: return VK_NUMPAD7;
		case ImGuiKey_Keypad8: return VK_NUMPAD8;
		case ImGuiKey_Keypad9: return VK_NUMPAD9;
		case ImGuiKey_KeypadDecimal: return VK_DECIMAL;
		case ImGuiKey_KeypadDivide: return VK_DIVIDE;
		case ImGuiKey_KeypadMultiply: return VK_MULTIPLY;
		case ImGuiKey_KeypadSubtract: return VK_SUBTRACT;
		case ImGuiKey_KeypadAdd: return VK_ADD;
		case ImGuiKey_KeypadEnter: return IM_VK_KEYPAD_ENTER;
		case ImGuiKey_LeftShift: return VK_LSHIFT;
		case ImGuiKey_LeftCtrl: return VK_LCONTROL;
		case ImGuiKey_LeftAlt: return VK_LMENU;
		case ImGuiKey_LeftSuper: return VK_LWIN;
		case ImGuiKey_RightShift: return VK_RSHIFT;
		case ImGuiKey_RightCtrl: return VK_RCONTROL;
		case ImGuiKey_RightAlt: return VK_RMENU;
		case ImGuiKey_RightSuper: return VK_RWIN;
		case ImGuiKey_Menu: return VK_APPS;
		case  ImGuiKey_0: return '0';
		case ImGuiKey_1: return '1';
		case ImGuiKey_2: return '2';
		case ImGuiKey_3: return '3';
		case ImGuiKey_4: return '4';
		case ImGuiKey_5: return '5';
		case ImGuiKey_6: return '6';
		case ImGuiKey_7: return '7';
		case ImGuiKey_8: return '8';
		case ImGuiKey_9: return '9';
		case ImGuiKey_A: return 'A';
		case ImGuiKey_B: return 'B';
		case ImGuiKey_C: return 'C';
		case ImGuiKey_D: return 'D';
		case ImGuiKey_E: return 'E';
		case ImGuiKey_F: return 'F';
		case ImGuiKey_G: return 'G';
		case ImGuiKey_H: return 'H';
		case ImGuiKey_I: return 'I';
		case ImGuiKey_J: return 'J';
		case ImGuiKey_K: return 'K';
		case ImGuiKey_L: return 'L';
		case ImGuiKey_M: return 'M';
		case ImGuiKey_N: return 'N';
		case ImGuiKey_O: return 'O';
		case ImGuiKey_P: return 'P';
		case ImGuiKey_Q: return 'Q';
		case ImGuiKey_R: return 'R';
		case ImGuiKey_S: return 'S';
		case ImGuiKey_T: return 'T';
		case ImGuiKey_U: return 'U';
		case ImGuiKey_V: return 'V';
		case ImGuiKey_W: return 'W';
		case ImGuiKey_X: return 'X';
		case ImGuiKey_Y: return 'Y';
		case ImGuiKey_Z: return 'Z';
		case ImGuiKey_F1: return VK_F1;
		case ImGuiKey_F2: return VK_F2;
		case ImGuiKey_F3: return VK_F3;
		case ImGuiKey_F4: return VK_F4;
		case ImGuiKey_F5: return VK_F5;
		case ImGuiKey_F6: return VK_F6;
		case ImGuiKey_F7: return VK_F7;
		case ImGuiKey_F8: return VK_F8;
		case ImGuiKey_F9: return VK_F9;
		case ImGuiKey_F10: return VK_F10;
		case ImGuiKey_F11: return VK_F11;
		case ImGuiKey_F12: return VK_F12;
		case ImGuiKey_F13: return VK_F13;
		case ImGuiKey_F14: return VK_F14;
		case ImGuiKey_F15: return VK_F15;
		case ImGuiKey_F16: return VK_F16;
		case ImGuiKey_F17: return VK_F17;
		case ImGuiKey_F18: return VK_F18;
		case ImGuiKey_F19: return VK_F19;
		case ImGuiKey_F20: return VK_F20;
		case ImGuiKey_F21: return VK_F21;
		case ImGuiKey_F22: return VK_F22;
		case ImGuiKey_F23: return VK_F23;
		case ImGuiKey_F24: return VK_F24;
		case ImGuiKey_AppBack: return VK_BROWSER_BACK;
		case ImGuiKey_AppForward: return VK_BROWSER_FORWARD;
		default: return 0;
		}
	}
	static const char* KeyNames[] = {
	"OFF",
	"VK_LBUTTON",
	"VK_RBUTTON",
	"VK_CANCEL",
	"VK_MBUTTON",
	"VK_XBUTTON1",
	"VK_XBUTTON2",
	"Unknown",
	"VK_BACK",
	"VK_TAB",
	"Unknown",
	"Unknown",
	"VK_CLEAR",
	"VK_RETURN",
	"Unknown",
	"Unknown",
	"VK_SHIFT",
	"VK_CONTROL",
	"VK_MENU",
	"VK_PAUSE",
	"VK_CAPITAL",
	"VK_KANA",
	"Unknown",
	"VK_JUNJA",
	"VK_FINAL",
	"VK_KANJI",
	"Unknown",
	"VK_ESCAPE",
	"VK_CONVERT",
	"VK_NONCONVERT",
	"VK_ACCEPT",
	"VK_MODECHANGE",
	"VK_SPACE",
	"VK_PRIOR",
	"VK_NEXT",
	"VK_END",
	"VK_HOME",
	"VK_LEFT",
	"VK_UP",
	"VK_RIGHT",
	"VK_DOWN",
	"VK_SELECT",
	"VK_PRINT",
	"VK_EXECUTE",
	"VK_SNAPSHOT",
	"VK_INSERT",
	"VK_DELETE",
	"VK_HELP",
	"0",
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"9",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"A",
	"B",
	"C",
	"D",
	"E",
	"F",
	"G",
	"H",
	"I",
	"J",
	"K",
	"L",
	"M",
	"N",
	"O",
	"P",
	"Q",
	"R",
	"S",
	"T",
	"U",
	"V",
	"W",
	"X",
	"Y",
	"Z",
	"VK_LWIN",
	"VK_RWIN",
	"VK_APPS",
	"Unknown",
	"VK_SLEEP",
	"VK_NUMPAD0",
	"VK_NUMPAD1",
	"VK_NUMPAD2",
	"VK_NUMPAD3",
	"VK_NUMPAD4",
	"VK_NUMPAD5",
	"VK_NUMPAD6",
	"VK_NUMPAD7",
	"VK_NUMPAD8",
	"VK_NUMPAD9",
	"VK_MULTIPLY",
	"VK_ADD",
	"VK_SEPARATOR",
	"VK_SUBTRACT",
	"VK_DECIMAL",
	"VK_DIVIDE",
	"VK_F1",
	"VK_F2",
	"VK_F3",
	"VK_F4",
	"VK_F5",
	"VK_F6",
	"VK_F7",
	"VK_F8",
	"VK_F9",
	"VK_F10",
	"VK_F11",
	"VK_F12",
	"VK_F13",
	"VK_F14",
	"VK_F15",
	"VK_F16",
	"VK_F17",
	"VK_F18",
	"VK_F19",
	"VK_F20",
	"VK_F21",
	"VK_F22",
	"VK_F23",
	"VK_F24",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"VK_NUMLOCK",
	"VK_SCROLL",
	"VK_OEM_NEC_EQUAL",
	"VK_OEM_FJ_MASSHOU",
	"VK_OEM_FJ_TOUROKU",
	"VK_OEM_FJ_LOYA",
	"VK_OEM_FJ_ROYA",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"Unknown",
	"VK_LSHIFT",
	"VK_RSHIFT",
	"VK_LCONTROL",
	"VK_RCONTROL",
	"VK_LMENU",
	"VK_RMENU"
	};
	static const uint8_t KeyCodesEx[] =
	{
		VK_TAB,
		VK_LEFT,
		VK_RIGHT,
		VK_UP,
		VK_DOWN,
		VK_PRIOR,
		VK_NEXT,
		VK_HOME,
		VK_END,
		VK_INSERT,
		VK_DELETE,
		VK_BACK,
		VK_SPACE,
		VK_RETURN,
		VK_ESCAPE,
		VK_OEM_7,
		VK_OEM_COMMA,
		VK_OEM_MINUS,
		VK_OEM_PERIOD,
		VK_OEM_2,
		VK_OEM_1,
		VK_OEM_PLUS,
		VK_OEM_4,
		VK_OEM_5,
		VK_OEM_6,
		VK_OEM_3,
		VK_CAPITAL,
		VK_SCROLL,
		VK_NUMLOCK,
		VK_SNAPSHOT,
		VK_PAUSE,
		VK_NUMPAD0,
		VK_NUMPAD1,
		VK_NUMPAD2,
		VK_NUMPAD3,
		VK_NUMPAD4,
		VK_NUMPAD5,
		VK_NUMPAD6,
		VK_NUMPAD7,
		VK_NUMPAD8,
		VK_NUMPAD9,
		VK_DECIMAL,
		VK_DIVIDE,
		VK_MULTIPLY,
		VK_SUBTRACT,
		VK_ADD,
		IM_VK_KEYPAD_ENTER,
		VK_LSHIFT,
		VK_LCONTROL,
		VK_LMENU,
		VK_LWIN,
		VK_RSHIFT,
		VK_RCONTROL,
		VK_RMENU,
		VK_RWIN,
		VK_APPS,
		'0',
		'1',
		'2',
		'3',
		'4',
		'5',
		'6',
		'7',
		'8',
		'9',
		'A',
		'B',
		'C',
		'D',
		'E',
		'F',
		'G',
		'H',
		'I',
		'J',
		'K',
		'L',
		'M',
		'N',
		'O',
		'P',
		'Q',
		'R',
		'S',
		'T',
		'U',
		'V',
		'W',
		'X',
		'Y',
		'Z',
		VK_F1,
		VK_F2,
		VK_F3,
		VK_F4,
		VK_F5,
		VK_F6,
		VK_F7,
		VK_F8,
		VK_F9,
		VK_F10,
		VK_F11,
		VK_F12,
		VK_F13,
		VK_F14,
		VK_F15,
		VK_F16,
		VK_F17,
		VK_F18,
		VK_F19,
		VK_F20,
		VK_F21,
		VK_F22,
		VK_F23,
		VK_F24,
		VK_BROWSER_BACK,
		VK_BROWSER_FORWARD
	};

	static const std::unordered_map<uint32_t, const char*> KEY_STRING_MAP =
	{
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
		{ 0xE8,            "N/A" }
	};

	std::string KeyCodeToString(UINT keyCode)
	{
		auto it = KEY_STRING_MAP.find(keyCode);

		if (it != KEY_STRING_MAP.end())
			return it->second;

		char buffer[256];
		size_t length = GetKeyNameTextA(MapVirtualKeyA(keyCode, MAPVK_VK_TO_VSC) << 16, buffer, 256);
		if (length == 0)
			return std::string("N/A");
		return std::string(buffer, length);
	}
}
