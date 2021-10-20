#pragma once

#include <SkyboltCommon/Math/Box2.h>
#include <osg/Camera>
#include <osg/Texture2D>

#include <functional>

class TextureBlitter : public osg::Camera::DrawCallback
{
public:
	bool blitTexture(const osg::Texture2D& src, osg::Texture2D& dst, const skybolt::Box2i& srcRect, const skybolt::Box2i& dstRect);
	
private:
	void operator() (osg::RenderInfo& renderInfo) const override;

private:
	using Operation = std::function<void(osg::RenderInfo& renderInfo)>;
	mutable std::vector<Operation> mOperations;
};
