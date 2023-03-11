#pragma once
#include <xsi_application.h>
#include <xsi_longarray.h>
#include <xsi_doublearray.h>
#include <xsi_floatarray.h>

#include <string>
#include <vector>
#include <set>

#include "../gltf_io/export.h"

const double M_PI = 3.14159265358979323846;

enum FileType { Ascii, Binary, Unknown };

void log_message(const XSI::CString &message, const XSI::siSeverityType level = XSI::siSeverityType::siInfoMsg);

XSI::CString to_string(const std::vector<double>& array);
XSI::CString to_string(const std::vector<float>& array);
XSI::CString to_string(const std::vector<int>& array);
XSI::CString to_string(const std::vector<LONG>& array);
XSI::CString to_string(const std::vector<ULONG>& array);
XSI::CString to_string(const std::vector<size_t>& array);
XSI::CString to_string(const std::vector<std::string>& array);
XSI::CString to_string(const std::vector<unsigned char>& array);
XSI::CString to_string(const std::vector<unsigned int>& array);
XSI::CString to_string(const XSI::CLongArray &array);
XSI::CString to_string(const XSI::CDoubleArray& array);
XSI::CString to_string(const XSI::CFloatArray& array);
XSI::CString to_string(const std::set<ULONG> &set);
XSI::CString to_string(const std::vector<Vertex>& array);
XSI::CString to_string(const XSI::MATH::CMatrix4& m);

std::string get_file_extension(const std::string& path);
FileType detect_file_type(const std::string& path);

XSI::CString full_file_name_from_path(const XSI::CString& file_path);
XSI::CString file_name_from_path(const XSI::CString& file_path);
bool create_dir(const std::string& file_path);
bool is_file_exists(const XSI::CString& file_path);
XSI::CString file_path_to_folder(const XSI::CString& file_path);
bool is_extension_supported(const std::string& ext);

bool is_array_contains(const size_t value, const std::vector<size_t>& array);