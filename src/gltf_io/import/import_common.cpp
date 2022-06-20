#include <vector>

#include "../../tiny_gltf/tiny_gltf.h"

std::vector<float> get_float_buffer(const tinygltf::Model& model, const tinygltf::Accessor& accessor, const bool decode_normalized)
{
	int32_t components = tinygltf::GetNumComponentsInType(accessor.type);
	int component_type = accessor.componentType;

	std::vector<float> to_return(components * accessor.count);

	const tinygltf::BufferView& buffer_view = model.bufferViews[accessor.bufferView];
	const tinygltf::Buffer& buffer = model.buffers[buffer_view.buffer];

	if (component_type == TINYGLTF_COMPONENT_TYPE_FLOAT)
	{
		const float* data = reinterpret_cast<const float*>(&buffer.data[buffer_view.byteOffset + accessor.byteOffset]);
		for (size_t i = 0; i < accessor.count; ++i)
		{
			for (int32_t c = 0; c < components; c++)
			{
				to_return[components * i + c] = data[i * components + c];
			}
		}
	}
	else if (component_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
	{
		const unsigned char* data = reinterpret_cast<const unsigned char*>(&buffer.data[buffer_view.byteOffset + accessor.byteOffset]);
		for (size_t i = 0; i < accessor.count; ++i)
		{
			for (int32_t c = 0; c < components; c++)
			{
				to_return[components * i + c] = decode_normalized ? float(data[i * components + c]) / 255.0f : data[i * components + c];
			}
		}
	}
	else if (component_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
	{
		const unsigned short* data = reinterpret_cast<const unsigned short*>(&buffer.data[buffer_view.byteOffset + accessor.byteOffset]);
		for (size_t i = 0; i < accessor.count; ++i)
		{
			for (int32_t c = 0; c < components; c++)
			{
				to_return[components * i + c] = decode_normalized ? float(data[i * components + c]) / 65535.0f : data[i * components + c];
			}
		}
	}
	else if (component_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
	{
		const unsigned int* data = reinterpret_cast<const unsigned int*>(&buffer.data[buffer_view.byteOffset + accessor.byteOffset]);
		for (size_t i = 0; i < accessor.count; ++i)
		{
			for (int32_t c = 0; c < components; c++)
			{
				to_return[components * i + c] = data[i * components + c];
			}
		}
	}
	else if (component_type == TINYGLTF_COMPONENT_TYPE_BYTE)
	{
		const char* data = reinterpret_cast<const char*>(&buffer.data[buffer_view.byteOffset + accessor.byteOffset]);
		for (size_t i = 0; i < accessor.count; ++i)
		{
			for (int32_t c = 0; c < components; c++)
			{
				to_return[components * i + c] = decode_normalized ? float(data[i * components + c]) / 127.0f : data[i * components + c];
			}
		}
	}
	else if (component_type == TINYGLTF_COMPONENT_TYPE_SHORT)
	{
		const short* data = reinterpret_cast<const short*>(&buffer.data[buffer_view.byteOffset + accessor.byteOffset]);
		for (size_t i = 0; i < accessor.count; ++i)
		{
			for (int32_t c = 0; c < components; c++)
			{
				to_return[components * i + c] = decode_normalized ? float(data[i * components + c]) / 32767.0f : data[i * components + c];
			}
		}
	}
	else if (component_type == TINYGLTF_COMPONENT_TYPE_INT)
	{
		const int* data = reinterpret_cast<const int*>(&buffer.data[buffer_view.byteOffset + accessor.byteOffset]);
		for (size_t i = 0; i < accessor.count; ++i)
		{
			for (int32_t c = 0; c < components; c++)
			{
				to_return[components * i + c] = data[i * components + c];
			}
		}
	}
	else if (component_type == TINYGLTF_COMPONENT_TYPE_DOUBLE)
	{
		const double* data = reinterpret_cast<const double*>(&buffer.data[buffer_view.byteOffset + accessor.byteOffset]);
		for (size_t i = 0; i < accessor.count; ++i)
		{
			for (int32_t c = 0; c < components; c++)
			{
				to_return[components * i + c] = data[i * components + c];
			}
		}
	}
	else
	{
		std::vector<float> empty(0);
		return empty;
	}

	return to_return;
}