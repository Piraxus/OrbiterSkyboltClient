/*
Copyright 2021 Matthew Reid

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#pragma once

#include <SkyboltVis/Renderable/Planet/Tile/TileSource/TileSource.h>

class ZTreeMgr;

class OrbiterTileSource : public skybolt::vis::TileSource
{
public:
	OrbiterTileSource(std::unique_ptr<ZTreeMgr> treeMgr);
	~OrbiterTileSource() override;

	//!@ThreadSafe
	osg::ref_ptr<osg::Image> createImage(const skybolt::QuadTreeTileKey& key, std::function<bool()> cancelSupplier) const;

	//!@ThreadSafe
	bool hasAnyChildren(const skybolt::QuadTreeTileKey& key) const override;

	//! @returns the highest key with source data in the given key's ancestral hierarchy
	//!@ThreadSafe
	virtual std::optional<skybolt::QuadTreeTileKey> getHighestAvailableLevel(const skybolt::QuadTreeTileKey& key) const  override;

protected:
	virtual osg::ref_ptr<osg::Image> createImage(const std::uint8_t* buffer, std::size_t sizeBytes)const = 0;

private:
	std::unique_ptr<ZTreeMgr> mTreeMgr;
	mutable std::mutex mTreeMgrMutex;
};