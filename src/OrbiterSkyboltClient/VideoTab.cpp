/*
Copyright 2021 Matthew Reid

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include "VideoTab.h"
#include "SkyboltClient.h"

#include <resource_video.h>

using namespace oapi;

VideoTab::VideoTab (SkyboltClient *gc, HINSTANCE _hInst, HINSTANCE _hOrbiterInst, HWND hVideoTab)
{
	int width = GetSystemMetrics(SM_CXSCREEN);
	int height = GetSystemMetrics(SM_CYSCREEN);
	defaultDevice.ddsdFullscreenMode = {width, height};
	defaultDevice.pddsdModes.push_back(defaultDevice.ddsdFullscreenMode);

	gclient      = gc;
	hInst        = _hInst;
	hOrbiterInst = _hOrbiterInst;
	hTab         = hVideoTab;
	Initialise (&defaultDevice);
}

INT_PTR VideoTab::WndProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_VID_DEVICE:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				DWORD idx = SendDlgItemMessage (hWnd, IDC_VID_DEVICE, CB_GETCURSEL, 0, 0);
				SkyboltDeviceInfo *dev = &defaultDevice;
				if (idx) {
					// gclient->SelectDevice (dev); // Skybolt does not support selecting video device
					SelectDevice (dev);
				}
				return TRUE;
			}
			break;
		case IDC_VID_MODE:
			if (HIWORD(wParam) == CBN_SELCHANGE) {
				DWORD idx;
				idx = SendDlgItemMessage (hWnd, IDC_VID_MODE, CB_GETCURSEL, 0, 0);
				SelectMode (&defaultDevice, idx);
				return TRUE;
			}
			break;
		case IDC_VID_PAGEFLIP:
			if (HIWORD(wParam) == BN_CLICKED) {
				SelectPageflip();
				return TRUE;
			}
			break;
		case IDC_VID_FULL:
			if (HIWORD(wParam) == BN_CLICKED) {
				SelectDispmode (&defaultDevice, FALSE);
				return TRUE;
			}
			break;
		case IDC_VID_WINDOW:
			if (HIWORD(wParam) == BN_CLICKED) {
				SelectDispmode (&defaultDevice, TRUE);
				return TRUE;
			}
			break;
		case IDC_VID_WIDTH:
			if (HIWORD(wParam) == EN_CHANGE) {
				SelectWidth ();
				return TRUE;
			}
			break;
		case IDC_VID_HEIGHT:
			if (HIWORD(wParam) == EN_CHANGE) {
				SelectHeight ();
				return TRUE;
			}
			break;
		case IDC_VID_ASPECT:
			if (HIWORD(wParam) == BN_CLICKED) {
				SelectFixedAspect ();
				SelectWidth ();
				return TRUE;
			}
			break;
		case IDC_VID_4X3:
		case IDC_VID_16X10:
		case IDC_VID_16X9:
			if (HIWORD(wParam) == BN_CLICKED) {
				aspect_idx = LOWORD(wParam)-IDC_VID_4X3;
				SelectWidth ();
				return TRUE;
			}
			break;
		}
		break;
	}
	return FALSE;
}

void VideoTab::Initialise (SkyboltDeviceInfo *dev)
{
	GraphicsClient::VIDEODATA *data = gclient->GetVideoData();
	
	char cbuf[20];
	DWORD i, ndev, idx;
	ndev = 1;
	const char* deviceName = "Default Device";

	SendDlgItemMessage (hTab, IDC_VID_DEVICE, CB_RESETCONTENT, 0, 0);
	for (i = 0; i < ndev; i++) {
		SendMessage (GetDlgItem (hTab, IDC_VID_DEVICE), CB_ADDSTRING, 0,
			TEXT((LPARAM)deviceName));
	}

	if (SendDlgItemMessage (hTab, IDC_VID_DEVICE, CB_SETCURSEL, data->deviceidx, 0) == CB_ERR) {
		if ((idx = SendDlgItemMessage (hTab, IDC_VID_DEVICE, CB_FINDSTRINGEXACT, -1, (LPARAM)deviceName)) == CB_ERR)
			idx = 0;
		SendDlgItemMessage (hTab, IDC_VID_DEVICE, CB_SETCURSEL, idx, 0);
	}
	SendDlgItemMessage (hTab, IDC_VID_ENUM, BM_SETCHECK, data->forceenum ? BST_CHECKED : BST_UNCHECKED, 0);
	SendDlgItemMessage (hTab, IDC_VID_STENCIL, BM_SETCHECK, data->trystencil ? BST_CHECKED : BST_UNCHECKED, 0);
	SendDlgItemMessage (hTab, IDC_VID_VSYNC, BM_SETCHECK, data->novsync ? BST_CHECKED : BST_UNCHECKED, 0);
	SendDlgItemMessage (hTab, IDC_VID_PAGEFLIP, BM_SETCHECK, data->pageflip ? BST_UNCHECKED : BST_CHECKED, 0);

	SetWindowText (GetDlgItem (hTab, IDC_VID_WIDTH), _itoa (data->winw, cbuf, 10));
	SetWindowText (GetDlgItem (hTab, IDC_VID_HEIGHT), _itoa (data->winh, cbuf, 10));

	if (data->winw == (4*data->winh)/3 || data->winh == (3*data->winw)/4)
		aspect_idx = 1;
	else if (data->winw == (16*data->winh)/10 || data->winh == (10*data->winw)/16)
		aspect_idx = 2;
	else if (data->winw == (16*data->winh)/9 || data->winh == (9*data->winw)/16)
		aspect_idx = 3;
	else
		aspect_idx = 0;
	SendDlgItemMessage (hTab, IDC_VID_ASPECT, BM_SETCHECK, aspect_idx ? BST_CHECKED : BST_UNCHECKED, 0);
	if (aspect_idx) aspect_idx--;
	SendDlgItemMessage (hTab, IDC_VID_4X3+aspect_idx, BM_SETCHECK, BST_CHECKED, 0);

	SelectDevice(dev);
	SendDlgItemMessage (hTab, data->fullscreen ? IDC_VID_FULL:IDC_VID_WINDOW, BM_CLICK, 0, 0);
	SelectDispmode (dev, data->fullscreen ? FALSE:TRUE);

	ShowWindow (GetDlgItem (hTab, IDC_VID_INFO), SW_HIDE);
}

void VideoTab::SelectDevice (SkyboltDeviceInfo *dev)
{
	DWORD i, j;
	char cbuf[256];
	SkyboltDeviceInfo::FullscreenMode &cmode = dev->ddsdFullscreenMode;
	DWORD nres = 0, *wres = new DWORD[dev->pddsdModes.size()], *hres = new DWORD[dev->pddsdModes.size()];

	SendDlgItemMessage (hTab, IDC_VID_MODE, CB_RESETCONTENT, 0, 0);
	SendDlgItemMessage (hTab, IDC_VID_BPP, CB_RESETCONTENT, 0, 0);

	for (i = 0; i < dev->pddsdModes.size(); i++) {
		SkyboltDeviceInfo::FullscreenMode *ddsd = &dev->pddsdModes[i];
		DWORD w = ddsd->dwWidth, h = ddsd->dwHeight;
		for (j = 0; j < nres; j++) if (wres[j] == w && hres[j] == h) break;
		if (j == nres) wres[nres] = w, hres[nres] = h, nres++;
	}
	for (i = 0; i < nres; i++) {
		sprintf (cbuf, "%d x %d", wres[i], hres[i]);
		SendDlgItemMessage (hTab, IDC_VID_MODE, CB_ADDSTRING, 0, (LPARAM)cbuf);
		SendDlgItemMessage (hTab, IDC_VID_MODE, CB_SETITEMDATA, i, (LPARAM)(hres[i]<<16 | wres[i]));
		if (wres[i] == cmode.dwWidth && hres[i] == cmode.dwHeight)
			SendDlgItemMessage (hTab, IDC_VID_MODE, CB_SETCURSEL, i, 0);
	}
	for (i = 0; i < 2; i++)
		EnableWindow (GetDlgItem (hTab, IDC_VID_FULL+i), TRUE);
	for (i = 0; i < 2; i++)
		EnableWindow (GetDlgItem (hTab, IDC_VID_FULL+i), dev->bDesktopCompatible);
	delete []wres;
	delete []hres;
}

// Respond to user selection of fullscreen/window mode
void VideoTab::SelectDispmode (SkyboltDeviceInfo *dev, BOOL bWindow)
{
	DWORD i;
	for (i = 0; i < 6; i++)
		EnableWindow (GetDlgItem (hTab, IDC_VID_STATIC5+i), !bWindow);
	for (i = 0; i < 9; i++)
		EnableWindow (GetDlgItem (hTab, IDC_VID_STATIC7+i), bWindow);
	if (!bWindow) {
		if (SendDlgItemMessage (hTab, IDC_VID_PAGEFLIP, BM_GETCHECK, 0, 0) == BST_CHECKED)
			EnableWindow (GetDlgItem (hTab, IDC_VID_VSYNC), FALSE);
	} else {
		if (SendDlgItemMessage (hTab, IDC_VID_ASPECT, BM_GETCHECK, 0, 0) != BST_CHECKED) {
			for (i = 0; i < 3; i++)
				EnableWindow (GetDlgItem (hTab, IDC_VID_4X3+i), FALSE);
		}
	}
}

// Respond to user selection of fullscreen resolution
void VideoTab::SelectMode (SkyboltDeviceInfo *dev, DWORD idx)
{
	DWORD data, w, h, bpp, usebpp;
	usebpp = 0;
	data = SendDlgItemMessage (hTab, IDC_VID_MODE, CB_GETITEMDATA, idx, 0);
	w    = data & 0xFFFF;
	h    = data >> 16;
	// check that this resolution is compatible with the current bpp setting
	idx  = SendDlgItemMessage (hTab, IDC_VID_BPP, CB_GETCURSEL, 0, 0);
	bpp  = SendDlgItemMessage (hTab, IDC_VID_BPP, CB_GETITEMDATA, idx, 0);

	// if a bpp change was required, notify the bpp control
	if (bpp != usebpp) {
		char cbuf[20];
		SendDlgItemMessage (hTab, IDC_VID_BPP, CB_SELECTSTRING, -1,
			(LPARAM)_itoa (usebpp, cbuf, 10));
	}
}

void VideoTab::SelectPageflip ()
{
	bool disable_pageflip = (SendDlgItemMessage (hTab, IDC_VID_PAGEFLIP, BM_GETCHECK, 0, 0) == BST_CHECKED);
	EnableWindow (GetDlgItem (hTab, IDC_VID_VSYNC), disable_pageflip ? FALSE:TRUE);
}


static int aspect_wfac[4] = {4,16,16};
static int aspect_hfac[4] = {3,10,9};

void VideoTab::SelectWidth ()
{
	if (SendDlgItemMessage (hTab, IDC_VID_ASPECT, BM_GETCHECK, 0, 0) == BST_CHECKED) {
		char cbuf[128];
		int w, h, wfac = aspect_wfac[aspect_idx], hfac = aspect_hfac[aspect_idx];
		GetWindowText (GetDlgItem (hTab, IDC_VID_WIDTH),  cbuf, 127); w = atoi(cbuf);
		GetWindowText (GetDlgItem (hTab, IDC_VID_HEIGHT), cbuf, 127); h = atoi(cbuf);
		if (w != (wfac*h)/hfac) {
			h = (hfac*w)/wfac;
			SetWindowText (GetDlgItem (hTab, IDC_VID_HEIGHT), itoa (h, cbuf, 10));
		}
	}
}

void VideoTab::SelectHeight ()
{
	if (SendDlgItemMessage (hTab, IDC_VID_ASPECT, BM_GETCHECK, 0, 0) == BST_CHECKED) {
		char cbuf[128];
		int w, h, wfac = aspect_wfac[aspect_idx], hfac = aspect_hfac[aspect_idx];
		GetWindowText (GetDlgItem (hTab, IDC_VID_WIDTH),  cbuf, 127); w = atoi(cbuf);
		GetWindowText (GetDlgItem (hTab, IDC_VID_HEIGHT), cbuf, 127); h = atoi(cbuf);
		if (h != (hfac*w)/wfac) {
			w = (wfac*h)/hfac;
			SetWindowText (GetDlgItem (hTab, IDC_VID_WIDTH), itoa (w, cbuf, 10));
		}
	}
}

void VideoTab::SelectFixedAspect ()
{
	bool fixed_aspect = (SendDlgItemMessage (hTab, IDC_VID_ASPECT, BM_GETCHECK, 0, 0) == BST_CHECKED);
	for (int i = 0; i < 3; i++)
		EnableWindow (GetDlgItem (hTab, IDC_VID_4X3+i), fixed_aspect ? TRUE:FALSE);
}

void VideoTab::UpdateConfigData ()
{
	char cbuf[128];
	DWORD i, dat, w, h, bpp, ndev;
	ndev = 1;
	GraphicsClient::VIDEODATA *data = gclient->GetVideoData();

	// device parameters
	i   = SendDlgItemMessage (hTab, IDC_VID_DEVICE, CB_GETCURSEL, 0, 0);
	if (i >= ndev) i = 0; // should not happen
	data->deviceidx = i;
	i   = SendDlgItemMessage (hTab, IDC_VID_MODE, CB_GETCURSEL, 0, 0);
	dat = SendDlgItemMessage (hTab, IDC_VID_MODE, CB_GETITEMDATA, i, 0);
	w   = dat & 0xFFFF;
	h   = dat >> 16;
	i   = SendDlgItemMessage (hTab, IDC_VID_BPP, CB_GETCURSEL, 0, 0);
	bpp = SendDlgItemMessage (hTab, IDC_VID_BPP, CB_GETITEMDATA, i, 0);


	data->fullscreen = (SendDlgItemMessage (hTab, IDC_VID_FULL, BM_GETCHECK, 0, 0) == BST_CHECKED);
	data->novsync    = (SendDlgItemMessage (hTab, IDC_VID_VSYNC, BM_GETCHECK, 0, 0) == BST_CHECKED);
	data->pageflip   = (SendDlgItemMessage (hTab, IDC_VID_PAGEFLIP, BM_GETCHECK, 0, 0) != BST_CHECKED);
	data->trystencil = (SendDlgItemMessage (hTab, IDC_VID_STENCIL, BM_GETCHECK, 0, 0) == BST_CHECKED);
	data->forceenum  = (SendDlgItemMessage (hTab, IDC_VID_ENUM, BM_GETCHECK, 0, 0) == BST_CHECKED);
	GetWindowText (GetDlgItem (hTab, IDC_VID_WIDTH),  cbuf, 127); data->winw = atoi(cbuf);
	GetWindowText (GetDlgItem (hTab, IDC_VID_HEIGHT), cbuf, 127); data->winh = atoi(cbuf);
}
