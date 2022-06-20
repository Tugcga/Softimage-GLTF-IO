#pragma once
#include <unordered_map>

#include <xsi_materiallibrary.h>
#include <xsi_material.h>
#include <xsi_imageclip2.h>
#include <xsi_scene.h>

#include "../tiny_gltf/tiny_gltf.h"

#include "import.h"

XSI::MATH::CTransformation import_transform(const tinygltf::Node& node);
XSI::X3DObject import_mesh(const tinygltf::Model& model, const tinygltf::Mesh& mesh, const XSI::CString &object_name, const XSI::MATH::CTransformation& object_tfm, XSI::X3DObject& parent_object, const std::unordered_map<int, XSI::Material>& material_map, const ImportMeshOptions &options);
void import_images(const tinygltf::Model& model, const XSI::Scene &scene, const XSI::CString& scene_name, const XSI::CString& file_path, std::unordered_map<int, XSI::CString> &images_map, std::unordered_map<int, XSI::ImageClip2>& clips_map);
bool import_material(const tinygltf::Model& model, const tinygltf::Material& material, const int mat_index, XSI::MaterialLibrary& library, const std::unordered_map<int, XSI::CString>& image_map, const std::unordered_map<int, XSI::ImageClip2>& image_clips, std::unordered_map<int, XSI::Material> &material_map);
XSI::X3DObject import_camera(const tinygltf::Model& model, const tinygltf::Camera& camera, const XSI::CString& camera_name, const XSI::MATH::CTransformation& camera_tfm, XSI::X3DObject& parent_object);
void import_animation(const tinygltf::Model& model, const std::unordered_map<ULONG, XSI::X3DObject>& nodes_map);