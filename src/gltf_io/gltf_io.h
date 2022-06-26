#pragma once
#include <unordered_map>

#include <xsi_materiallibrary.h>
#include <xsi_material.h>
#include <xsi_imageclip2.h>
#include <xsi_scene.h>
#include <xsi_progressbar.h>
#include <xsi_transformation.h>
#include <xsi_camera.h>

#include "../tiny_gltf/tiny_gltf.h"

#include "import.h"
#include "export.h"

XSI::MATH::CTransformation import_transform(const tinygltf::Node& node);
XSI::X3DObject import_mesh(const tinygltf::Model& model, const tinygltf::Mesh& mesh, const XSI::CString &object_name, const XSI::MATH::CTransformation& object_tfm, XSI::X3DObject& parent_object, std::unordered_map<int, XSI::Material>& material_map, std::unordered_map<ULONG, std::vector<float>>& envelop_map, const ImportOptions &options);
void import_skin(const tinygltf::Model& model, XSI::X3DObject& xsi_object, const int skin_index, const std::unordered_map<ULONG, std::vector<float>>& envelope_data, std::unordered_map<ULONG, XSI::X3DObject>& nodes_map);
void import_images(const tinygltf::Model& model, const XSI::Scene &scene, const XSI::CString& scene_name, const XSI::CString& file_path, std::unordered_map<int, XSI::CString> &images_map, std::unordered_map<int, XSI::ImageClip2>& clips_map);
bool import_material(const tinygltf::Model& model, const tinygltf::Material& material, const int mat_index, XSI::MaterialLibrary& library, const std::unordered_map<int, XSI::CString>& image_map, const std::unordered_map<int, XSI::ImageClip2>& image_clips, std::unordered_map<int, XSI::Material> &material_map);
XSI::X3DObject import_camera(const tinygltf::Model& model, const tinygltf::Camera& camera, const XSI::CString& camera_name, const XSI::MATH::CTransformation& camera_tfm, XSI::X3DObject& parent_object);
void import_animation(XSI::ProgressBar& bar, const tinygltf::Model& model, const std::unordered_map<ULONG, XSI::X3DObject>& nodes_map, const ImportOptions& options);

XSI::MATH::CTransformation from_global_to_local(XSI::X3DObject& xsi_object, XSI::MATH::CTransformation& global_tfm);
void export_skin(tinygltf::Model& model, const ULONG skin_index, XSI::X3DObject& xsi_object, const std::unordered_map<ULONG, ULONG>& object_to_node);
void export_material(tinygltf::Model& model, XSI::Material& xsi_material, const ExportOptions& options, std::unordered_map<ULONG, ULONG>& materials_map, std::unordered_map<ULONG, ULONG> &textures_map);
void export_mesh(tinygltf::Node& node, tinygltf::Model& model, XSI::X3DObject& xsi_object, const ExportOptions& options, std::unordered_map<ULONG, ULONG>& materials_map, std::unordered_map<ULONG, ULONG> &textures_map, std::vector<XSI::X3DObject>& envelope_meshes);
tinygltf::Node export_node(XSI::X3DObject& xsi_object, bool& is_correct, const ExportOptions& options, std::unordered_map<ULONG, ULONG>& materials_map, std::unordered_map<ULONG, ULONG> &textures_map, std::vector<XSI::X3DObject>& envelope_meshes, std::unordered_map<ULONG, ULONG>& object_to_node, tinygltf::Model& model);
tinygltf::Node export_camera(const XSI::Camera& camera, bool& is_correct, tinygltf::Model& model);
void export_transform(const XSI::MATH::CTransformation& xsi_tfm, tinygltf::Node& node);