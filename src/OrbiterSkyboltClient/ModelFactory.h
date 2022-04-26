/*
Copyright 2021 Matthew Reid

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "GraphicsAPI.h"

#include "OrbiterModel.h"
#include <SkyboltVis/SkyboltVisFwd.h>

#include <osg/Program>
#include <osg/Texture2D>

#include <functional>
#include <memory>

using SurfaceHandleFromTextureIdProvider = std::function<SURFHANDLE(MESHHANDLE mesh, int)>;
using TextureProvider = std::function<osg::ref_ptr<osg::Texture2D>(SURFHANDLE)>; //! Returns null if texture not found

struct ModelFactoryConfig
{
	SurfaceHandleFromTextureIdProvider surfaceHandleFromTextureIdProvider;
	TextureProvider textureProvider;
	osg::ref_ptr<osg::Program> program;
};

class ModelFactory
{
public:
	ModelFactory(const ModelFactoryConfig& config);
	std::unique_ptr<OrbiterModel> createModel(MESHHANDLE hMesh, OBJHANDLE handle, int meshId, int meshVisibilityCategoryFlags) const;

	static osg::ref_ptr<osg::Geometry> createGeometry(const MESHGROUP& data);

private:
	struct CreateMeshResult
	{
		osg::ref_ptr<osg::Node> node;
		std::vector<int> meshGroupToGeometryIndex; //!< Maps orbiter mesh group ID to osg geometry ID. Index is -1 if the mesh group has no geometry
	};
	ModelFactory::CreateMeshResult getOrCreateMesh(MESHHANDLE mesh) const;
	void populateStateSet(osg::StateSet& stateSet, MESHHANDLE mesh, const MESHGROUP& group) const;

private:
	SurfaceHandleFromTextureIdProvider mSurfaceHandleFromTextureIdProvider;
	TextureProvider mTextureProvider;
	osg::ref_ptr<osg::Program> mProgram;
	mutable std::map<MESHHANDLE, CreateMeshResult> mMeshCache;
};
