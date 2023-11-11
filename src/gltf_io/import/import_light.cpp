#include <xsi_x3dobject.h>
#include <xsi_kinematics.h>
#include <xsi_light.h>
#include <xsi_shader.h>
#include <xsi_color.h>
#include <xsi_color4f.h>

#include "../../tiny_gltf/tiny_gltf.h"

#include "../../utilities/utilities.h"
#include "../import.h"

XSI::X3DObject import_light(const tinygltf::Model& model, const tinygltf::Light& light, const XSI::CString& light_name, const XSI::MATH::CTransformation& light_tfm, XSI::X3DObject& parent_object)
{
	/* each light contains :
		name: string
		color: array of doubles
		intensity: double
		type: string (directional, point, spot)
			spot, point use position from transform
			spot, directional use local direction (0, 0, -1) from transform
		range (ignored): double, 0.0 (infinite) for directional, > 0.0 for point and spot

		additional paramters for the spot:
			innerConeAngle (ignored)
			outerConeAngle
		both in radians, from 0.0 to pi/2
	*/

	XSI::Light xsi_light;
	std::string light_type = light.type;
	XSI::CString light_preset = 
		light_type == "directional" ? "LightInfinite" : (
		light_type == "spot" ? "LightSpot" : "LightPoint");
	XSI::CStatus is_add = parent_object.AddLight(light_preset, false, light_name, xsi_light);
	if (is_add == XSI::CStatus::OK)
	{
		// transform for all types
		xsi_light.GetKinematics().GetLocal().PutTransform(light_tfm);

		// set cone angle for spot
		if (light_type == "spot")
		{
			tinygltf::SpotLight spot = light.spot;
			XSI::Parameter xsi_cone = xsi_light.GetParameter("LightCone");
			if (xsi_cone.IsValid())
			{
				// use only outer angle
				// inner angle does not supported in Softimage for default lights
				xsi_cone.PutValue(std::max(0.0, std::min(179.9, spot.outerConeAngle * 2.0 * 180.0 / XSI::MATH::PI)));
			}
		}

		// read shaders
		XSI::CRefArray xsi_shaders = xsi_light.GetShaders();
		// new created light contains only one shader in the list
		// check it
		XSI::Shader xsi_shader = xsi_shaders[0];
		XSI::CString xsi_shader_progid = xsi_shader.GetProgID();
		// shader should be Soft Light
		if (xsi_shader_progid == "Softimage.soft_light.1.0")
		{
			// get parameters
			XSI::Parameter xsi_color = xsi_shader.GetParameter("color");
			XSI::Parameter xsi_intensity = xsi_shader.GetParameter("intensity");

			// set these parameters
			XSI::Parameter xsi_color_red = xsi_color.GetParameter("red");
			XSI::Parameter xsi_color_green = xsi_color.GetParameter("green");
			XSI::Parameter xsi_color_blue = xsi_color.GetParameter("blue");

			if (light.color.size() >= 3)
			{
				xsi_color_red.PutValue(light.color[0]);
				xsi_color_green.PutValue(light.color[1]);
				xsi_color_blue.PutValue(light.color[2]);
			}

			xsi_intensity.PutValue(light.intensity);
		}
	}

	return xsi_light;
}