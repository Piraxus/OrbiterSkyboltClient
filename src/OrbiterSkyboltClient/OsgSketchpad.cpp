/*
Copyright 2021 Matthew Reid

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include "OpenGlContext.h"
#include "OsgSketchpad.h"

#include <osg/Drawable>
#include <assert.h>
#include <codecvt>
#include <locale>
#include <optional>


#include "ThirdParty/nanovg/nanovg.h"
#define NANOVG_GL3_IMPLEMENTATION	// Use GL2 implementation.
#include "ThirdParty/nanovg/nanovg_gl.h"

struct SketchpadCommand
{
	virtual ~SketchpadCommand() = default;
};

struct ClearCommand : SketchpadCommand
{
	osg::Vec4f color;
};

struct LineCommand : SketchpadCommand
{
	osg::Vec2i p0;
	osg::Vec2i p1;
	OsgPen pen;
};

struct RectangleCommand : SketchpadCommand
{
	osg::Vec2i p0;
	osg::Vec2i p1;
	std::optional<OsgPen> pen;
	std::optional<OsgBrush> brush;
};

struct EllipseCommand : SketchpadCommand
{
	osg::Vec2i p0;
	osg::Vec2i p1;
	std::optional<OsgPen> pen;
	std::optional<OsgBrush> brush;
};

struct PolygonCommand : SketchpadCommand
{
	std::vector<osg::Vec2i> points;
	std::optional<OsgPen> pen;
	std::optional<OsgBrush> brush;
};

struct PolylineCommand : SketchpadCommand
{
	std::vector<osg::Vec2i> points;
	OsgPen pen;
};

struct TextCommand : SketchpadCommand
{
	osg::Vec2i point;
	std::string text;
	osg::Vec4i color;
	std::optional<osg::Vec4i> backgroundColor;
	int align;
	OsgFont font;
};

class SketchpadDrawable : public osg::Drawable
{
public:
	SketchpadDrawable(const std::shared_ptr<NVGcontext>& nvgContext) : m_nvgContext(nvgContext)
	{
		assert(m_nvgContext);
	}

	void drawImplementation(osg::RenderInfo& renderInfo) const override
	{
		auto vg = m_nvgContext.get();

		auto camera = renderInfo.getCurrentCamera();
		auto viewport = camera->getViewport();
		nvgBeginFrame(vg, viewport->width(), viewport->height(), /* pxRatio */ 1.0);

		for (const auto& c : mCommands)
		{
			if (const auto clear = dynamic_cast<ClearCommand*>(c.get()); clear)
			{
				glClearColor(clear->color.r(), clear->color.g(), clear->color.b(), clear->color.a());
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
			}
			else if (const auto line = dynamic_cast<LineCommand*>(c.get()); line)
			{
				nvgBeginPath(vg);
				nvgMoveTo(vg, line->p0.x(), line->p0.y());
				nvgLineTo(vg, line->p1.x(), line->p1.y());

				nvgStrokeFromPen(vg, line->pen);
			}
			else if (const auto rectangle = dynamic_cast<RectangleCommand*>(c.get()); rectangle)
			{
				nvgBeginPath(vg);
				auto size = rectangle->p1 - rectangle->p0;
				nvgRect(vg, rectangle->p0.x(), rectangle->p0.y(), std::abs(size.x()), std::abs(size.y()));

				if (rectangle->brush)
				{
					nvgStrokeFromBrush(vg, *rectangle->brush);
				}

				if (rectangle->pen)
				{
					nvgStrokeFromPen(vg, *rectangle->pen);
				}
			}
			else if (const auto ellipse = dynamic_cast<EllipseCommand*>(c.get()); ellipse)
			{
				nvgBeginPath(vg);
				auto center = (ellipse->p0 + ellipse->p1) / 2;
				auto radius = (ellipse->p1 - ellipse->p0) / 2;
				nvgEllipse(vg, center.x(), center.y(), std::abs(radius.x()), std::abs(radius.y()));
				
				if (ellipse->brush)
				{
					nvgStrokeFromBrush(vg, *ellipse->brush);
				}

				if (ellipse->pen)
				{
					nvgStrokeFromPen(vg, *ellipse->pen);
				}
			}
			else if (const auto polygon = dynamic_cast<PolygonCommand*>(c.get()); polygon)
			{
				nvgBeginPath(vg);
				auto firstPoint = polygon->points.front();
				nvgMoveTo(vg, firstPoint.x(), firstPoint.y());

				for (size_t i = 1; i < polygon->points.size(); ++i)
				{
					auto point = polygon->points[i];
					nvgLineTo(vg, point.x(), point.y());
				}

				nvgClosePath(vg);

				if (polygon->brush)
				{
					nvgStrokeFromBrush(vg, *polygon->brush);
				}

				if (polygon->pen)
				{
					nvgStrokeFromPen(vg, *polygon->pen);
				}
			}
			else if (const auto polyline = dynamic_cast<PolylineCommand*>(c.get()); polyline)
			{
				nvgBeginPath(vg);
				auto firstPoint = polyline->points.front();
				nvgMoveTo(vg, firstPoint.x(), firstPoint.y());

				for (size_t i = 1; i < polyline->points.size(); ++i)
				{
					auto point = polyline->points[i];
					nvgLineTo(vg, point.x(), point.y());
				}

				nvgStrokeFromPen(vg, polyline->pen);
			}
			else if (const auto text = dynamic_cast<TextCommand*>(c.get()); text)
			{
				nvgFontSize(vg, text->font.heightPixels);
				nvgFontFace(vg, text->font.name.c_str());
				nvgTextAlign(vg, text->align);

				nvgResetTransform(vg);
				nvgTranslate(vg, text->point.x(), text->point.y());
				nvgRotate(vg, text->font.rotationRadians);

				if (text->backgroundColor)
				{
					float bounds[4];
					nvgTextBounds(vg, 0, 0, text->text.c_str(), NULL, bounds);
					nvgBeginPath(vg);
					nvgRect(vg, bounds[0],bounds[1], bounds[2]-bounds[0], bounds[3]-bounds[1]);
					nvgFillColor(vg, nvgRGBA(text->backgroundColor->r(), text->backgroundColor->g(), text->backgroundColor->b(), text->backgroundColor->a()));
					nvgFill(vg);
				}

				nvgFillColor(vg, nvgRGBA(text->color.r(), text->color.g(), text->color.b(), text->color.a()));
				nvgText(vg, 0, 0, text->text.c_str(), nullptr);

				nvgResetTransform(vg); // undo rotation
			}
		}

		nvgEndFrame(vg);

		mCommands.clear();

		// Cleanup OpenGL state after nanoVG
		glDisable(GL_BLEND);
	}

	void addCommand(std::shared_ptr<SketchpadCommand> command)
	{
		mCommands.emplace_back(std::move(command));
	}

private:
	static void nvgStrokeFromPen(NVGcontext* vg, const OsgPen& pen)
	{
		nvgStrokeWidth(vg, osg::maximum(1, pen.width));
		nvgStrokeColor(vg, nvgRGBA(pen.color.r(), pen.color.g(), pen.color.b(), pen.color.a()));
		// TODO: add support for dashed lines. Not currently supported by nanoVG.
		nvgStroke(vg);
	}

	static void nvgStrokeFromBrush(NVGcontext* vg, const OsgBrush& brush)
	{
		nvgFillColor(vg, nvgRGBA(brush.color.r(), brush.color.g(), brush.color.b(), brush.color.a()));
		nvgFill(vg);
	}

private:
	std::shared_ptr<NVGcontext> m_nvgContext;
	mutable std::vector<std::shared_ptr<SketchpadCommand>> mCommands;
};

std::shared_ptr<NVGcontext> CreateNanoVgContext()
{
	std::shared_ptr<NVGcontext> context(nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES), [] (NVGcontext* vg) {
		nvgDeleteGL3(vg);
	});
	nvgCreateFont(context.get(), "Fixed", "C:/Windows/Fonts/cour.ttf");
	nvgCreateFont(context.get(), "Sans", "C:/Windows/Fonts/arial.ttf");
	nvgCreateFont(context.get(), "Serif", "C:/Windows/Fonts/times.ttf");
	return context;
}

OsgSketchpad::OsgSketchpad(const osg::ref_ptr<osg::Camera>& camera, SURFHANDLE surface, const std::shared_ptr<NVGcontext>& context) :
	oapi::Sketchpad(surface),
	mCamera(camera)
{
	mDrawable = new SketchpadDrawable(context);
	mCamera->addChild(mDrawable);
}

OsgSketchpad::~OsgSketchpad() = default;

void OsgSketchpad::fillBackground(const osg::Vec4f& color)
{
	auto c = std::make_shared<ClearCommand>();
	c->color = color;
	mDrawable->addCommand(c);

	// Hack to reset text alignment at start of frame.
	// Unfortunatly Orbiter doesn't reset text alignment explicitly.
	SetTextAlign();
}

oapi::Font* OsgSketchpad::SetFont(oapi::Font* font) const
{
	oapi::Font* oldFont = mFont;
	mFont = dynamic_cast<OsgFont*>(font);
	return oldFont;
}

oapi::Pen* OsgSketchpad::SetPen(oapi::Pen* pen) const
{
	oapi::Pen* oldPen = mPen;
	mPen = dynamic_cast<OsgPen*>(pen);
	return oldPen;
}

oapi::Brush* OsgSketchpad::SetBrush(oapi::Brush* brush) const
{
	oapi::Brush* oldBrush = mBrush;
	mBrush = dynamic_cast<OsgBrush*>(brush);
	return oldBrush;
}

static NVGalign toNvgAlign(OsgSketchpad::TAlign_horizontal tah)
{
	switch (tah)
	{
		case OsgSketchpad::LEFT:
			return NVGalign::NVG_ALIGN_LEFT;
		case OsgSketchpad::CENTER:
			return NVGalign::NVG_ALIGN_CENTER;
		case OsgSketchpad::RIGHT:
			return NVGalign::NVG_ALIGN_RIGHT;
	}
	assert(!"Should not get here");
	return NVGalign::NVG_ALIGN_LEFT;
}

static NVGalign toNvgAlign(OsgSketchpad::TAlign_vertical tav)
{
	switch (tav)
	{
		case OsgSketchpad::TOP:
			return NVGalign::NVG_ALIGN_TOP;
		case OsgSketchpad::BASELINE:
			return NVGalign::NVG_ALIGN_BASELINE;
		case OsgSketchpad::BOTTOM:
			return NVGalign::NVG_ALIGN_BOTTOM;
	}
	assert(!"Should not get here");
	return NVGalign::NVG_ALIGN_TOP;
}

void OsgSketchpad::SetTextAlign(TAlign_horizontal tah, TAlign_vertical tav)
{
	mTextAlign = toNvgAlign(tah) | toNvgAlign(tav);
}

DWORD OsgSketchpad::SetTextColor(DWORD col)
{
	DWORD oldTextColor = mTextColor;
	mTextColor = col;
	return oldTextColor;
}

DWORD OsgSketchpad::SetBackgroundColor(DWORD col)
{
	DWORD oldTextBackgroundColor = mTextBackgroundColor;
	mTextBackgroundColor = col;
	return oldTextBackgroundColor;
}

void OsgSketchpad::SetBackgroundMode(BkgMode mode)
{
	mTextBackgroundMode = mode;
}

static float getCharacterWidth(const OsgFont& font)
{
	return font.heightPixels * 0.7f;
}

DWORD OsgSketchpad::GetCharSize()
{
	if (mFont)
	{
		return MAKELONG(mFont->heightPixels, getCharacterWidth(*mFont));
	}
	return MAKELONG(10, 7);
}

DWORD OsgSketchpad::GetTextWidth(const char *str, int len)
{
	if (len == 0)
	{
		len = strlen(str);
	}

	if (mFont)
	{
		return getCharacterWidth(*mFont) * len;
	}
	return 7 * len;
}

// From https://stackoverflow.com/questions/4804298/how-to-convert-wstring-into-string
static std::string wstringToString(const std::wstring& wstr)
{
	using convert_typeX = std::codecvt_utf8<wchar_t>;
	std::wstring_convert<convert_typeX, wchar_t> converterX;

	return converterX.to_bytes(wstr);
}

bool OsgSketchpad::Text(int x, int y, const char *str, int len)
{
	if (mFont)
	{
		auto c = std::make_shared<TextCommand>();
		c->point = osg::Vec2i(x - mOrigin.x(), y - mOrigin.y());
		c->text = std::string(str, len);
		c->color = rgbIntToVec4iColor(mTextColor);
		c->align = mTextAlign;
		c->font = *mFont;

		if (mTextBackgroundMode == BkgMode::BK_OPAQUE)
		{
			c->backgroundColor = rgbIntToVec4iColor(mTextBackgroundColor);
		}

		mDrawable->addCommand(c);
		return true;
	}
	return false;
}

bool OsgSketchpad::TextW(int x, int y, const LPWSTR wstr, int len)
{
	std::string str = wstringToString(std::wstring(wstr, len));
	return Text(x, y, str.c_str(), len);
}

void OsgSketchpad::LineTo(int x, int y)
{
	Line(mPenPosition.x(), mPenPosition.y(), x, y);
	MoveTo(x, y);
}

void OsgSketchpad::Line(int x0, int y0, int x1, int y1)
{
	if (mPen && mPen->style != OsgPen::Style::Invisible)
	{
		auto c = std::make_shared<LineCommand>();
		c->p0 = osg::Vec2i(x0 - mOrigin.x(), y0 - mOrigin.y());
		c->p1 = osg::Vec2i(x1 - mOrigin.x(), y1 - mOrigin.y());
		c->pen = *mPen;
		mDrawable->addCommand(c);
		MoveTo(x1, y1);
	}
}

void OsgSketchpad::Rectangle(int x0, int y0, int x1, int y1)
{
	auto c = std::make_shared<RectangleCommand>();
	c->p0 = osg::Vec2i(x0 - mOrigin.x(), y0 - mOrigin.y());
	c->p1 = osg::Vec2i(x1 - mOrigin.x(), y1 - mOrigin.y());
	c->pen = mPen ? std::optional<OsgPen>(*mPen) : std::nullopt;
	c->brush = mBrush ? std::optional<OsgBrush>(*mBrush) : std::nullopt;
	mDrawable->addCommand(c);
}

void OsgSketchpad::Ellipse(int x0, int y0, int x1, int y1)
{
	auto c = std::make_shared<EllipseCommand>();
	c->p0 = osg::Vec2i(x0 - mOrigin.x(), y0 - mOrigin.y());
	c->p1 = osg::Vec2i(x1 - mOrigin.x(), y1 - mOrigin.y());
	c->pen = mPen ? std::optional<OsgPen>(*mPen) : std::nullopt;
	c->brush = mBrush ? std::optional<OsgBrush>(*mBrush) : std::nullopt;
	mDrawable->addCommand(c);
}

void OsgSketchpad::Polygon(const oapi::IVECTOR2 *pt, int npt)
{
	assert(npt > 0);
	auto c = std::make_shared<PolygonCommand>();
	for (int i = 0; i < npt; ++i)
	{
		const oapi::IVECTOR2& v = pt[i];
		c->points.emplace_back(osg::Vec2i(v.x - mOrigin.x(), v.y - mOrigin.y()));
	}
	c->pen = mPen ? std::optional<OsgPen>(*mPen) : std::nullopt;
	c->brush = mBrush ? std::optional<OsgBrush>(*mBrush) : std::nullopt;
	mDrawable->addCommand(c);
}

void OsgSketchpad::Polyline(const oapi::IVECTOR2 *pt, int npt)
{
	if (mPen && mPen->style != OsgPen::Style::Invisible)
	{
		assert(npt > 0);
		auto c = std::make_shared<PolylineCommand>();
		for (int i = 0; i < npt; ++i)
		{
			const oapi::IVECTOR2& v = pt[i];
			c->points.emplace_back(osg::Vec2i(v.x - mOrigin.x(), v.y - mOrigin.y()));
		}
		c->pen = *mPen;
		mDrawable->addCommand(c);
	}
}
