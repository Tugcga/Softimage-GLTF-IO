#include <xsi_application.h>
#include <xsi_quaternion.h>
#include <xsi_transformation.h>
#include <xsi_model.h>
#include <xsi_null.h>
#include <xsi_kinematics.h>

#include "../gltf_io.h"
#include "../../utilities/utilities.h"

bool load_model(tinygltf::Model& model, const char* filename)
{
	tinygltf::TinyGLTF loader;
	std::string err;
	std::string warn;

	FileType file_type = detect_file_type(filename);
	bool state = false;
	if (file_type == FileType::Ascii)
	{
		state = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
	}
	else if (file_type == FileType::Binary)
	{
		state = loader.LoadBinaryFromFile(&model, &err, &warn, filename);
	}

	if (!warn.empty())
	{
		log_message(XSI::CString("WARN: ") + XSI::CString(warn.c_str()), XSI::siWarningMsg);
	}

	if (!err.empty())
	{
		log_message(XSI::CString("ERR: ") + XSI::CString(err.c_str()), XSI::siErrorMsg);
	}

	if (!state)
	{
		log_message(XSI::CString("Failed to load glTF: ") + XSI::CString(filename), XSI::siWarningMsg);
	}

	return state;
}

void process_node(const tinygltf::Model& model, const tinygltf::Node& node, const int node_index, XSI::X3DObject &parent, const ImportMeshOptions &mesh_options)
{
	//calculate node transform and recreate root transform for the child nodes
	XSI::MATH::CTransformation local_tfm = import_transform(node);
	XSI::CString node_name = node.name.c_str();
	if (node_name.Length() == 0)
	{
		node_name = "Node_" + XSI::CString(node_index);
	}

	XSI::X3DObject next_parent;
	if ((node.mesh >= 0) && (node.mesh < model.meshes.size()))
	{
		//this node contains the mesh
		tinygltf::Mesh mesh = model.meshes[node.mesh];
		next_parent = import_mesh(model, mesh, node_name, local_tfm, parent, mesh_options);  // return last primitivefrom the mesh
	}

	if (!next_parent.IsValid())
	{
		XSI::Null node_null;
		parent.AddNull(node_name, node_null);
		node_null.GetKinematics().GetLocal().PutTransform(local_tfm);
		next_parent = node_null;
	}
	for (size_t i = 0; i < node.children.size(); i++)
	{
		process_node(model, model.nodes[node.children[i]], node.children[i], next_parent, mesh_options);
	}
}

bool import_gltf(const XSI::CString file_path)
{
	tinygltf::Model model;
	bool is_load = load_model(model, file_path.GetAsciiString());

	const tinygltf::Scene& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];
	XSI::Model xsi_root = XSI::Application().GetActiveSceneRoot();

	ImportMeshOptions mesh_options
	{
		false, // weld_vertices
	};

	for (size_t i = 0; i < scene.nodes.size(); ++i)
	{
		XSI::Null node_null;
		XSI::CString scene_name = scene.name.c_str();
		if (scene_name.Length() == 0)
		{
			scene_name = "Scene";
		}
		xsi_root.AddNull(scene_name, node_null);
		process_node(model, model.nodes[scene.nodes[i]], scene.nodes[i], node_null, mesh_options);
	}

	return is_load;
}