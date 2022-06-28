#include <vector>
#include <unordered_map>

#include <xsi_application.h>
#include <xsi_model.h>
#include <xsi_x3dobject.h>
#include <xsi_meshbuilder.h>
#include <xsi_kinematics.h>
#include <xsi_kinematicstate.h>
#include <xsi_primitive.h>
#include <xsi_polygonmesh.h>
#include <xsi_geometryaccessor.h>
#include <xsi_clusterpropertybuilder.h>
#include <xsi_material.h>
#include <xsi_envelopeweight.h>

#include "../../tiny_gltf/tiny_gltf.h"

#include "../../utilities/utilities.h"
#include "../import.h"

std::vector<LONG> get_polygon_inidices(const tinygltf::Model& model, const tinygltf::Primitive& primitive, const ULONG first_index)
{
	if (primitive.indices >= 0)
	{
		const tinygltf::Accessor& polygon_accessor = model.accessors[primitive.indices];
		int component_type = polygon_accessor.componentType;
		std::vector<LONG> to_return(polygon_accessor.count);

		const tinygltf::BufferView& buffer_view = model.bufferViews[polygon_accessor.bufferView];
		const tinygltf::Buffer& buffer = model.buffers[buffer_view.buffer];

		if (component_type == 5123)
		{
			const unsigned short* polygons = reinterpret_cast<const unsigned short*>(&buffer.data[buffer_view.byteOffset + polygon_accessor.byteOffset]);
			for (size_t i = 0; i < polygon_accessor.count; ++i)
			{
				to_return[i] = polygons[i] + first_index;
			}
		}
		else if (component_type == 5120)
		{
			const signed char* polygons = reinterpret_cast<const signed char*>(&buffer.data[buffer_view.byteOffset + polygon_accessor.byteOffset]);
			for (size_t i = 0; i < polygon_accessor.count; ++i)
			{
				to_return[i] = polygons[i] + first_index;
			}
		}
		else if (component_type == 5121)
		{
			const unsigned char* polygons = reinterpret_cast<const unsigned char*>(&buffer.data[buffer_view.byteOffset + polygon_accessor.byteOffset]);
			for (size_t i = 0; i < polygon_accessor.count; ++i)
			{
				to_return[i] = polygons[i] + first_index;
			}
		}
		else if (component_type == 5122)
		{
			const signed short* polygons = reinterpret_cast<const signed short*>(&buffer.data[buffer_view.byteOffset + polygon_accessor.byteOffset]);
			for (size_t i = 0; i < polygon_accessor.count; ++i)
			{
				to_return[i] = polygons[i] + first_index;
			}
		}
		else if (component_type == 5125)
		{
			const unsigned int* polygons = reinterpret_cast<const unsigned int*>(&buffer.data[buffer_view.byteOffset + polygon_accessor.byteOffset]);
			for (size_t i = 0; i < polygon_accessor.count; ++i)
			{
				to_return[i] = polygons[i] + first_index;
			}
		}
		else
		{
			std::vector<LONG> empty(0);
			return empty;
		}

		return to_return;
	}
	else
	{
		std::vector<LONG> empty(0);
		return empty;
	}

}

std::vector<double> get_double_buffer(const tinygltf::Model& model, const tinygltf::Accessor& accessor)
{
	int32_t components = tinygltf::GetNumComponentsInType(accessor.type);

	int component_type = accessor.componentType;

	if (component_type != 5126)
	{
		// this buffer should contains float values, but it actual contains something different
		// return empty array
		std::vector<double> empty(0);
		return empty;
	}

	std::vector<double> to_return(components * accessor.count);

	const tinygltf::BufferView& buffer_view = model.bufferViews[accessor.bufferView];
	const tinygltf::Buffer& buffer = model.buffers[buffer_view.buffer];
	const float* positions = reinterpret_cast<const float*>(&buffer.data[buffer_view.byteOffset + accessor.byteOffset]);
	for (size_t i = 0; i < accessor.count; ++i)
	{
		for (int32_t c = 0; c < components; c++)
		{
			to_return[components * i + c] = positions[i * components + c];
		}
	}

	return to_return;
}

int get_closest_index(const std::vector<double> &positions, const double x, const double y, const double z)
{
	int count = positions.size() / 3;
	for (int i = 0; i < count; i++)
	{
		double px = positions[3 * i];
		double py = positions[3 * i + 1];
		double pz = positions[3 * i + 2];

		if ((px - x)*(px - x) + (py - y)*(py - y) + (pz - z)*(pz - z) < 0.00001)
		{
			return i;
		}
	}

	return -1;
}

LONG attribute_length(const std::string &name)
{
	if (name.compare("NORMAL") == 0 || name.find("TEXCOORD") == 0)
	{
		return 3;
	}
	else if (name.find("COLOR") == 0)
	{
		return 4;
	}
	else
	{
		return 0;
	}
}

void increase_envelop_weight_value(std::unordered_map<ULONG, std::vector<float>>& envelop_map,
	const ULONG joint, const float weight, const ULONG vertex_index,
	const ULONG start_vertex_index, const ULONG vertex_count)
{
	//try to find the key
	std::unordered_map<ULONG, std::vector<float>>::iterator joint_it = envelop_map.find(joint);
	if (joint_it == envelop_map.end())
	{//the joint is new for the map
		//create new array for this key
		std::vector<float> new_array(start_vertex_index, 0.0f);
		envelop_map[joint] = new_array;
		joint_it = envelop_map.find(joint);
	}

	std::vector<float>& envelop_data = joint_it->second;
	//try to increase the size, in fact we does not need this, because we resize all arrays at the first step
	if (envelop_data.size() < start_vertex_index + vertex_count)
	{
		envelop_data.resize(start_vertex_index + vertex_count, 0.0f);
	}
	envelop_data[start_vertex_index + vertex_index] += weight * 100.0f;  // in gltf weights from 0 to 1, in Softimage from 0 to 100
}

void add_envelop_data(const ULONG start_vertex_index, const ULONG vertex_count, const std::vector<ULONG> &joints, const std::vector<float> &weights, 
	std::unordered_map<ULONG, std::vector<float>> &envelop_map)
{
	//start_vertex_index is mesh vertex index of the first vertex in the current submesh
	//the key for envelop_map is joint index, value - array of weight for all mesh vertices

	//for all joint the size of the array should be the same ( = total vertex count)
	//so, at first we should resize all arrays, and only then setup values
	for (auto & pair : envelop_map)
	{
		std::vector<float>& joint_weights = pair.second;
		joint_weights.resize(start_vertex_index + vertex_count, 0.0f);
	}

	ULONG steps = joints.size() / (4 * vertex_count);  // at present time we does not have gltf examples with several joints attributes
	for (ULONG step = 0; step < steps; step++)
	{
		ULONG shift = step * 4 * vertex_count;
		for (ULONG i = 0; i < vertex_count; i++)
		{
			ULONG j1 = joints[shift + 4 * i];
			ULONG j2 = joints[shift + 4 * i + 1];
			ULONG j3 = joints[shift + 4 * i + 2];
			ULONG j4 = joints[shift + 4 * i + 3];

			float w1 = weights[shift + 4 * i];
			float w2 = weights[shift + 4 * i + 1];
			float w3 = weights[shift + 4 * i + 2];
			float w4 = weights[shift + 4 * i + 3];

			//in the map we should set the weights for the vertex i for all keys j1, j2, j3 and j4
			increase_envelop_weight_value(envelop_map, j1, w1, i, start_vertex_index, vertex_count);
			increase_envelop_weight_value(envelop_map, j2, w2, i, start_vertex_index, vertex_count);
			increase_envelop_weight_value(envelop_map, j3, w3, i, start_vertex_index, vertex_count);
			increase_envelop_weight_value(envelop_map, j4, w4, i, start_vertex_index, vertex_count);
		}
	}
}

XSI::X3DObject import_mesh(const tinygltf::Model& model, 
	const tinygltf::Mesh &mesh, 
	const XSI::CString& object_name, 
	const XSI::MATH::CTransformation &object_tfm,
	XSI::X3DObject &parent_object,
	std::unordered_map<int, XSI::Material>& material_map, 
	std::unordered_map<ULONG, std::vector<float>> &envelop_map,
	const ImportOptions& options)
{
	//create the mesh
	XSI::X3DObject xsi_object;
	XSI::CMeshBuilder mesh_builder;
	parent_object.AddPolygonMesh(object_name, xsi_object, mesh_builder);
	ULONG vertex_start_index = 0;  // increase this value after each primitive submesh, this will allows to count new vertices not from 0, but from the index of previous submeshes
	ULONG triangles_start_index = 0;
	ULONG samples_start_index = 0;

	//we should assign cluster materials after all polygons will be created
	//so, write the data into some buffers
	std::vector<std::vector<LONG>> primitives_material_triangles;
	std::vector<XSI::Material> primitives_material;

	//also save to some buffers data for normals, colors and uvs
	//for each attribute we save separate array of data
	std::unordered_map<std::string, std::vector<float>> attributes_map;  // key - attribute name, value - array of values
	std::unordered_map<ULONG, std::vector<float>> shapes_map;  // key - shape index, value - positions

	for (size_t primitive_index = 0; primitive_index < mesh.primitives.size(); primitive_index++)
	{
		tinygltf::Primitive primitive = mesh.primitives[primitive_index];

		//get positions
		int position_attr_index = primitive.attributes["POSITION"];
		const tinygltf::Accessor& position_accessor = model.accessors[position_attr_index];
		std::vector<double> positions = get_double_buffer(model, position_accessor);

		if (positions.size() == 0)
		{
			//something wrong here, because buffer with point positions is empty (the type of data is not float)
			continue;
		}

		//get triangle indices
		std::vector<LONG> polygons = get_polygon_inidices(model, primitive, vertex_start_index);
		ULONG vertex_count = positions.size() / 3;
		ULONG triangles_count = polygons.size() / 3;
		ULONG samples_count = polygons.size();
		std::vector<LONG> polygon_sizes(triangles_count, 3);

		if (polygons.size() == 0)
		{
			// skip the mesh, because polygon indices buffer has invalid data type
			continue;
		}

		mesh_builder.AddVertices(vertex_count, positions.data());
		mesh_builder.AddPolygons(triangles_count, polygon_sizes.data(), polygons.data());

		positions.clear();
		positions.shrink_to_fit();
		polygon_sizes.clear();
		polygon_sizes.shrink_to_fit();
		
		int material_index = primitive.material;
		std::unordered_map<int, XSI::Material>::iterator material_it = material_map.find(material_index);
		if (material_it != material_map.end())
		{
			XSI::Material material = material_it->second;
			//form array with triangle indices
			std::vector<LONG> material_triangles(triangles_count);
			for (ULONG i = 0; i < triangles_count; i++)
			{
				material_triangles[i] = i + triangles_start_index;
			}
			primitives_material.push_back(material);
			primitives_material_triangles.push_back(material_triangles);
		}

		std::vector<ULONG> skin_joints(0);
		std::vector<float> skin_weights(0);

		//save other attributes
		for (const std::pair<const std::string, int>& attribute : primitive.attributes)
		{
			//get accessor to the attribute
			const tinygltf::Accessor& accessor = model.accessors[attribute.second];
			std::string attribute_name = attribute.first;
			if (attribute_name == "POSITION")
			{
				continue;
			}
			if ((attribute_name.compare("NORMAL") == 0 && !options.is_import_normals) || 
				(attribute_name.find("TEXCOORD") == 0 && !options.is_import_uvs) ||
				(attribute_name.find("COLOR") == 0 && !options.is_import_colors) ||
				(attribute_name.find("JOINTS") == 0 && !options.is_import_skin) ||
				(attribute_name.find("WEIGHTS") == 0 && !options.is_import_skin))
			{
				continue;
			}

			LONG attr_length = attribute_length(attribute_name);
			if (attributes_map.find(attribute_name) == attributes_map.end())
			{
				//this is a new attribute, previously we does not read data for it
				//but may be previous primitives were readed
				//normals and uvs are 3-values, but colors are 4-values
				std::vector<float> attibute_data(samples_start_index * attr_length, 0.0f);
				attributes_map[attribute_name] = attibute_data;
			}

			std::vector<float>& attr_data = attributes_map.at(attribute_name);
			LONG prev_size = attr_data.size();
			//fill attr_data by empty values, if some previous primitives does not contains the data
			for (LONG i = 0; i < samples_start_index * attr_length - prev_size; i++)
			{
				attr_data.push_back(0.0f);
			}

			if (attribute_name.compare("NORMAL") == 0 && options.is_import_normals)
			{
				std::vector<float> normals = get_float_buffer(model, accessor);
				if (normals.size() == 0)
				{
					// there are no valid normal data
					continue;
				}
				//from gltf we obtain normal for each vertex
				//but in Softimage we should set the normal for each polygon corner - sample
				//so, we should convert per-point array to per-sample array
				for (ULONG i = 0; i < polygons.size(); i++)
				{
					LONG v = polygons[i] - vertex_start_index;  // vertex index
					attr_data.push_back(normals[3 * v]);
					attr_data.push_back(normals[3 * v + 1]);
					attr_data.push_back(normals[3 * v + 2]);
				}
				normals.clear();
				normals.shrink_to_fit();
			}
			else if (attribute_name.find("TEXCOORD") == 0 && options.is_import_uvs)
			{
				std::vector<float> uvs = get_float_buffer(model, accessor);
				if (uvs.size() == 0)
				{
					// there are no valid uvs data
					continue;
				}
				//we should convert from 2-values to three-values
				ULONG coords_count = uvs.size() / 2;
				for (ULONG i = 0; i < polygons.size(); i++)
				{
					ULONG v = polygons[i] - vertex_start_index;
					attr_data.push_back(uvs[2 * v]);
					attr_data.push_back(1.0f - uvs[2 * v + 1]);
					attr_data.push_back(0.0f);
				}
				uvs.clear();
				uvs.shrink_to_fit();
			}
			else if (attribute_name.find("COLOR") == 0 && options.is_import_colors)
			{
				std::vector<float> colors = get_float_buffer(model, accessor);
				if (colors.size() == 0)
				{
					continue;
				}
				int components = tinygltf::GetNumComponentsInType(accessor.type);
				int colors_type = accessor.componentType;
				for (ULONG i = 0; i < polygons.size(); i++)
				{
					LONG v = polygons[i] - vertex_start_index;
					for (ULONG c = 0; c < components; c++)
					{
						if (colors_type == 5121)
						{//unsigned char
							attr_data.push_back(colors[components * v + c] / 255.0f);
						}
						else if (colors_type == 5123)
						{//unsigned short, 65535 is maximal value
							attr_data.push_back(colors[components * v + c] / 65535.0f);
						}
						else
						{//float
							attr_data.push_back(colors[components * v + c]);
						}
					}
					for (ULONG c = components; c < 4; c++)
					{
						attr_data.push_back(0.0f);
					}
				}
				colors.clear();
				colors.shrink_to_fit();
			}
			else if (attribute_name.find("JOINTS") == 0 && options.is_import_skin)
			{
				std::vector<ULONG> joints = get_integer_buffer(model, accessor);
				skin_joints.reserve(skin_joints.size() + std::distance(joints.begin(), joints.end()));
				skin_joints.insert(skin_joints.end(), joints.begin(), joints.end());
			}
			else if (attribute_name.find("WEIGHTS") == 0 && options.is_import_skin)
			{
				std::vector<float> weights = get_float_buffer(model, accessor);
				skin_weights.reserve(skin_weights.size() + std::distance(weights.begin(), weights.end()));
				skin_weights.insert(skin_weights.end(), weights.begin(), weights.end());
			}
			//also valid are: TANGENT
		}

		if (skin_joints.size() > 0 && skin_weights.size() > 0 && options.is_import_skin)
		{
			//these two arrays contains data from all envelop properties of the subobject
			add_envelop_data(vertex_start_index, vertex_count, skin_joints, skin_weights, envelop_map);
		}

		skin_joints.clear();
		skin_joints.shrink_to_fit();
		skin_weights.clear();
		skin_weights.shrink_to_fit();

		//next shapes
		//we supports only position shape, because Softimage can not allows to deform normals or tangents
		if (options.is_import_shapes)
		{
			for (ULONG shape_index = 0; shape_index < primitive.targets.size(); shape_index++)
			{
				std::map<std::string, int>& gltf_shape = primitive.targets[shape_index];
				if (gltf_shape.find("POSITION") != gltf_shape.end())
				{
					//this shape contains positions data
					if (shapes_map.find(shape_index) == shapes_map.end())
					{
						std::vector<float> shape_data(vertex_start_index * 3, 0.0f);
						shapes_map[shape_index] = shape_data;
					}
					std::vector<float>& shape_data = shapes_map.at(shape_index);
					LONG prev_size = shape_data.size();
					for (LONG i = 0; i < vertex_start_index * 3 - prev_size; i++)
					{
						shape_data.push_back(0.0f);
					}

					int shape_positions_index = gltf_shape.at("POSITION");
					const tinygltf::Accessor& shape_accessor = model.accessors[shape_positions_index];
					std::vector<float> shape_positions(0);
					if (shape_accessor.sparse.isSparse)
					{
						//for sparse accessor we can obtain positions only for several points
						std::vector<float> values = read_float_buffer_view(
							model,
							model.bufferViews[shape_accessor.sparse.values.bufferView],
							shape_accessor.componentType,
							shape_accessor.sparse.values.byteOffset,
							tinygltf::GetNumComponentsInType(shape_accessor.type),
							shape_accessor.sparse.count,
							false);

						std::vector<ULONG> indices = read_integer_buffer_view(
							model,
							model.bufferViews[shape_accessor.sparse.indices.bufferView],
							shape_accessor.sparse.indices.componentType,
							shape_accessor.sparse.indices.byteOffset,
							1,
							shape_accessor.sparse.count);

						shape_positions.resize(3 * vertex_count, 0.0);
						for (ULONG i = 0; i < indices.size(); i++)
						{
							ULONG indx = indices[i];
							shape_positions[3 * indx] = values[3 * i];
							shape_positions[3 * indx + 1] = values[3 * i + 1];
							shape_positions[3 * indx + 2] = values[3 * i + 2];
						}
					}
					else
					{
						shape_positions = get_float_buffer(model, shape_accessor);
						//this array contains deltas for point positions
					}
					//save readed values
					if (shape_positions.size() > 0)
					{
						shape_data.reserve(shape_data.size() + std::distance(shape_positions.begin(), shape_positions.end()));
						shape_data.insert(shape_data.end(), shape_positions.begin(), shape_positions.end());
					}
				}
			}
		}
		
		polygons.clear();
		polygons.shrink_to_fit();

		vertex_start_index += vertex_count;
		triangles_start_index += triangles_count;
		samples_start_index += samples_count;
	}

	// Generate the new mesh
	mesh_builder.Build(false);

	//next add attributes
	XSI::PolygonMesh xsi_mesh = xsi_object.GetActivePrimitive().GetGeometry();
	XSI::CClusterPropertyBuilder cluster_builder = xsi_mesh.GetClusterPropertyBuilder();
	for (auto const& pair : attributes_map)
	{
		std::string attr_name = pair.first;
		const std::vector<float>& attr_data = pair.second;

		XSI::ClusterProperty cluster;
		if (attr_name.compare("NORMAL") == 0 && options.is_import_normals)
		{
			cluster = cluster_builder.AddUserNormal(true);
		}
		else if (attr_name.find("TEXCOORD") == 0 && options.is_import_uvs)
		{
			cluster = cluster_builder.AddUV();
		}
		else if (attr_name.find("COLOR") == 0 && options.is_import_colors)
		{
			cluster = cluster_builder.AddVertexColor();
		}
		if (cluster.IsValid())
		{
			cluster.SetValues(attr_data.data(), samples_start_index);
		}
	}

	XSI::CValue io_value;
	if (options.is_import_shapes)
	{
		for (auto it = shapes_map.cbegin(); it != shapes_map.cend(); ++it)
		{
			ULONG shape_index = it->first;
			std::vector<float> shape_values = it->second;

			XSI::ClusterProperty cluster = cluster_builder.AddShapeKey();
			cluster.SetValues(shape_values.data(), vertex_start_index);

			//apply created shape key
			//if we call the command, then the second shape is not visible in the shape manager
			//may be we should call it with other parameters
			/*XSI::CValueArray shape_args(8);
			shape_args[0] = XSI::CValue(cluster);
			shape_args[3] = XSI::CValue(3);
			shape_args[5] = XSI::CValue(5);
			shape_args[7] = XSI::CValue(2);
			XSI::Application().ExecuteCommand("ApplyShapeKey", shape_args, io_value);*/
		}
	}

	if (primitives_material.size() > 0)
	{
		//apply the first material to the whole object
		XSI::Material main_material = primitives_material[0];
		XSI::CValueArray apply_material_args(2);
		XSI::CValueArray objects(2);
		objects[0] = xsi_object;
		objects[1] = main_material;
		apply_material_args[0] = objects;
		apply_material_args[1] = XSI::siRemoveFromExistingClusters;
		XSI::CValue io_value;
		XSI::Application().ExecuteCommand("AssignMaterial", apply_material_args, io_value);

		//apply cluster materials
		for (ULONG i = 0; i < primitives_material.size(); i++)
		{
			XSI::Material material = primitives_material[i];
			std::vector<LONG> material_triangles = primitives_material_triangles[i];
			mesh_builder.SetPolygonsMaterial(material, material_triangles.size(), material_triangles.data());
			material_triangles.clear();
			material_triangles.shrink_to_fit();
		}
	}

	//set object transform
	xsi_object.GetKinematics().GetLocal().PutTransform(object_tfm);

	//clear buffers
	attributes_map.clear();
	primitives_material.clear();
	primitives_material_triangles.clear();
	shapes_map.clear();

	return xsi_object;
}

void import_skin(const tinygltf::Model& model,
	XSI::X3DObject& xsi_object,
	const int skin_index,
	const std::unordered_map<ULONG, std::vector<float>>& envelope_data,
	std::unordered_map<ULONG, XSI::X3DObject>& nodes_map)
{
	if (skin_index >= 0 && skin_index < model.skins.size() && xsi_object.GetType() == "polymsh")
	{
		XSI::PolygonMesh polymesh = xsi_object.GetActivePrimitive().GetGeometry();
		XSI::CClusterPropertyBuilder cp_builder = polymesh.GetClusterPropertyBuilder();
		XSI::CGeometryAccessor polymesh_acc = polymesh.GetGeometryAccessor();
		LONG vertex_count = polymesh_acc.GetVertexCount();

		tinygltf::Skin skin = model.skins[skin_index];
		std::vector<int>& skin_joints = skin.joints;
		if (skin.inverseBindMatrices >= 0)
		{
			std::vector<float> ibm = get_float_buffer(model, model.accessors[skin.inverseBindMatrices]);
			//from these matrices we can extract global transform of the skinned object
			//we can use, for example, the first deformer
			ULONG node_i = envelope_data.begin()->first;
			if (node_i >= 0 && node_i < skin_joints.size())
			{
				ULONG node_index = skin_joints[node_i];
				std::unordered_map<ULONG, XSI::X3DObject>::iterator node_it = nodes_map.find(node_index);
				if (node_it != nodes_map.end())
				{
					XSI::X3DObject& first_deformer = node_it->second;
					//get the global transform matrix
					XSI::MATH::CMatrix4 d_m = first_deformer.GetKinematics().GetGlobal().GetTransform().GetMatrix4();
					//build the matrix from ibm
					XSI::MATH::CMatrix4 m;
					for (ULONG i = 0; i < 4; i++)
					{
						for (ULONG j = 0; j < 4; j++)
						{
							m.SetValue(i, j, ibm[16 * node_i + 4 * i + j]);
						}
					}
					XSI::MATH::CMatrix4 obj_global_m;
					obj_global_m.Mul(m, d_m);
					XSI::MATH::CTransformation obj_global_tfm;
					obj_global_tfm.SetMatrix4(obj_global_m);
					xsi_object.GetKinematics().GetGlobal().PutTransform(obj_global_tfm);
				}
			}
		}
		XSI::CRefArray deformers(0);
		std::vector<ULONG> proper_keys(0);
		for (auto const& pair : envelope_data)
		{
			ULONG joint_index = pair.first;
			if (joint_index < skin_joints.size())
			{
				ULONG joint_node_index = skin_joints[joint_index];
				std::unordered_map<ULONG, XSI::X3DObject>::iterator joint_node_it = nodes_map.find(joint_node_index);
				if (joint_node_it != nodes_map.end())
				{
					XSI::X3DObject& deformer = joint_node_it->second;
					if (deformer.IsValid() && pair.second.size() == vertex_count)
					{
						deformers.Add(deformer);
						proper_keys.push_back(joint_index);
					}
				}
			}
		}

		if (proper_keys.size() > 0)
		{
			XSI::EnvelopeWeight ewp = cp_builder.AddEnvelopeWeight(deformers, true);

			for (ULONG i = 0; i < proper_keys.size(); i++)
			{
				ewp.SetValues(deformers[i], envelope_data.at(proper_keys[i]).data(), vertex_count);
			}
		}

		deformers.Clear();
		proper_keys.clear();
	}
	
}