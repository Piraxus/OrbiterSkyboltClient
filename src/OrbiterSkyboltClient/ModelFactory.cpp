/*
Copyright 2021 Matthew Reid

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "ModelFactory.h"

#include <SkyboltVis/OsgImageHelpers.h>
#include <SkyboltVis/OsgStateSetHelpers.h>
#include <SkyboltVis/Renderable/Model/Model.h>

#include <osg/Geode>
#include <osg/Geometry>

using namespace oapi;
using namespace skybolt;

ModelFactory::ModelFactory(const ModelFactoryConfig& config) :
	mSurfaceHandleFromTextureIdProvider(std::move(config.surfaceHandleFromTextureIdProvider)),
	mTextureProvider(std::move(config.textureProvider)),
	mProgram(config.program)
{
}

std::unique_ptr<skybolt::vis::Model> ModelFactory::createModel(MESHHANDLE hMesh) const
{
	vis::ModelConfig config;
	config.node = getOrCreateMesh(hMesh);
	return std::make_unique<vis::Model>(config);
}

osg::ref_ptr<osg::Geometry> ModelFactory::createGeometry(const MESHGROUP& data)
{
	osg::Geometry* geometry = new osg::Geometry();

	osg::Vec3Array* vertices = new osg::Vec3Array(data.nVtx);
	osg::Vec3Array* normals = new osg::Vec3Array(data.nVtx);
	osg::Vec2Array* uvs = new osg::Vec2Array(data.nVtx);
	osg::UIntArray* indexBuffer = new osg::UIntArray(data.nIdx);

	for (int i = 0; i < (int)data.nVtx; ++i)
	{
		const auto& v = data.Vtx[i];
		// Note swap of coordinates from left-handed to right-handed
		(*vertices)[i] = osg::Vec3f(v.z, v.x, -v.y);
		(*normals)[i] = osg::Vec3f(v.nz, v.nx, -v.ny);
		(*uvs)[i] = osg::Vec2f(v.tu, v.tv);
	}

	for (int i = 0; i < (int)data.nIdx; i += 3)
	{
		// Note swap of 2 and 3rd index to change winding from left-handed to right-handed
		(*indexBuffer)[i] = data.Idx[i];
		(*indexBuffer)[i + 1] = data.Idx[i + 2];
		(*indexBuffer)[i + 2] = data.Idx[i + 1];
	}

	geometry->setVertexArray(vertices);
	geometry->setNormalArray(normals, osg::Array::Binding::BIND_PER_VERTEX);
	geometry->setTexCoordArray(0, uvs);
	geometry->addPrimitiveSet(new osg::DrawElementsUInt(osg::PrimitiveSet::TRIANGLES, indexBuffer->size(), (GLuint*)indexBuffer->getDataPointer()));
	return geometry;
}

osg::ref_ptr<osg::Node> ModelFactory::getOrCreateMesh(MESHHANDLE mesh) const
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
		for (DWORD i = 0; i < nGrp; i++)
		{
			MESHGROUP* group = oapiMeshGroup(mesh, i);
			auto geometry = createGeometry(*group);

			populateStateSet(*geometry->getOrCreateStateSet(), mesh, *group);
			geode->addDrawable(geometry);
		}

		geode->getOrCreateStateSet()->setAttribute(mProgram);
		mMeshCache[mesh] = geode;
		return geode;
	}
}


osg::Vec4f toOsgVec4f(const COLOUR4& c)
{
	return reinterpret_cast<const osg::Vec4f&>(c);
}

void ModelFactory::populateStateSet(osg::StateSet& stateSet, MESHHANDLE mesh, const MESHGROUP& group) const
{
	SURFHANDLE handle = mSurfaceHandleFromTextureIdProvider(mesh, group.TexIdx);

	osg::ref_ptr<osg::Texture2D> texture = mTextureProvider(handle);
	if (texture)
	{
		stateSet.setTextureAttributeAndModes(0, texture);
		stateSet.addUniform(vis::createUniformSampler2d("albedoSampler", 0));
	}
	else
	{
		stateSet.setDefine("UNIFORM_ALBEDO");

		MATERIAL* material = oapiMeshMaterial(mesh, group.MtrlIdx);
		osg::Vec4 albedo = material ? vis::srgbToLinear(toOsgVec4f((material->diffuse))) : osg::Vec4(0,0,0,1);

		stateSet.addUniform(new osg::Uniform("albedoColor", albedo));

		if (albedo.a() < 0.9999f)
		{
			vis::makeStateSetTransparent(stateSet, vis::TransparencyMode::Classic);
		}
	}
}
