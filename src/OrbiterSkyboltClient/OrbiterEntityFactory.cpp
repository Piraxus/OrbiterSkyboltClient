/*
Copyright 2021 Matthew Reid

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "OrbiterEntityFactory.h"
#include "ModelFactory.h"
#include "ObjectUtil.h"
#include "OrbiterVisibilityCategory.h"
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
	mModelFactory(config.modelFactory),
	mGraphicsClient(config.graphicsClient)
{
	assert(mEntityFactory);
	assert(mScene);
	assert(mModelFactory);
	assert(mGraphicsClient);
}

OrbiterEntityFactory::~OrbiterEntityFactory() = default;

sim::EntityPtr OrbiterEntityFactory::createEntity(OBJHANDLE object) const
{
	switch (oapiGetObjectType(object))
	{
		case OBJTP_VESSEL:
			return createVessel(oapiGetVesselInterface(object));
		case OBJTP_PLANET:
			return createPlanet(object);
		//case OBJTP_STAR:
		//	return new vStar(_hObj, scene);
		case OBJTP_SURFBASE:
			return createBase(object);
//		default:
	//		return new vObject(_hObj, scene);
		default:
			return nullptr;
	}
}

osg::Vec3d orbiterVector3ToOsg(const VECTOR3& v)
{
	return osg::Vec3d(v.z, v.x, -v.y);
}

static int toVisibilityCategoryMask(int vismode)
{
	int mask = vis::VisibilityCategory::defaultCategories;
	if (vismode & MESHVIS_COCKPIT)
	{
		mask |= OrbiterVisibilityCategory::cockpitView;
	}
	if (vismode & MESHVIS_VC)
	{
		mask |= OrbiterVisibilityCategory::virtualCockpitView;
	}
	if (vismode & MESHVIS_EXTERNAL)
	{
		mask |= OrbiterVisibilityCategory::externalView;
	}
	return mask;
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
			if (vismode != MESHVIS_NEVER)
			{
				VECTOR3 offset;
				vessel->GetMeshOffset(i, offset);

				vis::ModelPtr model = mModelFactory->createModel(hMesh);
				model->setVisibilityCategoryMask(toVisibilityCategoryMask(vismode));

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

sim::EntityPtr OrbiterEntityFactory::createPlanet(OBJHANDLE object) const
{
	std::string name = getName(object);
	if (name == "Earth")
	{
		return mEntityFactory->createEntity("PlanetEarth");
	}
	return nullptr;
}

sim::EntityPtr OrbiterEntityFactory::createBase(OBJHANDLE object) const
{
	sim::EntityPtr entity = std::make_shared<sim::Entity>();

	SimVisBindingsComponentPtr simVisBindingComponent(new SimVisBindingsComponent);
	entity->addComponent(simVisBindingComponent);

	VisObjectsComponentPtr visObjectsComponent(new VisObjectsComponent(mScene.get()));
	entity->addComponent(visObjectsComponent);

	entity->addComponent(std::make_shared<sim::Node>());

	MESHHANDLE *sbs, *sas;
	DWORD nsbs, nsas;
	mGraphicsClient->GetBaseStructures(object, &sbs, &nsbs, &sas, &nsas);

	for (int i = 0; i < nsbs; i++)
	{
		MESHHANDLE mesh = sbs[i];
		vis::ModelPtr model = mModelFactory->createModel(mesh);
		visObjectsComponent->addObject(model);

		SimVisBindingPtr simVis(new SimpleSimVisBinding(entity.get(), model,
			osg::Vec3(),
			osg::Quat()
		));
		simVisBindingComponent->bindings.push_back(simVis);
	}
	for (int i = 0; i < nsas; i++)
	{
		MESHHANDLE mesh = sas[i];
		vis::ModelPtr model = mModelFactory->createModel(mesh);
		visObjectsComponent->addObject(model);

		SimVisBindingPtr simVis(new SimpleSimVisBinding(entity.get(), model,
			osg::Vec3(),
			osg::Quat()
		));
		simVisBindingComponent->bindings.push_back(simVis);
	}

	return entity;
}
