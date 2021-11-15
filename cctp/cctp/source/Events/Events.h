#pragma once

struct InputEvent
{
	bool RepeatedKey;
	int16_t Input;
	uint32_t Port;
	float Data;
};

struct WindowClosedEvent {};
struct WindowDestroyedEvent {};
struct WindowEndMoveEvent {};
struct WindowEnterFullscreenEvent {};
struct WindowExitFullscreenEvent {};
struct WindowLostFocusEvent {};
struct WindowMaximizedEvent {};
struct WindowMinimizedEvent {};
struct WindowMoveEvent {};
struct WindowReceivedFocusEvent {};
struct WindowResizedEvent {};
struct WindowRestoredEvent {};