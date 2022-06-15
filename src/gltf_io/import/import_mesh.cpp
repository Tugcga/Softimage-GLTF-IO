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

#include "../../tiny_gltf/tiny_gltf.h"

#include "../../utilities/utilities.h"
#include "../import.h"

std::vector<LONG> get_polygon_inidices(const tinygltf::Model& model, const tinygltf::Primitive& primitive)
{
	const tinygltf::Accessor& polygon_accessor = model.accessors[primitive.indices];

	std::vector<LONG> to_return(polygon_accessor.count);

	const tinygltf::BufferView& buffer_view = model.bufferViews[polygon_accessor.bufferView];
	const tinygltf::Buffer& buffer = model.buffers[buffer_view.buffer];

	const unsigned short* positions = reinterpret_cast<const unsigned short*>(&buffer.data[buffer_view.byteOffset + polygon_accessor.byteOffset]);
	for (size_t i = 0; i < polygon_accessor.count; ++i)
	{
		to_return[i] = positions[i];
	}

	return to_return;
}

std::vector<float> get_float_buffer(const tinygltf::Model& model, const tinygltf::Accessor& accessor)
{
	int32_t components = tinygltf::GetNumComponentsInType(accessor.type);

	std::vector<float> to_return(components * accessor.count);

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

std::vector<double> get_double_buffer(const tinygltf::Model& model, const tinygltf::Accessor& accessor)
{
	int32_t components = tinygltf::GetNumComponentsInType(accessor.type);

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

XSI::X3DObject import_mesh(const tinygltf::Model& model, const tinygltf::Mesh &mesh, const XSI::CString& object_name, const XSI::MATH::CTransformation &object_tfm, XSI::X3DObject &parent_object, const ImportMeshOptions& options)
{
	XSI::X3DObject object;
	log_message("import the mesh " + XSI::CString(mesh.name.c_str()));
	for (size_t primitive_index = 0; primitive_index < mesh.primitives.size(); primitive_index++)
	{
		tinygltf::Primitive primitive = mesh.primitives[primitive_index];

		//get positions
		int position_attr_index = primitive.attributes["POSITION"];
		const tinygltf::Accessor& position_accessor = model.accessors[position_attr_index];
		std::vector<double> positions = get_double_buffer(model, position_accessor);

		//proces vertices
		std::unordered_map<ULONG, ULONG> vertices_map;  // key - old vertex index, value - new vertex index
		std::vector<double> welded_positions(0);
		if (options.weld_vertices)
		{
			//we should create new array with vertices and also create a map from old index to the new index
			//and use this map for triangles
			ULONG old_vertex_count = positions.size() / 3;
			ULONG welded_index = 0;
			for (ULONG i = 0; i < old_vertex_count; i++)
			{
				double x = positions[3*i];
				double y = positions[3 * i + 1];
				double z = positions[3 * i + 2];

				//find in the welded positions close vertex
				int index = get_closest_index(welded_positions, x, y, z);
				if (index == -1)
				{//there are no close points
					welded_positions.push_back(x);
					welded_positions.push_back(y);
					welded_positions.push_back(z);
					vertices_map[i] = welded_index;
					welded_index++;
				}
				else
				{
					vertices_map[i] = index;
				}
			}
		}

		//get triangle indices
		std::vector<LONG> polygons = get_polygon_inidices(model, primitive);
		ULONG vertex_count = positions.size() / 3;
		ULONG triangles_count = polygons.size() / 3;
		ULONG samples_count = polygons.size();
		std::vector<LONG> polygon_sizes(triangles_count, 3);

		//create the mesh
		XSI::X3DObject xsi_object;
		XSI::CMeshBuilder mesh_builder;
		parent_object.AddPolygonMesh(object_name + (mesh.primitives.size() == 1 ? "" : "_" + XSI::CString(primitive_index)), xsi_object, mesh_builder);
		if (options.weld_vertices)
		{
			mesh_builder.AddVertices(welded_positions.size() / 3, welded_positions.data());
			//we should form new array for triangles
			std::vector<LONG> weld_polygons(triangles_count * 3);
			for (ULONG i = 0; i < polygons.size(); i++)
			{
				weld_polygons[i] = vertices_map[polygons[i]];
			}

			mesh_builder.AddPolygons(triangles_count, polygon_sizes.data(), weld_polygons.data());
		}
		else
		{
			mesh_builder.AddVertices(vertex_count, positions.data());
			mesh_builder.AddPolygons(triangles_count, polygon_sizes.data(), polygons.data());
		}
		// Generate the new mesh
		mesh_builder.Build(false);

		//add other attributes
		XSI::PolygonMesh xsi_mesh = xsi_object.GetActivePrimitive().GetGeometry();
		XSI::CClusterPropertyBuilder cluster_builder = xsi_mesh.GetClusterPropertyBuilder();
		for (const std::pair<const std::string, int>& attribute : primitive.attributes)
		{
			//get accessor to the attribute
			const tinygltf::Accessor& accessor = model.accessors[attribute.second];
			if (attribute.first.compare("NORMAL") == 0)
			{
				std::vector<float> normals = get_float_buffer(model, accessor);
				//from gltf we obtain normal for each vertex
				//but in Softimage we should set the normal for each polygon corner - sample
				//so, we should convert per-point array t per-sample array
				std::vector<float> xsi_normals(samples_count * 3);
				for (ULONG i = 0; i < polygons.size(); i++)
				{
					LONG v = polygons[i];  // vertex index
					xsi_normals[3 * i] = normals[3 * v];
					xsi_normals[3 * i + 1] = normals[3 * v + 1];
					xsi_normals[3 * i + 2] = normals[3 * v + 2];
				}
				XSI::ClusterProperty normal_cluster = cluster_builder.AddUserNormal(options.weld_vertices ? false : true);  // make cluster non-complete in the case when we weld vertices, because it crash. For non-welded vertices complete cluster works ok
				normal_cluster.SetValues(xsi_normals.data(), samples_count);
			}
			else if (attribute.first.find("TEXCOORD") == 0)
			{
				std::vector<float> uvs = get_float_buffer(model, accessor);
				//we should convert from 2-values to thre-values
				ULONG coords_count = uvs.size() / 2;
				std::vector<float> xsi_uvs(3 * samples_count, 0.0f);
				for (ULONG i = 0; i < polygons.size(); i++)
				{
					ULONG v = polygons[i];
					xsi_uvs[3 * i] = uvs[2 * v];
					xsi_uvs[3 * i + 1] = 1.0f - uvs[2 * v + 1];  // flip v axis
				}

				XSI::ClusterProperty uv_cluster = cluster_builder.AddUV();
				uv_cluster.SetValues(xsi_uvs.data(), samples_count);
			}
			//also valid are: TANGENT, COLOR_n, JOINTS_n, WEIGHTS_n
		}

		//set object transform
		xsi_object.GetKinematics().GetLocal().PutTransform(object_tfm);

		object = xsi_object;
	}

	return object;
}