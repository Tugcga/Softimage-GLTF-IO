#include <unordered_map>

#include <xsi_x3dobject.h>
#include <xsi_kinematics.h>
#include <xsi_parameter.h>
#include <xsi_fcurve.h>
#include <xsi_vector3f.h>
#include <xsi_progressbar.h>

#include "../../tiny_gltf/tiny_gltf.h"

#include "../../utilities/utilities.h"
#include "../import.h"

void quaternion_to_angles(float x, float y, float z, float w, float &r_x, float &r_y, float &r_z)
{
	float sinr_cosp = 2 * (w * x + y * z);
	float cosr_cosp = 1 - 2 * (x * x + y * y);
	r_x = std::atan2(sinr_cosp, cosr_cosp);

	float sinp = 2 * (w * y - z * x);
	if (std::abs(sinp) >= 1)
	{
		r_y = std::copysign(M_PI / 2, sinp);
	}
	else
	{
		r_y = std::asin(sinp);
	}

	float siny_cosp = 2 * (w * z + x * y);
	float cosy_cosp = 1 - 2 * (y * y + z * z);
	r_z = std::atan2(siny_cosp, cosy_cosp);
}

void plot_animation(const std::vector<float> &times,
	const std::vector<float> & values,
	ULONG data_length, ULONG data_shift,
	const XSI::siFCurveKeyInterpolation curve_type,
	XSI::FCurve &x_curve, XSI::FCurve& y_curve, XSI::FCurve& z_curve,
	float animation_frames_per_second)
{
	//we assume that input time is orderer in increasing order
	//get the minimum and maximum time
	int min_frame = int(times[0] * animation_frames_per_second);
	int max_frame = int(times[times.size() - 1] * animation_frames_per_second);
	int point = 0;
	for (int i = min_frame; i < max_frame + 1; i++)
	{
		float t_current = (i - min_frame) / animation_frames_per_second;
		//for each integer frame we should find the greatest key, less the current value
		//increase point value while obtain greater value than i
		bool is_increase = true;
		while (is_increase)
		{
			//calculate frame at next point
			float v = times[point + 1] * animation_frames_per_second;
			if (v > i)
			{
				is_increase = false;
			}
			else
			{
				point++;
			}
			if (point == times.size() - 1)
			{
				is_increase = false;
			}
		}
		if (point == times.size() - 1)
		{//this is the last point, use this value
			x_curve.AddKey(i, values[data_length * point + data_shift], curve_type);
			y_curve.AddKey(i, values[data_length * point + data_shift + 1], curve_type);
			z_curve.AddKey(i, values[data_length * point + data_shift + 2], curve_type);
		}
		else
		{
			float t_prev = times[point];
			float t_next = times[point + 1];
			float t_distance = t_next - t_prev;
			float t = (t_current - t_prev) / t_distance;

			XSI::MATH::CVector3f value;  // store here result value
			XSI::MATH::CVector3f v_prev;
			XSI::MATH::CVector3f v_next;
			XSI::MATH::CVector3f b_prev;
			XSI::MATH::CVector3f a_next;
			if (data_shift == 3)
			{
				v_prev = XSI::MATH::CVector3f(values[data_length * point + data_shift], values[data_length * point + data_shift + 1], values[data_length * point + data_shift + 2]);
				v_next = XSI::MATH::CVector3f(values[data_length * (point + 1) + data_shift], values[data_length * (point + 1) + data_shift + 1], values[data_length * (point + 1) + data_shift + 2]);
				b_prev = XSI::MATH::CVector3f(values[data_length * point + 2 * data_shift], values[data_length * point + 2 * data_shift + 1], values[data_length * point + 2 * data_shift + 2]);
				a_next = XSI::MATH::CVector3f(values[data_length * (point + 1)], values[data_length * (point + 1) + 1], values[data_length * (point + 1) + 2]);
			}
			else if (data_shift == 4)
			{
				XSI::MATH::CQuaternion v_prev_q = XSI::MATH::CQuaternion(values[data_length * point + data_shift], 
					values[data_length * point + data_shift + 1],
					values[data_length * point + data_shift + 2], 
					values[data_length * point + data_shift + 3]);
				XSI::MATH::CQuaternion v_next_q = XSI::MATH::CQuaternion(values[data_length * (point + 1) + data_shift],
					values[data_length * (point + 1) + data_shift + 1], 
					values[data_length * (point + 1) + data_shift + 2],
					values[data_length * (point + 1) + data_shift + 3]);
				XSI::MATH::CQuaternion b_prev_q = XSI::MATH::CQuaternion(values[data_length * point + 2 * data_shift],
					values[data_length * point + 2 * data_shift + 1],
					values[data_length * point + 2 * data_shift + 2],
					values[data_length * point + 2 * data_shift + 3]);
				XSI::MATH::CQuaternion a_next_q = XSI::MATH::CQuaternion(values[data_length * (point + 1)],
					values[data_length * (point + 1) + 1],
					values[data_length * (point + 1) + 2], 
					values[data_length * (point + 1) + 3]);

				//extract Eulear angles and form vector3 values
				double r_x;
				double r_y;
				double r_z;
				v_prev_q.GetXYZAnglesValues(r_x, r_y, r_z);
				v_prev = XSI::MATH::CVector3f(r_x, r_y, r_z);
				v_prev.ScaleInPlace(180.0f / M_PI);

				v_next_q.GetXYZAnglesValues(r_x, r_y, r_z);
				v_next = XSI::MATH::CVector3f(r_x, r_y, r_z);
				v_next.ScaleInPlace(180.0f / M_PI);

				b_prev_q.GetXYZAnglesValues(r_x, r_y, r_z);
				b_prev = XSI::MATH::CVector3f(r_x, r_y, r_z);
				b_prev.ScaleInPlace(180.0f / M_PI);

				a_next_q.GetXYZAnglesValues(r_x, r_y, r_z);
				a_next = XSI::MATH::CVector3f(r_x, r_y, r_z);
				a_next.ScaleInPlace(180.0f / M_PI);
			}

			v_prev.ScaleInPlace(2 * t * t * t - 3 * t * t + 1);
			b_prev.ScaleInPlace(t_distance * (t * t * t - 2 * t * t + t));
			v_next.ScaleInPlace(-2 * t * t * t + 3 * t * t);
			a_next.ScaleInPlace(t_distance * (t * t * t - t * t));

			value.Add(v_prev, b_prev);
			value.AddInPlace(v_next);
			value.AddInPlace(a_next);

			//set curve keys
			x_curve.AddKey(i, value.GetX(), curve_type);
			y_curve.AddKey(i, value.GetY(), curve_type);
			z_curve.AddKey(i, value.GetZ(), curve_type);
		}
	}
}

void import_animation(XSI::ProgressBar& bar, const tinygltf::Model& model, const std::unordered_map<ULONG, XSI::X3DObject> &nodes_map, const ImportOptions &options)
{
	for (ULONG anim_index = 0; anim_index < model.animations.size(); anim_index++)
	{
		tinygltf::Animation animation = model.animations[anim_index];
		bar.PutCaption("Animation " + XSI::CString(animation.name.c_str()));
		for (ULONG channel_index = 0; channel_index < animation.channels.size(); channel_index++)
		{
			tinygltf::AnimationChannel channel = animation.channels[channel_index];
			tinygltf::AnimationSampler sampler = animation.samplers[channel.sampler];
			ULONG node_index = channel.target_node;
			if (nodes_map.find(node_index) != nodes_map.end())
			{
				XSI::X3DObject xsi_object = nodes_map.at(node_index);
				XSI::CParameterRefArray xsi_params = xsi_object.GetParameters();

				std::string target_path = channel.target_path;
				const tinygltf::Accessor& time_accessor = model.accessors[sampler.input];
				const tinygltf::Accessor& values_accessor = model.accessors[sampler.output];
				XSI::siFCurveKeyInterpolation curve_type = sampler.interpolation == "CUBICSPLINE" ? XSI::siCubicKeyInterpolation : (
					sampler.interpolation == "STEP" ? XSI::siConstantKeyInterpolation : XSI::siLinearKeyInterpolation);
				//read times
				std::vector<float> times = get_float_buffer(model, time_accessor);
				//and values
				std::vector<float> values = get_float_buffer(model, values_accessor, true);
				if (times.size() > 0 && values.size() > 0)
				{
					if (target_path == "translation")
					{
						if ((curve_type != XSI::siCubicKeyInterpolation && values.size() == times.size() * 3) || 
							(curve_type == XSI::siCubicKeyInterpolation && values.size() == times.size() * 9))
						{
							ULONG data_length = curve_type == XSI::siCubicKeyInterpolation ? 9 : 3;
							ULONG data_shift = curve_type == XSI::siCubicKeyInterpolation ? 3 : 0;
							XSI::Parameter x = xsi_params.GetItem("posx");
							XSI::Parameter y = xsi_params.GetItem("posy");
							XSI::Parameter z = xsi_params.GetItem("posz");
							XSI::FCurve x_curve;
							XSI::FCurve y_curve;
							XSI::FCurve z_curve;
							x.AddFCurve(XSI::siDefaultFCurve, x_curve);
							y.AddFCurve(XSI::siDefaultFCurve, y_curve);
							z.AddFCurve(XSI::siDefaultFCurve, z_curve);

							if (curve_type == XSI::siCubicKeyInterpolation)
							{
								//for cubic interpolation we should bake transform to each frame
								curve_type = XSI::siLinearKeyInterpolation;

								plot_animation(times, values, data_length, data_shift, curve_type, x_curve, y_curve, z_curve, options.animation_frames_per_second);
							}
							else
							{
								//for non-cubic interpolation we simply set keys
								for (ULONG i = 0; i < times.size(); i++)
								{
									float time = times[i] * options.animation_frames_per_second;
									//set values to fcurves
									x_curve.AddKey(time, values[data_length * i + data_shift], curve_type);
									y_curve.AddKey(time, values[data_length * i + data_shift + 1], curve_type);
									z_curve.AddKey(time, values[data_length * i + data_shift + 2], curve_type);
								}
							}
						}
					}
					else if (target_path == "rotation")
					{
						if ((curve_type != XSI::siCubicKeyInterpolation && values.size() == times.size() * 4) ||
							(curve_type == XSI::siCubicKeyInterpolation && values.size() == times.size() * 12))
						{
							ULONG data_length = curve_type == XSI::siCubicKeyInterpolation ? 12 : 4;
							ULONG data_shift = curve_type == XSI::siCubicKeyInterpolation ? 4 : 0;

							XSI::Parameter x = xsi_params.GetItem("rotx");
							XSI::Parameter y = xsi_params.GetItem("roty");
							XSI::Parameter z = xsi_params.GetItem("rotz");

							XSI::FCurve x_curve;
							XSI::FCurve y_curve;
							XSI::FCurve z_curve;

							x.AddFCurve(XSI::siDefaultFCurve, x_curve);
							y.AddFCurve(XSI::siDefaultFCurve, y_curve);
							z.AddFCurve(XSI::siDefaultFCurve, z_curve);

							if (curve_type == XSI::siCubicKeyInterpolation)
							{
								//for cubic interpolation we should bake transform to each frame
								curve_type = XSI::siLinearKeyInterpolation;

								//WARNING: result animation is incorrect
								//may be we use wrong interpolation formula (the same as for 3d-vectors), or may be this is result of ambiguous quaternion convertion into Eulear angles
								plot_animation(times, values, data_length, data_shift, curve_type, x_curve, y_curve, z_curve, options.animation_frames_per_second);
							}
							else
							{
								for (ULONG i = 0; i < times.size(); i++)
								{
									float time = times[i] * options.animation_frames_per_second;
									XSI::MATH::CQuaternion r(XSI::MATH::CVector4(values[4 * i], values[4 * i + 1], values[4 * i + 2], values[4 * i + 3]));
									//NOTE: there are some problems with conversion from quaternion to axis angles
									//in gltf values store in one format, but after extracting rotation the Softimage shift it by period 2 * M_PI

									//get axis rotation values
									double r_x;
									double r_y;
									double r_z;
									r.GetXYZAnglesValues(r_x, r_y, r_z);

									//set values to fcurves
									x_curve.AddKey(time, r_x * 180.0f / M_PI, curve_type);
									y_curve.AddKey(time, r_y * 180.0f / M_PI, curve_type);
									z_curve.AddKey(time, r_z * 180.0f / M_PI, curve_type);
								}
							}
						}
					}
					else if (target_path == "scale")
					{
						if ((curve_type != XSI::siCubicKeyInterpolation && values.size() == times.size() * 3) ||
							(curve_type == XSI::siCubicKeyInterpolation && values.size() == times.size() * 9))
						{
							ULONG data_length = curve_type == XSI::siCubicKeyInterpolation ? 9 : 3;
							ULONG data_shift = curve_type == XSI::siCubicKeyInterpolation ? 3 : 0;
							XSI::Parameter x = xsi_params.GetItem("sclx");
							XSI::Parameter y = xsi_params.GetItem("scly");
							XSI::Parameter z = xsi_params.GetItem("sclz");
							XSI::FCurve x_curve;
							XSI::FCurve y_curve;
							XSI::FCurve z_curve;
							x.AddFCurve(XSI::siDefaultFCurve, x_curve);
							y.AddFCurve(XSI::siDefaultFCurve, y_curve);
							z.AddFCurve(XSI::siDefaultFCurve, z_curve);

							if (curve_type == XSI::siCubicKeyInterpolation)
							{
								curve_type = XSI::siLinearKeyInterpolation;
								plot_animation(times, values, data_length, data_shift, curve_type, x_curve, y_curve, z_curve, options.animation_frames_per_second);
							}
							else
							{
								for (ULONG i = 0; i < times.size(); i++)
								{
									float time = times[i] * options.animation_frames_per_second;
									x_curve.AddKey(time, values[data_length * i + data_shift], curve_type);
									y_curve.AddKey(time, values[data_length * i + data_shift + 1], curve_type);
									z_curve.AddKey(time, values[data_length * i + data_shift + 2], curve_type);
								}
							}
						}
					}
					else
					{
						//does not support other animations
						//does not support weights animation, because limited support of the shape import
					}
				}

				times.clear();
				times.shrink_to_fit();
				values.clear();
				values.shrink_to_fit();
			}
		}

		bar.Increment();
	}
}