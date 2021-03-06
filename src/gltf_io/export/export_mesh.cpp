#include <set>
#include <unordered_map>

#include <xsi_x3dobject.h>
#include <xsi_primitive.h>
#include <xsi_geometryaccessor.h>
#include <xsi_geometry.h>
#include <xsi_polygonmesh.h>
#include <xsi_material.h>
#include <xsi_vector3f.h>
#include <xsi_envelopeweight.h>
#include <xsi_progressbar.h>

#include "../gltf_io.h"
#include "../../utilities/utilities.h"

LONG get_vertex_index(Vertex &v, const std::vector<Vertex> &export_vertices, const std::set<ULONG> &indices)
{
	std::set<ULONG>::iterator set_it;
	for (set_it = indices.begin(); set_it != indices.end(); ++set_it) 
	{
		ULONG i = *set_it;
		if (v.is_coincide(export_vertices[i]))
		{
			return i;
		}
	}

	return -1;
}

LONG get_value_index(const std::vector<ULONG> &array, const ULONG value)
{
	for (ULONG i = 0; i < array.size(); i++)
	{
		if (array[i] == value)
		{
			return i;
		}
	}

	return -1;
}

void write_mesh_data(tinygltf::Model& model, tinygltf::Primitive &prim, const std::vector<Vertex>& data)
{
	//here we should conver initial data array to several arrays with different attributes
	std::vector<float> positions(data.size() * 3);
	std::vector<float> normals(data.size() * 3);
	ULONG uvs_count = data[0].uvs.size() / 2;  // we assume that all vertex has the same number of uv coordinates as the first vertex
	ULONG colors_count = data[0].colors.size() / 4;
	ULONG envelopes_count = data[0].weights.size() / 4;
	std::vector<std::vector<float>> all_weights(envelopes_count);
	std::vector<std::vector<unsigned short>> all_joints(envelopes_count);
	for (ULONG e = 0; e < envelopes_count; e++)
	{
		all_weights[e].resize(4 * data.size(), 0.0f);
		all_joints[e].resize(4 * data.size(), 0);
	}

	std::vector<std::vector<float>> uv_channels(uvs_count);
	for (ULONG i = 0; i < uvs_count; i++)
	{
		uv_channels[i].resize(2 * data.size(), 0.0f);
	}
	std::vector<std::vector<float>> color_channels(colors_count);

	for (ULONG i = 0; i < colors_count; i++)
	{
		color_channels[i].resize(4 * data.size(), 0.0f);
	}
	//form arrays
	std::vector<double> min_values{ FLT_MAX, FLT_MAX, FLT_MAX };
	std::vector<double> max_values{ FLT_MIN, FLT_MIN, FLT_MIN };
	std::vector<float> uvs_min(2 * uvs_count, FLT_MAX);  // write here all min pairs for all uvs
	std::vector<float> uvs_max(2 * uvs_count, FLT_MIN);
	std::vector<float> colors_min(4 * colors_count, FLT_MAX);
	std::vector<float> colors_max(4 * colors_count, FLT_MIN);
	for (ULONG v = 0; v < data.size(); v++)
	{
		const Vertex& vertex = data[v];
		float x = vertex.position.GetX();
		float y = vertex.position.GetY();
		float z = vertex.position.GetZ();
		positions[3 * v] = x;
		positions[3 * v + 1] = y;
		positions[3 * v + 2] = z;

		if (x < min_values[0]) { min_values[0] = x; }
		if (y < min_values[1]) { min_values[1] = y; }
		if (z < min_values[2]) { min_values[2] = z; }

		if (x > max_values[0]) { max_values[0] = x; }
		if (y > max_values[1]) { max_values[1] = y; }
		if (z > max_values[2]) { max_values[2] = z; }

		normals[3 * v] = vertex.normal.GetX();
		normals[3 * v + 1] = vertex.normal.GetY();
		normals[3 * v + 2] = vertex.normal.GetZ();

		//next copy uvs
		ULONG vertex_uv_count = vertex.uvs.size() / 2;
		for (ULONG i = 0; i < uvs_count; i++)
		{
			//if vertex does not contains i-th uv (uv array is shorter), then nothing to do, values will be 0.0
			if (i < vertex_uv_count)
			{
				float u_value = vertex.uvs[2 * i];
				float v_value = 1.0f - vertex.uvs[2 * i + 1];  // flip v-value
				uv_channels[i][2 * v] = u_value;
				uv_channels[i][2 * v + 1] = v_value;

				if (u_value < uvs_min[2*i]){ uvs_min[2 * i] = u_value; }
				if (u_value > uvs_max[2 * i]) { uvs_max[2 * i] = u_value; }

				if (v_value < uvs_min[2 * i + 1]) { uvs_min[2 * i + 1] = v_value; }
				if (v_value > uvs_max[2 * i + 1]) { uvs_max[2 * i + 1] = v_value; }
			}
		}

		//and colors
		ULONG vertex_color_count = vertex.colors.size() / 4;
		for (ULONG i = 0; i < colors_count; i++)
		{
			if (i < vertex_color_count)
			{
				float r = vertex.colors[4 * i];
				float g = vertex.colors[4 * i + 1];
				float b = vertex.colors[4 * i + 2];
				float a = vertex.colors[4 * i + 3];

				color_channels[i][4 * v] = r;
				color_channels[i][4 * v + 1] = g;
				color_channels[i][4 * v + 2] = b;
				color_channels[i][4 * v + 3] = a;

				if (r < colors_min[4 * i]) { colors_min[4 * i] = r; }
				if (r > colors_max[4 * i]) { colors_max[4 * i] = r; }

				if (g < colors_min[4 * i + 1]) { colors_min[4 * i + 1] = g; }
				if (g > colors_max[4 * i + 1]) { colors_max[4 * i + 1] = g; }

				if (b < colors_min[4 * i + 2]) { colors_min[4 * i + 2] = b; }
				if (b > colors_max[4 * i + 2]) { colors_max[4 * i + 2] = b; }

				if (a < colors_min[4 * i + 3]) { colors_min[4 * i + 3] = a; }
				if (a > colors_max[4 * i + 3]) { colors_max[4 * i + 3] = a; }
			}
		}

		//envelope weights and joints
		for (ULONG e = 0; e < envelopes_count; e++)
		{
			std::vector<float>& weights = all_weights[e];
			std::vector<unsigned short>& joints = all_joints[e];
			weights[4 * v] = vertex.weights[4 * e] / 100.0f;
			weights[4 * v + 1] = vertex.weights[4 * e + 1] / 100.0f;
			weights[4 * v + 2] = vertex.weights[4 * e + 2] / 100.0f;
			weights[4 * v + 3] = vertex.weights[4 * e + 3] / 100.0f;

			joints[4 * v] = vertex.joints[4 * e];
			joints[4 * v + 1] = vertex.joints[4 * e + 1];
			joints[4 * v + 2] = vertex.joints[4 * e + 2];
			joints[4 * v + 3] = vertex.joints[4 * e + 3];
		}
	}

	//next convert arrays to bytes
	//for positions
	const unsigned char* position_bytes = reinterpret_cast<const unsigned char*>(&positions[0]);
	std::vector<unsigned char> position_byte_vector(position_bytes, position_bytes + sizeof(float) * positions.size());
	//for normals
	const unsigned char* normal_bytes = reinterpret_cast<const unsigned char*>(&normals[0]);
	std::vector<unsigned char> normal_byte_vector(normal_bytes, normal_bytes + sizeof(float) * normals.size());

	//assign positions attribute
	prim.attributes["POSITION"] = add_data_to_buffer(model, position_byte_vector, data.size(), false, false, TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_TYPE_VEC3, min_values, max_values);

	//next normals attribute
	min_values.clear();
	max_values.clear();
	prim.attributes["NORMAL"] = add_data_to_buffer(model, normal_byte_vector, data.size(), false, false, TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_TYPE_VEC3, min_values, max_values);

	//all uvs
	for (ULONG i = 0; i < uvs_count; i++)
	{
		const unsigned char* uv_bytes = reinterpret_cast<const unsigned char*>(&uv_channels[i][0]);
		std::vector<unsigned char> uv_byte_vector(uv_bytes, uv_bytes + sizeof(float) * uv_channels[i].size());
		//extract min and max values
		std::vector<double> uv_min{ uvs_min[2 * i], uvs_min[2*i + 1] };
		std::vector<double> uv_max{ uvs_max[2 * i], uvs_max[2 * i + 1] };
		prim.attributes["TEXCOORD_" + std::to_string(i)] = add_data_to_buffer(model, uv_byte_vector, data.size(), false, false, TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_TYPE_VEC2, uv_min, uv_max);

		uv_min.clear();
		uv_max.clear();
	}

	//all colors
	for (ULONG i = 0; i < colors_count; i++)
	{
		const unsigned char* color_bytes = reinterpret_cast<const unsigned char*>(&color_channels[i][0]);
		std::vector<unsigned char> color_byte_vector(color_bytes, color_bytes + sizeof(float) * color_channels[i].size());

		std::vector<double> color_min{ colors_min[4 * i], colors_min[4 * i + 1], colors_min[4 * i + 2], colors_min[4 * i + 3] };
		std::vector<double> color_max{ colors_max[4 * i], colors_max[4 * i + 1], colors_max[4 * i + 2], colors_max[4 * i + 3] };
		prim.attributes["COLOR_" + std::to_string(i)] = add_data_to_buffer(model, color_byte_vector, data.size(), false, false, TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_TYPE_VEC4, color_min, color_max);

		color_byte_vector.clear();
		color_byte_vector.shrink_to_fit();
		color_min.clear();
		color_min.shrink_to_fit();
		color_max.clear();
		color_max.shrink_to_fit();
	}

	//all weights and joints
	for (ULONG e = 0; e < envelopes_count; e++)
	{
		const unsigned char* weight_bytes = reinterpret_cast<const unsigned char*>(&all_weights[e][0]);
		std::vector<unsigned char> weight_byte_vector(weight_bytes, weight_bytes + sizeof(float) * all_weights[e].size());
		prim.attributes["WEIGHTS_" + std::to_string(e)] = add_data_to_buffer(model, weight_byte_vector, data.size(), false, false, TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_TYPE_VEC4, min_values, max_values);

		const unsigned char* joints_bytes = reinterpret_cast<const unsigned char*>(&all_joints[e][0]);
		std::vector<unsigned char> joints_byte_vector(joints_bytes, joints_bytes + sizeof(float) * all_joints[e].size());
		prim.attributes["JOINTS_" + std::to_string(e)] = add_data_to_buffer(model, joints_byte_vector, data.size(), false, false, TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT, TINYGLTF_TYPE_VEC4, min_values, max_values);

		weight_byte_vector.clear();
		weight_byte_vector.shrink_to_fit();
		joints_byte_vector.clear();
		joints_byte_vector.shrink_to_fit();
	}

	//clear temporary buffers
	positions.clear();
	positions.shrink_to_fit();
	normals.clear();
	normals.shrink_to_fit();
	all_weights.clear();
	all_weights.shrink_to_fit();
	all_joints.clear();
	all_joints.shrink_to_fit();
	uv_channels.clear();
	uv_channels.shrink_to_fit();
	color_channels.clear();
	color_channels.shrink_to_fit();
	min_values.clear();
	min_values.shrink_to_fit();
	max_values.clear();
	max_values.shrink_to_fit();
	uvs_min.clear();
	uvs_min.shrink_to_fit();
	uvs_max.clear();
	uvs_max.shrink_to_fit();
	colors_min.clear();
	colors_min.shrink_to_fit();
	colors_max.clear();
	colors_max.shrink_to_fit();
	position_byte_vector.clear();
	position_byte_vector.shrink_to_fit();
	normal_byte_vector.clear();
	normal_byte_vector.shrink_to_fit();
}

void write_shapes_data(tinygltf::Model &model, tinygltf::Primitive &prim, std::vector<Vertex>& vertices)
{
	if (vertices.size() > 0)
	{
		ULONG shapes_count = vertices[0].shapes.size() / 3;
		for (ULONG shape_index = 0; shape_index < shapes_count; shape_index++)
		{
			//form float array of values
			std::vector<float> shape(vertices.size() * 3);
			std::vector<double> min_values(3, DBL_MAX);
			std::vector<double> max_values(3, DBL_MIN);
			for (ULONG v = 0; v < vertices.size(); v++)
			{
				const Vertex& vertex = vertices[v];
				float x = vertex.shapes[3 * shape_index];
				float y = vertex.shapes[3 * shape_index + 1];
				float z = vertex.shapes[3 * shape_index + 2];
				shape[3 * v] = x;
				shape[3 * v + 1] = y;
				shape[3 * v + 2] = z;

				if (x < min_values[0]) { min_values[0] = x; }
				if (x > max_values[0]) { max_values[0] = x; }

				if (y < min_values[1]) { min_values[1] = y; }
				if (y > max_values[1]) { max_values[1] = y; }

				if (z < min_values[2]) { min_values[2] = z; }
				if (z > max_values[2]) { max_values[2] = z; }
			}

			const unsigned char* shape_bytes = reinterpret_cast<const unsigned char*>(&shape[0]);
			std::vector<unsigned char> shape_byte_vector(shape_bytes, shape_bytes + sizeof(float) * shape.size());

			std::map<std::string, int> prim_shape;
			prim_shape["POSITION"] = add_data_to_buffer(model, shape_byte_vector, vertices.size(), false, false, TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_TYPE_VEC3, min_values, max_values);
			prim.targets.push_back(prim_shape);

			shape_byte_vector.clear();
			shape_byte_vector.shrink_to_fit();
			shape.clear();
			shape.shrink_to_fit();
			min_values.clear();
			min_values.shrink_to_fit();
			max_values.clear();
			max_values.shrink_to_fit();
		}
	}
}

void export_mesh(tinygltf::Node &node, 
	tinygltf::Model& model, 
	XSI::ProgressBar& bar,
	XSI::X3DObject& xsi_object, 
	const ExportOptions& options, 
	std::unordered_map<ULONG, ULONG>& materials_map, 
	std::unordered_map<ULONG, ULONG> &textures_map,
	std::vector<XSI::X3DObject> &envelope_meshes)
{
	XSI::PolygonMesh xsi_mesh = xsi_object.GetActivePrimitive().GetGeometry();

	int subdivs = 0;
	float ga_angle = 60.0;
	bool ga_use_angle = true;
	XSI::Property ga_property;
	xsi_object.GetPropertyFromName("geomapprox", ga_property);
	if (ga_property.IsValid())
	{
		subdivs = ga_property.GetParameterValue("gapproxmordrsl");
		ga_angle = ga_property.GetParameterValue("gapproxmoan");
		ga_use_angle = ga_property.GetParameterValue("gapproxmoad");
	}

	XSI::CGeometryAccessor xsi_acc = xsi_mesh.GetGeometryAccessor(XSI::siConstructionModeModeling, XSI::siCatmullClark, subdivs, false, ga_use_angle, ga_angle);
	
	//export all assigned materials
	XSI::CRefArray materials = xsi_acc.GetMaterials();  // materials here can be repeated, if different clusters has the same material
	for (ULONG i = 0; i < materials.GetCount(); i++)
	{
		XSI::Material material(materials[i]);
		if (options.is_export_materials)
		{
			export_material(model, bar, material, options, materials_map, textures_map);
		}
	}
	bar.PutStatusText("Object: " + xsi_object.GetFullName());

	//read geometry data
	XSI::CLongArray triangle_vertices;
	XSI::CLongArray triangle_nodes;
	XSI::CFloatArray node_normals;
	XSI::CDoubleArray vertex_positions;
	XSI::CLongArray polygon_materials;  // index in the large material list (with repetitions) for each polygon
	XSI::CLongArray triangle_polygons;  // polygon index for each triangle
	LONG triangles_count = xsi_acc.GetTriangleCount();
	LONG nodes_count = xsi_acc.GetNodeCount();
	ULONG vertex_count = xsi_acc.GetVertexCount();
	xsi_acc.GetTriangleVertexIndices(triangle_vertices);
	xsi_acc.GetTriangleNodeIndices(triangle_nodes);
	xsi_acc.GetNodeNormals(node_normals);
	xsi_acc.GetVertexPositions(vertex_positions);
	xsi_acc.GetPolygonMaterialIndices(polygon_materials);
	xsi_acc.GetPolygonTriangleIndices(triangle_polygons);

	//read mesh uvs
	XSI::CRefArray xsi_uvs = xsi_acc.GetUVs();
	ULONG uvs_count = options.is_export_uvs ? xsi_uvs.GetCount() : 0;
	std::vector<float> uvs_array(uvs_count * 2 * nodes_count);  // contains all uv coordinates for nodes, at first data for the first uv (for all nodes), then for the second and so on
	XSI::CFloatArray uv;
	for (ULONG i = 0; i < uvs_count; i++)
	{
		XSI::ClusterProperty uv_prop(xsi_uvs[i]);
		uv_prop.GetValues(uv);
		//copy data from uv array to the whole buffer
		for (ULONG j = 0; j < nodes_count; j++)
		{
			uvs_array[2 * nodes_count * i + 2 * j] = uv[3 * j];
			uvs_array[2 * nodes_count * i + 2 * j + 1] = uv[3 * j + 1];
		}
	}

	//in the same way read vertex colors
	XSI::CRefArray xsi_colors = xsi_acc.GetVertexColors();
	ULONG colors_count = options.is_export_colors ? xsi_colors.GetCount() : 0;
	std::vector<float> colors_array(colors_count * 4 * nodes_count);  // we will store 4 colors
	XSI::CFloatArray colors;
	for (ULONG i = 0; i < colors_count; i++)
	{
		XSI::ClusterProperty colors_prop(xsi_colors[i]);
		colors_prop.GetValues(colors);
		for (ULONG j = 0; j < nodes_count; j++)
		{
			colors_array[4 * nodes_count * i + 4 * j] = colors[4 * j];
			colors_array[4 * nodes_count * i + 4 * j + 1] = colors[4 * j + 1];
			colors_array[4 * nodes_count * i + 4 * j + 2] = colors[4 * j + 2];
			colors_array[4 * nodes_count * i + 4 * j + 3] = colors[4 * j + 3];
		}
	}

	//envelope weights
	XSI::CRefArray xsi_envelopes = xsi_acc.GetEnvelopeWeights();
	bool export_envelope = xsi_envelopes.GetCount() > 0 && options.is_export_skin;
	XSI::CFloatArray envelope_weights;
	ULONG envelope_deformers_count = 0;
	//we should export only the first envelope data
	if (export_envelope)
	{
		XSI::EnvelopeWeight envelope_data(xsi_envelopes[0]);
		envelope_data.GetValues(envelope_weights);  // weights are per-point, not per-sample, it contains weights for each point and all deformers (start from first point and all deformers, then for the second point and so on)
		//for export we sould write into each primitive weights and deformers and connect the skin index to the whole mesh
		//in mesh attributes we should simply write joint indexes in the skin property
		//so, we can write indexes from 0 to n and then in the skin propery write actual gltf indexes of the corresponding nodes
		//when we export envelope for the mesh we should remember, that this object requires skin export after we export all scene hierarchy
		envelope_deformers_count = envelope_data.GetDeformers().GetCount();
	}

	//shapes
	XSI::CRefArray xsi_shapes = xsi_acc.GetShapeKeys();
	ULONG shapes_count = options.is_export_shapes ? xsi_shapes.GetCount() : 0;
	std::vector<float> shapes_array(shapes_count * 3 * vertex_count);
	for (ULONG i = 0; i < shapes_count; i++)
	{
		XSI::ClusterProperty shape_prop(xsi_shapes[i]);
		XSI::CFloatArray shape_values;
		shape_prop.GetValues(shape_values);
		//copy values to the array
		for (ULONG j = 0; j < vertex_count; j++)
		{
			shapes_array[vertex_count * 3 * i + 3 * j] = shape_values[3 * j];
			shapes_array[vertex_count * 3 * i + 3 * j + 1] = shape_values[3 * j + 1];
			shapes_array[vertex_count * 3 * i + 3 * j + 2] = shape_values[3 * j + 2];
		}
	}

	//split the mesh into several submeshes
	//these arrays are the same size = the number of different materials
	//we will get actual gltf material index next by using materials_map
	std::vector<ULONG> submesh_materials;  // value - material object id for the sumbesh, here materials without repetitions
	std::vector<std::vector<ULONG>> submesh_triangles;  // store triangles, which form polygons with different materials (object id) into different arrays
	std::vector<ULONG> triangle_submesh(triangles_count); // store here the index of submesh for each triangle
	std::vector<ULONG> triangle_index_submesh(triangles_count);  // store here index of the triangle inside submesh (which is <= total mesh index)
	for (ULONG t = 0; t < triangles_count; t++)
	{
		LONG triangle_polygon = triangle_polygons[t]; // polygon index
		ULONG polygon_material_id = ((XSI::Material)(materials[polygon_materials[triangle_polygon]])).GetObjectID();

		LONG index = get_value_index(submesh_materials, polygon_material_id);
		if (index == -1)
		{//material is new
			triangle_submesh[t] = submesh_materials.size();
			triangle_index_submesh[t] = 0;
			submesh_materials.push_back(polygon_material_id);
			std::vector<ULONG> new_array(1);
			new_array[0] = t;
			submesh_triangles.push_back(new_array);
		}
		else
		{//add triangle to the index-s material list
			triangle_index_submesh[t] = submesh_triangles[index].size();
			triangle_submesh[t] = index;
			submesh_triangles[index].push_back(t);
		}
	}

	//for the mesh we should form at least three array:
	// 1. positions
	// 2. normals
	// 3. triangle indices
	//we identify defferent nodes as one vertex if positions and normals are coincide
	//if at least one data is different, then we setup these nodes as different vertices
	
	//we will save exported data into different buffers for different submeshes
	ULONG submesh_count = submesh_materials.size();
	std::vector<std::vector<Vertex>> all_export_vertices(submesh_count);
	std::vector<std::vector<unsigned int>> all_export_indices(submesh_count);
	for (ULONG i = 0; i < submesh_count; i++)
	{
		all_export_indices[i].resize(submesh_triangles[i].size() * 3);
	}

	std::vector<ULONG> submesh_weights_length(submesh_count, 0);  // store here the maximum length of the envelope data for each submesh
	std::vector<std::unordered_map<LONG, std::set<ULONG>>> all_vertex_to_Vertex_map(submesh_count);  // key - vertex index in the mesh, value - set of already exported Vertex indices (in export_vertices array)
	//use this map to check is created Vertex is new or one of previous Vertices
	for (ULONG t = 0; t < triangles_count; t++)
	{
		ULONG submesh_index = triangle_submesh[t];
		std::vector<Vertex>& export_vertices = all_export_vertices[submesh_index];
		std::vector<unsigned int>& export_indices = all_export_indices[submesh_index];
		std::unordered_map<LONG, std::set<ULONG>>& vertex_to_Vertex_map = all_vertex_to_Vertex_map[submesh_index];

		ULONG t_submesh = triangle_index_submesh[t];
		for (ULONG i = 0; i < 3; i++)
		{
			//get i-th angle of the t-th triangle
			LONG vertex_index = triangle_vertices[3 * t + i];
			LONG node_index = triangle_nodes[3 * t + i];

			//for the Vertex
			Vertex v;
			v.position = XSI::MATH::CVector3(vertex_positions[3*vertex_index], vertex_positions[3 * vertex_index + 1], vertex_positions[3 * vertex_index + 2]);
			v.normal = XSI::MATH::CVector3f(node_normals[3*node_index], node_normals[3 * node_index + 1], node_normals[3 * node_index + 2]);
			//copy all uvs
			v.uvs.resize(2 * uvs_count);
			for (ULONG uv_index = 0; uv_index < uvs_count; uv_index++)
			{
				v.uvs[2 * uv_index] = uvs_array[2 * nodes_count * uv_index + 2*node_index];
				v.uvs[2 * uv_index + 1] = uvs_array[2 * nodes_count * uv_index + 2 * node_index + 1];
			}
			//and colors
			v.colors.resize(4*colors_count);
			for (ULONG colors_index = 0; colors_index < colors_count; colors_index++)
			{
				v.colors[4 * colors_index] = colors_array[4 * nodes_count * colors_index + 4 * node_index];
				v.colors[4 * colors_index + 1] = colors_array[4 * nodes_count * colors_index + 4 * node_index + 1];
				v.colors[4 * colors_index + 2] = colors_array[4 * nodes_count * colors_index + 4 * node_index + 2];
				v.colors[4 * colors_index + 3] = colors_array[4 * nodes_count * colors_index + 4 * node_index + 3];
			}

			//envelope weights
			if (export_envelope && envelope_weights.GetCount() > 0 && envelope_deformers_count > 0)
			{
				//we should select the segment of actual weights for the given vertex
				ULONG length = 0;
				for (ULONG d = 0; d < envelope_deformers_count; d++)
				{
					float env_value = envelope_weights[envelope_deformers_count * vertex_index + d];
					if (env_value > EPSILON)
					{
						v.weights.push_back(env_value);
						v.joints.push_back(d);
						length++;
					}
				}
				if (submesh_weights_length[submesh_index] < length)
				{
					submesh_weights_length[submesh_index] = length;
				}
			}

			//shapes
			v.shapes.resize(3 * shapes_count, 0.0f);
			for (ULONG shape_index = 0; shape_index < shapes_count; shape_index ++)
			{
				v.shapes[3 * shape_index] = shapes_array[3 * vertex_count * shape_index + 3 * vertex_index];
				v.shapes[3 * shape_index + 1] = shapes_array[3 * vertex_count * shape_index + 3 * vertex_index + 1];
				v.shapes[3 * shape_index + 2] = shapes_array[3 * vertex_count * shape_index + 3 * vertex_index + 2];
			}

			//try to find index in already created vertices
			LONG known_index = -1;
			bool is_mesh_vertex_new = false;
			auto v_index_it = vertex_to_Vertex_map.find(vertex_index);
			if (v_index_it == vertex_to_Vertex_map.end())
			{//this is a new Vertex, because there are no Vertices from the same mesh vertex
				//nothing to do
				is_mesh_vertex_new = true;
			}
			else
			{
				known_index = get_vertex_index(v, export_vertices, v_index_it->second);
			}

			if (known_index == -1)
			{//this is a new Vertex
				known_index = export_vertices.size();
				export_vertices.push_back(v);

				if (is_mesh_vertex_new)
				{
					std::set<ULONG> new_set;
					new_set.insert(known_index);
					vertex_to_Vertex_map[vertex_index] = new_set;
				}
				else
				{
					v_index_it->second.insert(known_index);
				}
			}

			//ok, known_index is index of the current Vertex
			//use it for triangle index
			export_indices[3 * t_submesh + i] = known_index;
		}
	}

	//after we form all Vertices, we should align sizes of envelope data inside each submesh
	if (export_envelope)
	{
		for (ULONG submesh_index = 0; submesh_index < submesh_count; submesh_index++)
		{
			ULONG length = submesh_weights_length[submesh_index];
			if (length > 0)
			{
				//set the length x4
				length = 4 * ((length / 4) + (length % 4 == 0 ? 0 : 1));
				std::vector<Vertex>& vertices = all_export_vertices[submesh_index];
				for (ULONG v = 0; v < vertices.size(); v++)
				{
					vertices[v].weights.resize(length, 0.0f);
					vertices[v].joints.resize(length, 0);
				}
			}
		}
	}

	//next we should export each submesh as mesh primitive
	if (submesh_count > 0)
	{
		tinygltf::Mesh mesh;
		for (ULONG submesh_index = 0; submesh_index < submesh_count; submesh_index++)
		{
			//create primitive
			tinygltf::Primitive prim;
			prim.mode = TINYGLTF_MODE_TRIANGLES;

			//get submesh vertices
			std::vector<Vertex>& export_vertices = all_export_vertices[submesh_index];
			//and triangle indices
			const std::vector<unsigned int>& export_indices = all_export_indices[submesh_index];
			if (export_vertices.size() > 0 && export_indices.size() > 0)
			{
				int indices = add_triangle_indices_to_buffer(model, export_indices, TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT, TINYGLTF_TYPE_SCALAR);
				prim.indices = indices;
				write_mesh_data(model, prim, export_vertices);

				ULONG materil_id = submesh_materials[submesh_index];
				//try to find this material
				auto mat_it = materials_map.find(materil_id);
				if (mat_it != materials_map.end())
				{
					prim.material = mat_it->second;
				}

				//export shapes (targets)
				write_shapes_data(model, prim, export_vertices);

				mesh.primitives.push_back(prim);
			}
		}

		if (export_envelope)
		{
			//add skin to the mesh, but later we should setup this skin property
			tinygltf::Skin skin;
			node.skin = model.skins.size();
			model.skins.push_back(skin);

			envelope_meshes.push_back(xsi_object);
		}

		//also for the mesh write weights of shapes
		//TODO: try to obtain actual weights form shape manager (it use long and strange names)
		for (ULONG shape_index = 0; shape_index < shapes_count; shape_index++)
		{
			mesh.weights.push_back(0.0);
		}

		node.mesh = model.meshes.size();
		model.meshes.push_back(mesh);
	}

	all_export_indices.clear();
	all_export_indices.shrink_to_fit();
	all_export_vertices.clear();
	all_export_vertices.shrink_to_fit();
	all_vertex_to_Vertex_map.clear();
	all_vertex_to_Vertex_map.shrink_to_fit();
	triangle_index_submesh.clear();
	triangle_index_submesh.shrink_to_fit();
	triangle_submesh.clear();
	triangle_submesh.shrink_to_fit();
	submesh_triangles.clear();
	submesh_triangles.shrink_to_fit();
	submesh_materials.clear();
	submesh_materials.shrink_to_fit();
	colors_array.clear();
	colors_array.shrink_to_fit();
	uvs_array.clear();
	uvs_array.shrink_to_fit();
	submesh_weights_length.clear();
	submesh_weights_length.shrink_to_fit();
	shapes_array.clear();
	shapes_array.shrink_to_fit();
}