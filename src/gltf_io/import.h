#pragma once
#include <xsi_application.h>

struct ImportMeshOptions
{
	bool weld_vertices;
};

bool import_gltf(const XSI::CString file_path);