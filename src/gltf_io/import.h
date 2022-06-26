#pragma once
#include <xsi_application.h>

#include "../tiny_gltf/tiny_gltf.h"

struct ImportOptions
{
	bool is_import_normals;
	bool is_import_uvs;
	bool is_import_colors;
	bool is_import_shapes;
	bool is_import_skin;
	float animation_frames_per_second;
};

bool import_gltf(const XSI::CString file_path,
	const bool is_import_normals,
	const bool is_import_uvs,
	const bool is_import_colors,
	const bool is_import_shapes,
	const bool is_import_skin,
	const bool is_import_materials,
	const bool is_import_cameras,
	const bool is_import_animations,
	const float animation_frames_per_second);

std::vector<float> read_float_buffer_view(const tinygltf::Model& model, const tinygltf::BufferView& buffer_view, int component_type, size_t byte_offset, int32_t components, int count, bool decode_normalized);
std::vector<float> get_float_buffer(const tinygltf::Model& model, const tinygltf::Accessor& accessor, const bool decode_normalized = false);
std::vector<ULONG> read_integer_buffer_view(const tinygltf::Model& model, const tinygltf::BufferView& buffer_view, int component_type, size_t byte_offset, int32_t components, int count);
std::vector<ULONG> get_integer_buffer(const tinygltf::Model& model, const tinygltf::Accessor& accessor);