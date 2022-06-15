// SoftimageGLTF IO
// Initial code generated by Softimage SDK Wizard
// Executed Mon Jun 13 15:34:29 UTC+0500 2022 by Shekn
// 
// Tip: You need to compile the generated code before you can load the plug-in.
// After you compile the plug-in, you can load it by clicking Update All in the Plugin Manager.
#include <xsi_application.h>
#include <xsi_context.h>
#include <xsi_pluginregistrar.h>
#include <xsi_status.h>
#include <xsi_command.h>
#include <xsi_argument.h>

#include "utilities/utilities.h"
#include "gltf_io/import.h"

SICALLBACK XSILoadPlugin(XSI::PluginRegistrar& in_reg)
{
	in_reg.PutAuthor("Shekn");
	in_reg.PutName("SoftimageGLTF IO");
	in_reg.PutVersion(1, 0);
	//RegistrationInsertionPoint - do not remove this line
	in_reg.RegisterCommand("GLTFImport", "GLTFImport");
	//in_reg.RegisterCommand("GLTFExport", "GLTFExport");

	return XSI::CStatus::OK;
}

SICALLBACK XSIUnloadPlugin(const XSI::PluginRegistrar& in_reg)
{
	XSI::CString str_plugin_name;
	str_plugin_name = in_reg.GetName();
	XSI::Application().LogMessage(str_plugin_name + " has been unloaded.", XSI::siVerboseMsg);
	return XSI::CStatus::OK;
}

SICALLBACK GLTFImport_Init(XSI::CRef& in_ctxt)
{
	XSI::Context ctxt(in_ctxt);
	XSI::Command cmd;
	cmd = ctxt.GetSource();
	cmd.PutDescription("Import *.gltf file to the scene");
	cmd.SetFlag(XSI::siNoLogging, false);

	XSI::ArgumentArray args;
	args = cmd.GetArguments();
	args.Add("file_path");

	return XSI::CStatus::OK;
}

SICALLBACK GLTFImport_Execute(XSI::CRef& in_ctxt)
{
	XSI::Context ctxt(in_ctxt);
	XSI::CValueArray args = ctxt.GetAttribute("Arguments");

	//extract input arguments
	XSI::CString file_path = args[0];

	//import_gltf(file_path);
	//import_gltf("D:\\Graphic\\For Softimage\\Projects\\Softimage GLTF\\Models\\cube_and_plane_01.gltf");
	//import_gltf("D:\\Graphic\\For Softimage\\Projects\\Softimage GLTF\\Models\\BrainStem.gltf");
	//import_gltf("D:\\Graphic\\For Softimage\\Projects\\Softimage GLTF\\Models\\BoxVertexColors.gltf");
	import_gltf("D:\\Graphic\\For Softimage\\Projects\\Softimage GLTF\\Models\\BoomBox.gltf");

	return XSI::CStatus::OK;
}

