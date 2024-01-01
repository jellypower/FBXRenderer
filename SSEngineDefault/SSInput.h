#pragma once
#include "framework.h"
#include "SSNativeTypes.h"



enum class EKeyCode {
	KEY_A,
	KEY_B,
	KEY_C,
	KEY_D,
	KEY_E,
	KEY_F,
	KEY_G,
	KEY_H,
	KEY_I,
	KEY_J,
	KEY_K,
	KEY_L,
	KEY_M,
	KEY_N,
	KEY_O,
	KEY_P,
	KEY_Q,
	KEY_R,
	KEY_S,
	KEY_T,
	KEY_U,
	KEY_V,
	KEY_W,
	KEY_X,
	KEY_Y,
	KEY_Z,

	KEY_UP,
	KEY_DOWN,
	KEY_LEFT,
	KEY_RIGHT,

	Unknown,
	Count
};

enum class EMouseCode {
	MOUSE_LEFT,
	MOUSE_RIGHT,
	MOUSE_WHEEL,

	Unknown,
	Count
};

class SSInput
{
	friend LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	friend int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow);
private:
	static SSInput* _instance;
	bool _keyState[static_cast<int32>(EKeyCode::Count)];
	bool _mouseState[static_cast<int32>(EMouseCode::Count)];

	Vector2i32 _mousePos;
	Vector2i32 _mouseDelta;


	

public:
	FORCEINLINE static SSInput* Get();
	FORCEINLINE static bool GetKey(EKeyCode keyCode) { return _instance->_keyState[static_cast<uint32>(keyCode)]; }
	FORCEINLINE static bool GetMouse(EMouseCode mouseCode) { return _instance->_mouseState[static_cast<uint32>(mouseCode)]; }
	FORCEINLINE static Vector2f GetMouseDelta() { return _instance->GetMouseDeltaInternal(); }
private:
	SSInput();

	FORCEINLINE static void Release();

	void ProcessInputReset();
	void ProcessInputEventForWindowsInternal(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
	Vector2f GetMouseDeltaInternal();
};


FORCEINLINE SSInput* SSInput::Get() {
	if (_instance == nullptr)
		_instance = DBG_NEW SSInput();
	return _instance;
}

inline SSInput::SSInput()
{
	for (uint32 i = 0; i < static_cast<uint32>(EKeyCode::Count); i++)
		_keyState[i] = false;

	for (uint32 i = 0; i < static_cast<uint32>(EMouseCode::Count); i++)
		_mouseState[i] = false;
}

FORCEINLINE void SSInput::Release() {
	delete _instance;
	_instance = nullptr;
}