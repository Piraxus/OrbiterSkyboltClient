/*
Copyright 2021 Matthew Reid

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "GraphicsAPI.h"

#include <SkyboltEngine/SkyboltEngineFwd.h>
#include <SkyboltSim/SkyboltSimFwd.h>
#include <SkyboltVis/SkyboltVisFwd.h>

#include <map>
#include <memory>

class ModelFactory;
class VESSEL;

struct OrbiterEntityFactoryConfig
{
	skybolt::EntityFactory* entityFactory;
	skybolt::vis::ScenePtr scene;
	std::shared_ptr<ModelFactory> modelFactory;
	oapi::GraphicsClient* graphicsClient;
	skybolt::vis::ShaderPrograms* shaderPrograms;
};

class OrbiterEntityFactory
{
public:
	OrbiterEntityFactory(const OrbiterEntityFactoryConfig& config);
	~OrbiterEntityFactory();

	//! Returns null if entity could not be created
	skybolt::sim::EntityPtr createEntity(OBJHANDLE object) const;

	skybolt::sim::EntityPtr createVessel(OBJHANDLE object, VESSEL* vessel) const;

	skybolt::sim::EntityPtr createPlanet(OBJHANDLE object) const;

	skybolt::sim::EntityPtr createBase(OBJHANDLE object) const;

	skybolt::sim::EntityPtr createStar(OBJHANDLE object) const;

private:
	skybolt::EntityFactory* mEntityFactory;
	skybolt::vis::ScenePtr mScene;
	std::shared_ptr<ModelFactory> mModelFactory;
	oapi::GraphicsClient* mGraphicsClient;
	skybolt::vis::ShaderPrograms* mShaderPrograms;
};
