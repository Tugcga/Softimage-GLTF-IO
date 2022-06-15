#include <fstream>
#include <array>

#include "utilities.h"

FileType detect_file_type(const std::string& path)
{
	// Quickly open the file as binary and check if there's the gltf binary magic number
	{
		auto probe = std::ifstream(path, std::ios_base::binary);
		if (!probe) return FileType::Unknown;

		std::array<char, 5> buffer;
		for (size_t i{ 0 }; i < 4; ++i) probe >> buffer[i];
		buffer[4] = 0;

		if (std::string("glTF") == std::string(buffer.data()))
		{
			return FileType::Binary;
		}
	}

	// If we don't have any better, check the file extension.
	auto extension = path.substr(path.find_last_of('.') + 1);
	std::transform(std::begin(extension), std::end(extension), std::begin(extension), [](char c)
	{
		return char(::tolower(int(c)));
	});
	if (extension == "gltf") return FileType::Ascii;
	if (extension == "glb") return FileType::Binary;

	return FileType::Unknown;
}