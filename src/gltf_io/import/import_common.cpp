#include <vector>

#include <xsi_application.h>

#include "../../tiny_gltf/tiny_gltf.h"

#include "../../utilities/utilities.h"

std::vector<float> read_float_buffer_view(const tinygltf::Model& model, const tinygltf::BufferView& buffer_view, int component_type, size_t byte_offset, int32_t components, int count, bool decode_normalized)
{
	const tinygltf::Buffer& buffer = model.buffers[buffer_view.buffer];
	const uint32_t component_size = tinygltf::GetComponentSizeInBytes(component_type);  // the size of one component in bytes
	const size_t byte_stride = buffer_view.byteStride == 0 ? components * component_size : buffer_view.byteStride;
	const size_t index_stride = byte_stride / component_size;

	std::vector<float> to_return(components * count);
	if (component_type == TINYGLTF_COMPONENT_TYPE_FLOAT)
	{
		const float* data = reinterpret_cast<const float*>(&buffer.data[buffer_view.byteOffset + byte_offset]);
		for (size_t i = 0; i < count; ++i)
		{
			for (int32_t c = 0; c < components; c++)
			{
				to_return[components * i + c] = data[i * index_stride + c];
			}
		}
	}
	else if (component_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
	{
		const unsigned char* data = reinterpret_cast<const unsigned char*>(&buffer.data[buffer_view.byteOffset + byte_offset]);
		for (size_t i = 0; i < count; ++i)
		{
			for (int32_t c = 0; c < components; c++)
			{
				to_return[components * i + c] = decode_normalized ? float(data[i * index_stride + c]) / 255.0f : data[i * index_stride + c];
			}
		}
	}
	else if (component_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
	{
		const unsigned short* data = reinterpret_cast<const unsigned short*>(&buffer.data[buffer_view.byteOffset + byte_offset]);
		for (size_t i = 0; i < count; ++i)
		{
			for (int32_t c = 0; c < components; c++)
			{
				to_return[components * i + c] = decode_normalized ? float(data[i * index_stride + c]) / 65535.0f : data[i * index_stride + c];
			}
		}
	}
	else if (component_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
	{
		const unsigned int* data = reinterpret_cast<const unsigned int*>(&buffer.data[buffer_view.byteOffset + byte_offset]);
		for (size_t i = 0; i < count; ++i)
		{
			for (int32_t c = 0; c < components; c++)
			{
				to_return[components * i + c] = data[i * index_stride + c];
			}
		}
	}
	else if (component_type == TINYGLTF_COMPONENT_TYPE_BYTE)
	{
		const char* data = reinterpret_cast<const char*>(&buffer.data[buffer_view.byteOffset + byte_offset]);
		for (size_t i = 0; i < count; ++i)
		{
			for (int32_t c = 0; c < components; c++)
			{
				to_return[components * i + c] = decode_normalized ? float(data[i * index_stride + c]) / 127.0f : data[i * index_stride + c];
			}
		}
	}
	else if (component_type == TINYGLTF_COMPONENT_TYPE_SHORT)
	{
		const short* data = reinterpret_cast<const short*>(&buffer.data[buffer_view.byteOffset + byte_offset]);
		for (size_t i = 0; i < count; ++i)
		{
			for (int32_t c = 0; c < components; c++)
			{
				to_return[components * i + c] = decode_normalized ? float(data[i * index_stride + c]) / 32767.0f : data[i * index_stride + c];
			}
		}
	}
	else if (component_type == TINYGLTF_COMPONENT_TYPE_INT)
	{
		const int* data = reinterpret_cast<const int*>(&buffer.data[buffer_view.byteOffset + byte_offset]);
		for (size_t i = 0; i < count; ++i)
		{
			for (int32_t c = 0; c < components; c++)
			{
				to_return[components * i + c] = data[i * index_stride + c];
			}
		}
	}
	else if (component_type == TINYGLTF_COMPONENT_TYPE_DOUBLE)
	{
		const double* data = reinterpret_cast<const double*>(&buffer.data[buffer_view.byteOffset + byte_offset]);
		for (size_t i = 0; i < count; ++i)
		{
			for (int32_t c = 0; c < components; c++)
			{
				to_return[components * i + c] = data[i * index_stride + c];
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

std::vector<float> get_float_buffer(const tinygltf::Model& model, const tinygltf::Accessor& accessor, const bool decode_normalized)
{
	int32_t components = tinygltf::GetNumComponentsInType(accessor.type);
	int component_type = accessor.componentType;

	const tinygltf::BufferView& buffer_view = model.bufferViews[accessor.bufferView];

	return read_float_buffer_view(model, buffer_view, component_type, accessor.byteOffset, components, accessor.count, decode_normalized);
}

std::vector<ULONG> read_integer_buffer_view(const tinygltf::Model& model, const tinygltf::BufferView& buffer_view, int component_type, size_t byte_offset, int32_t components, int count)
{
	const tinygltf::Buffer& buffer = model.buffers[buffer_view.buffer];
	const uint32_t component_size = tinygltf::GetComponentSizeInBytes(component_type);
	const size_t byte_stride = buffer_view.byteStride == 0 ? components * component_size : buffer_view.byteStride;
	const size_t index_stride = byte_stride / component_size;
	std::vector<ULONG> to_return(components * count);

	if (component_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
	{
		const unsigned char* data = reinterpret_cast<const unsigned char*>(&buffer.data[buffer_view.byteOffset + byte_offset]);
		for (size_t i = 0; i < count; ++i)
		{
			for (int32_t c = 0; c < components; c++)
			{
				to_return[components * i + c] = data[i * index_stride + c];
			}
		}
	}
	else if (component_type == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
	{
		const unsigned short* data = reinterpret_cast<const unsigned short*>(&buffer.data[buffer_view.byteOffset + byte_offset]);
		for (size_t i = 0; i < count; ++i)
		{
			for (int32_t c = 0; c < components; c++)
			{
				to_return[components * i + c] = data[i * index_stride + c];
			}
		}
	}
	else
	{
		std::vector<ULONG> empty(0);
		return empty;
	}

	return to_return;
}

std::vector<ULONG> get_integer_buffer(const tinygltf::Model& model, const tinygltf::Accessor& accessor)
{
	int32_t components = tinygltf::GetNumComponentsInType(accessor.type);
	int component_type = accessor.componentType;
	
	const tinygltf::BufferView& buffer_view = model.bufferViews[accessor.bufferView];
	return read_integer_buffer_view(model, buffer_view, component_type, accessor.byteOffset, components, accessor.count);
}