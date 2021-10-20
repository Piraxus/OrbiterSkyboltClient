#pragma once

#include <windows.h>
#include <gl/GL.h>
#include "ThirdParty/GL/glext.h"

HGLRC createOpenGlContext(HDC realDc);

extern PFNGLCOPYIMAGESUBDATANVPROC glCopyImageSubData;
