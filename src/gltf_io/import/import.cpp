#include <xsi_application.h>
#include <xsi_quaternion.h>
#include <xsi_transformation.h>
#include <xsi_model.h>
#include <xsi_null.h>
#include <xsi_kinematics.h>
#include <xsi_scene.h>
#include <xsi_project.h>
#include <xsi_comapihandler.h>
#include <xsi_progressbar.h>
#include <xsi_uitoolkit.h>

#include <ctime>

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

void process_node(XSI::ProgressBar &bar,
	const tinygltf::Model& model, 
	const tinygltf::Node& node, 
	const int node_index, 
	XSI::X3DObject &parent, 
	std::unordered_map<int, XSI::Material> &material_map, 
	const ImportOptions &options,
	std::unordered_map<ULONG, XSI::X3DObject> &nodes_map,
	std::vector<std::tuple<int, XSI::X3DObject, std::unordered_map<ULONG, std::vector<float>>>> &envelopes,
	const bool is_import_cameras,
	const bool is_import_lighs)
{
	//calculate node transform and recreate root transform for the child nodes
	XSI::MATH::CTransformation local_tfm = import_transform(node);
	XSI::CString node_name = node.name.c_str();
	if (node_name.Length() == 0)
	{
		node_name = "Node_" + XSI::CString(node_index);
	}

	XSI::CValue io_value;
	bar.PutCaption("Node " + node_name);

	XSI::X3DObject next_parent;
	if (node.mesh >= 0 && node.mesh < model.meshes.size())
	{
		//this node contains the mesh
		tinygltf::Mesh mesh = model.meshes[node.mesh];
		std::unordered_map<ULONG, std::vector<float>> envelop_map; // key - deformer index in the skin property, value - array of weights for all vertices in the mesh (so, for subobjects we should extend these arrays)
		next_parent = import_mesh(model, mesh, node_name, local_tfm, parent, material_map, envelop_map, options);  // return last primitivefrom the mesh

		if (node.skin >= 0 && envelop_map.size() > 0)
		{
			envelopes.push_back(std::make_tuple(node.skin, next_parent, envelop_map));
		}

	}
	else if (node.camera >= 0 && node.camera < model.cameras.size() && is_import_cameras)
	{
		tinygltf::Camera camera = model.cameras[node.camera];
		next_parent = import_camera(model, camera, node_name, local_tfm, parent);
	}
	else if (is_import_lighs && node.light >= 0 && node.light < model.lights.size())
	{
		tinygltf::Light light = model.lights[node.light];
		next_parent = import_light(model, light, node_name, local_tfm, parent);
	}

	if (!next_parent.IsValid())
	{
		XSI::Null node_null;
		parent.AddNull(node_name, node_null);
		node_null.GetKinematics().GetLocal().PutTransform(local_tfm);
		next_parent = node_null;
	}

	nodes_map[node_index] = next_parent;

	bar.Increment();

	for (size_t i = 0; i < node.children.size(); i++)
	{
		process_node(bar, model, model.nodes[node.children[i]], node.children[i], next_parent, material_map, options, nodes_map, envelopes, is_import_cameras, is_import_lighs);
	}
}

bool import_gltf(const XSI::CString file_path,
	const bool is_import_normals,
	const bool is_import_uvs,
	const bool is_import_colors,
	const bool is_import_shapes,
	const bool is_import_skin,
	const bool is_import_materials,
	const bool is_import_cameras,
	const bool is_import_lights,
	const bool is_import_animations,
	const float animation_frames_per_second)
{
	XSI::ProgressBar bar = XSI::Application().GetUIToolkit().GetProgressBar();
	bar.PutCancelEnabled(false);
	bar.PutVisible(true);
	bar.PutStatusText("Parsing file");

	tinygltf::Model model;
	bool is_load = load_model(model, file_path.GetAsciiString());
	if (is_load)
	{
		double start_time = clock();
		//the number of steps: nodes in the model + materials + animations + skins
		ULONG steps = model.nodes.size();
		if (is_import_materials)
		{
			steps += model.materials.size();
		}
		if (is_import_animations)
		{
			steps += model.animations.size();
		}
		if (is_import_skin)
		{
			steps += model.skins.size();
		}
		bar.PutMaximum(steps);
		bar.PutValue(0);

		const tinygltf::Scene& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];

		ImportOptions options
		{
			is_import_normals,
			is_import_uvs,
			is_import_colors,
			is_import_shapes,
			is_import_skin,
			animation_frames_per_second,
		};

		XSI::CString scene_name = scene.name.c_str();
		if (scene_name.Length() == 0)
		{
			scene_name = file_name_from_path(file_path);
		}

		std::unordered_map<int, XSI::Material> material_map;  // key - material index, value - xsi material
		ULONG materials_count = model.materials.size();
		if (materials_count > 0 && is_import_materials)
		{
			XSI::Scene scene = XSI::Application().GetActiveProject().GetActiveScene();
			XSI::CValueArray args(1);
			args[0] = scene_name;
			XSI::CValue create_out;
			XSI::Application().ExecuteCommand("CreateLibrary", args, create_out);
			XSI::CValueArray create_out_array(create_out);
			XSI::MaterialLibrary library = create_out_array[0];

			//at first we should import all images for textures
			std::unordered_map<int, XSI::CString> images_map;  // key - image index, value - full path to the image file
			std::unordered_map<int, XSI::ImageClip2> clips_map;
			import_images(model, scene, scene_name, file_path, images_map, clips_map);

			for (ULONG i = 0; i < model.materials.size(); i++)
			{
				tinygltf::Material material = model.materials[i];
				bar.PutCaption("Material " + XSI::CString(material.name.c_str()));
				bool is_create = import_material(model, material, i, library, images_map, clips_map, material_map);

				bar.Increment();
			}

			images_map.clear();
			clips_map.clear();
		}

		std::unordered_map<ULONG, XSI::X3DObject> nodes_map;  // key - node index, value - corresponding object in the Softimage
		std::vector<std::tuple<int, XSI::X3DObject, std::unordered_map<ULONG, std::vector<float>>>> envelopes(0);  // store here envelopes data for mesh object int he scene
		//each array element is a 3-tuple (model skin index (use it for actual deformers indices), Softimage polygonmesh obect, envelop data map (key - deformer indices, value - weights for all vertices int he mesh))

		XSI::Model xsi_root = XSI::Application().GetActiveSceneRoot();
		XSI::Null node_null;
		xsi_root.AddNull(scene_name, node_null);
		for (size_t i = 0; i < scene.nodes.size(); ++i)
		{
			process_node(bar, model, model.nodes[scene.nodes[i]], scene.nodes[i], node_null, material_map, options, nodes_map, envelopes, is_import_cameras, is_import_lights);
		}

		//after scene parsing we can setup the skin for each object
		if (is_import_skin)
		{
			bar.PutCaption("Skin");
			for (ULONG i = 0; i < envelopes.size(); i++)
			{
				auto t = envelopes[i];
				import_skin(model, std::get<1>(t), std::get<0>(t), std::get<2>(t), nodes_map);
				bar.Increment();
			}
		}

		if (is_import_animations)
		{
			import_animation(bar, model, nodes_map, options);
		}

		nodes_map.clear();
		envelopes.clear();

		double finish_time = clock();
		double time = (finish_time - start_time) / CLOCKS_PER_SEC;
		log_message("Import GLTF/GLB time: " + XSI::CString(time) + " sec.");
	}

	bar.PutVisible(false);

	return is_load;
}