/*
Copyright 2021 Matthew Reid

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "OrbiterModel.h"
#include "ObjectUtil.h"
#include <SkyboltVis/OsgStateSetHelpers.h>

#include <osg/Drawable>
#include <osg/Geometry>
#include <osg/Geode>
#include <assert.h>

using namespace skybolt;

struct AlwyasCullCallback : public osg::DrawableCullCallback
{
	bool cull(osg::NodeVisitor* nv, osg::Drawable* drawable, osg::RenderInfo* renderInfo) const override
	{
		return true;
	}
};

static void applyOrbiterUserFlags(osg::Drawable& drawable, int flags)
{
	if (flags & MeshGroupData::userFlagSkip)
	{
		drawable.setCullCallback(new AlwyasCullCallback());
	}
	else
	{
		drawable.setCullCallback(nullptr);
	}
}

OrbiterModel::OrbiterModel(const OrbiterModelConfig& config) :
	Model(config),
	mOwningObject(config.owningObject),
	mMeshId(config.meshId),
	mMeshVisibilityCategoryFlags(config.meshVisibilityCategoryFlags),
	mMeshGroupData(config.meshGroupData)
{
	for (const auto& data : mMeshGroupData)
	{
		osg::Geometry* geometry = getDrawable(*data)->asGeometry();
		if (geometry)
		{
			applyOrbiterUserFlags(*geometry, data->orbiterUserFlags);
		}
	}
}

bool OrbiterModel::getMeshGroupData(int groupId, GROUPREQUESTSPEC& grs)
{
	std::optional<MeshGroupData> data = getMeshGroupData(groupId);
	if (data)
	{
		osg::Geometry* geometry = getDrawable(*data)->asGeometry();
		if (geometry)
		{
			if (grs.Vtx)
			{
				auto vertices = reinterpret_cast<const osg::Vec3f*>(geometry->getVertexArray()->getDataPointer());
				auto normals = reinterpret_cast<const osg::Vec3f*>(geometry->getNormalArray()->getDataPointer());
				auto uvs = reinterpret_cast<const osg::Vec2f*>(geometry->getTexCoordArrayList().front()->getDataPointer());

				for (DWORD index = 0; index < grs.nVtx; ++index)
				{
					int i = grs.VtxPerm ? grs.VtxPerm[index] : index;
					osg::Vec3f orbiterVertexPos = skyboltToOrbiterVector3BodyAxes(vertices[i]);
					osg::Vec3f orbiterNormal = skyboltToOrbiterVector3BodyAxes(normals[i]);

					NTVERTEX& dst = grs.Vtx[index];
					dst.x = orbiterVertexPos.x();
					dst.y = orbiterVertexPos.y();
					dst.z = orbiterVertexPos.z();
					dst.nx = orbiterNormal.x();
					dst.ny = orbiterNormal.y();
					dst.nz = orbiterNormal.z();
					dst.tu = uvs[i].x();
					dst.tv = uvs[i].y();
				}
			}

			if (grs.Idx)
			{
				osg::Geometry::DrawElementsList elementsList;
				geometry->getDrawElementsList(elementsList);
				osg::DrawElements* elements = elementsList.front();

				grs.nIdx = elements->getNumIndices();
				{
					auto indices = reinterpret_cast<const unsigned int*>(elements->getDataPointer());

					for (DWORD index = 0; index < grs.nIdx; ++index)
					{
						int i = grs.VtxPerm ? grs.IdxPerm[index] : index;
						grs.Idx[index] = indices[i];
					}
				}
			}

			grs.MtrlIdx = data->orbiterMaterialIndex;
			grs.TexIdx = data->orbiterTextureIndex;
			grs.IdxPerm = 0;
			grs.VtxPerm = 0;
			return true;
		}
	}
	return false;
}

bool OrbiterModel::setMeshGroupData(int groupId, GROUPEDITSPEC& ges)
{
	osg::Drawable* drawable = getDrawableForGroupId(groupId);
	if (drawable)
	{
		osg::Geometry* geometry = drawable->asGeometry();
		if (geometry)
		{
			DWORD flags = ges.flags;

			if (flags & (GRPEDIT_SETUSERFLAG | GRPEDIT_ADDUSERFLAG | GRPEDIT_DELUSERFLAG))
			{
				int& userFlags = mMeshGroupData[groupId]->orbiterUserFlags;
				if (flags & GRPEDIT_SETUSERFLAG) userFlags = ges.UsrFlag;
				else if (flags & GRPEDIT_ADDUSERFLAG) userFlags |= ges.UsrFlag;
				else if (flags & GRPEDIT_DELUSERFLAG) userFlags &= ~ges.UsrFlag;

				applyOrbiterUserFlags(*geometry, userFlags);
			}

			if (flags & GRPEDIT_VTXMOD)
			{
				geometry->setDataVariance(osg::Object::DYNAMIC);

				auto& vertices = *reinterpret_cast<osg::Vec3Array*>(geometry->getVertexArray());
				auto& normals = *reinterpret_cast<osg::Vec3Array*>(geometry->getNormalArray());
				auto& uvs = *reinterpret_cast<osg::Vec2Array*>(geometry->getTexCoordArrayList().front().get());

				for (DWORD index = 0; index < ges.nVtx; ++index)
				{
					const NTVERTEX& v = ges.Vtx[index];
					int i = ges.vIdx ? ges.vIdx[index] : index;

					osg::Vec3f vertexPos = orbiterToSkyboltVector3BodyAxes(&v.x);
					osg::Vec3f normalPos = orbiterToSkyboltVector3BodyAxes(&v.nx);

					if      (flags & GRPEDIT_VTXCRDX)    vertices[i].x() = vertexPos.x();
					else if (flags & GRPEDIT_VTXCRDADDX) vertices[i].x() += vertexPos.x();
					if      (flags & GRPEDIT_VTXCRDY)    vertices[i].y() = vertexPos.y();
					else if (flags & GRPEDIT_VTXCRDADDY) vertices[i].y() += vertexPos.y();
					if      (flags & GRPEDIT_VTXCRDZ)    vertices[i].z() = vertexPos.z();
					else if (flags & GRPEDIT_VTXCRDADDZ) vertices[i].z() += vertexPos.z();
					if      (flags & GRPEDIT_VTXNMLX)    normals[i].x() = normalPos.x();
					else if (flags & GRPEDIT_VTXNMLADDX) normals[i].x() += normalPos.x();
					if      (flags & GRPEDIT_VTXNMLY)    normals[i].y() = normalPos.y();
					else if (flags & GRPEDIT_VTXNMLADDY) normals[i].y() += normalPos.y();
					if      (flags & GRPEDIT_VTXNMLZ)    normals[i].z() = normalPos.z();
					else if (flags & GRPEDIT_VTXNMLADDZ) normals[i].z() += normalPos.z();
					if      (flags & GRPEDIT_VTXTEXU)    uvs[i].x() = v.tu;
					else if (flags & GRPEDIT_VTXTEXADDU) uvs[i].x() += v.tu;
					if      (flags & GRPEDIT_VTXTEXV)    uvs[i].y() = v.tv;
					else if (flags & GRPEDIT_VTXTEXADDV) uvs[i].y() += v.tv;
				}

				if (ges.flags & GRPEDIT_VTXCRD)
				{
					vertices.dirty();
				}

				if (ges.flags & GRPEDIT_VTXNML)
				{
					normals.dirty();
				}

				if (ges.flags & GRPEDIT_VTXTEX)
				{
					uvs.dirty();
				}
			}
			return true;
		}
	}
	return false;
}

void OrbiterModel::setMeshTexture(int groupId, const osg::ref_ptr<osg::Texture2D>& texture)
{
	osg::Drawable* drawable = getDrawableForGroupId(groupId);
	if (drawable)
	{
		drawable->getOrCreateStateSet()->setTextureAttributeAndModes(0, texture);
	}
}

void OrbiterModel::useMeshAsMfd(int groupId, const osg::ref_ptr<osg::Program>& program, bool alphaBlend)
{
	if (mMfdGroupIds.find(groupId) == mMfdGroupIds.end())
	{
		std::optional<MeshGroupData> groupData = getMeshGroupData(groupId);
		if (groupData)
		{
			osg::Drawable* drawable = getDrawable(*groupData);
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

				// FIXME: Hack to make HUD visible. For some reason orbiter sets it to be invisible.
				{
					groupData->orbiterUserFlags &= ~MeshGroupData::userFlagSkip;
					applyOrbiterUserFlags(*drawable, groupData->orbiterUserFlags);
				}
			}
		}
	}
}

std::optional<MeshGroupData> OrbiterModel::getMeshGroupData(int groupId) const
{
	if (groupId >= 0 && groupId < int(mMeshGroupData.size()))
	{
		return mMeshGroupData[groupId];
	}
	return std::nullopt;
}

osg::Drawable* OrbiterModel::getDrawable(const MeshGroupData& data) const
{
	osg::Geode* geode = mNode->asGeode();
	if (geode)
	{
		return geode->getDrawable(data.osgGeometryIndex);
	}
	return nullptr;
}

osg::Drawable* OrbiterModel::getDrawableForGroupId(int groupId) const
{
	std::optional<MeshGroupData> data = getMeshGroupData(groupId);
	if (data)
	{
		return getDrawable(*data);
	}
	return nullptr;
}