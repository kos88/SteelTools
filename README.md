# Steel Tools

The project aims to give a set of tools currently focused to face rigging.

At the moment is **all developed for Maya**, but there is the potential to abstract the core concepts and make the 
deformers available to multiple DCCs and or engines.

### Sticky Lips

One click setup to create secondary animation on character lips.

### Cornea Push

Thickness preserving push deformer to simulate cornea push on eyelids

---
# How to build

You need to have installed
- Cmake >= 3.10
- Maya >= 2023
- C++ compiler needed for the related maya version

The following variables must be specified to build for Maya:

```shell
-DBUILD_MAYA=TRUE
-DMAYA_VERSION=2028
-DCMAKE_INSTALL_PREFIX="../release"
```

## Cmake install

When developing the **Debug** build will create a .mod file that points to [folder](maya/scripts)
You can edit the python interface and use the [file](scripts/MayaPythonReloader.py) to refresh them.

When the build is in **Release** the script folder will be copied to the install directory, ready to be deployed.

## Deploy
A windows installer is provided through inno setup. https://jrsoftware.org/isinfo.php
You can build an installer for the compiled versions with the helper [file](scripts/buildAllMaya.bat)



