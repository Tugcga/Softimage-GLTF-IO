#include <set>
#include <ctime>

#include <xsi_application.h>
#include <xsi_x3dobject.h>
#include <xsi_camera.h>
#include <xsi_project.h>
#include <xsi_scene.h>
#include <xsi_model.h>
#include <xsi_progressbar.h>
#include <xsi_uitoolkit.h>

#include "../gltf_io.h"
#include "../export.h"
#include "../../utilities/utilities.h"

bool is_object_visible(XSI::X3DObject& xsi_object)
{
	XSI::Property visibility_prop;
	xsi_object.GetPropertyFromName("visibility", visibility_prop);
	if (visibility_prop.IsValid())
	{
		return visibility_prop.GetParameterValue("viewvis");
	}
	return false;
}

int export_iterate(XSI::ProgressBar &bar,
	XSI::CRef &obj,
	const ExportOptions &options, 
	std::set<ULONG> &exported_objects, 
	std::unordered_map<ULONG, ULONG> &materials_map, 
	std::unordered_map<ULONG, ULONG> &textures_map, 
	std::vector<XSI::X3DObject> &envelope_meshes,
	std::unordered_map<ULONG, ULONG> &object_to_node,
	tinygltf::Model &model)
{
	XSI::CString obj_class = obj.GetClassIDName();
	int node_index = -1;
	ULONG xsi_id;
	XSI::CRefArray xsi_children(0);
	bool is_correct = false;
	XSI::X3DObject xsi_obj(obj);

	bar.PutStatusText((obj_class == "Camera" ? "Camera: " : "Object: ") + xsi_obj.GetFullName());
	
	if (xsi_obj.IsValid())
	{
		//check object visibility
		bool is_visible = is_object_visible(xsi_obj);
		if ((!is_visible && options.export_hide) || is_visible || xsi_obj.GetObjectID() == options.scene_root_id)
		{
			if (obj_class == "Camera" && options.is_export_cameras)
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
				xsi_id = xsi_obj.GetObjectID();
				if (exported_objects.find(xsi_id) == exported_objects.end())
				{
					tinygltf::Node new_node = export_node(bar, xsi_obj, is_correct, options, materials_map, textures_map, envelope_meshes, object_to_node, model);
					exported_objects.insert(xsi_id);
					if (is_correct)
					{
						node_index = model.nodes.size();
						model.nodes.push_back(new_node);
						object_to_node[xsi_id] = node_index;
					}

					xsi_children = xsi_obj.GetChildren();
				}
			}
		}
	}

	//next iterate throw children subobjects
	for (ULONG i = 0; i < xsi_children.GetCount(); i++)
	{
		XSI::CRef child = xsi_children[i];
		int child_index = export_iterate(bar, child, options, exported_objects, materials_map, textures_map, envelope_meshes, object_to_node, model);
		if (is_correct && child_index >= 0)
		{
			model.nodes[node_index].children.push_back(child_index);
		}
	}

	return node_index;
}

bool export_gltf(const XSI::CString &file_path, const XSI::CRefArray &objects,
	bool embed_images,
	bool embed_buffers,
	bool is_export_uvs,
	bool is_export_colors,
	bool is_export_shapes,
	bool is_export_skin,
	bool is_export_materials,
	bool is_export_cameras,
	bool is_export_animations,
	float animation_frames_per_second,
	int animation_start,
	int animation_end,
	bool export_hide)
{
	XSI::ProgressBar bar = XSI::Application().GetUIToolkit().GetProgressBar();
	bar.PutCancelEnabled(false);
	bar.PutValue(0);
	bar.PutVisible(true);
	double start_time = clock();

	XSI::Project project = XSI::Application().GetActiveProject();
	XSI::Scene scnene = project.GetActiveScene();
	XSI::Model root = scnene.GetRoot();

	XSI::CString output_path = file_path_to_folder(file_path);  // path without last //
	ExportOptions options
	{
		output_path,
		is_export_uvs,
		is_export_colors,
		is_export_shapes,
		is_export_skin,
		is_export_materials,
		is_export_cameras,
		is_export_animations,
		animation_frames_per_second,
		animation_start,
		animation_end,
		export_hide,
		root.GetObjectID(), //scene_root_id
	};

	std::set<ULONG> exported_objects;  // store here id-s of exported objects
	tinygltf::Model model;
	tinygltf::Scene scene;

	std::unordered_map<ULONG, ULONG> materials_map;  // key - material object id, value - index in the gltf materials list
	std::unordered_map<ULONG, ULONG> textures_map; // key - image clip id, value - index of the texture in the gltf textures list
	//we always use default samplers
	std::vector<XSI::X3DObject> envelope_meshes;
	std::unordered_map<ULONG, ULONG> object_to_node;  // map from Softimage object id to gltf node index

	tinygltf::Buffer data_buffer;  // we will store all binary data in this buffer
	model.buffers.push_back(data_buffer);

	for (ULONG i = 0; i < objects.GetCount(); i++)
	{
		XSI::CRef obj = objects[i];
		int scene_node_index = export_iterate(bar, obj, options, exported_objects, materials_map, textures_map, envelope_meshes, object_to_node, model);
		if (scene_node_index >= 0)
		{
			scene.nodes.push_back(scene_node_index);
		}
	}

	//export second part of the skin
	if (options.is_export_skin)
	{
		for (ULONG i = 0; i < envelope_meshes.size(); i++)
		{
			bar.PutStatusText("Skin: " + envelope_meshes[i].GetFullName());
			export_skin(model, i, envelope_meshes[i], object_to_node);
		}
	}

	//export animations
	if (options.is_export_animations && options.animation_end - options.animation_start >= 0)
	{
		export_animation(model, bar, options, object_to_node);
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

	//does not need it
	tinygltf::WriteImageDataFunction WriteImageData = &tinygltf::WriteImageData;
	tinygltf::FsCallbacks fs = 
	{
		&tinygltf::FileExists, &tinygltf::ExpandFilePath,
		&tinygltf::ReadWholeFile, &tinygltf::WriteWholeFile,
		nullptr
	};
	gltf.SetImageWriter(WriteImageData, &fs);

	std::string path_str = file_path.GetAsciiString();
	std::string ext = get_file_extension(path_str);
	gltf.WriteGltfSceneToFile(&model, path_str,
		embed_images,
		embed_buffers,
		true, // pretty print
		ext == "glb"); // write binary

	//clear buffers
	exported_objects.clear();
	materials_map.clear();
	textures_map.clear();
	envelope_meshes.clear();
	envelope_meshes.shrink_to_fit();
	object_to_node.clear();

	bar.PutVisible(false);

	double finish_time = clock();
	double time = (finish_time - start_time) / CLOCKS_PER_SEC;
	log_message("Export GLTF/GLB time: " + XSI::CString(time) + " sec.");
	
	return true;
}