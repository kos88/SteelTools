//
// Created by Costantino Fracas on 24/05/2026.
//

#include "StickyLipsNode.h"
#include "SteelMayaCommon.h"

#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MMatrix.h>
#include <maya/MPoint.h>
#include <maya/MFnMeshData.h>
#include <maya/MItGeometry.h>
#include <maya/MFnIntArrayData.h>
#include <maya/MGlobal.h>

MTypeId StickyLipsNode::id = SteelMaya::kStickyLipsNodeId;
MString StickyLipsNode::name = SteelMaya::kStickyLipsNodeName;

MObject StickyLipsNode::s_ForwardInfluence;
MObject StickyLipsNode::s_BackwardInfluence;
MObject StickyLipsNode::s_Direction;
MObject StickyLipsNode::s_EdgeLoopA;
MObject StickyLipsNode::s_EdgeLoopB;


void* StickyLipsNode::creator() {
    return new StickyLipsNode();
}

MStatus StickyLipsNode::initialize() {
    MFnNumericAttribute nAttr;
    MFnTypedAttribute tAttr;

    s_ForwardInfluence = nAttr.create("forwardInfluence", "fwd", MFnNumericData::kFloat, 0.0);
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    addAttribute(s_ForwardInfluence);
    attributeAffects(s_ForwardInfluence, outputGeom);

    s_BackwardInfluence = nAttr.create("backwardInfluence", "bwd", MFnNumericData::kFloat, 0.0);
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    addAttribute(s_BackwardInfluence);
    attributeAffects(s_BackwardInfluence, outputGeom);

    s_Direction = nAttr.create("direction", "dir", MFnNumericData::kFloat, 0.0);
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    addAttribute(s_Direction);
    attributeAffects(s_Direction, outputGeom);

    // Using string for edge loop component tags
    s_EdgeLoopA = tAttr.create("edgeLoopA", "elp", MFnData::kString);
    tAttr.setStorable(true);
    addAttribute(s_EdgeLoopA);
    attributeAffects(s_EdgeLoopA, outputGeom);

    s_EdgeLoopB = tAttr.create("edgeLoopB", "elp", MFnData::kString);
    tAttr.setStorable(true);
    addAttribute(s_EdgeLoopB);
    attributeAffects(s_EdgeLoopB, outputGeom);

    return MS::kSuccess;
}

void printIntArray(const MIntArray& printable, const MString& name)
{
    MString output = name + ": [";
    for (unsigned int i = 0; i < printable.length(); i++) {
        output += printable[i];
        if (i < printable.length() - 1) output += ", ";
    }
    output += "]";
    MGlobal::displayInfo(output);
}

MStatus StickyLipsNode::deform(MDataBlock& block,
                               MItGeometry& iter,
                               const MMatrix& mat,
                               unsigned int multiIndex)
{
    MStatus returnStatus;
    // Envelope data from the base class.
    // The envelope is simply a scale factor.
    //
    const MDataHandle envData = block.inputValue(envelope, &returnStatus);
    if (MS::kSuccess != returnStatus)
        return returnStatus;

    const float envelope = envData.asFloat();

    // float fwd = block.inputValue(s_ForwardInfluence).asFloat();
    // float bwd = block.inputValue(s_BackwardInfluence).asFloat();
    // float dir = block.inputValue(s_Direction).asFloat();

    // MObject edgeLoopAObj = block.inputValue(s_EdgeLoopA).data();
    // MFnIntArrayData fnIntArrayDataA(edgeLoopAObj);
    // MIntArray edgeLoopA = fnIntArrayDataA.array();
    //
    // MObject edgeLoopBObj = block.inputValue(s_EdgeLoopB).data();
    // MFnIntArrayData fnIntArrayDataB(edgeLoopBObj);
    // MIntArray edgeLoopB = fnIntArrayDataB.array();
    //
    //
    // MGlobal::displayInfo(MString("StickyLipsNode deforming: fwd=") + fwd + " bwd=" + bwd + " dir=" + dir);
    MGlobal::displayInfo(MString("Deforming at input index...") + multiIndex);


    // Get the matrix which is used to define the direction and scale
    // of the offset.
    //
    // MMatrix omat = matData.asMatrix();
    // MMatrix omatinv = omat.inverse();


    MArrayDataHandle hInputArray  = block.inputArrayValue(input); // Input plug
    hInputArray.jumpToElement(multiIndex); // Input[x] plug
    MDataHandle currentInput = hInputArray.inputValue();
    MDataHandle currentMeshHandle = currentInput.child(inputGeom);
    MObject meshObj = currentMeshHandle.asMesh();

    MItGeometry subIter;
    MFnGeometryData::SubsetState state = getGeometryIterator(subIter, block, currentMeshHandle, multiIndex);
    if (state == MFnGeometryData::kCompleteGroup)
    {
        MGlobal::displayWarning("All geo provided.. skipping");
        return MS::kFailure;
    }
    if (state == MFnGeometryData::kInvalidGroup)
    {
        MGlobal::displayError("Fuck my life...invalid!");
        return MS::kFailure;
    }
    if (state == MFnGeometryData::kEmptyGroup)
    {
        MGlobal::displayError("Fuck my life...NO geo?");
        return MS::kFailure;
    }

    // // Get tag names
    // const MString tagA = block.inputValue(s_EdgeLoopA).asString();
    // const MString tagB = block.inputValue(s_EdgeLoopB).asString();

    const MFnGeometryData meshData(meshObj, &returnStatus);
    if (returnStatus != MS::kSuccess)
    {
        MGlobal::displayError("Wtf man...");
        return returnStatus;
    }

    MStringArray keysObj;
    meshData.componentTags(keysObj);
    MGlobal::displayInfo("Component tags found...");
    for (auto& element: keysObj)
    {
        MGlobal::displayInfo(element);
    }

    if (meshData.hasComponentTag("Gerry_scotty"))
        MGlobal::displayInfo("Bingo.. bongo... stare bene solo in congo");


    // Iterate through points of the geo subset
    for (; !subIter.isDone(); subIter.next())
    {
        MPoint pt = subIter.position();
        MGlobal::displayInfo(MString("mesh idx: ") + iter.index());
        // ... do work ...
        // subIter.setPosition(pt);
    }

    // // iterate through each point in the geometry
    // //
    // for ( ; !iter.isDone(); iter.next())
    // {
    //     MPoint pt = iter.position();
    //     // MGlobal::displayInfo(MString("mesh idx: ") + iter.index());
    //     // pt *= omatinv;
    //
    //     const float weight = weightValue(block, multiIndex,iter.index());
    //
    //     // offset algorithm
    //     //
    //     pt.y = pt.y + envelope * weight + 10;
    //     //
    //     // end of offset algorithm
    //
    //     // pt *= omat;
    //     iter.setPosition(pt);
    // }


    return MS::kSuccess;
}
