import maya.cmds as cmds
import maya.mel as mel

plugin_name = "SteelToolsMaya"

cmds.file(new=True, force=True)
if cmds.pluginInfo(plugin_name, q=True, loaded=True):
    cmds.unloadPlugin(plugin_name)
    print(f"Unloaded plugin {plugin_name}")

cmds.loadPlugin(pluginName)

# Reload AE template
mel.eval('source "AEsteelStickyLipsTemplate.mel"')
print("Reloaded AE template")