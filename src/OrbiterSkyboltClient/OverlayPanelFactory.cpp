/*
Copyright 2021 Matthew Reid

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "OverlayPanelFactory.h"

#include <SkyboltVis/OsgGeometryHelpers.h>

#include <osg/Geometry>

using namespace skybolt;

static osg::ref_ptr<osg::Geometry> create2dPanelGeometry(const MESHGROUP& data, const osg::Vec2f& scale, const osg::Vec2f& offset, bool flipV = false)
{
	osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry();

	osg::Vec3Array* vertices = new osg::Vec3Array(data.nVtx);
	osg::Vec2Array* uvs = new osg::Vec2Array(data.nVtx);
	osg::ref_ptr<osg::UIntArray> indexBuffer = new osg::UIntArray(data.nIdx);

	for (int i = 0; i < (int)data.nVtx; ++i)
	{
		const auto& v = data.Vtx[i];
		(*vertices)[i] = osg::Vec3f(v.x * scale.x() + offset.x(), v.y * scale.y() + offset.y(), 0);
		(*uvs)[i] = osg::Vec2f(v.tu, flipV ? 1.f - v.tv : v.tv);
	}

	for (int i = 0; i < (int)data.nIdx; i += 3)
	{
		// Note swap of 2 and 3rd index to change winding from left-handed to right-handed
		(*indexBuffer)[i] = data.Idx[i];
		(*indexBuffer)[i + 1] = data.Idx[i + 2];
		(*indexBuffer)[i + 2] = data.Idx[i + 1];
	}

	geometry->setVertexArray(vertices);
	geometry->setTexCoordArray(0, uvs);
	geometry->addPrimitiveSet(new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, indexBuffer->size(), (GLuint*)indexBuffer->getDataPointer()));
	vis::configureDrawable(*geometry);
	return geometry;
}

OverlayPanelFactory::OverlayPanelFactory(WindowSizeProvider windowSizeProvider, TextureProvider textureProvider, MfdSurfaceProvider mfdSurfaceProvider) :
	mWindowSizeProvider(std::move(windowSizeProvider)),
	mTextureProvider(std::move(textureProvider)),
	mMfdSurfaceProvider(std::move(mfdSurfaceProvider))
{
}

osg::ref_ptr<osg::Geode> OverlayPanelFactory::createOverlayPanel(SURFHANDLE *hSurf, MESHHANDLE hMesh, MATRIX3 *T, float alpha, bool additive)
{
	// Calculate scale and offset to convert to OpenGL NDC coordinates
	osg::Vec2i windowSize = mWindowSizeProvider();
	osg::Vec2f scale(float(T->m11) / windowSize.x(), -float(T->m22) / windowSize.y());
	osg::Vec2f offset(float(T->m13) / windowSize.x(), 1.0f - float(T->m23) / windowSize.y());

	osg::ref_ptr<osg::Geode> geode = new osg::Geode();

	DWORD ngrp = oapiMeshGroupCount(hMesh);
	for (DWORD i = 0; i < ngrp; i++) {
		MESHGROUP *grp = oapiMeshGroup(hMesh, i);
		if (grp->UsrFlag & 2) continue; // skip this group
		
		bool flipV = false;

		SURFHANDLE newsurf = nullptr;
		if (grp->TexIdx == SPEC_DEFAULT) {
			newsurf = 0;
		}
		else if (grp->TexIdx == SPEC_INHERIT) {
			// nothing to do
		}
		else if (grp->TexIdx >= TEXIDX_MFD0) {
			int mfdidx = grp->TexIdx - TEXIDX_MFD0;
			newsurf = mMfdSurfaceProvider(mfdidx);
			flipV = true; // MFD V coordinates need to be flipped. TODO: investigate why.
			if (!newsurf) continue;
		}
		else if (hSurf) {
			newsurf = hSurf[grp->TexIdx];
		}
		else {
			newsurf = oapiGetTextureHandle(hMesh, grp->TexIdx + 1);
		}

		osg::ref_ptr<osg::Geometry> geometry = create2dPanelGeometry(*grp, scale, offset, flipV);
		geometry->setCullingActive(false);
		geode->addDrawable(geometry);

		auto texture = mTextureProvider(newsurf);
		if (texture)
		{
			geometry->getOrCreateStateSet()->setTextureAttribute(0, texture->albedo);
		}
	}

	return geode;
}