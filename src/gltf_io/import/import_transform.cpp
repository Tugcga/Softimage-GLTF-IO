#include <xsi_quaternion.h>
#include <xsi_transformation.h>

#include <vector>

#include "../../tiny_gltf/tiny_gltf.h"
#include "../../utilities/utilities.h"

XSI::MATH::CTransformation import_transform(const tinygltf::Node& node)
{
	XSI::MATH::CTransformation node_tfm;
	std::vector<double> node_matrix = node.matrix;
	if (node_matrix.size() > 0)
	{
		//set from matrix
		XSI::MATH::CMatrix4 tfm_matrix;
		tfm_matrix.Set(
			node_matrix[0], node_matrix[1], node_matrix[2], node_matrix[3],
			node_matrix[4], node_matrix[5], node_matrix[6], node_matrix[7], 
			node_matrix[8], node_matrix[9], node_matrix[10], node_matrix[11], 
			node_matrix[12], node_matrix[13], node_matrix[14], node_matrix[15]);
		node_tfm.SetMatrix4(tfm_matrix);
	}
	else
	{
		std::vector<double> node_translation = node.translation;
		if (node_translation.size() == 3)
		{
			node_tfm.SetTranslationFromValues(node_translation[0], node_translation[1], node_translation[2]);
		}
		std::vector<double> node_rotation = node.rotation;
		if (node_rotation.size() == 4)
		{
			XSI::MATH::CQuaternion rotation;
			rotation.Set(node_rotation[3], node_rotation[0], node_rotation[1], node_rotation[2]);
			node_tfm.SetRotationFromQuaternion(rotation);
		}
		std::vector<double> node_scale = node.scale;
		if (node_scale.size() == 3)
		{
			node_tfm.SetScalingFromValues(node_scale[0], node_scale[1], node_scale[2]);
		}
	}
	return node_tfm;
}