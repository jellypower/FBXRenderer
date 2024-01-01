#include "SSFrameInfo.h"

SSFrameInfo* SSFrameInfo::_instance = nullptr;

constexpr double FRAME_LOW_LIMIT = 1 / 1000.0;

void SSFrameInfo::ProcessPerFrameBeginEventInternal()
{
	_frameCount++;

	_currentTick = GetTickCount64();

	_previousTick = _currentTick;
	_currentTick = GetTickCount64();

	uint64 tickDiff = _currentTick - _previousTick;

	if (tickDiff == 0) _deltaTime = FRAME_LOW_LIMIT;
	else _deltaTime = tickDiff / (double)1000;

	_elapsedTime += _deltaTime;
}

void SSFrameInfo::ProcessWindowResizeEventInternal(uint32 width, uint32 height)
{
	_windowSize.X = width;
	_windowSize.Y = height;
}
