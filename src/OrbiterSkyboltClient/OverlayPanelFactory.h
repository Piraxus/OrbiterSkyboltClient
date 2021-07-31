/*
Copyright 2021 Matthew Reid

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "GraphicsAPI.h"

#include <osg/Geode>
#include <osg/Texture2D>

#include <functional>

using WindowSizeProvider = std::function<osg::Vec2i()>;
using TextureProvider = std::function<osg::ref_ptr<osg::Texture2D>(SURFHANDLE)>; //! Returns null if texture not found
using MfdSurfaceProvider = std::function<SURFHANDLE(int mfdId)>; //! Returns null if MFD not found

class OverlayPanelFactory
{
public:
	OverlayPanelFactory(WindowSizeProvider windowSizeProvider, TextureProvider textureProvider, MfdSurfaceProvider mfdSurfaceProvider);

	osg::ref_ptr<osg::Geode> createOverlayPanel(SURFHANDLE *hSurf, MESHHANDLE hMesh, MATRIX3 *T, float alpha, bool additive);

private:
	WindowSizeProvider mWindowSizeProvider;
	TextureProvider mTextureProvider;
	MfdSurfaceProvider mMfdSurfaceProvider;
};
