/*
Copyright 2021 Matthew Reid

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "OrbiterEntityFactory.h"

#include "DistantCelestialBodyFactory.h"
#include "ModelFactory.h"
#include "OrbiterModel.h"
#include "ObjectUtil.h"
#include <SkyboltEngine/EntityFactory.h>
#include <SkyboltEngine/VisObjectsComponent.h>
#include <SkyboltEngine/SimVisBinding/SimVisBinding.h>
#include <SkyboltSim/Entity.h>
#include <SkyboltVis/Scene.h>
#include <SkyboltSim/Components/NameComponent.h>
#include <SkyboltSim/Components/Node.h>
#include <SkyboltVis/Renderable/Model/Model.h>
#include <SkyboltVis/Shader/ShaderProgramRegistry.h>

#include "VesselAPI.h"

using namespace oapi;
using namespace skybolt;

OrbiterEntityFactory::OrbiterEntityFactory(const OrbiterEntityFactoryConfig& config) :
	mEntityFactory(config.entityFactory),
	mScene(config.scene),
	mModelFactory(config.modelFactory),
	mGraphicsClient(config.graphicsClient),
	mShaderPrograms(config.shaderPrograms)
{
	assert(mEntityFactory);
	assert(mScene);
	assert(mModelFactory);
	assert(mGraphicsClient);
	assert(mShaderPrograms);
}

OrbiterEntityFactory::~OrbiterEntityFactory() = default;

sim::EntityPtr OrbiterEntityFactory::createEntity(OBJHANDLE object) const
{
	switch (oapiGetObjectType(object))
	{
		case OBJTP_VESSEL:
			return createVessel(object, oapiGetVesselInterface(object));
		case OBJTP_PLANET:
			return createPlanet(object);
		case OBJTP_STAR:
			return createStar(object);
		case OBJTP_SURFBASE:
			return createBase(object);
		default:
			return nullptr;
	}
}

osg::Vec3d orbiterVector3ToOsg(const VECTOR3& v)
{
	return osg::Vec3d(v.z, v.x, -v.y);
}

sim::EntityPtr OrbiterEntityFactory::createVessel(OBJHANDLE object, VESSEL* vessel) const
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
			int visFlags = (int)vessel->GetMeshVisibilityMode(i);
			if (visFlags != MESHVIS_NEVER)
			{
				VECTOR3 offset;
				vessel->GetMeshOffset(i, offset);

				vis::ModelPtr model = mModelFactory->createModel(hMesh, object, i, visFlags);
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

extern Orbiter *g_pOrbiter;

sim::EntityPtr OrbiterEntityFactory::createPlanet(OBJHANDLE object) const
{
	std::string name = getName(object);

	double radius = oapiGetSize(object);

	char cbuf[256];
	mGraphicsClient->PlanetTexturePath(name.c_str(), cbuf);
	std::string planetTexturePath = cbuf;

	nlohmann::json planetJson = {
		{"radius", radius},
		{"ocean", false},
		{"surface", {
			{"elevation", {
				{"format", "orbiterElevation"},
				{"url", planetTexturePath},
				{"maxLevel", 13} // TODO: determine correct maximum for tile source used
			}},
			{"albedo", {
				{"format", "orbiterImage"},
				{"url", planetTexturePath},
				{"maxLevel", 13}
			}},
			{"uniformDetail", {
				{"texture", "Environment/Ground/Ground026_1K_Color.jpg"}
			}},
		}}
	};

	if (oapiPlanetHasAtmosphere(object))
	{
		// TODO: Map constants to our scattering model for planets other than Earth.
		// Earth should always use the most accurate parameters from Bruenton's model.
		//const ATMCONST* constants = oapiGetPlanetAtmConstants(object);
		
		if (name == "Earth")
		{
			planetJson["atmosphere"] = {
				{"earthReyleighScatteringCoefficient", 1.24062e-6},
				{"rayleighScaleHeight", 8000.0},
				{"mieScaleHeight", 1200.0},
				{"mieAngstromAlpha", 0.0},
				{"mieAngstromBeta", 5.328e-3},
				{"mieSingleScatteringAlbedo", 0.9},
				{"miePhaseFunctionG", 0.8},
				{"useEarthOzone", true}
			};

			planetJson["ocean"] = true;

			planetJson["clouds"] = {
				{"map", "Environment/Cloud/cloud_combined_8192.png"}
			};
		}
		else if (name == "Mars")
		{
			planetJson["atmosphere"] = {
				{"reyleighScatteringCoefficientTable", {
					{"coefficients", {5.8e-6, 5.8e-6, 20.0e-6}},
					{"wavelengthsNm", {440, 510, 680}},
				}},
				{"rayleighScaleHeight", 11000.0},
				{"mieScaleHeight", 1600.0},
				{"mieAngstromAlpha", 0.0},
				{"mieAngstromBeta", 5.328e-3},
				{"mieSingleScatteringAlbedo", 0.9},
				{"miePhaseFunctionG", 0.8},
				{"useEarthOzone", false},
				{"bottomRadius", radius - 4000},
				{"topRadius", radius + 75000},
			};
		}
	}

	nlohmann::json j = {
		{"components", {{
			{"planet", planetJson}
		}}}
	};

	return mEntityFactory->createEntityFromJson(j, name, math::dvec3Zero(), math::dquatIdentity());
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

	for (int i = 0; i < (int)nsbs; i++)
	{
		MESHHANDLE mesh = sbs[i];
		vis::ModelPtr model = mModelFactory->createModel(mesh, object, i, MESHVIS_EXTERNAL);
		visObjectsComponent->addObject(model);

		SimVisBindingPtr simVis(new SimpleSimVisBinding(entity.get(), model,
			osg::Vec3(),
			osg::Quat()
		));
		simVisBindingComponent->bindings.push_back(simVis);
	}
	for (int i = 0; i < (int)nsas; i++)
	{
		MESHHANDLE mesh = sas[i];
		vis::ModelPtr model = mModelFactory->createModel(mesh, object, i, MESHVIS_EXTERNAL);
		visObjectsComponent->addObject(model);

		SimVisBindingPtr simVis(new SimpleSimVisBinding(entity.get(), model,
			osg::Vec3(),
			osg::Quat()
		));
		simVisBindingComponent->bindings.push_back(simVis);
	}

	return entity;
}

skybolt::sim::EntityPtr OrbiterEntityFactory::createStar(OBJHANDLE object) const
{
	if (getName(object) == "Sun")
	{
		DistantCelestialBodyCreationArgs args;
		args.scene = mScene.get();
		args.objectHandle = object;
		args.program = mShaderPrograms->getRequiredProgram("sun");
		return createDistantCelestialBody(args);
	}
	return nullptr;
}
