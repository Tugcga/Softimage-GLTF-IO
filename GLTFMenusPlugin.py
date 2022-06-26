import win32com.client
from win32com.client import constants

null = None
false = 0
true = 1
app = Application

prev_import_params = None

def XSILoadPlugin(in_reg):
    in_reg.Author = "Shekn"
    in_reg.Name = "GLTFMenusPlugin"
    in_reg.Major = 1
    in_reg.Minor = 0

    # RegistrationInsertionPoint - do not remove this line
    in_reg.RegisterCommand("GLTFImportOpen", "GLTFImportOpen")
    in_reg.RegisterMenu(constants.siMenuMainFileImportID, "Softimage GLTF", False, False)

    return True


def XSIUnloadPlugin(in_reg):
    strPluginName = in_reg.Name
    app.LogMessage(str(strPluginName) + str(" has been unloaded."), constants.siVerbose)
    return True


def SoftimageGLTF_Init(ctxt):
    menu = ctxt.source
    menu.AddCommandItem("Import GLTF/GLB ...", "GLTFImportOpen")


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

    layout.AddGroup("Mesh Attributes")
    layout.AddItem("import_normals", "Normals")
    layout.AddItem("import_uvs", "UVs")
    layout.AddItem("import_colors", "Vertex Colors")
    layout.AddItem("import_shapes", "Deform Shapes")
    layout.AddItem("import_skin", "Envelope Skin")
    layout.EndGroup()

    layout.AddGroup("Scene Items")
    layout.AddItem("import_materials", "Materials")
    layout.AddItem("import_cameras", "Cameras")
    layout.AddItem("import_animations", "Animations")
    layout.AddItem("animation_frames_per_second", "Frames in Second")
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