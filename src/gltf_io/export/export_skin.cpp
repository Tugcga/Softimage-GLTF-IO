#include <xsi_x3dobject.h>
#include <xsi_primitive.h>
#include <xsi_geometryaccessor.h>
#include <xsi_geometry.h>
#include <xsi_polygonmesh.h>
#include <xsi_envelopeweight.h>
#include <xsi_kinematics.h>
#include <xsi_statickinematicstate.h>

#include "../gltf_io.h"
#include "../export.h"
#include "../../utilities/utilities.h"

XSI::X3DObject get_nondeform_parent(const XSI::X3DObject &object)
{
	XSI::X3DObject parent = object.GetParent();
	if (parent.GetObjectID() == object.GetObjectID())
	{//parent is the same, retun it
		return object;
	}

	XSI::StaticKinematicState parent_kine = parent.GetStaticKinematicState();
	if (parent_kine.IsValid())
	{
		return get_nondeform_parent(parent);
	}
	else
	{
		return parent;
	}
}

void export_skin(tinygltf::Model &model, const ULONG skin_index, XSI::X3DObject &xsi_object, const std::unordered_map<ULONG, ULONG> &object_to_node)
{
	if (skin_index < model.skins.size())
	{
		//XSI::MATH::CMatrix4 xsi_object_matrix = xsi_object.GetKinematics().GetGlobal().GetTransform().GetMatrix4();
		XSI::MATH::CMatrix4 xsi_object_matrix = xsi_object.GetStaticKinematicState().GetTransform().GetMatrix4();

		XSI::PolygonMesh xsi_mesh = xsi_object.GetActivePrimitive().GetGeometry();
		XSI::CGeometryAccessor xsi_acc = xsi_mesh.GetGeometryAccessor();

		XSI::CRefArray xsi_envelopes = xsi_acc.GetEnvelopeWeights();
		if (xsi_envelopes.GetCount() > 0)
		{
			XSI::EnvelopeWeight envelope_data(xsi_envelopes[0]);
			XSI::CRefArray deformers = envelope_data.GetDeformers();
			ULONG deformers_count = deformers.GetCount();
			if (deformers_count > 0)
			{
				std::vector<float> bind_matrices(deformers_count * 16, 0.0f);
				std::vector<int>& joints = model.skins[skin_index].joints;
				for (ULONG d = 0; d < deformers_count; d++)
				{
					XSI::X3DObject deformer(deformers[d]);
					XSI::MATH::CMatrix4 deform_matrix = deformer.GetStaticKinematicState().GetTransform().GetMatrix4();  // this is global transform
					deform_matrix.InvertInPlace();

					//multiply object matrix with deform inverse matrix
					XSI::MATH::CMatrix4 m;
					m.Mul(xsi_object_matrix, deform_matrix);

					ULONG deformer_id = deformer.GetObjectID();
					auto def_it = object_to_node.find(deformer_id);
					if (def_it != object_to_node.end())
					{
						joints.push_back(def_it->second);

						//next we should write this matrix into float array
						for (ULONG i = 0; i < 4; i++)
						{
							for (ULONG j = 0; j < 4; j++)
							{
								bind_matrices[16 * d + 4 * i + j] = m.GetValue(i, j);
							}
						}
					}
					else
					{
						log_message("Object " + deformer.GetName() + " is skin deformer, but this object is not selected for export. This will produce incorrect skinning data", XSI::siWarningMsg);
					}
				}
				model.skins[skin_index].inverseBindMatrices = add_float_to_buffer(model, bind_matrices, true, TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_TYPE_MAT4);
				//as sceleton root find the first object with invalid static kine for the first deformer
				XSI::X3DObject xsi_scel = get_nondeform_parent(deformers[0]);
				ULONG xsi_scel_id = xsi_scel.GetObjectID();
				auto scel_it = object_to_node.find(xsi_scel_id);
				if (scel_it != object_to_node.end())
				{
					model.skins[skin_index].skeleton = scel_it->second;
				}
			}
		}
		else
		{
			log_message("Exporter to GLTF/GLB try to export skin, but object " + xsi_object.GetName() + " does not contain envelope data", XSI::siWarningMsg);
		}
	}	
}