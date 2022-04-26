/*
Copyright 2021 Matthew Reid

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "OrbiterModel.h"
#include <SkyboltVis/OsgStateSetHelpers.h>

#include <osg/Drawable>
#include <osg/Geode>
#include <assert.h>

using namespace skybolt;

OrbiterModel::OrbiterModel(const OrbiterModelConfig& config) :
	Model(config),
	mOwningObject(config.owningObject),
	mMeshId(config.meshId),
	mMeshVisibilityCategoryFlags(config.meshVisibilityCategoryFlags),
	mMeshGroupToGeometryIndex(config.meshGroupToGeometryIndex)
{
}

void OrbiterModel::setMeshTexture(int groupId, const osg::ref_ptr<osg::Texture2D>& texture)
{
	osg::Drawable* drawable = getDrawable(groupId);
	if (drawable)
	{
		drawable->getOrCreateStateSet()->setTextureAttributeAndModes(0, texture);
	}
}

void OrbiterModel::useMeshAsMfd(int groupId, const osg::ref_ptr<osg::Program>& program, bool alphaBlend)
{
	if (mMfdGroupIds.find(groupId) == mMfdGroupIds.end())
	{
		osg::Drawable* drawable = getDrawable(groupId);
		if (drawable)
		{
			osg::ref_ptr<osg::StateSet> stateSet = new osg::StateSet();
			stateSet->setAttribute(program, osg::StateAttribute::ON);
			stateSet->setDefine("FLIP_V");
			stateSet->addUniform(vis::createUniformSampler2d("albedoSampler", 0));
			if (alphaBlend)
			{
				vis::makeStateSetTransparent(*stateSet, vis::TransparencyMode::Classic);
			}
			else
			{
				stateSet->setDefine("OUTPUT_PREMULTIPLIED_ALPHA");
			}
			drawable->setStateSet(stateSet);

			mMfdGroupIds.insert(groupId);
		}
	}
}

osg::Drawable* OrbiterModel::getDrawable(int groupId) const
{
	osg::Geode* geode = mNode->asGeode();
	if (geode)
	{
		if (groupId >= 0 && groupId < int(mMeshGroupToGeometryIndex.size()))
		{
			int drawableIndex = mMeshGroupToGeometryIndex[groupId];
			if (drawableIndex >= 0 && drawableIndex < int(geode->getNumDrawables()))
			{
				return geode->getDrawable(drawableIndex);
			}
		}
	}
	return nullptr;
}
