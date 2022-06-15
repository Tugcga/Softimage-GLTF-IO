#pragma once
#include <xsi_application.h>

#include <string>
#include <vector>

enum FileType { Ascii, Binary, Unknown };

void log_message(const XSI::CString &message, const XSI::siSeverityType level = XSI::siSeverityType::siInfoMsg);

XSI::CString to_string(const std::vector<double>& array);
XSI::CString to_string(const std::vector<float>& array);
XSI::CString to_string(const std::vector<int>& array);
XSI::CString to_string(const std::vector<LONG>& array);

FileType detect_file_type(const std::string& path);