#include <xsi_camera.h>
#include <xsi_kinematics.h>
#include <xsi_kinematicstate.h>

#include "../gltf_io.h"
#include "../../utilities/utilities.h"

tinygltf::Node export_camera(const XSI::Camera& camera, bool& is_correct)
{
	tinygltf::Node new_node;

	//set the name
	new_node.name = camera.GetName().GetAsciiString();

	//export transform
	XSI::MATH::CTransformation xsi_tfm = camera.GetKinematics().GetLocal().GetTransform();
	export_transform(xsi_tfm, new_node);

	is_correct = true;
	return new_node;
}