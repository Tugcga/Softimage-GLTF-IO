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

XSI::CString to_string(const std::vector<ULONG>& array)
{
	XSI::CString to_return = XSI::CString(array.size()) + ": [";
	for (size_t i = 0; i < array.size(); i++)
	{
		to_return += XSI::CString(array[i]) + (i == array.size() - 1 ? "]" : ", ");
	}

	return to_return;
}

XSI::CString to_string(const std::vector<size_t>& array)
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

XSI::CString to_string(const std::vector<unsigned char>& array)
{
	XSI::CString to_return = XSI::CString(array.size()) + ": [";
	for (size_t i = 0; i < array.size(); i++)
	{
		to_return += XSI::CString(array[i]) + (i == array.size() - 1 ? "]" : ", ");
	}

	return to_return;
}

XSI::CString to_string(const std::vector<unsigned int>& array)
{
	XSI::CString to_return = XSI::CString(array.size()) + ": [";
	for (size_t i = 0; i < array.size(); i++)
	{
		to_return += XSI::CString((int)array[i]) + (i == array.size() - 1 ? "]" : ", ");
	}

	return to_return;
}

XSI::CString to_string(const XSI::CLongArray &array)
{
	XSI::CString to_return = XSI::CString(array.GetCount()) + ": [";
	for (size_t i = 0; i < array.GetCount(); i++)
	{
		to_return += XSI::CString(array[i]) + (i == array.GetCount() - 1 ? "]" : ", ");
	}

	return to_return;
}

XSI::CString to_string(const XSI::CDoubleArray& array)
{
	XSI::CString to_return = XSI::CString(array.GetCount()) + ": [";
	for (size_t i = 0; i < array.GetCount(); i++)
	{
		to_return += XSI::CString(array[i]) + (i == array.GetCount() - 1 ? "]" : ", ");
	}

	return to_return;
}

XSI::CString to_string(const XSI::CFloatArray& array)
{
	XSI::CString to_return = XSI::CString(array.GetCount()) + ": [";
	for (size_t i = 0; i < array.GetCount(); i++)
	{
		to_return += XSI::CString(array[i]) + (i == array.GetCount() - 1 ? "]" : ", ");
	}

	return to_return;
}

XSI::CString to_string(const std::set<ULONG>& set)
{
	XSI::CString to_return = XSI::CString(set.size()) + ": {";
	std::set<ULONG>::iterator set_it;
	ULONG i = 0;
	for (set_it = set.begin(); set_it != set.end(); ++set_it)
	{
		to_return += XSI::CString(*set_it) + (i == set.size() - 1 ? "}" : ", ");
		i++;
	}

	return to_return;
}

XSI::CString to_string(const std::vector<Vertex> &array)
{
	XSI::CString to_return = XSI::CString(array.size()) + ": [";
	for (size_t i = 0; i < array.size(); i++)
	{
		Vertex v = array[i];
		XSI::CString s = v.to_string();
		to_return += s + (i == array.size() - 1 ? "]" : ", ");
	}

	return to_return;
}

XSI::CString to_string(const XSI::MATH::CMatrix4 &m)
{
	XSI::CString to_return = "";
	for (ULONG i = 0; i < 4; i++)
	{
		XSI::CString part = "";
		for (ULONG j = 0; j < 4; j++)
		{
			part += XSI::CString(m.GetValue(i, j)) + " ";
		}
		to_return += part + " | ";
	}
	return to_return;
}