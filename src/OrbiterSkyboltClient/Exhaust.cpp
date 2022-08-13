/*
Copyright 2021 Matthew Reid

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <OrbiterAPI.h>
#include <VesselAPI.h>

#include "Exhaust.h"
#include "ObjectUtil.h"
#include <SkyboltEngine/SimVisBinding/GeocentricToNedConverter.h>
#include <SkyboltVis/OsgTextureHelpers.h>
#include <SkyboltVis/Renderable/Beams.h>

#include <osgDB/ReadFile>
#include <osg/Depth>

using namespace skybolt;

Exhaust::Exhaust(VESSEL* vessel, const sim::Entity* entity, const vis::BeamsPtr& beams, const TextureProvider& textureProvider) :
	SimpleSimVisBinding(entity),
	mTextureProvider(textureProvider),
	mVessel(vessel),
	mBeams(beams)
{
	assert(mVessel);
	addVisObject(mBeams);
}

static osg::Vec3f toOsgVec3f(const sim::Vector3& v)
{
	return osg::Vec3f(v.x, v.y, v.z);
}

void Exhaust::syncVis(const GeocentricToNedConverter& converter)
{
	static osg::ref_ptr<osg::Texture2D> defaultTexture = vis::createSrgbTexture(osgDB::readImageFile("Textures/Exhaust.dds"));

	SimpleSimVisBinding::syncVis(converter);
	
	std::vector<vis::Beams::BeamParams> beamsParams;

	EXHAUSTSPEC es;
	for (DWORD i = 0; i < mVessel->GetExhaustCount(); i++)
	{
		if (double level = mVessel->GetExhaustLevel(i); level > 0.0)
		{
			mVessel->GetExhaustSpec(i, &es);

			const std::optional<TextureGroup>& textureGroup = es.tex ? mTextureProvider(es.tex) : defaultTexture;
			osg::ref_ptr<osg::Texture2D> texture = textureGroup ? textureGroup->albedo : defaultTexture;
			
			float alpha = std::min(1.0, *es.level);
			if (es.modulate > 0.0)
			{
				alpha *= ((1.f - float(es.modulate)) + (float)rand() * es.modulate/(float)RAND_MAX);
			}

			vis::Beams::BeamParams params;
			params.relPosition = toOsgVec3f(orbiterToSkyboltVector3BodyAxes(*es.lpos));
			params.relDirection = -toOsgVec3f(orbiterToSkyboltVector3BodyAxes(*es.ldir));
			params.length = es.lsize;
			params.radius = es.wsize * 0.5;
			params.alpha = alpha;
			params.texture = texture;

			beamsParams.push_back(params);
		}
	}

	mBeams->setBeams(beamsParams);
}
