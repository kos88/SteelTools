import os
import sys
import subprocess
# import reloader
import inspect
import argparse

# Parse command-line arguments
parser = argparse.ArgumentParser(description='Open Maya with specified version.')
parser.add_argument('--version', type=str, default='2028', help='The version of Maya to open.')
args = parser.parse_args()

this_directory = os.path.abspath(os.path.split(__file__)[0])
base_dir = os.path.dirname(os.path.dirname(this_directory))
module_dir = base_dir + '\\releases'
print(f"Module dir is {module_dir}")

MAYA_EXE = f'C:/Program Files/Autodesk/Maya{args.version}/bin/maya.exe'
MAYA_PREFS = os.path.join(base_dir, 'Steel_maya_prefs')
# Start with clean prefs
os.environ['MAYA_APP_DIR'] = MAYA_PREFS
print("Maya preferences for this session in : {}".format(os.environ['MAYA_APP_DIR']))
# Add this as maya module
os.environ['MAYA_MODULE_PATH'] = module_dir
print("module pah: ", os.environ['MAYA_MODULE_PATH'])
# pyton_reloader_code = inspect.getsourcelines(modules_reloader.reload_python_modules)[0]
# reloader_button = f'shelfButton -rpt true -i1 "pythonFamily.png" -l {pyton_reloader_code}'
# Open maya 2020 with this
command = " ""evalDeferred \"loadPlugin SteelToolsMaya; print \\\"SteelTools loaded!\\\"\";"
# command = " ""print \\\"SteelTools NOT loaded, do you have the SteelTools.mod inside the dev directory pointing to the right paths?\\\"\";"
subprocess.Popen([MAYA_EXE, '-command', command, ' -noAutoloadPlugins'])
# subprocess.Popen([MAYA_EXE, ' -noAutoloadPlugins'])