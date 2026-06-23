from PySide6 import QtCore
from PySide6.QtCore import QSettings
from typing import Union

class SettingsHelper:
    """Small helper to have persistent values because option var is just noooo"""

    ORGANIZATION = "EastSide Software"
    APPLICATION = "SteelToolsMaya"

    def __init__(self, group_name: str):
        # Initialize QSettings
        self.settings = QSettings(self.ORGANIZATION, self.APPLICATION)
        self._group_name = group_name

    def get_value(self, key: str, default: Union[str, float, int, bool]) -> Union[str, float, int, bool]:

        self.settings.beginGroup(self._group_name)

        if isinstance(default, str):
            value = self.settings.value(key, default, type=str)
        elif isinstance(default, bool):
            # QSettings stores bool as string, handle conversion
            value = self.settings.value(key, str(default), type=str)
            value = value.lower() in ('true', '1', 'yes')
        elif isinstance(default, float):
            value = self.settings.value(key, default, type=float)
        elif isinstance(default, int):
            value = self.settings.value(key, default, type=int)
        else:
            value = self.settings.value(key, default)

        # End the group scope
        self.settings.endGroup()
        return value

    def set_value(self, key: str, value: Union[str, float, int, bool]):
        self.settings.beginGroup(self._group_name)

        # Set the value
        self.settings.setValue(key, value)

        # End the group scope and sync to disk
        self.settings.endGroup()
        self.settings.sync()