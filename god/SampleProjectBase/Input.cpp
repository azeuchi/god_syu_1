#include "Input.h"

//--- グローバル変数
BYTE g_keyTable[256];
BYTE g_oldTable[256];

XINPUT_STATE g_padState[4];
XINPUT_STATE g_oldPadState[4];
bool g_padConnected[4];

HRESULT InitInput()
{
	// 一番最初の入力
	GetKeyboardState(g_keyTable);

	for (int i = 0; i < 4; ++i)
	{
		ZeroMemory(&g_padState[i], sizeof(XINPUT_STATE));
		ZeroMemory(&g_oldPadState[i], sizeof(XINPUT_STATE));
		g_padConnected[i] = false;
	}

	return S_OK;
}
void UninitInput()
{
}

void ApplyStickToDPad(XINPUT_GAMEPAD& gamepad)
{
	const int deadzone = 10000;
	if (gamepad.sThumbLY > deadzone) gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_UP;
	if (gamepad.sThumbLY < -deadzone) gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_DOWN;
	if (gamepad.sThumbLX < -deadzone) gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_LEFT;
	if (gamepad.sThumbLX > deadzone) gamepad.wButtons |= XINPUT_GAMEPAD_DPAD_RIGHT;
}

void UpdateInput()
{
	// 古い入力を更新
	memcpy_s(g_oldTable, sizeof(g_oldTable), g_keyTable, sizeof(g_keyTable));
	// 現在の入力を取得
	GetKeyboardState(g_keyTable);

	for (DWORD i = 0; i < 4; ++i)
	{
		g_oldPadState[i] = g_padState[i];
		DWORD dwResult = XInputGetState(i, &g_padState[i]);
		if (dwResult == ERROR_SUCCESS)
		{
			g_padConnected[i] = true;
			ApplyStickToDPad(g_padState[i].Gamepad);
		}
		else
		{
			g_padConnected[i] = false;
			ZeroMemory(&g_padState[i], sizeof(XINPUT_STATE));
		}
	}
}

bool IsKeyPress(BYTE key)
{
	return g_keyTable[key] & 0x80;
}
bool IsKeyTrigger(BYTE key)
{
	return (g_keyTable[key] ^ g_oldTable[key]) & g_keyTable[key] & 0x80;
}
bool IsKeyRelease(BYTE key)
{
	return (g_keyTable[key] ^ g_oldTable[key]) & g_oldTable[key] & 0x80;
}
bool IsKeyRepeat(BYTE key)
{
	return false;
}

bool IsPadPress(int padNo, WORD button)
{
	if (padNo < 0 || padNo >= 4 || !g_padConnected[padNo]) return false;
	return (g_padState[padNo].Gamepad.wButtons & button) != 0;
}

bool IsPadTrigger(int padNo, WORD button)
{
	if (padNo < 0 || padNo >= 4 || !g_padConnected[padNo]) return false;
	return ((g_padState[padNo].Gamepad.wButtons & button) != 0) &&
		((g_oldPadState[padNo].Gamepad.wButtons & button) == 0);
}

bool IsPadRelease(int padNo, WORD button)
{
	if (padNo < 0 || padNo >= 4 || !g_padConnected[padNo]) return false;
	return ((g_padState[padNo].Gamepad.wButtons & button) == 0) &&
		((g_oldPadState[padNo].Gamepad.wButtons & button) != 0);
}

bool IsPadConnected(int padNo)
{
	if (padNo < 0 || padNo >= 4) return false;
	return g_padConnected[padNo];
}