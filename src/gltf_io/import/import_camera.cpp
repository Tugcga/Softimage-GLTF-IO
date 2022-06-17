#include <xsi_x3dobject.h>
#include <xsi_camera.h>
#include <xsi_kinematics.h>

#include "../../tiny_gltf/tiny_gltf.h"

#include "../../utilities/utilities.h"
#include "../import.h"

XSI::X3DObject import_camera(const tinygltf::Model& model, const tinygltf::Camera& camera, const XSI::CString& camera_name, const XSI::MATH::CTransformation& camera_tfm, XSI::X3DObject& parent_object)
{
	XSI::Camera xsi_camera;
	XSI::CStatus is_create = parent_object.AddCamera("Camera", camera_name, xsi_camera);
	if (is_create == XSI::CStatus::OK)
	{
		XSI::X3DObject interest = xsi_camera.GetInterest();

		XSI::MATH::CMatrix4 matrix = camera_tfm.GetMatrix4();
		//get third row
		double direction_x = matrix.GetValue(2, 0);
		double direction_y = matrix.GetValue(2, 1);
		double direction_z = matrix.GetValue(2, 2);
		//these values define local z-axis, iterest should be positions along negative z-axis

		double interest_distance = 5.0;
		double pos_x = camera_tfm.GetPosX();
		double pos_y = camera_tfm.GetPosY();
		double pos_z = camera_tfm.GetPosZ();

		//next set camera transform
		xsi_camera.GetKinematics().GetLocal().PutTransform(camera_tfm);

		XSI::MATH::CTransformation interest_tfm;
		interest_tfm.SetIdentity();

		interest_tfm.SetTranslationFromValues(
			pos_x - interest_distance * direction_x,
			pos_y - interest_distance * direction_y,
			pos_z - interest_distance * direction_z);

		interest.GetKinematics().GetLocal().PutTransform(interest_tfm);

		XSI::CParameterRefArray camera_params = xsi_camera.GetParameters();

		std::string camera_type = camera.type;
		if (camera_type == "orthographic")
		{
			XSI::Parameter proj_param = camera_params.GetItem("proj");
			proj_param.PutValue(0);

			XSI::Parameter fovtype_param = camera_params.GetItem("fovtype");
			fovtype_param.PutValue(1);  // horisontal orthographic

			XSI::Parameter aspect_param = camera_params.GetItem("aspect");
			aspect_param.PutValue(camera.orthographic.xmag / camera.orthographic.ymag);

			XSI::Parameter orthoheight_param = camera_params.GetItem("orthoheight");
			orthoheight_param.PutValue(camera.orthographic.ymag * 2);

			XSI::Parameter near_param = camera_params.GetItem("near");
			near_param.PutValue(camera.orthographic.znear);

			XSI::Parameter far_param = camera_params.GetItem("far");
			far_param.PutValue(camera.orthographic.zfar);
		}
		else
		{//perspective
			XSI::Parameter proj_param = camera_params.GetItem("proj");
			proj_param.PutValue(1);

			XSI::Parameter fovtype_param = camera_params.GetItem("fovtype");
			fovtype_param.PutValue(0);

			XSI::Parameter fov_param = camera_params.GetItem("fov");
			fov_param.PutValue(camera.perspective.yfov * 180.0 / 3.1415);

			XSI::Parameter aspect_param = camera_params.GetItem("aspect");
			aspect_param.PutValue(camera.perspective.aspectRatio);

			XSI::Parameter near_param = camera_params.GetItem("near");
			near_param.PutValue(camera.perspective.znear);

			XSI::Parameter far_param = camera_params.GetItem("far");
			far_param.PutValue(camera.perspective.zfar);
		}

		XSI::Parameter pixelratio_param = camera_params.GetItem("pixelratio");
		pixelratio_param.PutValue(1.0);
	}

	return xsi_camera;
}