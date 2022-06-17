#include "utilities.h"

void log_message(const XSI::CString& message, const XSI::siSeverityType level)
{
	XSI::Application().LogMessage(message, level);
}

XSI::CString to_string(const std::vector<double>& array)
{
	XSI::CString to_return = XSI::CString(array.size()) + ": [";
	for (size_t i = 0; i < array.size(); i++)
	{
		to_return += XSI::CString(array[i]) + (i == array.size() - 1 ? "]" : ", ");
	}

	return to_return;
}

XSI::CString to_string(const std::vector<float>& array)
{
	XSI::CString to_return = XSI::CString(array.size()) + ": [";
	for (size_t i = 0; i < array.size(); i++)
	{
		to_return += XSI::CString(array[i]) + (i == array.size() - 1 ? "]" : ", ");
	}

	return to_return;
}

XSI::CString to_string(const std::vector<int>& array)
{
	XSI::CString to_return = XSI::CString(array.size()) + ": [";
	for (size_t i = 0; i < array.size(); i++)
	{
		to_return += XSI::CString(array[i]) + (i == array.size() - 1 ? "]" : ", ");
	}

	return to_return;
}

XSI::CString to_string(const std::vector<LONG>& array)
{
	XSI::CString to_return = XSI::CString(array.size()) + ": [";
	for (size_t i = 0; i < array.size(); i++)
	{
		to_return += XSI::CString(array[i]) + (i == array.size() - 1 ? "]" : ", ");
	}

	return to_return;
}

XSI::CString to_string(const std::vector<std::string>& array)
{
	XSI::CString to_return = XSI::CString(array.size()) + ": [";
	for (size_t i = 0; i < array.size(); i++)
	{
		to_return += XSI::CString(array[i].c_str()) + (i == array.size() - 1 ? "]" : ", ");
	}

	return to_return;
}