import win32com.client
from win32com.client import constants as c

null = None
false = 0
true = 1
app = Application

alpha_mode_enum = [
    "Opaque", "OPAQUE",
    "Mask", "MASK",
    "Blend", "BLEND",
]

def XSILoadPlugin(in_reg):
    in_reg.Author = "Shekn Itrch"
    in_reg.Name = "GLTFShadersPlugin"
    in_reg.Major = 1
    in_reg.Minor = 0

    #RegistrationInsertionPoint - do not remove this line
    in_reg.RegisterShader("MetallicRoughness", 1, 0)


def XSIUnloadPlugin(in_reg):
    strPluginName = in_reg.Name
    app.LogMessage(str(strPluginName) + str(" has been unloaded."), c.siVerbose)
    return true


def add_output_closure(shader_def, name="closure"):
    param_options = XSIFactory.CreateShaderParamDefOptions()
    param_options.SetLongName(name)
    params = shader_def.OutputParamDefs
    param_def = params.AddParamDef2(name, c.siShaderDataTypeColor4, param_options)
    param_def.MainPort = False


def add_input_color4(param_options, params, default_value=1, name="color"):
    param_options.SetDefaultValue(default_value)
    params.AddParamDef(name, c.siShaderDataTypeColor4, param_options)


def add_input_color3(param_options, params, default_value=1, name="color"):
    param_options.SetDefaultValue(default_value)
    params.AddParamDef(name, c.siShaderDataTypeColor3, param_options)


def add_input_float(param_options, params, default_value=0, name="float", vis_min=None, vis_max=None):
    param_options.SetDefaultValue(default_value)
    if vis_min is not None and vis_max is not None:
        param_options.SetSoftLimit(vis_min, vis_max)
    params.AddParamDef(name, c.siShaderDataTypeScalar, param_options)


def add_input_string(param_options, params, default_value=0, name="string"):
    param_options.SetDefaultValue(default_value)
    params.AddParamDef(name, c.siShaderDataTypeString, param_options)


def add_input_boolean(param_options, params, default_value=True, name="boolean"):
    param_options.SetDefaultValue(default_value)
    params.AddParamDef(name, c.siShaderDataTypeBoolean, param_options)


def standart_param_options():
    param_options = XSIFactory.CreateShaderParamDefOptions()
    param_options.SetAnimatable(True)
    param_options.SetTexturable(True)
    param_options.SetReadOnly(False)
    param_options.SetInspectable(True)
    return param_options

def nonport_param_options():
    param_options = XSIFactory.CreateShaderParamDefOptions()
    param_options.SetAnimatable(True)
    param_options.SetTexturable(False)
    param_options.SetReadOnly(False)
    param_options.SetInspectable(True)
    return param_options


#------------------------------------------------------------

def GLTFShadersPlugin_MetallicRoughness_1_0_DefineInfo(in_ctxt):
    in_ctxt.SetAttribute("Category", "GLTF IO/")
    in_ctxt.SetAttribute("DisplayName", "pbrMetallicRoughness")
    return True


def GLTFShadersPlugin_MetallicRoughness_1_0_Define(in_ctxt):
    shaderDef = in_ctxt.GetAttribute("Definition")
    shaderDef.AddShaderFamily(c.siShaderFamilySurfaceMat)

    # Input Parameter: input
    params = shaderDef.InputParamDefs

    # parameters
    add_input_string(nonport_param_options(), params, "OPAQUE", "alphaMode")
    add_input_float(nonport_param_options(), params, 0.5, "alphaCutoff", 0.0, 1.0)
    add_input_boolean(nonport_param_options(), params, False, "doubleSided")

    add_input_color4(nonport_param_options(), params, [1.0, 1.0, 1.0, 1.0], "baseColorFactor")
    add_input_color4(standart_param_options(), params, 0.0, "baseColorTexture")

    add_input_float(nonport_param_options(), params, 1.0, "metallicFactor", 0.0, 1.0)
    add_input_float(nonport_param_options(), params, 1.0, "roughnessFactor", 0.0, 1.0)
    add_input_color4(standart_param_options(), params, 0.0, "metallicRoughnessTexture")

    add_input_color4(standart_param_options(), params, 0.0, "normalTexture")
    add_input_float(nonport_param_options(), params, 1.0, "scale", 0.0, 1.0)

    add_input_color4(standart_param_options(), params, 0.0, "occlusionTexture")
    add_input_float(nonport_param_options(), params, 1.0, "strength", 0.0, 1.0)

    add_input_color3(nonport_param_options(), params, [0.0, 0.0, 0.0], "emissiveFactor")
    add_input_color4(standart_param_options(), params, 0.0, "emissiveTexture")

    # Output Parameter
    add_output_closure(shaderDef, "BSDF")

    # next init ppg
    ppg_layout = shaderDef.PPGLayout
    ppg_layout.AddGroup("Parameters")
    ppg_layout.AddEnumControl("alphaMode", alpha_mode_enum, "Alpha Mode")
    ppg_layout.AddItem("alphaCutoff", "Alpha Cutoff")
    ppg_layout.AddItem("doubleSided", "Double Sided")
    ppg_layout.AddItem("baseColorFactor", "Base Color Factor")
    ppg_layout.AddItem("metallicFactor", "Metallic Factor")
    ppg_layout.AddItem("roughnessFactor", "Roughness Factor")
    ppg_layout.AddItem("scale", "Normal Scale")
    ppg_layout.AddItem("strength", "Occlusion Strength")
    ppg_layout.AddItem("emissiveFactor", "Emissive Factor")
    ppg_layout.EndGroup()

    ppg_layout.Language = "Python"
    ppg_layout.Logic = '''
def update(prop):
    alphaMode = prop.Parameters("alphaMode").Value
    if alphaMode == "MASK":
        prop.Parameters("alphaCutoff").ReadOnly = False
    else:
        prop.Parameters("alphaCutoff").ReadOnly = True

def OnInit():
    prop = PPG.Inspected(0)
    update(prop)

def alphaMode_OnChanged():
    prop = PPG.Inspected(0)
    update(prop)
'''

    # Renderer definition
    rendererDef = shaderDef.AddRendererDef("GLTF")
    rendererDef.SymbolName = "MetallicRoughness"

    return True