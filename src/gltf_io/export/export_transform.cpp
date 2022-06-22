#include <xsi_transformation.h>

#include "../gltf_io.h"
#include "../../utilities/utilities.h"

void export_transform(const XSI::MATH::CTransformation &xsi_tfm, tinygltf::Node &node)
{
	XSI::MATH::CVector3 position = xsi_tfm.GetTranslation();
	XSI::MATH::CQuaternion rotation = xsi_tfm.GetRotationQuaternion();
	XSI::MATH::CVector3 scale = xsi_tfm.GetScaling();

	node.translation = std::vector<double> {position.GetX(), position.GetY(), position.GetZ()};
	node.rotation = std::vector<double>{ rotation.GetX(), rotation.GetY(), rotation.GetZ(), rotation.GetW() };
	node.scale = std::vector<double>{ scale.GetX(), scale.GetY(), scale.GetZ() };
}