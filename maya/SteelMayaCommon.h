#ifndef STEEL_MAYA_COMMON_H
#define STEEL_MAYA_COMMON_H

#include <maya/MTypeId.h>
#include <maya/MString.h>

namespace SteelMaya {
    // Reserved range for personal/internal use is often 0x00000000 - 0x0007ffff
    // For this project we'll use a prefix
    const unsigned int kSteelNodeIdPrefix = 0x87000;

    const MTypeId kStickyLipsNodeId(kSteelNodeIdPrefix | 0x01);
    const MString kStickyLipsNodeName = "steelStickyLips";

    const MTypeId kCorneaPushNodeId(kSteelNodeIdPrefix | 0x02);
    const MString kCorneaPushNodeName = "steelCorneaPush";
}

#endif // STEEL_MAYA_COMMON_H
