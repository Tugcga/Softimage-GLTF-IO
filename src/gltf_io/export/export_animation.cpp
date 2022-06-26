#include <xsi_transformation.h>
#include <xsi_x3dobject.h>
#include <xsi_projectitem.h>
#include <xsi_kinematics.h>
#include <xsi_parameter.h>

#include "../gltf_io.h"
#include "../../utilities/utilities.h"

void write_parameters(tinygltf::Model &model, 
	tinygltf::Animation &animation,
	const int node_index,
	const int time_index, 
	std::string target_str,
	const XSI::Parameter &px, 
	const XSI::Parameter& py,
	const XSI::Parameter& pz,
	const int animation_steps, 
	const int animation_start, 
	const int animation_end)
{
	if (px.IsAnimated() || py.IsAnimated() || pz.IsAnimated())
	{
		const int components = target_str == "rotation" ? 4 : 3;
		std::vector<float> values(components * animation_steps);
		std::vector<double> min_values(components, DBL_MAX);
		std::vector<double> max_values(components, DBL_MIN);

		for (int f = animation_start; f < animation_end + 1; f++)
		{
			int p = f - animation_start;
			float x = px.GetValue(f);
			float y = py.GetValue(f);
			float z = pz.GetValue(f);
			float w = 0;
			if (components == 4)
			{
				XSI::MATH::CQuaternion q;
				q.SetFromXYZAnglesValues(x * M_PI / 180.0f, y * M_PI / 180.0f, z * M_PI / 180.0f);

				x = q.GetX();
				y = q.GetY();
				z = q.GetZ();
				w = q.GetW();
			}

			values[components * p] = x;
			values[components * p + 1] = y;
			values[components * p + 2] = z;

			if (x < min_values[0]) { min_values[0] = x; }
			if (x > max_values[0]) { max_values[0] = x; }

			if (y < min_values[1]) { min_values[1] = y; }
			if (y > max_values[1]) { max_values[1] = y; }

			if (z < min_values[2]) { min_values[2] = z; }
			if (z > max_values[2]) { max_values[2] = z; }

			//for quaternion also save the 4-th value and mmin, max values
			if (components == 4)
			{
				values[components * p + 3] = w;
				if (w < min_values[3]) { min_values[3] = w; }
				if (w > max_values[3]) { max_values[3] = w; }
			}
		}
		//write to the buffer
		const unsigned char* positions_bytes = reinterpret_cast<const unsigned char*>(&values[0]);
		std::vector<unsigned char> positions_byte_vector(positions_bytes, positions_bytes + sizeof(float) * values.size());

		int pos_index = add_data_to_buffer(model, positions_byte_vector, animation_steps, false, true, TINYGLTF_COMPONENT_TYPE_FLOAT, components == 3 ? TINYGLTF_TYPE_VEC3 : TINYGLTF_TYPE_VEC4, min_values, max_values);

		values.clear();
		values.shrink_to_fit();
		min_values.clear();
		min_values.shrink_to_fit();
		max_values.clear();
		max_values.shrink_to_fit();

		//next create sampler
		tinygltf::AnimationSampler sampler;
		sampler.interpolation = "LINEAR";
		sampler.input = time_index;
		sampler.output = pos_index;

		int sampler_index = animation.samplers.size();
		animation.samplers.push_back(sampler);

		//create a channel
		tinygltf::AnimationChannel channel;
		channel.sampler = sampler_index;
		channel.target_node = node_index;
		channel.target_path = target_str;

		animation.channels.push_back(channel);
	}
}

void export_animation(tinygltf::Model &model, const ExportOptions &options, const std::unordered_map<ULONG, ULONG> &object_to_node)
{
	//create array of times
	float step_time = 1.0f / options.animation_frames_per_second;
	int animation_steps = options.animation_end - options.animation_start + 1;
	std::vector<float> times(animation_steps);
	for (int f = options.animation_start; f < options.animation_end + 1; f++)
	{
		int p = f - options.animation_start;
		times[p] = step_time * p;
	}
	std::vector<double> min_values{ times[0] };
	std::vector<double> max_values{ times[times.size() - 1] };

	//write times into buffer
	const unsigned char* time_bytes = reinterpret_cast<const unsigned char*>(&times[0]);
	std::vector<unsigned char> times_byte_vector(time_bytes, time_bytes + sizeof(float) * times.size());
	int time_index = add_data_to_buffer(model, times_byte_vector, times.size(), false, true, TINYGLTF_COMPONENT_TYPE_FLOAT, TINYGLTF_TYPE_SCALAR, min_values, max_values);

	tinygltf::Animation animation;  // use one animation
	for (auto& pair : object_to_node)
	{
		ULONG xsi_id = pair.first;
		ULONG node_index = pair.second;

		//get Softimage object
		XSI::ProjectItem xsi_item = XSI::Application().GetObjectFromID(xsi_id);
		XSI::X3DObject xsi_object(xsi_item);
		if (xsi_object.IsValid())
		{
			XSI::KinematicState kine = xsi_object.GetKinematics().GetLocal();
			XSI::CParameterRefArray kine_params = kine.GetParameters();

			write_parameters(model,
				animation,
				node_index,
				time_index,
				"translation",
				kine_params.GetItem("posx"), kine_params.GetItem("posy"), kine_params.GetItem("posz"),
				animation_steps, options.animation_start, options.animation_end);

			write_parameters(model,
				animation,
				node_index,
				time_index,
				"rotation",
				kine_params.GetItem("rotx"), kine_params.GetItem("roty"), kine_params.GetItem("rotz"),
				animation_steps, options.animation_start, options.animation_end);

			write_parameters(model,
				animation,
				node_index,
				time_index,
				"scale",
				kine_params.GetItem("sclx"), kine_params.GetItem("scly"), kine_params.GetItem("sclz"),
				animation_steps, options.animation_start, options.animation_end);
		}
	}

	model.animations.push_back(animation);

	times.clear();
	times.shrink_to_fit();
	times_byte_vector.clear();
	times_byte_vector.shrink_to_fit();
}