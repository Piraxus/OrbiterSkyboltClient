#pragma once

#include <GraphicsAPI.h>

#include <SkyboltEngine/SkyboltEngineFwd.h>
#include <SkyboltSim/SkyboltSimFwd.h>

#include <functional>

class SkyboltParticleStream : public oapi::ParticleStream
{
public:
	using EntityFinder = std::function<skybolt::sim::EntityPtr(OBJHANDLE)>;
	using DestructionAction = std::function<void(SkyboltParticleStream*)>;
	SkyboltParticleStream(oapi::GraphicsClient* gc, PARTICLESTREAMSPEC *pss,
		const skybolt::EntityFactory& entityFactory, skybolt::sim::World* world, EntityFinder entityFinder, DestructionAction destructionAction);

	~SkyboltParticleStream();

	void update();

private:
	skybolt::sim::World* mWorld;
	EntityFinder mEntityFinder;
	DestructionAction mDestructionAction;
	skybolt::sim::EntityPtr mEntity;
	skybolt::sim::AttachmentComponentPtr mAttachmentComponent;
	skybolt::sim::ParticleEmitterPtr mParticleEmitter;
};
