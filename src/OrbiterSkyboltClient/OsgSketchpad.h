/*
Copyright 2021 Matthew Reid

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "GraphicsAPI.h"

#include <osg/Camera>

inline osg::Vec4i rgbIntToVec4iColor(DWORD col)
{
	unsigned char* c = reinterpret_cast<unsigned char*>(&col);
	return osg::Vec4i(c[0], c[1], c[2], 255);
}

struct OsgPen : public oapi::Pen
{
	OsgPen() : oapi::Pen(0, 0, 0) {} // FIXME: oapi::Pen should not have a constructor

	enum class Style
	{
		Invisible,
		Solid,
		Dashed
	};

	Style style;
	int width;
	osg::Vec4i color; // [0,255] per component

	static inline Style toStyle(int style)
	{
		return (Style)style;
	}
};

struct OsgBrush : public oapi::Brush
{
	OsgBrush() : oapi::Brush(0) {} // FIXME: oapi::Brush should not have a constructor

	osg::Vec4i color; // [0,255] per component
};

struct OsgFont : public oapi::Font
{
	OsgFont() : oapi::Font(0, false, "") {} // FIXME: oapi::Brush should not have a constructor
	std::string name;
	float heightPixels;
	float rotationRadians = 0;
};

struct NVGcontext;
std::shared_ptr<NVGcontext> CreateNanoVgContext(); // must be called within a valid OpenGL context

class OsgSketchpad : public oapi::Sketchpad
{
public:
	OsgSketchpad(const osg::ref_ptr<osg::Camera>& camera, SURFHANDLE surface, const std::shared_ptr<NVGcontext>& context);
	~OsgSketchpad();

	void fillBackground(const osg::Vec4f& color);

public: // oapi::Sketchpad interface

	oapi::Font *SetFont(oapi::Font *font) const override;

	oapi::Pen *SetPen(oapi::Pen *pen) const override;

	oapi::Brush *SetBrush(oapi::Brush *brush) const override;

	void SetTextAlign(TAlign_horizontal tah = LEFT, TAlign_vertical tav = TOP) override;

	DWORD SetTextColor(DWORD col) override;

	DWORD SetBackgroundColor(DWORD col) override;

	void SetBackgroundMode(BkgMode mode) override;

	DWORD GetCharSize() override;

	DWORD GetTextWidth(const char *str, int len = 0) override;

	void SetOrigin(int x, int y) override { mOrigin = osg::Vec2i(x, y); }

	void GetOrigin(int *x, int *y) const override { *x = mOrigin.x(), *y = mOrigin.y(); }

	bool Text(int x, int y, const char *str, int len) override;

	bool TextW(int x, int y, const LPWSTR str, int len) override;

	void Pixel(int x, int y, DWORD col) override {}

	void MoveTo(int x, int y) override { mPenPosition = osg::Vec2i(x, y); }

	void LineTo(int x, int y) override;

	void Line(int x0, int y0, int x1, int y1) override;

	void Rectangle(int x0, int y0, int x1, int y1) override;

	void Ellipse(int x0, int y0, int x1, int y1) override;

	void Polygon(const oapi::IVECTOR2 *pt, int npt) override;

	void Polyline(const oapi::IVECTOR2 *pt, int npt) override;

private:
	osg::ref_ptr<osg::Camera> mCamera;
	osg::ref_ptr<class SketchpadDrawable> mDrawable;
	mutable OsgFont* mFont = nullptr;
	mutable OsgPen* mPen = nullptr;
	mutable OsgBrush* mBrush = nullptr;
	DWORD mTextColor = 0;
	DWORD mTextBackgroundColor = 0;
	BkgMode mTextBackgroundMode = BK_TRANSPARENT;
	int mTextAlign = 0; // bitfield of enum type NVGalign
	osg::Vec2i mOrigin = osg::Vec2i(0, 0);
	osg::Vec2i mPenPosition = osg::Vec2i(0, 0);
};
