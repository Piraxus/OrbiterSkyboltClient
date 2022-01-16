#pragma once

#include <SkyboltSim/SkyboltSimFwd.h>
#include <SkyboltVis/SkyboltVisFwd.h>

#include <osg/Program>

typedef void* OBJHANDLE;

struct DistantCelestialBodyCreationArgs
{
	skybolt::vis::Scene* scene;
	OBJHANDLE objectHandle;
	osg::ref_ptr<osg::Program> program;
};

skybolt::sim::EntityPtr createDistantCelestialBody(const DistantCelestialBodyCreationArgs& args);
