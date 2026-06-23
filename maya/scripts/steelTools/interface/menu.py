import maya.cmds as cmds
import maya.mel as mel

import steelTools.interface.sticky_lips_creator as sl

MENU_NAME = 'steelToolsMenuBar'
MENU_TITLE = 'Steel Tools'


def steel_menu():

    if cmds.menu(MENU_NAME, exists=True):
        cmds.deleteUI(MENU_NAME)

    main_window = mel.eval('$temp1=$gMainWindow')

    menu_bar = cmds.menu(MENU_NAME, p=main_window, to=True, label=MENU_TITLE)

    cmds.menuItem(parent=menu_bar, divider=True, dividerLabel='Deformers')
    cmds.menuItem(parent=menu_bar, label='Create Sticky Lips', c=create_sticky_lips)
    cmds.menuItem(parent=menu_bar, label='Create Sticky Lips', optionBox=True, c=create_sticky_lips_window)
    cmds.menuItem(parent=menu_bar, label='Create Cornea Push', c=create_cornea_push)
    cmds.menuItem(parent=menu_bar, label='Create Cornea Push', optionBox=True, c=create_cornea_push_window)

def remove_menu():
    cmds.deleteUI(MENU_NAME)

def create_sticky_lips(*args):
    sl.StickyLipsCreator().create_deformer()

def create_sticky_lips_window(*args):
    sl.StickyLipsCreator().show_ui()

def create_cornea_push(*args):
    ...

def create_cornea_push_window(*args):
    ...