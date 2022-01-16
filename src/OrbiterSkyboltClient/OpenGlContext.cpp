#include "OpenGlContext.h"

#include <stdexcept>

// Code based on example from Nick Rolfe
// https://gist.github.com/nickrolfe/1127313ed1dbf80254b614a721b3ee9c

#define WGL_DRAW_TO_WINDOW_ARB                    0x2001
#define WGL_ACCELERATION_ARB                      0x2003
#define WGL_SUPPORT_OPENGL_ARB                    0x2010
#define WGL_DOUBLE_BUFFER_ARB                     0x2011
#define WGL_PIXEL_TYPE_ARB                        0x2013
#define WGL_COLOR_BITS_ARB                        0x2014
#define WGL_DEPTH_BITS_ARB                        0x2022
#define WGL_STENCIL_BITS_ARB                      0x2023

#define WGL_FULL_ACCELERATION_ARB                 0x2027
#define WGL_TYPE_RGBA_ARB                         0x202B

#define WGL_CONTEXT_MAJOR_VERSION_ARB             0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB             0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB              0x9126

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB          0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002

typedef HGLRC WINAPI wglCreateContextAttribsARB_type(HDC hdc, HGLRC hShareContext, const int *attribList);
wglCreateContextAttribsARB_type *wglCreateContextAttribsARB;

typedef BOOL WINAPI wglChoosePixelFormatARB_type(HDC hdc, const int *piAttribIList,
	const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
wglChoosePixelFormatARB_type *wglChoosePixelFormatARB;

static WNDCLASSEX createDummyWindowClass()
{
	// Before we can load extensions, we need a dummy OpenGL context, created using a dummy window.
	// We use a dummy window because you can only set the pixel format for a window once. For the
	// real window, we want to use wglChoosePixelFormatARB (so we can potentially specify options
	// that aren't available in PIXELFORMATDESCRIPTOR), but we can't load and use that before we
	// have a context.
	WNDCLASSEX windowClass = {0};
	windowClass.cbSize = sizeof(windowClass);
	windowClass.hInstance = GetModuleHandle(0);
	windowClass.style = CS_OWNDC;
	windowClass.hIcon = LoadIcon(windowClass.hInstance, MAKEINTRESOURCE(129));
	windowClass.lpfnWndProc = DefWindowProcA;
	windowClass.lpszClassName = "skyboltDummyWindow";

	if (!RegisterClassEx(&windowClass))
	{
		throw std::runtime_error("Failed to register dummy OpenGL window");
	}

	return windowClass;
}

static void initOpenglExtensions()
{
	static WNDCLASSEX windowClass = createDummyWindowClass();

	HWND dummyWindow = CreateWindowEx(WS_EX_APPWINDOW, "skyboltDummyWindow", "temporary", WS_SYSMENU | WS_BORDER | WS_MINIMIZEBOX, 0, 0, 0, 0, 0, 0, windowClass.hInstance, 0);

	if (!dummyWindow)
	{
		throw std::runtime_error("Failed to create dummy OpenGL window");
	}

	HDC dummyDc = GetDC(dummyWindow);

	PIXELFORMATDESCRIPTOR pfd;
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.cColorBits = 32;
	pfd.cAlphaBits = 8;
	pfd.iLayerType = PFD_MAIN_PLANE;
	pfd.cDepthBits = 24;
	pfd.cStencilBits = 8;

	int pixel_format = ChoosePixelFormat(dummyDc, &pfd);
	if (!pixel_format)
	{
		throw std::runtime_error("Failed to find a suitable pixel format");
	}
	if (!SetPixelFormat(dummyDc, pixel_format, &pfd))
	{
		throw std::runtime_error("Failed to set the pixel format");
	}

	HGLRC dummy_context = wglCreateContext(dummyDc);
	if (!dummy_context) {
		throw std::runtime_error("Failed to create a dummy OpenGL rendering context");
	}

	if (!wglMakeCurrent(dummyDc, dummy_context)) {
		throw std::runtime_error("Failed to activate dummy OpenGL rendering context");
	}

	wglCreateContextAttribsARB = (wglCreateContextAttribsARB_type*)wglGetProcAddress("wglCreateContextAttribsARB");
	wglChoosePixelFormatARB = (wglChoosePixelFormatARB_type*)wglGetProcAddress("wglChoosePixelFormatARB");

	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		throw std::runtime_error("Error initializing GLEW: " + std::string((char*)glewGetErrorString(err)));
	}
	
	wglMakeCurrent(dummyDc, 0);
	wglDeleteContext(dummy_context);
	ReleaseDC(dummyWindow, dummyDc);
	DestroyWindow(dummyWindow);
}

HGLRC createOpenGlContext(HDC real_dc)
{
	initOpenglExtensions();

	int pixel_format_attribs[] = {
		WGL_DRAW_TO_WINDOW_ARB,     GL_TRUE,
		WGL_SUPPORT_OPENGL_ARB,     GL_TRUE,
		WGL_DOUBLE_BUFFER_ARB,      GL_TRUE,
		WGL_ACCELERATION_ARB,       WGL_FULL_ACCELERATION_ARB,
		WGL_PIXEL_TYPE_ARB,         WGL_TYPE_RGBA_ARB,
		WGL_COLOR_BITS_ARB,         32,
		WGL_DEPTH_BITS_ARB,         24,
		WGL_STENCIL_BITS_ARB,       8,
		0
	};

	int pixel_format;
	UINT num_formats;
	wglChoosePixelFormatARB(real_dc, pixel_format_attribs, 0, 1, &pixel_format, &num_formats);
	if (!num_formats)
	{
		throw std::runtime_error("Failed to set the OpenGL pixel format");
	}

	PIXELFORMATDESCRIPTOR pfd;
	DescribePixelFormat(real_dc, pixel_format, sizeof(pfd), &pfd);
	if (!SetPixelFormat(real_dc, pixel_format, &pfd))
	{
		throw std::runtime_error("Failed to set the OpenGL pixel format");
	}

	int attributes[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
		WGL_CONTEXT_MINOR_VERSION_ARB, 5,
		WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
		0,
	};

	HGLRC context = wglCreateContextAttribsARB(real_dc, 0, attributes);
	if (!context)
	{
		throw std::runtime_error("Failed to create OpenGL context");
	}

	if (!wglMakeCurrent(real_dc, context))
	{
		throw std::runtime_error("Failed to activate OpenGL rendering context");
	}

	return context;
}
