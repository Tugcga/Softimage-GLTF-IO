#pragma once
#include <xsi_application.h>

struct ExportOptions
{
	bool embed_images;
	bool embed_buffers;
	XSI::CString output_path;
};

bool export_gltf(const XSI::CString &file_path, const XSI::CRefArray &objects);