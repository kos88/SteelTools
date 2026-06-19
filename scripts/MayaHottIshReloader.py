"""
Fake hot-reload helper for Maya plugin development.

Run this inside Maya's Script Editor (Python tab) or from a shelf button.


    import runpy
    runpy.run_path(r"path to ...MayaHotishReloader.py", run_name="__main__")


What it does:
    - Small settings window with a scene file path, a cmake build command,
      a "Rebuild" button, and a "rebuild on focus" checkbox.
    - Clicking Rebuild (or focusing Maya's main window, if the checkbox is on)
      runs: close scene -> unload plugin -> build -> load plugin -> reopen scene.
"""

import subprocess

import maya.cmds as cmds
import maya.OpenMayaUI as omui

from PySide6 import QtCore, QtWidgets
from shiboken6 import wrapInstance

SETTINGS_ORG = "hot_reload_tool"
SETTINGS_APP = "hot_reload_tool"


# -------------------------------------------------- Maya helpers ---------------------------------------------------- #
def close_scene():
    """Unload the current scene (discarding changes)."""
    cmds.file(new=True, force=True)


def unload_plugin(plugin_name: str):
    """Forcefully unload the plugin so the dll/so can be rebuilt."""
    if cmds.pluginInfo(plugin_name, query=True, loaded=True):
        cmds.unloadPlugin(plugin_name, force=True)


def load_plugin(plugin_name: str):
    """Load the freshly built plugin."""
    cmds.loadPlugin(plugin_name)


def reopen_scene(file_path: str):
    """Reopen the scene that contains the plugin data."""
    if file_path:
        cmds.file(file_path, open=True, force=True)


# -------------------------------------------------------------------------------------------------------------------- #
def maya_main_window():
    ptr = omui.MQtUtil.mainWindow()
    return wrapInstance(int(ptr), QtWidgets.QWidget)


def current_project_key():
    """Use the current Maya project path as a settings group, so each project remembers its own values."""
    project = cmds.workspace(query=True, rootDirectory=True)
    return project.replace("/", "_").replace(":", "_").replace("\\", "_")


def run_build_command(command):
    """Run the cmake build command, blocking, printing output to the script editor."""
    print(f"[hot_reload] running: {command}")
    result = subprocess.run(command, shell=True, capture_output=True, text=True)
    print(result.stdout)
    if result.returncode != 0:
        print(result.stderr)
        cmds.warning("[hot_reload] build failed, see script editor for details")
        return False
    return True


def do_rebuild(plugin_name: str, file_path: str, build_command):
    """The full sequence: close scene, unload plugin, build, load plugin, reopen scene."""
    close_scene()
    unload_plugin(plugin_name)

    if not run_build_command(build_command):
        return

    load_plugin(plugin_name)
    reopen_scene(file_path)
    print("[hot_reload] done")


class HottishReloadTool(QtWidgets.QDialog):

    def __init__(self, parent=maya_main_window()):
        super().__init__(parent)
        self.setWindowTitle("Hottish Reload")
        self.setMinimumWidth(420)

        self.plugin_field = QtWidgets.QLineEdit()
        self.plugin_field.setPlaceholderText("Plugin name (no extension)")

        self.file_field = QtWidgets.QLineEdit()
        self.file_field.setPlaceholderText("Scene to reopen (.ma / .mb)")
        browse_btn = QtWidgets.QPushButton("...")
        browse_btn.setFixedWidth(30)
        browse_btn.clicked.connect(self.pick_file)
        this_file_btn = QtWidgets.QPushButton("This file")
        this_file_btn.clicked.connect(self.use_current_file)

        self.build_field = QtWidgets.QLineEdit(
            "cmake --build build --target install"
        )
        build_browse_btn = QtWidgets.QPushButton("...")
        build_browse_btn.setFixedWidth(30)
        build_browse_btn.clicked.connect(self.pick_build_dir)

        self.rebuild_btn = QtWidgets.QPushButton("Rebuild")
        self.rebuild_btn.clicked.connect(self.on_rebuild_clicked)
        self.focus_checkbox = QtWidgets.QCheckBox("Rebuild on focus")

        # Single form layout for all rows — ensures aligned labels and fields
        form = QtWidgets.QFormLayout()

        # Plugin row (no extra buttons, but in a row to maintain alignment)
        plugin_row = QtWidgets.QHBoxLayout()
        plugin_row.addWidget(self.plugin_field)
        plugin_row.addStretch()  # keeps the field from stretching weirdly
        form.addRow("Plugin Name", plugin_row)

        # File row
        file_row = QtWidgets.QHBoxLayout()
        file_row.addWidget(self.file_field)
        file_row.addWidget(browse_btn)
        file_row.addWidget(this_file_btn)
        form.addRow("Reopen file", file_row)

        # Build row
        build_row = QtWidgets.QHBoxLayout()
        build_row.addWidget(self.build_field)
        build_row.addWidget(build_browse_btn)
        form.addRow("Build command", build_row)

        trigger_row = QtWidgets.QHBoxLayout()
        trigger_row.addWidget(self.rebuild_btn)
        trigger_row.addWidget(self.focus_checkbox)
        trigger_row.addStretch()

        layout = QtWidgets.QVBoxLayout(self)
        layout.addLayout(form)
        layout.addLayout(trigger_row)

        # Detect Maya regaining focus.
        parent.installEventFilter(self)

        self.settings = QtCore.QSettings(SETTINGS_ORG, SETTINGS_APP)
        self.load_settings()

        # Persist whenever a value changes.
        self.plugin_field.textChanged.connect(self.save_settings)
        self.file_field.textChanged.connect(self.save_settings)
        self.build_field.textChanged.connect(self.save_settings)
        self.focus_checkbox.toggled.connect(self.save_settings)

    def settings_key(self, name):
        return f"{current_project_key()}/{name}"

    def load_settings(self):
        self.plugin_field.setText(self.settings.value(self.settings_key("plugin_name")))
        self.file_field.setText(self.settings.value(self.settings_key("reopen_file"), ""))
        self.build_field.setText(self.settings.value(self.settings_key("build_command"),
                                                     "cmake --build build --target install"))
        rebuild_on_focus = self.settings.value(self.settings_key("rebuild_on_focus"), False)
        self.focus_checkbox.setChecked(rebuild_on_focus in (True, "true", "1"))

    def save_settings(self):
        self.settings.setValue(self.settings_key("reopen_file"), self.file_field.text())
        self.settings.setValue(self.settings_key("build_command"), self.build_field.text())
        self.settings.setValue(self.settings_key("rebuild_on_focus"), self.focus_checkbox.isChecked())
        self.settings.setValue(self.settings_key("plugin_name"), self.plugin_field.text())

    def pick_file(self):
        path, _ = QtWidgets.QFileDialog.getOpenFileName(self, "Select scene", "", "Maya Files (*.ma *.mb)")
        if path:
            self.file_field.setText(path)

    def use_current_file(self):
        path = cmds.file(query=True, sceneName=True)
        if path:
            self.file_field.setText(path)
        else:
            cmds.warning("[hot_reload] current scene has no file path yet (unsaved)")

    def pick_build_dir(self):
        directory = QtWidgets.QFileDialog.getExistingDirectory(self, "Select build directory")
        if directory:
            self.build_field.setText(f"cmake --build {directory} --target install")

    def eventFilter(self, watched, event):
        if event.type() == QtCore.QEvent.WindowActivate:
            if self.focus_checkbox.isChecked():
                self.on_rebuild_clicked()
        return super().eventFilter(watched, event)

    def on_rebuild_clicked(self):
        do_rebuild(self.plugin_field.text(), self.file_field.text(), self.build_field.text())


def show():
    global _hot_reload_tool
    try:
        _hot_reload_tool.close()
    except NameError:
        pass
    _hot_reload_tool = HottishReloadTool()
    _hot_reload_tool.show()


if __name__ == "__main__":
    show()