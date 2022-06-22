#include <xsi_x3dobject.h>
#include <xsi_kinematics.h>
#include <xsi_kinematicstate.h>

#include "../gltf_io.h"
#include "../../utilities/utilities.h"

tinygltf::Node export_node(XSI::X3DObject& xsi_object, bool& is_correct, const ExportOptions& options, std::unordered_map<ULONG, ULONG> &materials_map, std::unordered_map<ULONG, ULONG> &textures_map, tinygltf::Model& model)
{
	tinygltf::Node new_node;

	//set the name
	new_node.name = xsi_object.GetName().GetAsciiString();

	//export object transform
	XSI::MATH::CTransformation xsi_tfm = xsi_object.GetKinematics().GetLocal().GetTransform();
	export_transform(xsi_tfm, new_node);

	if (xsi_object.GetType() == "polymsh")
	{
		export_mesh(new_node, model, xsi_object, options, materials_map, textures_map);
	}

	is_correct = true;
	return new_node;
}