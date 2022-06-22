#include <experimental/filesystem>

#include <xsi_material.h>
#include <xsi_shaderparameter.h>
#include <xsi_shader.h>
#include <xsi_imageclip2.h>
#include <xsi_utils.h>

#include "../gltf_io.h"
#include "../../utilities/utilities.h"

#include "../../tiny_gltf/stb_image.h"
#include "../../tiny_gltf/tiny_gltf.h"

std::vector<XSI::ShaderParameter> get_root_shader_parameter(const XSI::CRefArray& first_level_shaders, const XSI::CString& root_parameter_name)
{
	std::vector<XSI::ShaderParameter> to_return;
	for (ULONG i = 0; i < first_level_shaders.GetCount(); i++)
	{
		XSI::Shader shader(first_level_shaders[i]);
		XSI::CRefArray shader_params = XSI::CRefArray(shader.GetParameters());
		for (ULONG j = 0; j < shader_params.GetCount(); j++)
		{
			XSI::Parameter parameter(shader_params.GetItem(j));
			XSI::CString parameter_name = parameter.GetName();
			bool is_input;
			XSI::siShaderParameterType param_type = shader.GetShaderParameterType(parameter_name, is_input);
			if (!is_input)
			{
				//this is output shader parameter
				XSI::CRefArray targets = shader.GetShaderParameterTargets(parameter_name);
				for (LONG k = 0; k < targets.GetCount(); k++)
				{
					XSI::ShaderParameter p = targets.GetItem(k);
					if (p.GetName() == root_parameter_name)
					{
						to_return.push_back(p);
					}
				}
			}
		}
	}
	return to_return;
}

//return index in the textures array (-1, if the port does not connected to any texture)
int export_texture(tinygltf::Model& model, const ExportOptions& options, const XSI::ShaderParameter &texture_port, std::unordered_map<ULONG, ULONG> &textures_map)
{
	const auto copy_options = std::experimental::filesystem::copy_options::overwrite_existing;
	//get the source of the port
	XSI::CRef source = texture_port.GetSource();
	if (source.IsValid())
	{//there is a connection
		XSI::ShaderParameter source_param(source);
		XSI::Shader shader(source_param.GetParent());

		if (shader.IsValid())
		{//port connected to the shader node
			XSI::CString prog_id = shader.GetProgID();
			if (prog_id == "Softimage.txt2d-image-explicit.1.0")
			{
				XSI::CParameterRefArray parameters = shader.GetParameters();
				XSI::Parameter tex_param = parameters.GetItem("tex");
				XSI::CRef tex_source = tex_param.GetSource();
				if (tex_source.IsValid())
				{
					XSI::ImageClip2 clip(tex_source);
					ULONG clip_id = clip.GetObjectID();

					auto clip_it = textures_map.find(clip_id);
					if (clip_it == textures_map.end())
					{//the clip is new
						XSI::CString file_path = clip.GetFileName();
						XSI::CString file_name = full_file_name_from_path(file_path);
						std::string ext = get_file_extension(file_name.GetAsciiString());
						if (is_extension_supported(ext))
						{
							tinygltf::Texture texture;
							texture.name = clip.GetName().GetAsciiString();

							int width, height, components;
							unsigned char *data = stbi_load(file_path.GetAsciiString(), &width, &height, &components, 0);

							tinygltf::Image image;
							//copy file from original location to the directory with output file
							if(options.embed_images)
							{ 
								image.uri = file_path.GetAsciiString();
								image.width = width;
								image.height = height;
								image.component = components;
								image.bits = 8;
								image.pixel_type = TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE;
								image.image.assign(data, data + width * height * components);
							}
							else
							{
								std::experimental::filesystem::copy_file(file_path.GetAsciiString(), XSI::CUtils::BuildPath(options.output_path, file_name).GetAsciiString(), copy_options);
								image.uri = file_name.GetAsciiString();
							}

							texture.source = model.images.size();
							model.images.push_back(image);

							//save texture to the model
							textures_map[clip_id] = model.textures.size();
							model.textures.push_back(texture);

							return model.textures.size() - 1;
						}
						else
						{
							log_message("Extension of the texture " + file_name + " is not png or jpg. These files are not supported. Skip it", XSI::siWarningMsg);
							return -1;
						}
					}
					else
					{
						return clip_it->second;
					}
				}
			}
		}
	}

	return -1;
}

void export_material(tinygltf::Model& model, XSI::Material& xsi_material, const ExportOptions& options, std::unordered_map<ULONG, ULONG>& materials_map, std::unordered_map<ULONG, ULONG> &textures_map)
{
	//may be this material already exported
	ULONG material_id = xsi_material.GetObjectID();
	auto m_it = materials_map.find(material_id);
	if (m_it == materials_map.end())
	{//this material is new
		XSI::CRefArray first_level_shaders = xsi_material.GetShaders();
		std::vector<XSI::ShaderParameter> surface_ports = get_root_shader_parameter(first_level_shaders, "surface");
		if (surface_ports.size() > 0)
		{
			XSI::CRef source = surface_ports[0].GetSource();
			if (source.IsValid())
			{
				XSI::ShaderParameter source_param(source);
				XSI::Shader shader(source_param.GetParent());

				if (shader.IsValid())
				{
					XSI::CString prog_id = shader.GetProgID();
					if (prog_id == "GLTFShadersPlugin.MetallicRoughness.1.0")
					{
						//this is a supported material, so, we can export it
						tinygltf::Material material;
						material.name = xsi_material.GetName().GetAsciiString();

						//at first we should read constant values of the shader node
						XSI::CParameterRefArray xsi_params = shader.GetParameters();
						XSI::Parameter alphaMode_param = xsi_params.GetItem("alphaMode");
						XSI::Parameter alphaCutoff_param = xsi_params.GetItem("alphaCutoff");
						XSI::Parameter doubleSided_param = xsi_params.GetItem("doubleSided");
						XSI::Parameter baseColorFactor_param = xsi_params.GetItem("baseColorFactor");
						XSI::Parameter metallicFactor_param = xsi_params.GetItem("metallicFactor");
						XSI::Parameter roughnessFactor_param = xsi_params.GetItem("roughnessFactor");
						XSI::Parameter scale_param = xsi_params.GetItem("scale");
						XSI::Parameter strength_param = xsi_params.GetItem("strength");
						XSI::Parameter emissiveFactor_param = xsi_params.GetItem("emissiveFactor");

						XSI::CString alphaMode_value = alphaMode_param.GetValue();
						material.alphaMode = alphaMode_value.GetAsciiString();

						float alphaCutoff_value = alphaCutoff_param.GetValue();
						material.alphaCutoff = alphaCutoff_value;

						bool doubleSided_value = doubleSided_param.GetValue();
						material.doubleSided = doubleSided_value;

						tinygltf::PbrMetallicRoughness pbr_mr;
						XSI::CParameterRefArray baseColorFactor_array = baseColorFactor_param.GetParameters();
						XSI::Parameter p[4];
						for (ULONG s = 0; s < 4; s++)
						{
							p[s] = XSI::Parameter(baseColorFactor_array[s]);
						}
						pbr_mr.baseColorFactor = std::vector<double>{(float)p[0].GetValue(), (float)p[1].GetValue() , (float)p[2].GetValue() , (float)p[3].GetValue() };

						float metallicFactor_value = metallicFactor_param.GetValue();
						pbr_mr.metallicFactor = metallicFactor_value;

						float roughnessFactor_value = roughnessFactor_param.GetValue();
						pbr_mr.roughnessFactor = roughnessFactor_value;

						tinygltf::NormalTextureInfo normal_texture;
						float scale_value = scale_param.GetValue();
						normal_texture.scale = scale_value;

						tinygltf::OcclusionTextureInfo occlusion_texture;
						float strength_value = strength_param.GetValue();
						occlusion_texture.strength = strength_value;

						XSI::CParameterRefArray emissiveFactor_array = emissiveFactor_param.GetParameters();
						for (ULONG s = 0; s < 3; s++)
						{
							p[s] = XSI::Parameter(emissiveFactor_array[s]);
						}
						material.emissiveFactor = std::vector<double>{ (float)p[0].GetValue(), (float)p[1].GetValue() , (float)p[2].GetValue() };

						//next textures
						tinygltf::TextureInfo base_color_texture;
						tinygltf::TextureInfo metallic_roughness_texture;
						tinygltf::TextureInfo emissive_texture;

						int base_color_index = export_texture(model, options, xsi_params.GetItem("baseColorTexture"), textures_map);
						if (base_color_index >= 0)
						{
							base_color_texture.index = base_color_index;
						}

						int metallic_roughness_index = export_texture(model, options, xsi_params.GetItem("metallicRoughnessTexture"), textures_map);
						if (metallic_roughness_index >= 0)
						{
							metallic_roughness_texture.index = metallic_roughness_index;
						}

						int emissive_index = export_texture(model, options, xsi_params.GetItem("emissiveTexture"), textures_map);
						if (emissive_index >= 0)
						{
							emissive_texture.index = emissive_index;
						}

						int normal_index = export_texture(model, options, xsi_params.GetItem("normalTexture"), textures_map);
						if (normal_index >= 0)
						{
							normal_texture.index = normal_index;
						}

						int occlusion_index = export_texture(model, options, xsi_params.GetItem("occlusionTexture"), textures_map);
						if (occlusion_index >= 0)
						{
							occlusion_texture.index = occlusion_index;
						}

						//set gltf material properties
						pbr_mr.baseColorTexture = base_color_texture;
						pbr_mr.metallicRoughnessTexture = metallic_roughness_texture;
						material.pbrMetallicRoughness = pbr_mr;
						material.normalTexture = normal_texture;
						material.occlusionTexture = occlusion_texture;
						material.emissiveTexture = emissive_texture;

						materials_map[material_id] = model.materials.size();
						model.materials.push_back(material);
					}
				}
			}
		}
	}
}