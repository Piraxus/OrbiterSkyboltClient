/*
Copyright 2021 Matthew Reid

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include "OrbiterImageTileSource.h"
#include "MemoryStreamBuf.h"
#include "OrbiterSkyboltClient/ThirdParty/ztreemgr.h"

#include <osgDB/Registry>
#include <boost/scope_exit.hpp>

OrbiterImageTileSource::OrbiterImageTileSource(const std::string& directory) :
	OrbiterTileSource(std::make_unique<ZTreeMgr>(directory.c_str(), ZTreeMgr::LAYER_SURF))
{
}

osg::ref_ptr<osg::Image> OrbiterImageTileSource::createImage(const std::uint8_t* buffer, std::size_t sizeBytes) const
{
	MemoryStreamBuf membuf((char*)(buffer), sizeBytes);
	std::istream istream(&membuf);

	osgDB::ReaderWriter *rw = osgDB::Registry::instance()->getReaderWriterForExtension("dds");
	osgDB::ReaderWriter::ReadResult res = rw->readImage(istream);
	osg::ref_ptr<osg::Image> image = res.getImage();

	if (image)
	{
		image->flipVertical();
	}

	return image;
}