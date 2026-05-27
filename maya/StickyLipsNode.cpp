//
// Created by Costantino Fracas on 24/05/2026.
//

#include "StickyLipsNode.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "SteelMayaCommon.h"

#include <maya/MItMeshEdge.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MMatrix.h>
#include <maya/MPoint.h>
#include <maya/MFnMeshData.h>
#include <maya/MItGeometry.h>
#include <maya/MFnMesh.h>
#include <maya/MPointArray.h>
#include <maya/MFnIntArrayData.h>
#include <maya/MFnSingleIndexedComponent.h>
#include <maya/MGlobal.h>

#include "DebugUtils.h"

MTypeId StickyLipsNode::id = SteelMaya::kStickyLipsNodeId;
MString StickyLipsNode::name = SteelMaya::kStickyLipsNodeName;

MObject StickyLipsNode::s_stickyAmount;
MObject StickyLipsNode::s_stickyFalloff;
MObject StickyLipsNode::s_distanceThreshold;
MObject StickyLipsNode::s_ForwardInfluence;
MObject StickyLipsNode::s_BackwardInfluence;
MObject StickyLipsNode::s_Direction;
MObject StickyLipsNode::s_EdgeLoopA;
MObject StickyLipsNode::s_EdgeLoopB;

MString StickyLipsNode::s_upperComponentTag = "upperEdge";
MString StickyLipsNode::s_lowerComponentTag = "lowerEdge";


constexpr bool build_debug_curves = false;


constexpr bool __build_debug_curves =
    #ifdef NDEBUG
    build_debug_curves;
    #else
    false;
    #endif

void* StickyLipsNode::creator() {
    return new StickyLipsNode();
}

MStatus StickyLipsNode::initialize() {
    MFnNumericAttribute nAttr;
    MFnTypedAttribute tAttr;

    s_distanceThreshold = nAttr.create("distanceThreshold", "dth", MFnNumericData::kFloat, 1.0);
    nAttr.setMin(0.0);
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    addAttribute(s_distanceThreshold);
    attributeAffects(s_distanceThreshold, outputGeom);

    s_stickyFalloff = nAttr.create("stickyFalloff", "sfl", MFnNumericData::kFloat, 1.0);
    nAttr.setMin(0.0);
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    addAttribute(s_stickyFalloff);
    attributeAffects(s_stickyFalloff, outputGeom);

    s_stickyAmount = nAttr.create("stickyAmount", "sta", MFnNumericData::kFloat, 1.0);
    nAttr.setMin(0.0);
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    addAttribute(s_stickyAmount);
    attributeAffects(s_stickyAmount, outputGeom);

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

    float stickyThreshold = block.inputValue(s_distanceThreshold).asFloat();
    float stickyFalloff = block.inputValue(s_stickyFalloff).asFloat();
    float stickyAmount = block.inputValue(s_stickyAmount).asFloat();

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
    const MFnGeometryData::SubsetState state = getGeometryIterator(subIter,
                                                                   block,
                                                                   currentMeshHandle,
                                                                   multiIndex
                                                                   );
    if (state == MFnGeometryData::kCompleteGroup) {
        MGlobal::displayWarning("All geo provided.. skipping"); return MS::kFailure;
    } if (state == MFnGeometryData::kInvalidGroup) {
        MGlobal::displayError("Fuck my life...invalid!"); return MS::kFailure;
    } if (state == MFnGeometryData::kEmptyGroup) {
        MGlobal::displayError("Fuck my life...NO geo?"); return MS::kFailure;
    }

    // // Get tag names
    // const MString tagA = block.inputValue(s_EdgeLoopA).asString();
    // const MString tagB = block.inputValue(s_EdgeLoopB).asString();

    MFnMesh                 fnMesh(meshObj);
    const MFnGeometryData   meshData(meshObj, &returnStatus);

    if (returnStatus != MS::kSuccess) {
        MGlobal::displayError("Shit happens..."); return returnStatus;
    }

    // MStringArray keysObj;
    // meshData.componentTags(keysObj);
    // MGlobal::displayInfo("Component tags found...");
    // for (auto& element: keysObj)
    // {
    //     MGlobal::displayInfo(element);
    // }

    // Validate component and types
    const bool hasUpper = meshData.hasComponentTag(s_upperComponentTag);
    const bool hasLower = meshData.hasComponentTag(s_lowerComponentTag);
    if (!hasUpper || !hasLower)
    {
        if (hasLower)
            MGlobal::displayWarning(MString("Missing 1 edge component tag named: ") + s_upperComponentTag);
        if (hasUpper)
            MGlobal::displayWarning(MString("Missing 1 edge component tag named:") + s_lowerComponentTag);
        else
            MGlobal::displayWarning(MString("Missing 2 edge component tag named:") + s_upperComponentTag + " and " + s_lowerComponentTag);

        return MS::kFailure;
    }
    if (meshData.componentTagType(s_upperComponentTag) != MFn::kMeshEdgeComponent) {
        MGlobal::displayWarning(MString("Component tag:") + s_upperComponentTag + " kMeshEdgeComponent");
        return MS::kFailure;
    }
    if (meshData.componentTagType(s_lowerComponentTag) != MFn::kMeshEdgeComponent) {
        MGlobal::displayWarning(MString("Component tag:") + s_lowerComponentTag + " kMeshEdgeComponent");
        return MS::kFailure;
    }

    // Extract component data
    MObject upperEdgeObj = meshData.componentTagContents(s_upperComponentTag);
    MObject lowerEdgeObj = meshData.componentTagContents(s_lowerComponentTag);

    // const MFnSingleIndexedComponent fnUpper(upperEdgeObj);
    // const MFnSingleIndexedComponent fnLower(lowerEdgeObj);
    //
    // MIntArray upperIndices, lowerIndices;
    // fnUpper.getElements(upperIndices);
    // fnLower.getElements(lowerIndices);

    // Reorder based on connectivity
    auto createOrderedVertices = [&](MObject& edgeObj)
    {
        struct Neighbours { int a = -1, b = -1; };
        std::unordered_map<int, Neighbours> adj;

        MItMeshEdge edgeIter(meshObj, edgeObj);
        for (; !edgeIter.isDone(); edgeIter.next())
        {
            int v0 = edgeIter.index(0);
            int v1 = edgeIter.index(1);

            auto& vertexA_adj = adj[v0];
            if (vertexA_adj.a == -1) vertexA_adj.a = v1;
            else vertexA_adj.b = v1;

            auto& vertexB_adj = adj[v1];
            if (vertexB_adj.a == -1) vertexB_adj.a = v0;
            else vertexB_adj.b = v0;
        }

        // Find start: endpoint has only one neighbour
        int start = adj.begin()->first;
        for (auto& [v, n] : adj)
            if (n.b == -1) { start = v; break; }

        // Walk
        std::vector<MVector> orderedPts;
        std::vector<int> originalIndexes;
        orderedPts.reserve(adj.size());

        int prev = -1;
        int cur  = start;
        while (cur != -1)
        {
            MPoint pt;
            fnMesh.getPoint(cur, pt, MSpace::kObject);

            // soa output
            orderedPts.emplace_back(pt);
            originalIndexes.push_back(cur);

            auto [nA, nB] = adj[cur];
            int next = (nA != prev) ? nA : nB;
            if (next == start) next = -1;
            prev = cur;
            cur  = next;
        }

        return std::make_pair(orderedPts, originalIndexes);

    };

    // ---------------- From here, we can probably move it outside of maya, if we want it generic and portable to other DCC
    auto pts_a = createOrderedVertices(upperEdgeObj);
    auto pts_b = createOrderedVertices(lowerEdgeObj);

    if (__build_debug_curves)
    {
        if (pts_a.first.size())
            DebugUtils::createDebugCurve("up", pts_a.first, true);
        if (pts_b.first.size())
            DebugUtils::createDebugCurve("down", pts_b.first, true);
    }

    // We now resample the segment with LESS points to have a even point number on both sides.
    // the side without resample, will simply "snap" on the middle line.
    // the side with the resample, will snap to the closest point of the middle line.

    // *-------*-------*-------*-------* << Original A, more points
    // |       |       |       |       |
    // *-------*-------*-------*-------* Original B, less points
    // |        \       \             /
    // *---------*--------*----------* Original B, less points


    // A-B data
    std::vector<MVector>* orderedPositionsUpEdge = &pts_a.first;
    std::vector<int>* orderedIndexToMeshIndexUpEdge = &pts_a.second;

    std::vector<MVector>* orderedPositionsBottomEdge = &pts_b.first;
    std::vector<int>* orderedIndexToMeshIndexBottomEdge = &pts_b.second;


    std::vector<int> resampleOriginalIndex; // resampled point > original orderedPts pos or originalIndexes mesh index
    std::vector<int> originalToResampleIndex; // from the original point, to the closest point on the resampled. Since we ALWAYS increase, we always have a fairly easy match.

    // Create pointers and numbers for larger and smaller
    auto* larger  = orderedPositionsUpEdge->size() >= orderedPositionsBottomEdge->size() ? &pts_a  : &pts_b;
    auto* smaller = orderedPositionsUpEdge->size() <  orderedPositionsBottomEdge->size() ? &pts_a  : &pts_b;
    const int largerCount  = static_cast<int>(larger->first.size());
    const int smallerCount = static_cast<int>(smaller->first.size());

    resampleOriginalIndex.resize(largerCount);
    originalToResampleIndex.resize(smallerCount, -1);

    // The virtual segment in between
    std::vector<MVector> averagedPositions(largerCount);

    for (int largerIdx = 0; largerIdx < largerCount; ++largerIdx)
    {
        double lerpT      = static_cast<double>(largerIdx) * (smallerCount - 1) / (largerCount - 1);
        int smallerIdx    = static_cast<int>(std::floor(lerpT));
        smallerIdx        = std::clamp(smallerIdx, 0, smallerCount - 2);
        double remainder  = lerpT - smallerIdx;

        resampleOriginalIndex[largerIdx]     = smallerIdx;
        originalToResampleIndex[smallerIdx]  = largerIdx;

        MVector resampledSmaller = smaller->first[smallerIdx] + (smaller->first[smallerIdx + 1] - smaller->first[smallerIdx]) * remainder;

        averagedPositions[largerIdx] = (larger->first[largerIdx] + resampledSmaller) * 0.5;
    }

    if (__build_debug_curves)
        DebugUtils::createDebugCurve("middle", averagedPositions, true);

    std::unordered_map<int, MVector> mid_pos; // we need to rethink this as we can probably generate some weights directly?

    // Now for each point A-Mid-B decide the position
    for (int x = 0; x < largerCount; x++)
    {
        MVector& posA = larger->first[x];
        int posAMeshIndex = larger->second[x];

        int smallerIdx = resampleOriginalIndex[x];
        MVector& posB  = smaller->first[smallerIdx];
        int posBMeshIndex = smaller->second[smallerIdx];

        MVector& posM = averagedPositions[x];

        // We'll likely not need the real distance.. or maybe we can just have the distance2?
        double distance = std::sqrt(std::pow((posA.x - posB.x), 2) + std::pow((posA.y - posB.y), 2) + std::pow((posA.z - posB.z), 2));

        // double blendVal = std::clamp(distance / stickyThreshold, 0.0, 1.0);
        // double blendVal;
        // if (distance < 1e-4)
        // {
        //     blendVal = 0.0; // force full seal
        // }
        // else
        // {
        // }
        double blendVal = std::pow(std::clamp(distance / stickyThreshold, 0.0, 1.0), stickyFalloff);

        MVector blendedA = posM + blendVal * (posA - posM);
        MVector blendedB = posM + blendVal * (posB - posM);

        mid_pos[posAMeshIndex] = blendedA;
        mid_pos[posBMeshIndex] = blendedB;



    }


    // // At this stage, we should probably just move the result, multiplied by direction vector * weight
    // for (; !subIter.isDone(); subIter.next()) // Iterate through points of the geo subset
    // {
    //     auto meshVtxIndex = subIter.index();
    //     // MPoint pt = subIter.position();
    //     // MGlobal::displayInfo(MString("mesh idx: ") + subIter.index());
    //
    //     if (mid_pos.contains(meshVtxIndex))
    //     {
    //         const auto& newP = mid_pos[meshVtxIndex];
    //         subIter.setPosition({0, 0, 0});
    //         MGlobal::displayInfo(MString("moving mesh idx: ") + subIter.index() + " to " + newP.x + ", " + newP.y  + ", " + newP.z);
    //     }
    //     else
    //     {
    //
    //     }
    //
    //
    //     // ... do work ...
    //     // subIter.setPosition(pt);
    // }

    // iterate through each point in the geometry
    //
    int count = 0;
    for ( ; !iter.isDone(); iter.next())
    {
        auto meshVtxIndex = iter.index();
        if (mid_pos.contains(meshVtxIndex))
        {
            const auto& newP = mid_pos[meshVtxIndex];
            iter.setPosition({newP.x, newP.y, newP.z});
            count++;
            // MGlobal::displayInfo(MString("moving mesh idx: ") + subIter.index() + " to " + newP.x + ", " + newP.y  + ", " + newP.z);
        }
    }
    MGlobal::displayInfo(MString("Moved ") + count + " points");


    // //
    //
    // // Iterate through points of the geo subset
    // for (; !subIter.isDone(); subIter.next())
    // {
    //     MPoint pt = subIter.position();
    //     MGlobal::displayInfo(MString("mesh idx: ") + iter.index());
    //
    //     int idx = subIter.index();
    //     bool inUpper = upperIndices.indexOf(idx) >= 0;
    //     bool inLower = lowerIndices.indexOf(idx) >= 0;
    //
    //     MGlobal::displayInfo(MString("idx: ") + idx
    //         + " upper: " + inUpper
    //         + " lower: " + inLower);
    //
    //     // ... do work ...
    //     // subIter.setPosition(pt);
    // }

    // iterate through each point in the geometry
    //
    for ( ; !iter.isDone(); iter.next())
    {
        MPoint pt = iter.position();
        // MGlobal::displayInfo(MString("mesh idx: ") + iter.index());
        // pt *= omatinv;

        const float weight = weightValue(block, multiIndex,iter.index());

        // offset algorithm
        //
        pt.y = pt.y + envelope * weight + 10;
        //
        // end of offset algorithm

        // pt *= omat;
        iter.setPosition(pt);
    }





    return MS::kSuccess;
}
