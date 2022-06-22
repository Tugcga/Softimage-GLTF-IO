#include <fstream>
#include <array>
#include <string>
#include <experimental/filesystem>

#include "utilities.h"

std::string get_file_extension(const std::string &path)
{
	std::string extension = path.substr(path.find_last_of('.') + 1);
	std::transform(std::begin(extension), std::end(extension), std::begin(extension), [](char c)
	{
		return char(::tolower(int(c)));
	});
	return extension;
}

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
	std::string extension = get_file_extension(path);
	if (extension == "gltf") return FileType::Ascii;
	if (extension == "glb") return FileType::Binary;

	return FileType::Unknown;
}

XSI::CString file_name_from_path(const XSI::CString &file_path)
{
	ULONG slash_position = file_path.ReverseFindString("\\");
	ULONG dot_position = file_path.ReverseFindString(".");
	return file_path.GetSubString(slash_position + 1, dot_position - slash_position - 1);
}

XSI::CString full_file_name_from_path(const XSI::CString& file_path)
{
	ULONG slash_position = file_path.ReverseFindString("\\");
	return file_path.GetSubString(slash_position + 1);
}

bool create_dir(const std::string& file_path)
{
	std::error_code code;
	bool is_create = std::experimental::filesystem::create_directories(file_path, code);

	return is_create;
}

bool is_file_exists(const XSI::CString &file_path)
{
	return std::experimental::filesystem::exists(std::string(file_path.GetAsciiString()));
}

XSI::CString file_path_to_folder(const XSI::CString &file_path)
{
	ULONG last_slash = file_path.ReverseFindString("\\");
	return file_path.GetSubString(0, last_slash);
}

bool is_extension_supported(const std::string &ext)
{
	return ext == "png" || ext == "jpg" || ext == "jpeg";
}