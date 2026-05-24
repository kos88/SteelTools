//
// Created by Costantino Fracas on 24/05/2026.
//

#include "CorneaPushNode.h"
#include "SteelMayaCommon.h"

#include <maya/MFnTypedAttribute.h>
#include <maya/MFnMeshData.h>
#include <maya/MGlobal.h>

MTypeId CorneaPushNode::id = SteelMaya::kCorneaPushNodeId;
MString CorneaPushNode::name = SteelMaya::kCorneaPushNodeName;

MObject CorneaPushNode::aUndeformedEyeball;
MObject CorneaPushNode::aDeformedEyeball;

void* CorneaPushNode::creator() {
    return new CorneaPushNode();
}

MStatus CorneaPushNode::initialize() {
    MFnTypedAttribute tAttr;

    aUndeformedEyeball = tAttr.create("undeformedEyeball", "ueb", MFnMeshData::kMesh);
    tAttr.setStorable(true);
    addAttribute(aUndeformedEyeball);
    attributeAffects(aUndeformedEyeball, outputGeom);

    aDeformedEyeball = tAttr.create("deformedEyeball", "deb", MFnMeshData::kMesh);
    tAttr.setStorable(true);
    addAttribute(aDeformedEyeball);
    attributeAffects(aDeformedEyeball, outputGeom);

    return MS::kSuccess;
}

MStatus CorneaPushNode::deform(MDataBlock& block,
                               MItGeometry& iter,
                               const MMatrix& mat,
                               unsigned int multiIndex) {
    
    float envelope = block.inputValue(MPxDeformerNode::envelope).asFloat();
    if (envelope == 0.0f) return MS::kSuccess;

    MGlobal::displayInfo("CorneaPushNode deforming eyelid area");

    return MS::kSuccess;
}
