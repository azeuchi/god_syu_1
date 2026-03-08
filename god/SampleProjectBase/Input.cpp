#define DIRECTINPUT_VERSION 0x0800
#include "Input.h"
#include <dinput.h>
#include <string>
#include <algorithm>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

//--- グローバル変数
BYTE g_keyTable[256];
BYTE g_oldTable[256];

XINPUT_STATE g_padState[4];
XINPUT_STATE g_oldPadState[4];
bool g_padConnected[4];

// デバイスのスロット割り当て管理用
struct PadDevice {
	bool isActive;
	bool isXInput;
	int deviceIndex;
};
PadDevice g_slotDevices[4];

// DirectInput用変数
LPDIRECTINPUT8 g_pDI = nullptr;
LPDIRECTINPUTDEVICE8 g_pJoysticks[4] = { nullptr };
int g_joystickCount = 0;

// 接続されているDirectInputコントローラーを探すコールバック
BOOL CALLBACK EnumJoysticksCallback(const DIDEVICEINSTANCE* pdidInstance, VOID* pContext)
{
	if (g_joystickCount >= 4) return DIENUM_STOP;

	// ゲーミングキーボードやマウスの拡張機能がコントローラーとして誤認識されるのを防ぐフィルター
	BYTE devType = GET_DIDEVICE_TYPE(pdidInstance->dwDevType);
	if (devType != DI8DEVTYPE_GAMEPAD && devType != DI8DEVTYPE_JOYSTICK && devType != DI8DEVTYPE_1STPERSON)
	{
		return DIENUM_CONTINUE;
	}

	// XInput機器（Xboxコントローラー等）がDirectInputで二重登録されるのを防ぐ簡易フィルター
	// 製品名に「Xbox」や「XINPUT」が含まれている場合は除外する
	bool isXInputDevice = false;

#ifdef UNICODE
	std::wstring name = pdidInstance->tszProductName;
	std::transform(name.begin(), name.end(), name.begin(), ::towlower);
	if (name.find(L"xbox") != std::wstring::npos ||
		name.find(L"xinput") != std::wstring::npos ||
		name.find(L"x360") != std::wstring::npos) {
		isXInputDevice = true;
	}
#else
	std::string name = pdidInstance->tszProductName;
	std::transform(name.begin(), name.end(), name.begin(), ::tolower);
	if (name.find("xbox") != std::string::npos ||
		name.find("xinput") != std::string::npos ||
		name.find("x360") != std::string::npos) {
		isXInputDevice = true;
	}
#endif

	if (isXInputDevice) return DIENUM_CONTINUE;

	HRESULT hr = g_pDI->CreateDevice(pdidInstance->guidInstance, &g_pJoysticks[g_joystickCount], nullptr);
	if (SUCCEEDED(hr))
	{
		g_pJoysticks[g_joystickCount]->SetDataFormat(&c_dfDIJoystick);

		HWND hwnd = GetActiveWindow();
		if (!hwnd) hwnd = GetForegroundWindow();
		if (hwnd) g_pJoysticks[g_joystickCount]->SetCooperativeLevel(hwnd, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);

		// スティックの入力範囲をXInputに合わせる
		DIPROPRANGE propRange;
		propRange.diph.dwSize = sizeof(DIPROPRANGE);
		propRange.diph.dwHeaderSize = sizeof(DIPROPHEADER);
		propRange.diph.dwHow = DIPH_BYOFFSET;
		propRange.lMin = -32768;
		propRange.lMax = 32767;

		propRange.diph.dwObj = DIJOFS_X;
		g_pJoysticks[g_joystickCount]->SetProperty(DIPROP_RANGE, &propRange.diph);
		propRange.diph.dwObj = DIJOFS_Y;
		g_pJoysticks[g_joystickCount]->SetProperty(DIPROP_RANGE, &propRange.diph);

		g_joystickCount++;
	}
	return DIENUM_CONTINUE;
}

HRESULT InitInput()
{
	// 一番最初の入力
	GetKeyboardState(g_keyTable);

	for (int i = 0; i < 4; ++i)
	{
		ZeroMemory(&g_padState[i], sizeof(XINPUT_STATE));
		ZeroMemory(&g_oldPadState[i], sizeof(XINPUT_STATE));
		g_padConnected[i] = false;

		g_slotDevices[i].isActive = false;
		g_slotDevices[i].isXInput = false;
		g_slotDevices[i].deviceIndex = -1;
	}

	// DirectInputの初期化
	HINSTANCE hInst = GetModuleHandle(nullptr);
	if (SUCCEEDED(DirectInput8Create(hInst, DIRECTINPUT_VERSION, IID_IDirectInput8, (void**)&g_pDI, nullptr)))
	{
		g_pDI->EnumDevices(DI8DEVCLASS_GAMECTRL, EnumJoysticksCallback, nullptr, DIEDFL_ATTACHEDONLY);
	}

	return S_OK;
}

void UninitInput()
{
	for (int i = 0; i < 4; ++i)
	{
		if (g_pJoysticks[i])
		{
			g_pJoysticks[i]->Unacquire();
			g_pJoysticks[i]->Release();
			g_pJoysticks[i] = nullptr;
		}
	}
	if (g_pDI)
	{
		g_pDI->Release();
		g_pDI = nullptr;
	}
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

	// 接続が切れたデバイスをスロットから除外する
	for (int i = 0; i < 4; ++i)
	{
		if (g_slotDevices[i].isActive)
		{
			if (g_slotDevices[i].isXInput)
			{
				XINPUT_STATE state;
				if (XInputGetState(g_slotDevices[i].deviceIndex, &state) != ERROR_SUCCESS)
				{
					g_slotDevices[i].isActive = false;
				}
			}
			else
			{
				int dIdx = g_slotDevices[i].deviceIndex;
				if (g_pJoysticks[dIdx])
				{
					if (FAILED(g_pJoysticks[dIdx]->Poll()))
					{
						g_pJoysticks[dIdx]->Acquire();
						if (FAILED(g_pJoysticks[dIdx]->Poll()))
						{
							g_slotDevices[i].isActive = false;
						}
					}
				}
				else
				{
					g_slotDevices[i].isActive = false;
				}
			}
		}
	}

	// 新しく検出されたXInputデバイスを空きスロットに登録する
	for (DWORD xi = 0; xi < 4; ++xi)
	{
		XINPUT_STATE state;
		if (XInputGetState(xi, &state) == ERROR_SUCCESS)
		{
			bool assigned = false;
			for (int i = 0; i < 4; ++i)
			{
				if (g_slotDevices[i].isActive && g_slotDevices[i].isXInput && g_slotDevices[i].deviceIndex == xi)
				{
					assigned = true;
					break;
				}
			}
			if (!assigned)
			{
				for (int i = 0; i < 4; ++i)
				{
					if (!g_slotDevices[i].isActive)
					{
						g_slotDevices[i].isActive = true;
						g_slotDevices[i].isXInput = true;
						g_slotDevices[i].deviceIndex = xi;
						break;
					}
				}
			}
		}
	}

	// 新しく検出されたDirectInputデバイスを空きスロットに登録する
	for (int di = 0; di < g_joystickCount; ++di)
	{
		if (g_pJoysticks[di])
		{
			if (SUCCEEDED(g_pJoysticks[di]->Poll()) || SUCCEEDED(g_pJoysticks[di]->Acquire()))
			{
				bool assigned = false;
				for (int i = 0; i < 4; ++i)
				{
					if (g_slotDevices[i].isActive && !g_slotDevices[i].isXInput && g_slotDevices[i].deviceIndex == di)
					{
						assigned = true;
						break;
					}
				}
				if (!assigned)
				{
					for (int i = 0; i < 4; ++i)
					{
						if (!g_slotDevices[i].isActive)
						{
							g_slotDevices[i].isActive = true;
							g_slotDevices[i].isXInput = false;
							g_slotDevices[i].deviceIndex = di;
							break;
						}
					}
				}
			}
		}
	}

	// 登録されたスロットに基づいて入力を更新する
	for (int i = 0; i < 4; ++i)
	{
		g_oldPadState[i] = g_padState[i];
		ZeroMemory(&g_padState[i], sizeof(XINPUT_STATE));
		g_padConnected[i] = g_slotDevices[i].isActive;

		if (g_slotDevices[i].isActive)
		{
			if (g_slotDevices[i].isXInput)
			{
				XInputGetState(g_slotDevices[i].deviceIndex, &g_padState[i]);
				ApplyStickToDPad(g_padState[i].Gamepad);
			}
			else
			{
				int di = g_slotDevices[i].deviceIndex;
				DIJOYSTATE js;
				if (SUCCEEDED(g_pJoysticks[di]->GetDeviceState(sizeof(DIJOYSTATE), &js)))
				{
					WORD buttons = 0;

					// 一般的なPS4/PS5コントローラーのDirectInputボタン配置をXInputに擬似的にマッピング
					if (js.rgbButtons[1] & 0x80) buttons |= XINPUT_GAMEPAD_A;      // ×ボタン
					if (js.rgbButtons[2] & 0x80) buttons |= XINPUT_GAMEPAD_B;      // 〇ボタン
					if (js.rgbButtons[0] & 0x80) buttons |= XINPUT_GAMEPAD_X;      // □ボタン
					if (js.rgbButtons[3] & 0x80) buttons |= XINPUT_GAMEPAD_Y;      // △ボタン
					if (js.rgbButtons[4] & 0x80) buttons |= XINPUT_GAMEPAD_LEFT_SHOULDER; // L1
					if (js.rgbButtons[5] & 0x80) buttons |= XINPUT_GAMEPAD_RIGHT_SHOULDER;// R1
					if (js.rgbButtons[8] & 0x80) buttons |= XINPUT_GAMEPAD_BACK;   // Share/Create
					if (js.rgbButtons[9] & 0x80) buttons |= XINPUT_GAMEPAD_START;  // Options
					if (js.rgbButtons[10] & 0x80) buttons |= XINPUT_GAMEPAD_LEFT_THUMB; // L3
					if (js.rgbButtons[11] & 0x80) buttons |= XINPUT_GAMEPAD_RIGHT_THUMB;// R3

					// 十字キー (POV) の変換
					if (LOWORD(js.rgdwPOV[0]) != 0xFFFF)
					{
						DWORD pov = js.rgdwPOV[0];
						if (pov == 0 || pov == 4500 || pov == 31500) buttons |= XINPUT_GAMEPAD_DPAD_UP;
						if (pov == 18000 || pov == 13500 || pov == 22500) buttons |= XINPUT_GAMEPAD_DPAD_DOWN;
						if (pov == 27000 || pov == 22500 || pov == 31500) buttons |= XINPUT_GAMEPAD_DPAD_LEFT;
						if (pov == 9000 || pov == 4500 || pov == 13500) buttons |= XINPUT_GAMEPAD_DPAD_RIGHT;
					}

					g_padState[i].Gamepad.wButtons = buttons;
					g_padState[i].Gamepad.sThumbLX = (SHORT)js.lX;
					g_padState[i].Gamepad.sThumbLY = (SHORT)-js.lY; // DirectInputはY軸が逆転していることが多いので反転

					ApplyStickToDPad(g_padState[i].Gamepad);
				}
			}
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

bool IsPadXInput(int padNo)
{
	if (padNo < 0 || padNo >= 4) return false;
	return g_slotDevices[padNo].isActive && g_slotDevices[padNo].isXInput;
}