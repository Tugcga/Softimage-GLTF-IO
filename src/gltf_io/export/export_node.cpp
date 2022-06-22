#include <xsi_x3dobject.h>
#include <xsi_kinematics.h>
#include <xsi_kinematicstate.h>

#include "../gltf_io.h"
#include "../../utilities/utilities.h"

tinygltf::Node export_node(const XSI::X3DObject& xsi_object, bool& is_correct)
{
	tinygltf::Node new_node;

	//set the name
	new_node.name = xsi_object.GetName().GetAsciiString();

	//export object transform
	XSI::MATH::CTransformation xsi_tfm = xsi_object.GetKinematics().GetLocal().GetTransform();
	export_transform(xsi_tfm, new_node);

	is_correct = true;
	return new_node;
}