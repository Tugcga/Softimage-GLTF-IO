#pragma once
#include <xsi_application.h>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../tiny_gltf/tiny_gltf.h"

struct ImportMeshOptions
{
	bool is_import_normals;
	bool is_import_uvs;
	bool is_import_colors;
	bool is_import_shapes;
	bool is_import_skin;
};

bool import_gltf(const XSI::CString file_path,
	const bool is_import_normals,
	const bool is_import_uvs,
	const bool is_import_colors,
	const bool is_import_shapes,
	const bool is_import_skin,
	const bool is_import_materials,
	const bool is_import_cameras,
	const bool is_import_animations);

std::vector<float> get_float_buffer(const tinygltf::Model& model, const tinygltf::Accessor& accessor, const bool decode_normalized = false);