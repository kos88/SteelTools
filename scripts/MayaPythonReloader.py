"""
Run in maya directly when editing python side of things.

import runpy

runpy.run_path(r"PATH TO THIS FILE")

Remember that when not in release, the script read from maya are the one in the dev/maya/scripts folder,
not the copied one for release!
"""
import sys
import maya.cmds as cmds

top_module_names = 'steelTools',

try:
    cmds.deleteUI("steelToolsMenuBar")
except:
    print("No menu to delete...")

to_delete = list()

for module_name in sys.modules.keys():
    top_module = module_name.split('.')[0]
    if top_module in top_module_names:
        print('deleting', module_name)
        to_delete.append(module_name)

for module_name in to_delete:
    del(sys.modules[module_name])

from steelTools.interface.menu import steel_menu
steel_menu()
print("Menu reloaded...")