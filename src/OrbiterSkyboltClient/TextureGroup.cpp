#include "TextureGroup.h"

std::filesystem::path addSuffixToBaseFilename(const std::filesystem::path& filename, const std::string& suffix)
{
	std::filesystem::path result = filename;
	result.replace_extension("");
	result += suffix;
	result.replace_extension(filename.extension());
	return result;
}
