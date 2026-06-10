# Steel Face Tools

The project aims to give a set of deformers related to face rigging.

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

The following variables are needed to be specified
-DBUILD_MAYA=TRUE
-DMAYA_VERSION=2028
-DCMAKE_INSTALL_PREFIX="../release"



