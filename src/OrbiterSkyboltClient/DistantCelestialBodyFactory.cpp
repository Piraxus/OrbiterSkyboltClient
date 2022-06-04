/*
Copyright 2021 Matthew Reid

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include "DistantCelestialBodyFactory.h"
#include <SkyboltSim/Entity.h>
#include <SkyboltSim/Components/Node.h>
#include <SkyboltVis/Light.h>
#include <SkyboltVis/Renderable/Billboard.h>
#include <SkyboltVis/OsgImageHelpers.h>
#include <SkyboltVis/OsgStateSetHelpers.h>
#include <SkyboltVis/OsgTextureHelpers.h>
#include <SkyboltEngine/VisObjectsComponent.h>
#include <SkyboltEngine/SimVisBinding/SimVisBinding.h>

#include "ObjectUtil.h"

#include <OrbiterAPI.h>

#include <osg/BlendFunc>
#include <osg/Depth>
#include <osg/Texture2D>
#include <osgDB/ReadFile>

using namespace skybolt;
using namespace sim;

class StartNodeUpdater : public sim::Component
{
public:
	StartNodeUpdater(sim::Entity* entity, OBJHANDLE objectHandle) :
		mEntity(entity),
		mObjectHandle(objectHandle)
	{
		assert(mEntity);
	}

	void updatePreDynamics(TimeReal dt, TimeReal dtWallClock) override
	{
		// Set sun position
		VECTOR3 sunPosition, cameraPosition;
		oapiGetGlobalPos(mObjectHandle, &sunPosition);
		oapiCameraGlobalPos(&cameraPosition);

		setPosition(*mEntity, orbiterToSkyboltVector3GlobalAxes(sunPosition));

		// Set light direction
		VECTOR3 dir = sunPosition - cameraPosition;
		normalise(dir);

		sim::Vector3 lightDirection = orbiterToSkyboltVector3GlobalAxes(dir);
		glm::vec3 tangent, bitangent;
		math::getOrthonormalBasis(lightDirection, tangent, bitangent);
		glm::mat3 m(lightDirection, tangent, bitangent);
		setOrientation(*mEntity, glm::quat(m));
	}

private:
	sim::Entity* mEntity;
	OBJHANDLE mObjectHandle;
};

EntityPtr createDistantCelestialBody(const DistantCelestialBodyCreationArgs& args)
{
	osg::StateSet* ss = new osg::StateSet;
	ss->setAttribute(args.program);
	ss->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
	ss->setMode(GL_DEPTH_CLAMP, osg::StateAttribute::ON);

	osg::Depth* depth = new osg::Depth;
	depth->setWriteMask(false);
	ss->setAttributeAndModes(depth, osg::StateAttribute::ON);

	osg::ref_ptr<osg::Texture2D> texture = vis::createSrgbTexture(osgDB::readImageFile("Textures/Star.dds"));
	ss->setTextureAttributeAndModes(0, texture);
	ss->addUniform(vis::createUniformSampler2d("albedoSampler", 0));

	osg::ref_ptr<osg::BlendFunc> blendFunc = new osg::BlendFunc;
	ss->setAttributeAndModes(blendFunc);

	EntityPtr entity(new Entity());
	entity->addComponent(std::make_shared<Node>());

	entity->addComponent(std::make_shared<StartNodeUpdater>(entity.get(), args.objectHandle));

	float diameterScale = 6.5f; // account for disk in texture image smaller than full image size
	float diameter = oapiGetSize(args.objectHandle) * diameterScale;
	vis::RootNodePtr node(new vis::Billboard(ss, diameter, diameter));

	auto visObjectsComponent = std::make_shared<VisObjectsComponent>(args.scene);
	entity->addComponent(visObjectsComponent);
	visObjectsComponent->addObject(node);

	vis::LightPtr light(new vis::Light(osg::Vec3f(-1,0,0)));
	visObjectsComponent->addObject(light);

	SimVisBindingsComponentPtr simVisBindingComponent(new SimVisBindingsComponent);
	entity->addComponent(simVisBindingComponent);
	
	auto binding = std::make_shared<SimpleSimVisBinding>(entity.get());
	binding->addVisObject(node);
	binding->addVisObject(light);
	simVisBindingComponent->bindings.push_back(binding);

	return entity;
}
