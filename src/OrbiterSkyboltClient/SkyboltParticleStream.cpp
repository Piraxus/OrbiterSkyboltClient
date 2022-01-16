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
	double halfSpeedSpread = pss->srcspread * 0.5; 

	// calculate automatic emission rate instead of using pss->srcrate
	// because pss->srcrate values are too low to give nice looking particle streams.
	double emissionRate = std::min(1000.0, 2.0 * pss->v0 / pss->srcsize);

	nlohmann::json json = {
	{"components", nlohmann::json::array({
		{{"node", {
		}}},
		{{"particleSystem", {
			{ "emissionRate", emissionRate },
			{ "radius", pss->srcsize * 0.5 },
			{ "elevationAngleMin", math::halfPiD() },
			{ "elevationAngleMax", math::halfPiD() },
			{ "speedMin", pss->v0 - halfSpeedSpread },
			{ "speedMax", pss->v0 + halfSpeedSpread },
			{ "upDirection", nlohmann::json::array({1, 0, 0}) },
			{ "lifetime", pss->lifetime },
			{ "radiusLinearGrowthPerSecond", pss->growthrate * 0.5 },
			{ "atmosphericSlowdownFactor", pss->atmslowdown },
			{ "albedoTexture", "Environment/Explosion01_light_nofire.png" }
		}}}
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
	mAttachmentComponent->setPositionRelBody(toSkyboltVector3BodyAxes(*pos));
	mAttachmentComponent->setOrientationRelBody(getOrientationFromDirection(-toSkyboltVector3BodyAxes(*dir)));

	mParticleEmitter->setEmissionAlphaMultiplier((entity && level) ? *level : 0.0);
	mParticleEmitter->setEmissionRateMultiplier((entity && level && *level > 0.0) ? 1.0 : 0.0);
}