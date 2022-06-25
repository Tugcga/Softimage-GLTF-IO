#include <vector>

#include <xsi_application.h>
#include <xsi_x3dobject.h>
#include <xsi_kinematics.h>
#include <xsi_kinematicstate.h>
#include <xsi_statickinematicstate.h>

#include "../../tiny_gltf/tiny_gltf.h"

//find local transform of the object, global_tfm is global tfm of this object
//if parent of the given object cantains valid static kine state, then use it instead of ordinary global tfm
XSI::MATH::CTransformation from_global_to_local(XSI::X3DObject& xsi_object, XSI::MATH::CTransformation& global_tfm)
{
	XSI::X3DObject parent = xsi_object.GetParent();
	XSI::StaticKinematicState parent_kine = parent.GetStaticKinematicState();
	XSI::MATH::CTransformation parent_global_tfm;
	if (parent_kine.IsValid())
	{
		parent_global_tfm = parent_kine.GetTransform();
	}
	else
	{
		parent_global_tfm = parent.GetKinematics().GetGlobal().GetTransform();
	}
	XSI::MATH::CMatrix4 parent_m = parent_global_tfm.GetMatrix4();
	parent_m.InvertInPlace();

	XSI::MATH::CMatrix4 object_m = global_tfm.GetMatrix4();
	XSI::MATH::CMatrix4 result_m;
	result_m.Mul(object_m, parent_m);

	XSI::MATH::CTransformation to_return;
	to_return.SetMatrix4(result_m);
	return to_return;
}

int add_data_to_buffer(tinygltf::Model& model, 
	std::vector<unsigned char>& byte_vector, 
	ULONG data_count, 
	const bool is_indices, 
	const bool ignore_target, 
	const int component_type, 
	const int data_type, 
	std::vector<double> min_value, 
	std::vector<double> max_value)
{
	std::vector<unsigned char>& buffer_data = model.buffers[0].data;
	//create new buffer view
	tinygltf::BufferView view;
	view.buffer = 0;  // point to the unique global data buffer
	view.byteOffset = buffer_data.size();
	view.byteLength = byte_vector.size();
	if (!ignore_target)
	{
		view.target = is_indices ? TINYGLTF_TARGET_ELEMENT_ARRAY_BUFFER : TINYGLTF_TARGET_ARRAY_BUFFER;
	}
	//increase byte_vector if it size is not divide to 4
	size_t rem = byte_vector.size() % 4;
	byte_vector.resize(byte_vector.size() + rem, 0);

	//append to data in the buffer
	buffer_data.reserve(buffer_data.size() + std::distance(byte_vector.begin(), byte_vector.end()));
	buffer_data.insert(buffer_data.end(), byte_vector.begin(), byte_vector.end());

	//create accessor
	tinygltf::Accessor accessor;
	accessor.bufferView = model.bufferViews.size();
	accessor.byteOffset = 0;
	accessor.componentType = component_type;
	accessor.count = data_count;
	accessor.type = data_type;

	std::copy(min_value.begin(), min_value.end(), std::back_inserter(accessor.minValues));
	std::copy(max_value.begin(), max_value.end(), std::back_inserter(accessor.maxValues));

	//add buffer view to the model
	model.bufferViews.push_back(view);
	model.accessors.push_back(accessor);

	return model.accessors.size() - 1;
}

//return index of the accessor
int add_triangle_indices_to_buffer(tinygltf::Model& model, const std::vector<unsigned int>& data, const int component_type, const int data_type)
{
	const unsigned char* bytes = reinterpret_cast<const unsigned char*>(&data[0]);
	std::vector<unsigned char> byte_vector(bytes, bytes + sizeof(unsigned int) * data.size());

	unsigned int min = ULONG_MAX;
	unsigned int max = 0;
	for (ULONG i = 0; i < data.size(); i++)
	{
		unsigned int v = data[i];
		if (v < min) { min = v; }
		if (v > max) { max = v; }
	}

	std::vector<double> min_value{ (double)min };
	std::vector<double> max_value{ (double)max };

	return add_data_to_buffer(model, byte_vector, data.size(), true, false, component_type, data_type, min_value, max_value);
}

int add_float_to_buffer(tinygltf::Model& model, const std::vector<float>& data, const bool ignore_target, const int component_type, const int data_type)
{
	ULONG items_count = data.size() / tinygltf::GetNumComponentsInType(data_type);
	const unsigned char* bytes = reinterpret_cast<const unsigned char*>(&data[0]);
	std::vector<unsigned char> byte_vector(bytes, bytes + sizeof(float) * data.size());

	std::vector<double> min_value(0);
	std::vector<double> max_value(0);

	return add_data_to_buffer(model, byte_vector, items_count, false, ignore_target, component_type, data_type, min_value, max_value);
}