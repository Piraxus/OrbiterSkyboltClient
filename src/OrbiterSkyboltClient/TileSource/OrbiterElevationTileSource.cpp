/*
Copyright 2021 Matthew Reid

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include "OrbiterElevationTileSource.h"
#include "OrbiterSkyboltClient/ThirdParty/ztreemgr.h"
#include <SkyboltVis/Renderable/Planet/Tile/HeightMap.h>

#include <osgDB/Registry>
#include <boost/scope_exit.hpp>

using namespace skybolt;

OrbiterElevationTileSource::OrbiterElevationTileSource(const std::string& directory) :
	OrbiterTileSource(std::make_unique<ZTreeMgr>(directory.c_str(), ZTreeMgr::LAYER_ELEV))
{
}

#pragma pack(push,1)
//! From Orbiter developer documentation 'PlanetTextures.odt'
struct ELEVFILEHEADER { // file header for patch elevation data file
	char id[4];            // ID string + version ('E','L','E',1)
	int hdrsize;           // header size (100 expected)
	int dtype;             // data format (0=flat, no data block;
						   // 8=uint8; -8=int8; 16=uint16; -16=int16)
	int xgrd, ygrd;         // data grid size (259 x 259 expected)
	int xpad, ypad;         // horizontal, vertical padding width
						   // (1, 1 expected)
	double scale;          // data scaling factor (1.0 expected)
	double offset;         // data offset (elevation =
						   // raw value * scale + offset)
	double latmin, latmax; // latitude range [rad]
	double lngmin, lngmax; // longitude range [rad]
	double emin, emax, emean; // min, max, mean elevation [m]
};
#pragma pack(pop)

osg::ref_ptr<osg::Image> OrbiterElevationTileSource::createImage(const std::uint8_t* buffer, std::size_t sizeBytes) const
{
	if (sizeBytes < sizeof(ELEVFILEHEADER))
	{
		return nullptr;
	}

	const ELEVFILEHEADER& header = reinterpret_cast<const ELEVFILEHEADER&>(*buffer);

	osg::ref_ptr<osg::Image> image = new osg::Image();
	image->allocateImage(256, 256, 1, GL_LUMINANCE, GL_UNSIGNED_SHORT);
	image->setInternalTextureFormat(GL_R16);
	uint16_t* ptr = (uint16_t*)image->getDataPointer();

	constexpr int expectedWidth = 259;
	constexpr int expectedHeight = 259;
	if (header.xgrd != expectedWidth && header.ygrd != expectedHeight)
	{
		return nullptr;
	}

	if (header.dtype == 8) // uint8
	{
		if ((int)sizeBytes > header.hdrsize + expectedWidth * expectedHeight)
		{
			return nullptr;
		}

		const std::uint8_t* source = buffer + header.hdrsize;
		for (int y = 1; y < 257; ++y)
		{
			for (int x = 1; x < 257; ++x)
			{
				*ptr++ = vis::getHeightmapSeaLevelValueInt() + source[y * 259 + x] + int(header.offset);
			}
		}
	}
	else if (header.dtype == -16) // int16
	{
		if ((int)sizeBytes > header.hdrsize + expectedWidth * expectedHeight * sizeof(std::int16_t))
		{
			return nullptr;
		}

		const std::int16_t* source = reinterpret_cast<const std::int16_t*>(buffer + header.hdrsize);
		for (int y = 1; y < 257; ++y)
		{
			for (int x = 1; x < 257; ++x)
			{
				// TODO: add support to skybolt for tile offsets and remove clamp. This needed for bodies like as Vesta
				// which have a height range exceeding 16 bit limits.
				*ptr++ = std::clamp(vis::getHeightmapSeaLevelValueInt() + source[y * 259 + x] + int(header.offset), 0, 65535);
			}
		}
	}
	else // if (header.dtype == 0) // flat
	{
		for (int i = 0; i < 256 * 256; ++i)
		{
			*ptr++ = vis::getHeightmapSeaLevelValueInt() + header.emean;
		}
	}

	return image;
}
