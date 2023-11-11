#include <xsi_light.h>
#include <xsi_kinematics.h>
#include <xsi_kinematicstate.h>
#include <xsi_property.h>
#include <xsi_shader.h>

#include "../gltf_io.h"
#include "../../utilities/utilities.h"

tinygltf::Node export_light(const XSI::Light& xsi_light, bool& is_correct, tinygltf::Model& model)
{
	tinygltf::Node new_node;

	// name
	new_node.name = xsi_light.GetName().GetAsciiString();

	// transform
	XSI::MATH::CTransformation xsi_tfm = xsi_light.GetKinematics().GetLocal().GetTransform();
	export_transform(xsi_tfm, new_node);

	tinygltf::Light light;
	// use the same name
	light.name = xsi_light.GetName().GetAsciiString();
	// type
	int xsi_type = xsi_light.GetParameter("Type").GetValue();
	light.type = 
		xsi_type == 1 ? "directional" : (
		xsi_type == 2 ? "spot" : "point");

	// for spot add cone angles
	if (xsi_type == 2)
	{
		XSI::Parameter xsi_cone = xsi_light.GetParameter("LightCone");
		if (xsi_cone.IsValid())
		{
			double xsi_cone_value = xsi_cone.GetValue();  // in degrees
			tinygltf::SpotLight spot;
			spot.outerConeAngle = xsi_cone_value * XSI::MATH::PI / (180.0 * 2.0);
			// set inner angle to half of outer angle
			// because inner angle is not supported by Softimage default spot light source
			spot.innerConeAngle = spot.outerConeAngle / 2.0;

			light.spot = spot;
		}
	}

	// read light shaders
	XSI::CRefArray xsi_shaders = xsi_light.GetShaders();
	// we supports only Soft Light shader on the first level of shader tree
	size_t shader_index = 0;
	while (shader_index < xsi_shaders.GetCount())
	{
		XSI::Shader xsi_shader = xsi_shaders[shader_index];
		XSI::CString xsi_shader_pro_id = xsi_shader.GetProgID();
		if (xsi_shader_pro_id == "Softimage.soft_light.1.0")
		{
			// find Soft Light shader
			// skip other nodes
			shader_index = xsi_shaders.GetCount();

			// read intensity and color parameters
			XSI::Parameter xsi_intensity = xsi_shader.GetParameter("intensity");
			XSI::Parameter xsi_color = xsi_shader.GetParameter("color");

			light.intensity = xsi_intensity.GetValue();

			light.color = std::vector<double>{ 
				xsi_color.GetParameter("red").GetValue(), 
				xsi_color.GetParameter("green").GetValue(), 
				xsi_color.GetParameter("blue").GetValue()};
		}

		shader_index++;
	}

	// setup in the model
	new_node.light = model.lights.size();
	model.lights.push_back(light);

	is_correct = true;
	return new_node;
}