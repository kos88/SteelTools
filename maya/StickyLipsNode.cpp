//
// Created by Costantino Fracas on 24/05/2026.
//

#include "StickyLipsNode.h"
#include "SteelMayaCommon.h"

#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnIntArrayData.h>
#include <maya/MGlobal.h>

MTypeId StickyLipsNode::id = SteelMaya::kStickyLipsNodeId;
MString StickyLipsNode::name = SteelMaya::kStickyLipsNodeName;

MObject StickyLipsNode::aForwardInfluence;
MObject StickyLipsNode::aBackwardInfluence;
MObject StickyLipsNode::aDirection;
MObject StickyLipsNode::aEdgeLoop;


void* StickyLipsNode::creator() {
    return new StickyLipsNode();
}

MStatus StickyLipsNode::initialize() {
    MFnNumericAttribute nAttr;
    MFnTypedAttribute tAttr;

    aForwardInfluence = nAttr.create("forwardInfluence", "fwd", MFnNumericData::kFloat, 0.0);
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    addAttribute(aForwardInfluence);
    attributeAffects(aForwardInfluence, outputGeom);

    aBackwardInfluence = nAttr.create("backwardInfluence", "bwd", MFnNumericData::kFloat, 0.0);
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    addAttribute(aBackwardInfluence);
    attributeAffects(aBackwardInfluence, outputGeom);

    aDirection = nAttr.create("direction", "dir", MFnNumericData::kFloat, 0.0);
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    addAttribute(aDirection);
    attributeAffects(aDirection, outputGeom);

    // Using IntArray for edge loop indices
    aEdgeLoop = tAttr.create("edgeLoop", "elp", MFnData::kIntArray);
    tAttr.setStorable(true);
    addAttribute(aEdgeLoop);
    attributeAffects(aEdgeLoop, outputGeom);

    return MS::kSuccess;
}

MStatus StickyLipsNode::deform(MDataBlock& block,
                              MItGeometry& iter,
                              const MMatrix& mat,
                              unsigned int multiIndex) {
    
    float envelope = block.inputValue(MPxDeformerNode::envelope).asFloat();
    if (envelope == 0.0f) return MS::kSuccess;

    float fwd = block.inputValue(aForwardInfluence).asFloat();
    float bwd = block.inputValue(aBackwardInfluence).asFloat();
    float dir = block.inputValue(aDirection).asFloat();

    MGlobal::displayInfo(MString("StickyLipsNode deforming: fwd=") + fwd + " bwd=" + bwd + " dir=" + dir);

    return MS::kSuccess;
}
