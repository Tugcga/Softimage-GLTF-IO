#include <set>

#include <xsi_application.h>
#include <xsi_x3dobject.h>
#include <xsi_camera.h>

#include "../gltf_io.h"
#include "../export.h"
#include "../../utilities/utilities.h"

int export_iterate(XSI::CRef &obj, const ExportOptions &options, std::set<ULONG> &exported_objects, std::unordered_map<ULONG, ULONG> &materials_map, std::unordered_map<ULONG, ULONG> &textures_map, tinygltf::Model &model)
{
	XSI::CString obj_class = obj.GetClassIDName();
	int node_index = -1;
	ULONG xsi_id;
	XSI::CRefArray xsi_children(0);
	bool is_correct = false;
	if (obj_class == "Null" || obj_class == "X3DObject" || obj_class == "Model" || obj_class == "CameraRig")
	{
		XSI::X3DObject xsi_obj(obj);
		xsi_id = xsi_obj.GetObjectID();
		if (exported_objects.find(xsi_id) == exported_objects.end())
		{
			tinygltf::Node new_node = export_node(xsi_obj, is_correct, options, materials_map, textures_map, model);
			exported_objects.insert(xsi_id);
			if (is_correct)
			{
				node_index = model.nodes.size();
				model.nodes.push_back(new_node);
			}

			xsi_children = xsi_obj.GetChildren();
		}
	}
	else if (obj_class == "Camera")
	{
		XSI::Camera xsi_camera(obj);
		xsi_id = xsi_camera.GetObjectID();
		if (exported_objects.find(xsi_id) == exported_objects.end())
		{
			tinygltf::Node new_node = export_camera(xsi_camera, is_correct, model);
			exported_objects.insert(xsi_id);
			if (is_correct)
			{
				node_index = model.nodes.size();
				model.nodes.push_back(new_node);
			}
			xsi_children = xsi_camera.GetChildren();
			
		}
	}
	else
	{
		log_message("unknown object class " + obj_class);
	}
	//all other object are unsopported, skip it

	//next iterate throw children subobjects
	for (ULONG i = 0; i < xsi_children.GetCount(); i++)
	{
		XSI::CRef child = xsi_children[i];
		int child_index = export_iterate(child, options, exported_objects, materials_map, textures_map, model);
		if (is_correct && child_index >= 0)
		{
			model.nodes[node_index].children.push_back(child_index);
		}
	}

	return node_index;
}

bool export_gltf(const XSI::CString &file_path, const XSI::CRefArray &objects)
{
	XSI::CString output_path = file_path_to_folder(file_path);  // path without last //
	ExportOptions options
	{
		true, // embed_images
		false, // embed_buffers
		output_path, // output_path
	};

	std::set<ULONG> exported_objects;  // store here id-s of exported objects
	tinygltf::Model model;
	tinygltf::Scene scene;

	std::unordered_map<ULONG, ULONG> materials_map;  // key - material object id, value - index in the gltf materials list
	std::unordered_map<ULONG, ULONG> textures_map; // key - image clip id, value - index of the texture in the gltf textures list
	//we always use default samplers

	for (ULONG i = 0; i < objects.GetCount(); i++)
	{
		XSI::CRef obj = objects[i];
		int scene_node_index = export_iterate(obj, options, exported_objects, materials_map, textures_map, model);
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
		options.embed_images, // embedImages
		options.embed_buffers, // embedBuffers
		true, // pretty print
		ext == "glb"); // write binary
		
	return true;
}