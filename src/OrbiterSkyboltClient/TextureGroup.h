#pragma once

#include <osg/Texture2D>

#include <filesystem>

struct TextureGroup
{
	TextureGroup(const osg::ref_ptr<osg::Texture2D>& albedo) : albedo(albedo) {}
	TextureGroup() {}

	osg::ref_ptr<osg::Texture2D> albedo; //!< Never null
	osg::ref_ptr<osg::Texture2D> normal; //!< May be null
	osg::ref_ptr<osg::Texture2D> specular; //!< May be null
};

//! @param suffix is a name to append to the filename base name, e.g appending "_spec" to "C:/mytex.dds" gives "C:/mytex_spec.dds"
std::filesystem::path addSuffixToBaseFilename(const std::filesystem::path& filename, const std::string& suffix);
