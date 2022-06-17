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

#include "../../tiny_gltf/tiny_gltf.h"

#include "../../utilities/utilities.h"
#include "../import.h"

std::vector<LONG> get_polygon_inidices(const tinygltf::Model& model, const tinygltf::Primitive& primitive, const ULONG first_index)
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

std::vector<float> get_float_buffer(const tinygltf::Model& model, const tinygltf::Accessor& accessor)
{
	int32_t components = tinygltf::GetNumComponentsInType(accessor.type);
	int component_type = accessor.componentType;

	std::vector<float> to_return(components * accessor.count);

	const tinygltf::BufferView& buffer_view = model.bufferViews[accessor.bufferView];
	const tinygltf::Buffer& buffer = model.buffers[buffer_view.buffer];

	if (component_type == 5126)
	{
		const float* data = reinterpret_cast<const float*>(&buffer.data[buffer_view.byteOffset + accessor.byteOffset]);
		for (size_t i = 0; i < accessor.count; ++i)
		{
			for (int32_t c = 0; c < components; c++)
			{
				to_return[components * i + c] = data[i * components + c];
			}
		}
	}
	else if (component_type == 5121)
	{
		const unsigned char* data = reinterpret_cast<const unsigned char*>(&buffer.data[buffer_view.byteOffset + accessor.byteOffset]);
		for (size_t i = 0; i < accessor.count; ++i)
		{
			for (int32_t c = 0; c < components; c++)
			{
				to_return[components * i + c] = data[i * components + c];
			}
		}
	}
	else if (component_type == 5123)
	{
		const unsigned short* data = reinterpret_cast<const unsigned short*>(&buffer.data[buffer_view.byteOffset + accessor.byteOffset]);
		for (size_t i = 0; i < accessor.count; ++i)
		{
			for (int32_t c = 0; c < components; c++)
			{
				to_return[components * i + c] = data[i * components + c];
			}
		}
	}
	else
	{
		std::vector<float> empty(0);
		return empty;
	}
	
	return to_return;
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

XSI::X3DObject import_mesh(const tinygltf::Model& model, const tinygltf::Mesh &mesh, const XSI::CString& object_name, const XSI::MATH::CTransformation &object_tfm, XSI::X3DObject &parent_object, const std::unordered_map<int, XSI::Material>& material_map, const ImportMeshOptions& options)
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
		if (material_map.find(material_index) != material_map.end())
		{
			XSI::Material material = material_map.at(material_index);
			//form array with triangle indices
			std::vector<LONG> material_triangles(triangles_count);
			for (ULONG i = 0; i < triangles_count; i++)
			{
				material_triangles[i] = i + triangles_start_index;
			}
			primitives_material.push_back(material);
			primitives_material_triangles.push_back(material_triangles);
		}

		//save add other attributes
		for (const std::pair<const std::string, int>& attribute : primitive.attributes)
		{
			//get accessor to the attribute
			const tinygltf::Accessor& accessor = model.accessors[attribute.second];
			std::string attribute_name = attribute.first;
			if (attribute_name == "POSITION")
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

			if (attribute.first.compare("NORMAL") == 0)
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
			else if (attribute.first.find("TEXCOORD") == 0)
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
			else if (attribute.first.find("COLOR") == 0)
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
			//also valid are: TANGENT, JOINTS_n, WEIGHTS_n
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
		if (attr_name.compare("NORMAL") == 0)
		{
			cluster = cluster_builder.AddUserNormal(true);
		}
		else if (attr_name.find("TEXCOORD") == 0)
		{
			cluster = cluster_builder.AddUV();
		}
		else if (attr_name.find("COLOR") == 0)
		{
			cluster = cluster_builder.AddVertexColor();
		}
		if (cluster.IsValid())
		{
			cluster.SetValues(attr_data.data(), samples_start_index);
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

	return xsi_object;
}