import win32com.client
from win32com.client import constants

null = None
false = 0
true = 1
app = Application

export_mode_enum = [
    "Selected objects", 0,
    "Whole scene", 1,
]

prev_import_params = None
prev_export_params = None

def XSILoadPlugin(in_reg):
    in_reg.Author = "Shekn"
    in_reg.Name = "GLTFMenusPlugin"
    in_reg.Major = 1
    in_reg.Minor = 0

    # RegistrationInsertionPoint - do not remove this line
    in_reg.RegisterCommand("GLTFImportOpen", "GLTFImportOpen")
    in_reg.RegisterCommand("GLTFExportOpen", "GLTFExportOpen")
    in_reg.RegisterMenu(constants.siMenuMainFileImportID, "Softimage GLTF Import", False, False)
    in_reg.RegisterMenu(constants.siMenuMainFileExportID, "Softimage GLTF Export", False, False)

    return True


def XSIUnloadPlugin(in_reg):
    strPluginName = in_reg.Name
    app.LogMessage(str(strPluginName) + str(" has been unloaded."), constants.siVerbose)
    return True


def SoftimageGLTFImport_Init(ctxt):
    menu = ctxt.source
    menu.AddCommandItem("Import GLTF/GLB ...", "GLTFImportOpen")


def SoftimageGLTFExport_Init(ctxt):
    menu = ctxt.source
    menu.AddCommandItem("Export GLTF/GLB ...", "GLTFExportOpen")


def GLTFImportOpen_Execute():
    app.LogMessage("GLTFImportOpen_Execute called", constants.siVerbose)

    scene_root = app.ActiveProject2.ActiveScene.Root
    prop = scene_root.AddProperty("CustomProperty", False, "GLTF_Import")

    prop.AddParameter3("file_path", constants.siString, "", "", "", False, False)

    param = prop.AddParameter3("import_normals", constants.siBool, True, False, False)
    param.Animatable = False

    param = prop.AddParameter3("import_uvs", constants.siBool, True, False, False)
    param.Animatable = False

    param = prop.AddParameter3("import_colors", constants.siBool, True, False, False)
    param.Animatable = False

    param = prop.AddParameter3("import_shapes", constants.siBool, True, False, False)
    param.Animatable = False

    param = prop.AddParameter3("import_skin", constants.siBool, True, False, False)
    param.Animatable = False

    param = prop.AddParameter3("import_materials", constants.siBool, True, False, False)
    param.Animatable = False

    param = prop.AddParameter3("import_cameras", constants.siBool, True, False, False)
    param.Animatable = False

    param = prop.AddParameter3("import_animations", constants.siBool, False, False, False)
    param.Animatable = False

    param = prop.AddParameter2("animation_frames_per_second", constants.siFloat, 30.0, 0.0, 999999.0, 1.0, 60.0)
    param.Animatable = False

    layout = prop.PPGLayout
    layout.Clear()
    layout.AddGroup("File Path")
    item = layout.AddItem("file_path", "File", constants.siControlFilePath)
    item.SetAttribute(constants.siUIOpenFile, True)
    item.SetAttribute(constants.siUIFileMustExist, False)
    filterstring = "GLTF/GLB files (*.gltf, *.glb)|*.gltf:*.glb|"
    item.SetAttribute(constants.siUIFileFilter, filterstring)
    layout.EndGroup()

    layout.AddGroup("Scene Items")
    layout.AddItem("import_materials", "Materials")
    layout.AddItem("import_cameras", "Cameras")
    layout.EndGroup()

    layout.AddGroup("Animations")
    layout.AddItem("import_animations", "Animations")
    layout.AddItem("animation_frames_per_second", "Frames in Second")
    layout.EndGroup()

    layout.AddGroup("Mesh Attributes")
    layout.AddItem("import_normals", "Normals")
    layout.AddItem("import_uvs", "UVs")
    layout.AddItem("import_colors", "Vertex Colors")
    layout.AddItem("import_shapes", "Deform Shapes")
    layout.AddItem("import_skin", "Envelope Skin")
    layout.EndGroup()

    layout.Language = "Python"
    layout.Logic = '''
def update(prop):
    import_animations = prop.Parameters("import_animations").Value
    if import_animations:
        prop.Parameters("animation_frames_per_second").ReadOnly = False
    else:
        prop.Parameters("animation_frames_per_second").ReadOnly = True

def OnInit():
    prop = PPG.Inspected(0)
    update(prop)

def import_animations_OnChanged():
    prop = PPG.Inspected(0)
    update(prop)
'''

    property_keys = ["import_normals", "import_uvs", "import_colors", "import_shapes", "import_skin", "import_materials", "import_cameras", "import_animations", "animation_frames_per_second"]
    # read previous import parameters
    global prev_import_params
    if prev_import_params is not None:
        for k in property_keys:
            prop.Parameters(k).Value = prev_import_params[k]

    rtn = app.InspectObj(prop, "", "Import GLTF/GLB file...", constants.siModal, False)
    if rtn is False:
        file_path = prop.Parameters("file_path").Value
        path_parts = file_path.split(".")
        if len(path_parts) > 0:
            path_ext = path_parts[-1]
            if path_ext in ["gltf", "glb"]:
                app.GLTFImport(file_path,
                    prop.Parameters("import_normals").Value,
                    prop.Parameters("import_uvs").Value,
                    prop.Parameters("import_colors").Value,
                    prop.Parameters("import_shapes").Value,
                    prop.Parameters("import_skin").Value,
                    prop.Parameters("import_materials").Value,
                    prop.Parameters("import_cameras").Value,
                    prop.Parameters("import_animations").Value,
                    prop.Parameters("animation_frames_per_second").Value)
            else:
                app.LogMessage("Select *.gltf or *.glb file", constants.siWarning)
    # save import parameters to the dictionary
    if prev_import_params is None:
        prev_import_params = {}
    for k in property_keys:
        prev_import_params[k] = prop.Parameters(k).Value
    app.DeleteObj(prop)
    return True


def GLTFExportOpen_Execute():
    app.LogMessage("GLTFExportOpen_Execute called", constants.siVerbose)

    scene_root = app.ActiveProject2.ActiveScene.Root
    prop = scene_root.AddProperty("CustomProperty", False, "GLTF_Export")

    prop.AddParameter3("file_path", constants.siString, "", "", "", False, False)

    param = prop.AddParameter3("embed_images", constants.siBool, False, False, False)
    param.Animatable = False

    param = prop.AddParameter3("embed_buffers", constants.siBool, False, False, False)
    param.Animatable = False

    param = prop.AddParameter3("export_uvs", constants.siBool, True, False, False)
    param.Animatable = False

    param = prop.AddParameter3("export_colors", constants.siBool, True, False, False)
    param.Animatable = False

    param = prop.AddParameter3("export_shapes", constants.siBool, True, False, False)
    param.Animatable = False

    param = prop.AddParameter3("export_skin", constants.siBool, True, False, False)
    param.Animatable = False

    param = prop.AddParameter2("export_mode", constants.siInt4, 0)
    param.Animatable = False

    param = prop.AddParameter3("export_materials", constants.siBool, True, False, False)
    param.Animatable = False

    param = prop.AddParameter3("export_cameras", constants.siBool, True, False, False)
    param.Animatable = False

    param = prop.AddParameter3("export_animations", constants.siBool, False, False, False)
    param.Animatable = False

    param = prop.AddParameter2("animation_frames_per_second", constants.siFloat, 30.0, 0.0, 999999.0, 1.0, 60.0)
    param.Animatable = False

    param = prop.AddParameter2("animation_start", constants.siInt4, 1, -999999, 999999, 0, 100)
    param.Animatable = False

    param = prop.AddParameter2("animation_end", constants.siInt4, 100, -999999, 999999, 0, 100)
    param.Animatable = False

    param = prop.AddParameter3("export_hide", constants.siBool, False, False, False)
    param.Animatable = False

    
    layout = prop.PPGLayout
    layout.Clear()
    layout.AddGroup("Export")
    item = layout.AddItem("file_path", "File", constants.siControlFilePath)
    filterstring = "GLTF/GLB files (*.gltf, *.glb)|*.gltf:*.glb|"
    item.SetAttribute(constants.siUIFileFilter, filterstring)
    layout.AddRow()
    layout.AddItem("embed_images", "Embed Images")
    layout.AddItem("embed_buffers", "Embed Buffer")
    layout.EndRow()
    layout.EndGroup()

    layout.AddGroup("Scene Items")
    layout.AddEnumControl("export_mode", export_mode_enum, "Export Mode")
    layout.AddRow()
    layout.AddItem("export_materials", "Materials")
    layout.AddItem("export_cameras", "Cameras")
    layout.AddItem("export_hide", "Hide Objects")
    layout.EndRow()
    layout.EndGroup()

    layout.AddGroup("Animations")
    layout.AddItem("export_animations", "Animations")
    layout.AddItem("animation_frames_per_second", "Frames in Second")
    layout.AddRow()
    layout.AddItem("animation_start", "Start")
    layout.AddItem("animation_end", "End")
    layout.EndRow()
    layout.EndGroup()

    layout.AddGroup("Mesh Attributes")
    layout.AddRow()
    layout.AddItem("export_uvs", "UVs")
    layout.AddItem("export_colors", "Vertex Colors")
    layout.EndRow()
    layout.AddRow()
    layout.AddItem("export_shapes", "Deform Shapes")
    layout.AddItem("export_skin", "Envelope Skin")
    layout.EndRow()
    layout.EndGroup()
    
    layout.Language = "Python"
    layout.Logic = '''
def update(prop):
    export_animations = prop.Parameters("export_animations").Value
    if export_animations:
        prop.Parameters("animation_frames_per_second").ReadOnly = False
        prop.Parameters("animation_start").ReadOnly = False
        prop.Parameters("animation_end").ReadOnly = False
    else:
        prop.Parameters("animation_frames_per_second").ReadOnly = True
        prop.Parameters("animation_start").ReadOnly = True
        prop.Parameters("animation_end").ReadOnly = True

def OnInit():
    prop = PPG.Inspected(0)
    update(prop)

def export_animations_OnChanged():
    prop = PPG.Inspected(0)
    update(prop)
'''

    property_keys = [
    "embed_images", 
    "embed_buffers", 
    "export_uvs", 
    "export_colors", 
    "export_shapes", 
    "export_skin", 
    "export_mode", 
    "export_materials", 
    "export_cameras", 
    "export_animations", 
    "animation_frames_per_second", 
    "animation_start", 
    "animation_end", 
    "export_hide"
    ]
    # read previous import parameters
    global prev_export_params
    if prev_export_params is not None:
        for k in property_keys:
            prop.Parameters(k).Value = prev_export_params[k]

    rtn = app.InspectObj(prop, "", "Export GLTF/GLB file...", constants.siModal, False)
    if rtn is False:
        file_path = prop.Parameters("file_path").Value
        path_parts = file_path.split(".")
        if len(path_parts) > 0:
            path_ext = path_parts[-1]
            if path_ext in ["gltf", "glb"]:
                objects = []
                export_mode = prop.Parameters("export_mode").Value
                if export_mode == 0:
                    objects = [obj for obj in app.Selection]
                else:
                    # select the root object
                    objects = [app.ActiveProject.ActiveScene.Root]

                app.GLTFExport(objects, file_path,
                    prop.Parameters("embed_images").Value,
                    prop.Parameters("embed_buffers").Value,
                    prop.Parameters("export_uvs").Value,
                    prop.Parameters("export_colors").Value,
                    prop.Parameters("export_shapes").Value,
                    prop.Parameters("export_skin").Value,
                    prop.Parameters("export_materials").Value,
                    prop.Parameters("export_cameras").Value,
                    prop.Parameters("export_animations").Value,
                    prop.Parameters("animation_frames_per_second").Value,
                    prop.Parameters("animation_start").Value,
                    prop.Parameters("animation_end").Value,
                    prop.Parameters("export_hide").Value)
            else:
                app.LogMessage("Define *.gltf or *.glb output file", constants.siWarning)
    # save import parameters to the dictionary
    if prev_export_params is None:
        prev_export_params = {}
    for k in property_keys:
        prev_export_params[k] = prop.Parameters(k).Value
    app.DeleteObj(prop)
    return True