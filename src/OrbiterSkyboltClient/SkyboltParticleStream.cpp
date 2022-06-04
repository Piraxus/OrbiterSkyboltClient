/*
Copyright 2021 Matthew Reid

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include "SkyboltParticleStream.h"
#include "ObjectUtil.h"

#include <SkyboltEngine/EntityFactory.h>
#include <SkyboltEngine/SimVisBinding/ParticlesVisBinding.h>
#include <SkyboltSim/World.h>
#include <SkyboltSim/Components/AttachmentComponent.h>
#include <SkyboltSim/Components/Node.h>
#include <SkyboltSim/Components/ParticleSystemComponent.h>
#include <SkyboltSim/Particles/ParticleSystem.h>
#include <SkyboltVis/OsgTextureHelpers.h>
#include <SkyboltCommon/Math/MathUtility.h>

#include <nlohmann/json.hpp>

#include <osgDB/ReadFile>

using namespace skybolt;
using namespace sim;

SkyboltParticleStream::SkyboltParticleStream(oapi::GraphicsClient* gc, PARTICLESTREAMSPEC* pss,
	const EntityFactory& entityFactory, World* world, EntityFinder entityFinder, DestructionAction destructionAction) :
	ParticleStream(gc, pss),
	mWorld(world),
	mEntityFinder(std::move(entityFinder)),
	mDestructionAction(std::move(destructionAction))
{
	double elevationAngleMin = math::halfPiD() - atan(0.5 * pss->srcspread);
	double elevationAngleMax = math::halfPiD();

	// calculate automatic emission rate instead of using pss->srcrate
	// because pss->srcrate values are too low to give nice looking particle streams.
	double emissionRate = std::min(1000.0, 2.0 * pss->v0 / pss->srcsize);

	nlohmann::json particleSystemJson = {
		{ "emissionRate", emissionRate },
		{ "radius", pss->srcsize * 0.5 },
		{ "elevationAngleMin", elevationAngleMin },
		{ "elevationAngleMax", elevationAngleMax },
		{ "speedMin", pss->v0 },
		{ "speedMax", pss->v0 },
		{ "upDirection", nlohmann::json::array({1, 0, 0}) },
		{ "lifetime", pss->lifetime },
		{ "radiusLinearGrowthPerSecond", pss->growthrate * 0.5 },
		{ "atmosphericSlowdownFactor", pss->atmslowdown },
		{ "zeroAtmosphericDensityAlpha", 0.0 },
		{ "earthSeaLevelAtmosphericDensityAlpha", 1.0 },
		{ "albedoTexture", "Environment/Explosion01_light_nofire.png" }
	};

	// Orbiter has no concept of particle temperature, so assume particles are hot if they are emissive
	if (pss->ltype == PARTICLESTREAMSPEC::EMISSIVE)
	{
		particleSystemJson["initialTemperatureDegreesCelcius"] = 2000;
		particleSystemJson["heatTransferCoefficent"] = 4.0;
	}

	nlohmann::json json = {
		{"components", nlohmann::json::array({
			{{"node", {
			}}},
			{{"particleSystem", particleSystemJson}}
		})}
	};

	std::string name = entityFactory.createUniqueObjectName("particles");
	mEntity = entityFactory.createEntityFromJson(json, name, Vector3(), Quaternion());

	AttachmentParams params;
	params.positionRelBody = Vector3();
	params.orientationRelBody = Quaternion(0, 0, 0, 1);
	mAttachmentComponent = std::make_shared<AttachmentComponent>(params, mEntity.get());
	mEntity->addComponent(mAttachmentComponent);

	world->addEntity(mEntity);

	auto particleSystem = mEntity->getFirstComponentRequired<sim::ParticleSystemComponent>()->getParticleSystem();
	mParticleEmitter = particleSystem->getOperationOfType<sim::ParticleEmitter>();
}

SkyboltParticleStream::~SkyboltParticleStream()
{
	mWorld->removeEntity(mEntity.get());
	mDestructionAction(this);
}

void SkyboltParticleStream::update()
{
	auto entity = hRef ? mEntityFinder(hRef) : nullptr;
	mAttachmentComponent->resetTarget(entity.get());
	mAttachmentComponent->setPositionRelBody(orbiterToSkyboltVector3BodyAxes(*pos));
	mAttachmentComponent->setOrientationRelBody(getOrientationFromDirection(-orbiterToSkyboltVector3BodyAxes(*dir)));

	mParticleEmitter->setEmissionAlphaMultiplier((entity && level) ? *level : 0.0);
	mParticleEmitter->setEmissionRateMultiplier((entity && level && *level > 0.0) ? 1.0 : 0.0);
}