#pragma once
#include <xsi_application.h>
#include <xsi_vector3f.h>

#include "../tiny_gltf/tiny_gltf.h"

const double EPSILON = 0.000001;
bool is_arrays_coincide(const std::vector<float>& array_a, const std::vector<float>& array_b);

struct ExportOptions
{
	bool embed_images;
	bool embed_buffers;
	XSI::CString output_path;
};

struct Vertex
{
	XSI::MATH::CVector3 position;
	XSI::MATH::CVector3f normal;
	std::vector<float> uvs;  // store all uvs here, so, the length of the array is x2
	std::vector<float> colors;  // this array size is x4
	std::vector<unsigned int> joints;  // store here numeric indices of deformers in the whole mesh list
	std::vector<float> weights;  // the length of these two arrays should be the same (at the export we will split it to 4-tuples), write here only non-zero values
	std::vector<float> shapes;// store here all vertex shift in shapes, this array size is x3

	bool is_coincide(const Vertex& other)
	{
		return abs(normal.GetX() - other.normal.GetX()) < EPSILON &&
			   abs(normal.GetY() - other.normal.GetY()) < EPSILON &&
			   abs(normal.GetZ() - other.normal.GetZ()) < EPSILON &&
			   is_arrays_coincide(uvs, other.uvs) &&
			   is_arrays_coincide(colors, other.colors);
	}

	XSI::CString to_string()
	{
		XSI::CString to_return = "(";
		to_return += XSI::CString(position.GetX()) + ", " + XSI::CString(position.GetY()) + ", " + XSI::CString(position.GetZ());
		to_return += "|";
		to_return += XSI::CString(normal.GetX()) + ", " + XSI::CString(normal.GetY()) + ", " + XSI::CString(normal.GetZ());

		for (ULONG i = 0; i < uvs.size() / 2; i++)
		{
			to_return += "|" + XSI::CString(uvs[2 * i]) + ", " + XSI::CString(uvs[2 * i + 1]);
		}

		for (ULONG i = 0; i < colors.size() / 4; i++)
		{
			to_return += "|" + XSI::CString(colors[4 * i]) + ", " + XSI::CString(colors[4 * i + 1]) + ", " + XSI::CString(colors[4 * i + 2]) + ", " + XSI::CString(colors[4 * i + 3]);
		}

		to_return += ")";

		return to_return;
	}
};

bool export_gltf(const XSI::CString &file_path, const XSI::CRefArray &objects);

int add_data_to_buffer(tinygltf::Model& model, std::vector<unsigned char>& byte_vector, ULONG data_count, const bool is_indices, const bool ignore_target, const int component_type, const int data_type, std::vector<double> min_value, std::vector<double> max_value);
int add_triangle_indices_to_buffer(tinygltf::Model& model, const std::vector<unsigned int>& data, const int component_type, const int data_type);
int add_float_to_buffer(tinygltf::Model& model, const std::vector<float>& data, const bool ignore_target, const int component_type, const int data_type);