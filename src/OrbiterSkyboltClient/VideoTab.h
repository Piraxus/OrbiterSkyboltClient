/*
Copyright 2021 Matthew Reid

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#pragma once

#include <vector>
#include <Windows.h>

namespace oapi { class SkyboltClient; }

struct SkyboltDeviceInfo
{
	struct FullscreenMode
	{
		int dwWidth;
		int dwHeight;
	};

	FullscreenMode ddsdFullscreenMode;
	std::vector<FullscreenMode> pddsdModes;
	bool bDesktopCompatible = true;
};

class VideoTab
{
public:
	VideoTab (oapi::SkyboltClient* gc, HINSTANCE _hInst, HINSTANCE _hOrbiterInst, HWND hVideoTab);

	INT_PTR WndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	// Video tab message handler

	void UpdateConfigData ();
	// copy dialog state back to parameter structure

protected:
	void Initialise (SkyboltDeviceInfo *dev);
	// Initialise dialog elements

	void SelectDevice (SkyboltDeviceInfo *dev);
	// Update dialog after user device selection

	void SelectDispmode (SkyboltDeviceInfo *dev, BOOL bWindow);
	// Update dialog after user fullscreen/window selection

	void SelectMode (SkyboltDeviceInfo *dev, DWORD idx);
	// Update dialog after fullscreen mode selection

	void SelectPageflip ();
	// Flip hardware pageflip on/off

	void SelectWidth ();
	// Update dialog after window width selection

	void SelectHeight ();
	// Update dialog after window height selection

	void SelectFixedAspect ();
	// Flip fixed window aspect ratio on/off

private:
	oapi::SkyboltClient *gclient;
	HINSTANCE hOrbiterInst; // orbiter instance handle
	HINSTANCE hInst;        // module instance handle
	HWND hTab;              // window handle of the video tab
	int aspect_idx;         // fixed aspect ratio index
	SkyboltDeviceInfo defaultDevice;
};
