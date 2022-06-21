#include <set>

#include <xsi_application.h>
#include <xsi_x3dobject.h>
#include <xsi_camera.h>

#include "../gltf_io.h"
#include "../../utilities/utilities.h"

tinygltf::Node export_node(XSI::X3DObject &xsi_object, bool &is_correct)
{
	log_message("export " + xsi_object.GetName());
	tinygltf::Node new_node;
	new_node.name = xsi_object.GetName().GetAsciiString();

	is_correct = true;
	return new_node;
}

tinygltf::Node export_camera(XSI::Camera &camera, bool& is_correct)
{
	log_message("export camera " + camera.GetName());
	tinygltf::Node new_node;

	new_node.name = camera.GetName().GetAsciiString();

	is_correct = true;
	return new_node;
}

int export_iterate(XSI::CRef &obj, std::set<ULONG> &exported_objects, tinygltf::Model &model)
{
	XSI::CString obj_class = obj.GetClassIDName();
	int node_index = -1;
	if (obj_class == "Null" || obj_class == "X3DObject" || obj_class == "Model" || obj_class == "CameraRig")
	{
		XSI::X3DObject xsi_obj(obj);
		ULONG xsi_id = xsi_obj.GetObjectID();
		if (exported_objects.find(xsi_id) == exported_objects.end())
		{
			bool is_correct = false;
			tinygltf::Node new_node = export_node(xsi_obj, is_correct);
			exported_objects.insert(xsi_id);
			if (is_correct)
			{
				node_index = model.nodes.size();
				model.nodes.push_back(new_node);
			}

			XSI::CRefArray xsi_children = xsi_obj.GetChildren();
			for (ULONG i = 0; i < xsi_children.GetCount(); i++)
			{
				XSI::CRef child = xsi_children[i];
				int child_index = export_iterate(child, exported_objects, model);
				if (is_correct && child_index >= 0)
				{
					model.nodes[node_index].children.push_back(child_index);
				}
			}
		}
	}
	else if (obj_class == "Camera")
	{
		XSI::Camera xsi_camera(obj);
		ULONG xsi_id = xsi_camera.GetObjectID();
		if (exported_objects.find(xsi_id) == exported_objects.end())
		{
			bool is_correct = false;
			tinygltf::Node new_node = export_camera(xsi_camera, is_correct);
			exported_objects.insert(xsi_id);
			if (is_correct)
			{
				node_index = model.nodes.size();
				model.nodes.push_back(new_node);
			}
			
			XSI::CRefArray xsi_children = xsi_camera.GetChildren();
			for (ULONG i = 0; i < xsi_children.GetCount(); i++)
			{
				XSI::CRef child = xsi_children[i];
				int child_index = export_iterate(child, exported_objects, model);
				if (is_correct && child_index >= 0)
				{
					model.nodes[node_index].children.push_back(child_index);
				}
			}
		}
	}
	else
	{
		log_message("unknown object class " + obj_class);
	}
	//all other object are unsopported, skip it

	return node_index;
}

bool export_gltf(const XSI::CString &file_path, const XSI::CRefArray &objects)
{
	std::set<ULONG> exported_objects;  // store here id-s of exported objects
	tinygltf::Model model;
	tinygltf::Scene scene;

	for (ULONG i = 0; i < objects.GetCount(); i++)
	{
		XSI::CRef obj = objects[i];
		int scene_node_index = export_iterate(obj, exported_objects, model);
		if (scene_node_index >= 0)
		{
			scene.nodes.push_back(scene_node_index);
		}
	}

	scene.name = file_name_from_path(file_path).GetAsciiString();
	model.scenes.push_back(scene);
	model.defaultScene = 0;

	tinygltf::Asset asset;
	asset.version = "2.0";
	asset.generator = "Softimage GLTF (tinygltf)";

	exported_objects.clear();

	//write output file
	tinygltf::TinyGLTF gltf;
	std::string path_str = file_path.GetAsciiString();
	std::string ext = get_file_extension(path_str);
	gltf.WriteGltfSceneToFile(&model, path_str,
		true, // embedImages
		true, // embedBuffers
		true, // pretty print
		ext == "glb"); // write binary

	return true;
}