#include "OpenGlContext.h"
#include "TextureBlitter.h"
#include <osg/RenderInfo>

#include <gl/GL.h>
#include <windows.h>

bool TextureBlitter::blitTexture(const osg::Texture2D& src, osg::Texture2D& dst, const skybolt::Box2i& srcRect, const skybolt::Box2i& dstRect)
{
	auto op = [src = &src, dst = &dst, srcRect, dstRect](osg::RenderInfo& renderInfo) {
		int context = renderInfo.getContextID();
		auto srcObj = src->getTextureObject(context);
		auto dstObj = dst->getTextureObject(context);

		if (!srcObj)
		{
			// Load source on GPU if not already loaded.
			// This is needed in case the source is not rendered.
			src->apply(*renderInfo.getState());
			srcObj = src->getTextureObject(context);
		}

		if (!dstObj)
		{
			// Load destination on GPU if not already loaded.
			// This is needed in case the destination is not rendered.
			dst->apply(*renderInfo.getState());
			dstObj = src->getTextureObject(context);
		}

		if (!srcObj || !dstObj)
		{
			assert(!"Should not get here");
			return false;
		}

		int srcName = srcObj->id();
		int dstName = dstObj->id();

		int srcLevel = 0;
		int dstLevel = 0;

		if (srcRect.size() != dstRect.size())
		{
			return false; // TODO: how do we handle this case?
		}

		// TODO: meet alignment constraints if src and dst are compressed.
		// Currently blits fail on DDS textures because rects do not align with DDS format requirements (blocks of 4 pixels).

		glCopyImageSubData(
			srcName,
			GL_TEXTURE_2D,
			srcLevel,
			srcRect.minimum.x,
			srcRect.minimum.y,
			0,
			dstName,
			GL_TEXTURE_2D,
			dstLevel,
			dstRect.minimum.x,
			dstRect.minimum.y,
			0,
			srcRect.size().x,
			srcRect.size().y,
			1);

		return true;
	};

	mOperations.push_back(op);

	return true;
}

void TextureBlitter::operator() (osg::RenderInfo& renderInfo) const
{
	for (const auto& op : mOperations)
	{
		op(renderInfo);
	}
	mOperations.clear();
}
