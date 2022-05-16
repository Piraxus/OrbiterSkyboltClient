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
	if (mTreeMgr)
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
	}
	return false;
}

//! @return index of tile with the given key if it exists. Returns -1 if it does not exist.
//! @param highestTileOut is the key for the highest available tile found in the given tile's ancestry.
static DWORD getHighestAvailableTile(const ZTreeMgr& treeMgr, int lvl, int ilat, int ilng, skybolt::QuadTreeTileKey& highestTileOut)
{
	int idx;
	if (lvl <= orbiterLevelZeroOffset)
	{
		idx = treeMgr.Idx(lvl, ilat, ilng);
	}
	else
	{
		// Get tile index from the parent
		int plvl = lvl-1;
		int pilat = ilat/2;
		int pilng = ilng/2;
		DWORD pidx = getHighestAvailableTile(treeMgr, plvl, pilat, pilng, highestTileOut);
		if (pidx == (DWORD)-1)
		{
			// The parent does not exist, therefore this tile does not exist
			return -1;
		}
		else
		{
			int cidx = ((ilat&1) << 1) + (ilng&1);
			idx = treeMgr.TOC()[pidx].child[cidx];
		}
	}

	if (idx != -1)
	{
		highestTileOut.level = lvl;
		highestTileOut.x = ilng;
		highestTileOut.y = ilat;
	}

	return idx;
}

std::optional<skybolt::QuadTreeTileKey> OrbiterTileSource::getHighestAvailableLevel(const skybolt::QuadTreeTileKey& key) const
{
	if (mTreeMgr)
	{
		skybolt::QuadTreeTileKey result;
		result.level = -1;
		getHighestAvailableTile(*mTreeMgr, key.level + orbiterLevelZeroOffset, key.y, key.x, result);
	
		if (result.level != -1)
		{
			result.level -= orbiterLevelZeroOffset;
			return result;
		}
	}
	return std::nullopt;
}
