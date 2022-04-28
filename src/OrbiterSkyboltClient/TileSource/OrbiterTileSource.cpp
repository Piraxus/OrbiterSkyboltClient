/*
Copyright 2021 Matthew Reid

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include "OrbiterTileSource.h"
#include "OrbiterSkyboltClient/ThirdParty/ztreemgr.h"
#include <SkyboltVis/Renderable/Planet/Tile/HeightMap.h>

#include <osgDB/Registry>
#include <boost/scope_exit.hpp>

using namespace skybolt;

OrbiterTileSource::OrbiterTileSource(std::unique_ptr<ZTreeMgr> treeMgr) :
	mTreeMgr(std::move(treeMgr))
{
	if (mTreeMgr->TOC().size() == 0) // If load failed
	{
		mTreeMgr.reset();
	}
}

OrbiterTileSource::~OrbiterTileSource() = default;

constexpr int orbiterLevelZeroOffset = 4; // Orbiter tile level numbering is skybolt level numbering +4.

osg::ref_ptr<osg::Image> OrbiterTileSource::createImage(const skybolt::QuadTreeTileKey& key, std::function<bool()> cancelSupplier) const
{
	if (!mTreeMgr)
	{
		return nullptr;
	}

	BYTE *buf;

	// ReadData is not thread-safe, requiring threads to have exclusive access
	std::scoped_lock<std::mutex> lock(mTreeMgrMutex);
	DWORD ndata = mTreeMgr->ReadData(key.level + orbiterLevelZeroOffset, key.y, key.x, &buf);

	if (ndata == 0)
	{
		return nullptr;
	}

	BOOST_SCOPE_EXIT(&mTreeMgr, &buf)
	{
		mTreeMgr->ReleaseData(buf);
	} BOOST_SCOPE_EXIT_END

	return createImage(buf, ndata);
}

bool OrbiterTileSource::hasAnyChildren(const skybolt::QuadTreeTileKey& key) const
{
	DWORD idx = mTreeMgr->Idx(key.level + orbiterLevelZeroOffset, key.y, key.x);
	if (idx != -1)
	{
		for (int c = 0; c < 4; ++c)
		{
			if (mTreeMgr->TOC()[idx].child[c] != -1)
			{
				return true;
			}
		}
	}
	return false;
}

static DWORD getHighestAvailableTile(const ZTreeMgr& treeMgr, int lvl, int ilat, int ilng, skybolt::QuadTreeTileKey& highestTileOut)
{
	if (lvl <= orbiterLevelZeroOffset) {
		return treeMgr.Idx(lvl, ilat, ilng);
	} else {
		int plvl = lvl-1;
		int pilat = ilat/2;
		int pilng = ilng/2;
		DWORD pidx = getHighestAvailableTile(treeMgr, plvl, pilat, pilng, highestTileOut);
		if (pidx == (DWORD)-1)
		{
			return pidx;
		}
		else
		{
			highestTileOut.level = plvl;
			highestTileOut.x = pilng;
			highestTileOut.y = pilat;
		}

		int cidx = ((ilat&1) << 1) + (ilng&1);
		return treeMgr.TOC()[pidx].child[cidx];
	}
}

std::optional<skybolt::QuadTreeTileKey> OrbiterTileSource::getHighestAvailableLevel(const skybolt::QuadTreeTileKey& key) const
{
	skybolt::QuadTreeTileKey result;
	result.level = -1;
	getHighestAvailableTile(*mTreeMgr, key.level + orbiterLevelZeroOffset, key.y, key.x, result);
	
	if (result.level != -1)
	{
		result.level -= orbiterLevelZeroOffset;
		return result;
	}
	return std::nullopt;
}
