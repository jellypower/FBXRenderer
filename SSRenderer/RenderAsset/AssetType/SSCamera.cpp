#include "SSCamera.h"


SSCamera::SSCamera()
	: _transform()
{
	_transform.Position = Vector4f(0,0,-1,0);
}


void SSCamera::UpdateResolutionWithClientRect(ID3D11Device* InDevice, HWND InHwnd)
{
	GetClientRect(InHwnd, &ScreenRect);
}
