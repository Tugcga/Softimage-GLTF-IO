#include <xsi_x3dobject.h>
#include <xsi_kinematics.h>
#include <xsi_kinematicstate.h>
#include <xsi_statickinematicstate.h>

#include "../gltf_io.h"
#include "../../utilities/utilities.h"

tinygltf::Node export_node(XSI::ProgressBar& bar, XSI::X3DObject& xsi_object,
	bool& is_correct,
	const ExportOptions& options,
	std::unordered_map<ULONG, ULONG> &materials_map, 
	std::unordered_map<ULONG, ULONG> &textures_map, 
	std::vector<XSI::X3DObject>& envelope_meshes,
	std::unordered_map<ULONG, ULONG>& object_to_node,
	tinygltf::Model& model)
{
	tinygltf::Node new_node;

	//set the name
	new_node.name = xsi_object.GetName().GetAsciiString();

	//if node is a skined mesh, then transform is not required
	bool is_export_transform = true;
	XSI::StaticKinematicState kine = xsi_object.GetStaticKinematicState();
	if (xsi_object.GetType() == "polymsh" && kine.IsValid())
	{
		is_export_transform = false;
	}

	if (is_export_transform)
	{
		//export object transform
		if (kine.IsValid())
		{
			//this state contains global transform, but we need local transform with respect to the parent
			export_transform(from_global_to_local(xsi_object, kine.GetTransform()), new_node);
		}
		else
		{
			export_transform(xsi_object.GetKinematics().GetLocal().GetTransform(), new_node);
		}
	}
	
	if (xsi_object.GetType() == "polymsh" && options.is_export_meshes)
	{
		export_mesh(new_node, model, bar, xsi_object, options, materials_map, textures_map, envelope_meshes);
	}

	is_correct = true;
	return new_node;
}