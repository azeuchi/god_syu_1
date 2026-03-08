#ifndef __INPUT_H__
#define __INPUT_H__

#include <Windows.h>
#include <Xinput.h>
#pragma comment(lib, "xinput.lib")
#undef max
#undef min

HRESULT InitInput();
void UninitInput();
void UpdateInput();

bool IsKeyPress(BYTE key);
bool IsKeyTrigger(BYTE key);
bool IsKeyRelease(BYTE key);
bool IsKeyRepeat(BYTE key);

bool IsPadPress(int padNo, WORD button);
bool IsPadTrigger(int padNo, WORD button);
bool IsPadRelease(int padNo, WORD button);
bool IsPadConnected(int padNo);

#endif // __INPUT_H__