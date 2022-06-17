#include <unordered_map>

#include <xsi_materiallibrary.h>
#include <xsi_material.h>
#include <xsi_shader.h>
#include <xsi_shaderparameter.h>
#include <xsi_utils.h>
#include <xsi_project.h>
#include <xsi_scene.h>
#include <xsi_imageclip2.h>

#include "../../tiny_gltf/tiny_gltf.h"
#include "../../utilities/utilities.h"

#include "../../tiny_gltf/stb_image_write.h"

XSI::ImageClip2 get_clip(const XSI::CRefArray& all_clips, const XSI::CString& image_path)
{
	XSI::ImageClip2 to_return;
	for (ULONG i = 0; i < all_clips.GetCount(); i++)
	{
		XSI::ImageClip2 clip(all_clips[i]);
		if (image_path == clip.GetFileName())
		{
			to_return = clip;
			break;
		}
	}
	return to_return;
}

void import_images(const tinygltf::Model& model, const XSI::Scene& scene, const XSI::CString &scene_name, const XSI::CString& file_path, std::unordered_map<int, XSI::CString>& images_map, std::unordered_map<int, XSI::ImageClip2>& clips_map)
{
	XSI::CRefArray all_clips = scene.GetImageClips();

	ULONG images_count = model.images.size();
	for (ULONG i = 0; i < images_count; i++)
	{
		tinygltf::Image img = model.images[i];
		XSI::CString image_path = "";
		XSI::CString img_uri = img.uri.c_str();
		if (img_uri.Length() > 0)
		{//there is a file, simply combine gltf file path with this image file
			ULONG last_slash = file_path.ReverseFindString("\\");
			image_path = XSI::CUtils::BuildPath(file_path.GetSubString(0, last_slash), img_uri);
		}
		else
		{//file is embeded into gltf, so, we should save it as separate image file
			int image_width = img.width;
			int image_height = img.height;
			int image_components = img.component;
			int image_pixel_type = img.pixel_type;
			int image_bits = img.bits;
			XSI::Project xsi_project = XSI::Application().GetActiveProject();

			XSI::CString image_name = img.name.c_str();
			if (image_name.Length() == 0)
			{
				image_name = "texture_" + XSI::CString(i);
			}
			XSI::CString image_folder = XSI::CUtils::BuildPath(xsi_project.GetPath(), "Pictures", "gltf_io", scene_name) + "\\";
			create_dir(std::string(image_folder.GetAsciiString()));
			image_path = XSI::CUtils::BuildPath(image_folder, image_name + ".png");
			
			int out = stbi_write_png(image_path.GetAsciiString(), image_width, image_height, image_components, img.image.data(), image_width * image_components);
			if (out == 0)
			{//fails to write the image
				image_path = "";
			}
		}

		if (image_path.Length() > 0)
		{
			images_map[i] = image_path;

			//create clip and add it to the scene
			XSI::CValue value_clip;
			XSI::ImageClip2 pre_clip = get_clip(all_clips, image_path);
			if (pre_clip.IsValid())
			{
				value_clip = pre_clip;
			}
			else
			{
				XSI::CValueArray args(1);
				args[0] = XSI::CValue(image_path);
				XSI::Application().ExecuteCommand("CreateImageClip", args, value_clip);
			}
			XSI::ImageClip2 clip(value_clip);
			clips_map[i] = clip;
		}
	}
}

void update_texture_transform(const tinygltf::ExtensionMap &extensions, double &scale_u, double &scale_v)
{
	//try to find texture transform extension
	if (extensions.find("KHR_texture_transform") != extensions.end())
	{
		tinygltf::Value transform_value = extensions.at("KHR_texture_transform");
		if (transform_value.IsObject())
		{
			//get all object keys
			std::vector<std::string> tfm_keys = transform_value.Keys();
			for (ULONG key_index = 0; key_index < tfm_keys.size(); key_index++)
			{
				std::string key_name = tfm_keys[key_index];
				//we support only scale, because there are no natural map from other settings (rotation, offset) to built-in texture node parameters
				if (key_name == "scale")
				{
					tinygltf::Value scale_value = transform_value.Get(key_name);
					if (scale_value.IsArray() && scale_value.ArrayLen() == 2)
					{
						tinygltf::Value scale_u_value = scale_value.Get(0);
						tinygltf::Value scale_v_value = scale_value.Get(1);
						if (scale_u_value.IsReal() && scale_v_value.IsReal())
						{
							scale_u = scale_u_value.GetNumberAsDouble();
							scale_v = scale_v_value.GetNumberAsDouble();
						}
					}
				}
			}
		}
	}
}

void connect_texture(int image_index, XSI::Parameter& xsi_parameter, const std::unordered_map<int, XSI::ImageClip2>& clips_map, const tinygltf::ExtensionMap & extensions, const int tex_coord)
{
	XSI::CRef new_source;
	XSI::CRef prev_source;
	xsi_parameter.ConnectFromPreset("Image", XSI::siTextureShaderFamily, prev_source, new_source);

	XSI::Shader images_node = new_source;
	XSI::CStatus is_connect = xsi_parameter.Connect(images_node, prev_source);
	XSI::Parameter tex_param = images_node.GetParameter("tex");
	XSI::ImageClip2 clip = clips_map.at(image_index);
	is_connect = tex_param.Connect(clip, prev_source);

	XSI::CParameterRefArray image_params = images_node.GetParameters();
	//we does not support different uv channels
	//because in Softimage we should set the name of the uvs, but gltf file contains only uv index
	//different object can have different names of the same uv index.

	double scale_u = 1.0;
	double scale_v = 1.0;
	update_texture_transform(extensions, scale_u, scale_v);

	//set these values to the texture
	XSI::Parameter repeats_param = image_params.GetItem("repeats");
	repeats_param.PutParameterValue("x", scale_u);
	repeats_param.PutParameterValue("y", scale_v);
}

void connect_pbr_texture(XSI::Parameter &xsi_parameter, const tinygltf::Model& model, const tinygltf::TextureInfo &texture_info,  const std::unordered_map<int, XSI::CString>& images_map, const std::unordered_map<int, XSI::ImageClip2>& clips_map)
{
	int image_index = texture_info.index;
	if (image_index >= 0 && image_index < model.images.size() && images_map.find(image_index) != images_map.end())
	{
		connect_texture(image_index, xsi_parameter, clips_map, texture_info.extensions, texture_info.texCoord);
	}
}

void connect_texture_normal(XSI::Parameter& xsi_parameter, const tinygltf::Model& model, const tinygltf::NormalTextureInfo& texture_info, const std::unordered_map<int, XSI::CString>& images_map, const std::unordered_map<int, XSI::ImageClip2>& clips_map)
{
	int image_index = texture_info.index;
	if (image_index >= 0 && image_index < model.images.size() && images_map.find(image_index) != images_map.end())
	{
		connect_texture(image_index, xsi_parameter, clips_map, texture_info.extensions, texture_info.texCoord);
	}
}

void connect_texture_occlusion(XSI::Parameter& xsi_parameter, const tinygltf::Model& model, const tinygltf::OcclusionTextureInfo& texture_info, const std::unordered_map<int, XSI::CString>& images_map, const std::unordered_map<int, XSI::ImageClip2>& clips_map)
{
	int image_index = texture_info.index;
	if (image_index >= 0 && image_index < model.images.size() && images_map.find(image_index) != images_map.end())
	{
		connect_texture(image_index, xsi_parameter, clips_map, texture_info.extensions, texture_info.texCoord);
	}
}

bool import_material(const tinygltf::Model& model, const tinygltf::Material &material, const int mat_index, XSI::MaterialLibrary &library, const std::unordered_map<int, XSI::CString> &images_map, const std::unordered_map<int, XSI::ImageClip2>& clips_map, std::unordered_map<int, XSI::Material> &material_map)
{
	//create new material
	XSI::CString mat_name = material.name.c_str();
	if (mat_name.Length() == 0)
	{
		mat_name = "GLTF_" + XSI::CString(mat_index);
	}
	XSI::Material xsi_material = library.CreateMaterial("Phong", mat_name);

	//connect gltf shader node
	XSI::CParameterRefArray mat_params = xsi_material.GetParameters();
	XSI::Parameter surface_param = mat_params.GetItem("surface");
	
	XSI::ShaderParameter old_phong_param = surface_param.GetSource();
	XSI::Shader old_phong = old_phong_param.GetParent();  // this shader in the category NestedShaders

	XSI::CRef new_source;
	XSI::CRef prev_source;
	surface_param.ConnectFromProgID("GLTFShadersPlugin.MetallicRoughness.1.0", prev_source, new_source);
	XSI::Parameter shadow_param = mat_params.GetItem("shadow");
	XSI::Parameter photon_param = mat_params.GetItem("photon");
	shadow_param.Disconnect();
	photon_param.Disconnect();

	XSI::CValueArray args(2);
	args[0] = XSI::CValue(old_phong.GetFullName());
	args[1] = XSI::CValue(xsi_material.GetFullName());
	XSI::CValue io_value;
	XSI::Application().ExecuteCommand("DisconnectAndDeleteOrUnnestShaders", args, io_value);

	if (old_phong.IsValid())
	{
		//remove NestedShader part from the path
		XSI::CString old_phong_raw_path = old_phong.GetFullName();
		ULONG last_dot_position = old_phong_raw_path.ReverseFindString(".");
		ULONG prev_dot_position = old_phong_raw_path.ReverseFindString(".", last_dot_position - 1);
		XSI::CString old_phong_path = old_phong_raw_path.GetSubString(0, prev_dot_position) + old_phong_raw_path.GetSubString(last_dot_position);

		args[0] = XSI::CValue(old_phong_path);
		args[1] = XSI::CValue(xsi_material.GetFullName());
		XSI::Application().ExecuteCommand("DisconnectAndDeleteOrUnnestShaders", args, io_value);
	}

	XSI::Shader shader = new_source;
	
	XSI::CParameterRefArray xsi_params = shader.GetParameters();
	XSI::Parameter alphaMode_param = xsi_params.GetItem("alphaMode");
	XSI::Parameter alphaCutoff_param = xsi_params.GetItem("alphaCutoff");
	XSI::Parameter doubleSided_param = xsi_params.GetItem("doubleSided");
	XSI::Parameter metallicFactor_param = xsi_params.GetItem("metallicFactor");
	XSI::Parameter roughnessFactor_param = xsi_params.GetItem("roughnessFactor");
	XSI::Parameter scale_param = xsi_params.GetItem("scale");
	XSI::Parameter strength_param = xsi_params.GetItem("strength");

	alphaMode_param.PutValue(XSI::CString(material.alphaMode.c_str()));
	alphaCutoff_param.PutValue(material.alphaCutoff);
	
	doubleSided_param.PutValue(material.doubleSided);
	metallicFactor_param.PutValue(material.pbrMetallicRoughness.metallicFactor);
	roughnessFactor_param.PutValue(material.pbrMetallicRoughness.roughnessFactor);
	scale_param.PutValue(material.normalTexture.scale);
	strength_param.PutValue(material.occlusionTexture.strength);

	XSI::Parameter baseColorFactor_param = xsi_params.GetItem("baseColorFactor");
	XSI::Parameter emissiveFactor_param = xsi_params.GetItem("emissiveFactor");

	baseColorFactor_param.PutParameterValue("red", material.pbrMetallicRoughness.baseColorFactor[0]);
	baseColorFactor_param.PutParameterValue("green", material.pbrMetallicRoughness.baseColorFactor[1]);
	baseColorFactor_param.PutParameterValue("blue", material.pbrMetallicRoughness.baseColorFactor[2]);
	baseColorFactor_param.PutParameterValue("alpha", material.pbrMetallicRoughness.baseColorFactor[3]);

	emissiveFactor_param.PutParameterValue("red", material.emissiveFactor[0]);
	emissiveFactor_param.PutParameterValue("green", material.emissiveFactor[1]);
	emissiveFactor_param.PutParameterValue("blue", material.emissiveFactor[2]);

	//next connect images
	//get image intex for the texture
	//if it equal to -1, then this texture is not connected
	XSI::Parameter baseColorTexture_param = xsi_params.GetItem("baseColorTexture");
	XSI::Parameter metallicRoughnessTexture_param = xsi_params.GetItem("metallicRoughnessTexture");
	XSI::Parameter normalTexture_param = xsi_params.GetItem("normalTexture");
	XSI::Parameter occlusionTexture_param = xsi_params.GetItem("occlusionTexture");
	XSI::Parameter emissiveTexture_param = xsi_params.GetItem("emissiveTexture");
	connect_pbr_texture(baseColorTexture_param, model, material.pbrMetallicRoughness.baseColorTexture, images_map, clips_map);
	connect_pbr_texture(metallicRoughnessTexture_param, model, material.pbrMetallicRoughness.metallicRoughnessTexture, images_map, clips_map);
	connect_texture_normal(normalTexture_param, model, material.normalTexture, images_map, clips_map);
	connect_texture_occlusion(occlusionTexture_param, model, material.occlusionTexture, images_map, clips_map);
	connect_pbr_texture(emissiveTexture_param, model, material.emissiveTexture, images_map, clips_map);

	material_map[mat_index] = xsi_material;

	return true;
}