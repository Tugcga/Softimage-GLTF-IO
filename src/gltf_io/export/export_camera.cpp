#include <xsi_camera.h>
#include <xsi_kinematics.h>
#include <xsi_kinematicstate.h>

#include "../gltf_io.h"
#include "../../utilities/utilities.h"

tinygltf::Node export_camera(const XSI::Camera& xsi_camera, bool& is_correct, tinygltf::Model &model)
{
	tinygltf::Node new_node;

	//set the name
	new_node.name = xsi_camera.GetName().GetAsciiString();

	//export transform
	XSI::MATH::CTransformation xsi_tfm = xsi_camera.GetKinematics().GetLocal().GetTransform();
	export_transform(xsi_tfm, new_node);

	XSI::CParameterRefArray camera_params = xsi_camera.GetParameters();

	tinygltf::Camera camera;
	XSI::Parameter proj_param = camera_params.GetItem("proj");
	int proj_value = proj_param.GetValue();

	XSI::Parameter aspect_param = camera_params.GetItem("aspect");
	float aspect_value = aspect_param.GetValue();

	XSI::Parameter near_param = camera_params.GetItem("near");
	float near_value = near_param.GetValue();

	XSI::Parameter far_param = camera_params.GetItem("far");
	float far_value = far_param.GetValue();

	if (proj_value == 0)
	{//orthographic camera
		camera.type = "orthographic";
		tinygltf::OrthographicCamera ortho_camera;

		XSI::Parameter orthoheight_param = camera_params.GetItem("orthoheight");
		float orthoheight_value = orthoheight_param.GetValue();
		ortho_camera.ymag = orthoheight_value / 2.0;
		ortho_camera.xmag = aspect_value * ortho_camera.ymag;

		ortho_camera.znear = near_value;
		ortho_camera.zfar = far_value;

		camera.orthographic = ortho_camera;
	}
	else
	{//perspective camera
		camera.type = "perspective";
		tinygltf::PerspectiveCamera persp_camera;

		XSI::Parameter fovtype_param = camera_params.GetItem("fovtype");
		int fovtype_value = fovtype_param.GetValue();

		XSI::Parameter fov_param = camera_params.GetItem("fov");
		float fov_value = fov_param.GetValue();  // in degrees, we should save it in radians
		if (fovtype_value == 0)
		{//vertical fov
			//in gltf camera use vertical fov
			persp_camera.yfov = fov_value * M_PI / 180.0f;
		}
		else
		{//horisontal fov
			//we should contart it to vertical fov
			float vert_fov = 2.0f * atan(tan(fov_value * M_PI / (2.0 * 180.0)) / aspect_value);
			persp_camera.yfov = vert_fov;
		}
		persp_camera.aspectRatio = aspect_value;

		persp_camera.znear = near_value;
		persp_camera.zfar = far_value;

		camera.perspective = persp_camera;
	}

	new_node.camera = model.cameras.size();
	model.cameras.push_back(camera);

	is_correct = true;
	return new_node;
}