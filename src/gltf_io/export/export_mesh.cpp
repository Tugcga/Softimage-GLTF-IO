#include <xsi_x3dobject.h>
#include <xsi_primitive.h>
#include <xsi_geometryaccessor.h>
#include <xsi_geometry.h>
#include <xsi_polygonmesh.h>
#include <xsi_material.h>

#include "../gltf_io.h"
#include "../../utilities/utilities.h"

void export_mesh(tinygltf::Node &node, tinygltf::Model& model, XSI::X3DObject& xsi_object, const ExportOptions& options, std::unordered_map<ULONG, ULONG>& materials_map, std::unordered_map<ULONG, ULONG> &textures_map)
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

	XSI::CGeometryAccessor xsi_acc = xsi_mesh.GetGeometryAccessor(XSI::siConstructionModeSecondaryShape, XSI::siCatmullClark, subdivs, false, ga_use_angle, ga_angle);
	//TODO: try to find user normals

	//export all assigned materials
	XSI::CRefArray materials = xsi_acc.GetMaterials();
	for (ULONG i = 0; i < materials.GetCount(); i++)
	{
		XSI::Material material(materials[i]);
		export_material(model, material, options, materials_map, textures_map);
	}
}