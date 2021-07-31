/*
Copyright 2021 Matthew Reid

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "OrbiterEntityFactory.h"
#include "ModelFactory.h"
#include "ObjectUtil.h"
#include <SkyboltEngine/EntityFactory.h>
#include <SkyboltEngine/VisObjectsComponent.h>
#include <SkyboltEngine/SimVisBinding/SimVisBinding.h>
#include <SkyboltSim/Entity.h>
#include <SkyboltSim/Components/NameComponent.h>
#include <SkyboltSim/Components/Node.h>
#include <SkyboltVis/Renderable/Model/Model.h>
#include <SkyboltVis/Scene.h>

#include "VesselAPI.h"

using namespace oapi;
using namespace skybolt;

OrbiterEntityFactory::OrbiterEntityFactory(const OrbiterEntityFactoryConfig& config) :
	mEntityFactory(config.entityFactory),
	mScene(config.scene),
	mModelFactory(config.modelFactory)
{
	assert(mEntityFactory);
	assert(mScene);
	assert(mModelFactory);
}

OrbiterEntityFactory::~OrbiterEntityFactory() = default;

sim::EntityPtr OrbiterEntityFactory::createEntity(OBJHANDLE object) const
{
	std::string name = getName(object);
	if (name == "Earth")
	{
		return mEntityFactory->createEntity("PlanetEarth");
	}

	switch (oapiGetObjectType(object))
	{
		case OBJTP_VESSEL:
			return createVessel(oapiGetVesselInterface(object));
		default:
			return nullptr;
			// TODO
			/*
		case OBJTP_PLANET:
			return new vPlanet(_hObj, scene);
		case OBJTP_STAR:
			return new vStar(_hObj, scene);
		case OBJTP_SURFBASE:
			return new vBase(_hObj, scene);
		default:
			return new vObject(_hObj, scene);
			*/
	}
}

osg::Vec3d orbiterVector3ToOsg(const VECTOR3& v)
{
	return osg::Vec3d(v.z, v.x, -v.y);
}

sim::EntityPtr OrbiterEntityFactory::createVessel(VESSEL* vessel) const
{
	sim::EntityPtr entity = std::make_shared<sim::Entity>();

	SimVisBindingsComponentPtr simVisBindingComponent(new SimVisBindingsComponent);
	entity->addComponent(simVisBindingComponent);

	VisObjectsComponentPtr visObjectsComponent(new VisObjectsComponent(mScene.get()));
	entity->addComponent(visObjectsComponent);

	entity->addComponent(std::make_shared<sim::Node>());

	int nmesh = (int)vessel->GetMeshCount();

	for (int i = 0; i < nmesh; ++i)
	{
		if (auto hMesh = vessel->GetMeshTemplate(i); hMesh)
		{
			int vismode = (int)vessel->GetMeshVisibilityMode(i);
			if (vismode != MESHVIS_NEVER)// && !(vismode & MESHVIS_EXTERNAL)) // TODO: mesh visibility layers
			{
				VECTOR3 offset;
				vessel->GetMeshOffset(i, offset);

				vis::ModelPtr model = mModelFactory->createModel(hMesh);
				visObjectsComponent->addObject(model);

				SimVisBindingPtr simVis(new SimpleSimVisBinding(entity.get(), model,
					orbiterVector3ToOsg(offset),
					osg::Quat()
				));
				simVisBindingComponent->bindings.push_back(simVis);
			}
		}
	}

	return entity;
}
