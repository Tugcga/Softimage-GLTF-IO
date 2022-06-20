#pragma once
#include <xsi_application.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../tiny_gltf/tiny_gltf.h"

struct ImportMeshOptions
{
	bool weld_vertices;
};

bool import_gltf(const XSI::CString file_path);
std::vector<float> get_float_buffer(const tinygltf::Model& model, const tinygltf::Accessor& accessor, const bool decode_normalized = false);