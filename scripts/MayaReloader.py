import maya.cmds as cmds

plugin_name = "SteelToolsMaya"

cmds.file(new=True, force=True)
if cmds.pluginInfo(plugin_name, q=True, loaded=True):
    cmds.unloadPlugin(plugin_name)
    print(f"Unloaded plugin {plugin_name}")

cmds.loadPlugin(pluginName)