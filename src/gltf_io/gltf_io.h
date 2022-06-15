#pragma once

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../tiny_gltf/tiny_gltf.h"

#include "import.h"

XSI::MATH::CTransformation import_transform(const tinygltf::Node& node);
XSI::X3DObject import_mesh(const tinygltf::Model& model, const tinygltf::Mesh& mesh, const XSI::CString &object_name, const XSI::MATH::CTransformation& object_tfm, XSI::X3DObject& parent_object, const ImportMeshOptions &options);