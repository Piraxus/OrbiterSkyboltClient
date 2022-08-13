/*
Copyright 2021 Matthew Reid

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "ModelFactory.h"
#include "ObjectUtil.h"
#include "OrbiterModel.h"
#include "OrbiterTextureIds.h"

#include <SkyboltVis/OsgImageHelpers.h>
#include <SkyboltVis/OsgStateSetHelpers.h>
#include <SkyboltVis/OsgGeometryFactory.h>
#include <SkyboltVis/OsgGeometryHelpers.h>
#include <osg/BlendEquation>
#include <osg/BlendFunc>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/CullFace>
#include <osgUtil/TangentSpaceGenerator>

#include <assert.h>

using namespace oapi;
using namespace skybolt;

ModelFactory::ModelFactory(const ModelFactoryConfig& config) :
	mSurfaceHandleFromTextureIdProvider(std::move(config.surfaceHandleFromTextureIdProvider)),
	mTextureProvider(std::move(config.textureProvider)),
	mProgram(config.program)
{
}

std::unique_ptr<OrbiterModel> ModelFactory::createModel(MESHHANDLE hMesh, OBJHANDLE handle, int meshId, int meshVisibilityCategoryFlags) const
{
	auto result = getOrCreateMesh(hMesh);
	OrbiterModelConfig config;
	config.node = result.node;
	config.owningObject = handle;
	config.meshId = meshId;
	config.meshVisibilityCategoryFlags = meshVisibilityCategoryFlags;
	config.meshGroupData = result.meshGroupData;
	return std::make_unique<OrbiterModel>(config);
}

static bool validateIndices(const MESHGROUP& data)
{
	for (int i = 0; i < (int)data.nIdx; ++i)
	{
		if (data.Idx[i] >= data.nVtx)
		{
			return false;
		}
	}
	return true;
}

osg::ref_ptr<osg::Geometry> ModelFactory::createGeometry(const MESHGROUP& data)
{
	assert(data.nVtx > 0);
	assert(data.nIdx > 0);

	osg::Geometry* geometry = new osg::Geometry();

	osg::Vec3Array* vertices = new osg::Vec3Array(data.nVtx);
	osg::Vec3Array* normals = new osg::Vec3Array(data.nVtx);
	osg::Vec2Array* uvs = new osg::Vec2Array(data.nVtx);
	osg::ref_ptr<osg::UIntArray> indexBuffer = new osg::UIntArray(data.nIdx);

	osg::BoundingBox boundingBox;

	for (int i = 0; i < (int)data.nVtx; ++i)
	{
		const auto& v = data.Vtx[i];
		// Note swap of coordinates from left-handed to right-handed
		osg::Vec3f pos = orbiterToSkyboltVector3BodyAxes(&v.x);
		boundingBox.expandBy(pos);

		(*vertices)[i] = pos;
		(*normals)[i] = orbiterToSkyboltVector3BodyAxes(&v.nx);
		(*uvs)[i] = osg::Vec2f(v.tu, v.tv);
	}

	for (int i = 0; i < (int)data.nIdx; i += 3)
	{
		// Note swap of 2nd and 3rd index to change winding from left-handed to right-handed
		(*indexBuffer)[i] = std::clamp((int)data.Idx[i], 0, (int)data.nVtx-1);
		(*indexBuffer)[i + 1] = std::clamp((int)data.Idx[i + 2], 0, (int)data.nVtx - 1);
		(*indexBuffer)[i + 2] = std::clamp((int)data.Idx[i + 1], 0, (int)data.nVtx - 1);
	}

	geometry->setVertexArray(vertices);
	geometry->setNormalArray(normals, osg::Array::Binding::BIND_PER_VERTEX);
	geometry->setTexCoordArray(0, uvs);
	geometry->addPrimitiveSet(new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, indexBuffer->size(), (GLuint*)indexBuffer->getDataPointer()));
	vis::configureDrawable(*geometry);

	geometry->setComputeBoundingBoxCallback(vis::createFixedBoundingBoxCallback(boundingBox));

	return geometry;
}

ModelFactory::CreateMeshResult ModelFactory::getOrCreateMesh(MESHHANDLE mesh) const
{
	auto i = mMeshCache.find(mesh);
	if (i != mMeshCache.end())
	{
		return i->second;
	}
	else
	{
		osg::Geode* geode = new osg::Geode();

		DWORD nGrp = oapiMeshGroupCount(mesh);

		ModelFactory::CreateMeshResult result;
		result.node = geode;
		result.meshGroupData.resize(nGrp);

		for (DWORD i = 0; i < nGrp; i++)
		{
			MESHGROUP* group = oapiMeshGroup(mesh, i);
			if (group->nVtx > 0 && group->nIdx > 0)
			{
				MeshGroupData data;
				data.osgGeometryIndex = geode->getNumDrawables();
				data.orbiterMaterialIndex = group->MtrlIdx;
				data.orbiterTextureIndex = group->TexIdx;
				data.orbiterUserFlags = group->UsrFlag;
				result.meshGroupData[i] = data;

				auto geometry = createGeometry(*group);

				prepareGeometryStateSet(*geometry, mesh, *group);
				geode->addDrawable(geometry);
			}
		}

		geode->getOrCreateStateSet()->setAttribute(mProgram);

		mMeshCache[mesh] = result;
		return result;
	}
}

static osg::Vec4f toOsgVec4f(const COLOUR4& c)
{
	return reinterpret_cast<const osg::Vec4f&>(c);
}

void ModelFactory::prepareGeometryStateSet(osg::Geometry& geometry, MESHHANDLE mesh, const MESHGROUP& group) const
{
	if (group.TexIdx == SPEC_INHERIT)
	{
		return;
	}

	osg::StateSet& stateSet = *geometry.getOrCreateStateSet();
	float specularity = 0.04;
	stateSet.addUniform(new osg::Uniform("specularity", osg::Vec3f(specularity, specularity, specularity)));
	stateSet.addUniform(new osg::Uniform("roughness", 0.5f));
	stateSet.setDefine("ENABLE_SPECULAR");

	if (group.TexIdx < TEXIDX_MFD0 || group.TexIdx == SPEC_DEFAULT) // not an MFD
	{
		std::optional<TextureGroup> textureGroup;
		if (group.TexIdx != SPEC_DEFAULT)
		{
			SURFHANDLE handle = mSurfaceHandleFromTextureIdProvider(mesh, group.TexIdx);
			textureGroup = mTextureProvider(handle);
		}

		if (textureGroup)
		{
			int unit = 0;
			stateSet.setTextureAttributeAndModes(unit, textureGroup->albedo);
			stateSet.addUniform(vis::createUniformSampler2d("albedoSampler", unit++));

			if (textureGroup->normal)
			{
				stateSet.setTextureAttributeAndModes(unit, textureGroup->normal);
				stateSet.addUniform(vis::createUniformSampler2d("normalSampler", unit++));
				stateSet.setDefine("ENABLE_NORMAL_MAP");

				osg::ref_ptr<osgUtil::TangentSpaceGenerator> tsg = new osgUtil::TangentSpaceGenerator();
				tsg->generate(&geometry, 0);
				geometry.setNormalArray(tsg->getNormalArray(), osg::Array::Binding::BIND_PER_VERTEX);
				geometry.setTexCoordArray(1, tsg->getTangentArray());
			}

			if (textureGroup->specular)
			{
				stateSet.setTextureAttributeAndModes(unit, textureGroup->specular);
				stateSet.addUniform(vis::createUniformSampler2d("specularSampler", unit++));
				stateSet.setDefine("ENABLE_SPECULAR_MAP");
			}

			// All textured models in orbiter are rendered with alpha blending. They should be drawn in creation order, not transparent sorted.
			// TODO: See if we can improve performance by disabling blending on models that don't need it. Unfortunatly orbiter doesn't
			// seem to provide this information.
			stateSet.setAttributeAndModes(new osg::BlendEquation(osg::BlendEquation::FUNC_ADD, osg::BlendEquation::FUNC_ADD));
			stateSet.setAttributeAndModes(new osg::BlendFunc(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE_MINUS_SRC_ALPHA));
		}
		else
		{
			stateSet.setDefine("UNIFORM_ALBEDO");

			MATERIAL* material = oapiMeshMaterial(mesh, group.MtrlIdx);
			osg::Vec4 albedo = material ? vis::srgbToLinear(toOsgVec4f((material->diffuse))) : osg::Vec4(0, 0, 0, 1);

			stateSet.addUniform(new osg::Uniform("albedoColor", albedo));

			if (albedo.a() < 0.9999f)
			{
				vis::makeStateSetTransparent(stateSet, vis::TransparencyMode::Classic);
			}
		}
	}
	else // MFD
	{
		// MFD texture will be set later
		stateSet.addUniform(vis::createUniformSampler2d("albedoSampler", 0));
	}
}
