//
// Created by Costantino Fracas on 24/05/2026.
//

#include "StickyLipsNode.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "SteelMayaCommon.h"

#include <maya/MFnStringData.h>
#include <maya/MItMeshEdge.h>
#include <maya/MItMeshVertex.h>
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


MObject StickyLipsNode::s_stickyAmount; // Seal the lips if below this distance, this is the parameter suitable for anim
MObject StickyLipsNode::s_stickyFalloff;  // The smooth amount on the sticky edge

MObject StickyLipsNode::s_distanceMinThreshold;  // Define the min range where the sticky starts
MObject StickyLipsNode::s_distanceMaxThreshold;  // Define the min range where the sticky ends

MObject StickyLipsNode::s_cornerAutoRelax;  // main influence of corner relax
MObject StickyLipsNode::s_cornerAutoRelaxStartAngle;  // the angle between where the relax starts
MObject StickyLipsNode::s_cornerAutoRelaxEndAngle;  // the angle between where the relax is at it's peak
MObject StickyLipsNode::s_cornerAutoRelaxDistance;  // the percent of half segment the corner relax propagates

MObject StickyLipsNode::s_propagateSmoothness;  // How far from the initial edge loop the surrounding area is affected
MObject StickyLipsNode::s_propagateInfluence; // How smooth the surrounding area behaves

MObject StickyLipsNode::s_EdgeLoopA;
MObject StickyLipsNode::s_EdgeLoopB;

MString StickyLipsNode::s_upperComponentTag = "upperEdge";
MString StickyLipsNode::s_lowerComponentTag = "lowerEdge";

#ifdef NDEBUG
#define DEBUG_PRINT(message) ((void)0)
#else
#define DEBUG_PRINT(message) MGlobal::displayInfo(message)
#endif

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

    s_stickyAmount = nAttr.create("sealDistance", "sta", MFnNumericData::kFloat, 1.0);
    nAttr.setMin(0.0);
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    addAttribute(s_stickyAmount);
    attributeAffects(s_stickyAmount, outputGeom);

    s_distanceMaxThreshold = nAttr.create("maxThreshold", "smxth", MFnNumericData::kFloat, 1.0);
    nAttr.setMin(0.0);
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    addAttribute(s_distanceMaxThreshold);
    attributeAffects(s_distanceMaxThreshold, outputGeom);


    s_distanceMinThreshold = nAttr.create("minThreshold", "smith", MFnNumericData::kFloat, 1.0);
    nAttr.setMin(0.0);
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    addAttribute(s_distanceMinThreshold);
    attributeAffects(s_distanceMinThreshold, outputGeom);

    s_stickyFalloff = nAttr.create("edgeSmooth", "sfl", MFnNumericData::kFloat, 1.0);
    nAttr.setMin(0.0);
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    addAttribute(s_stickyFalloff);
    attributeAffects(s_stickyFalloff, outputGeom);

    s_cornerAutoRelax = nAttr.create("cornerAutoRelax", "carx", MFnNumericData::kFloat, 1.0);
    nAttr.setMin(0.0);
    nAttr.setMax(1.0);
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    addAttribute(s_cornerAutoRelax);
    attributeAffects(s_cornerAutoRelax, outputGeom);

    s_cornerAutoRelaxStartAngle = nAttr.create("autoRelaxStartAngle", "crsa", MFnNumericData::kFloat, 20.0);
    nAttr.setMin(0.0);
    nAttr.setMax(90.0);
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    addAttribute(s_cornerAutoRelaxStartAngle);
    attributeAffects(s_cornerAutoRelaxStartAngle, outputGeom);

    s_cornerAutoRelaxEndAngle = nAttr.create("autoRelaxEndAngle", "crea", MFnNumericData::kFloat, 45.0);
    nAttr.setMin(0.0);
    nAttr.setMax(90.0);
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    addAttribute(s_cornerAutoRelaxEndAngle);
    attributeAffects(s_cornerAutoRelaxEndAngle, outputGeom);

    s_cornerAutoRelaxDistance = nAttr.create("autoRelaxDistance", "crd", MFnNumericData::kFloat, 0.3);
    nAttr.setMin(0.0);
    nAttr.setMax(1.0);
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    addAttribute(s_cornerAutoRelaxDistance);
    attributeAffects(s_cornerAutoRelaxDistance, outputGeom);

    s_propagateSmoothness = nAttr.create("propagateSmoothness", "psm", MFnNumericData::kFloat, 0.0);
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    addAttribute(s_propagateSmoothness);
    attributeAffects(s_propagateSmoothness, outputGeom);

    s_propagateInfluence = nAttr.create("propagateInfluence", "pin", MFnNumericData::kInt, 3);
    nAttr.setMin(0);
    nAttr.setMax(10);
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    addAttribute(s_propagateInfluence);
    attributeAffects(s_propagateInfluence, outputGeom);

    // Using string for edge loop component tags
    MStatus status;
    MFnStringData fnStringData;
    const MObject defaultStringA = fnStringData.create(s_upperComponentTag);
    s_EdgeLoopA = tAttr.create("edgeLoopNameA", "elpa", MFnData::kString, defaultStringA, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status)
    tAttr.setStorable(true);
    addAttribute(s_EdgeLoopA);
    attributeAffects(s_EdgeLoopA, outputGeom);

    const MObject defaultStringB = fnStringData.create(s_lowerComponentTag);
    s_EdgeLoopB = tAttr.create("edgeLoopNameB", "elpb", MFnData::kString, defaultStringB, &status);
    CHECK_MSTATUS_AND_RETURN_IT(status)
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

    const int propagateInfluence = block.inputValue(s_propagateInfluence).asInt();
    const float propagateSmoothness = block.inputValue(s_propagateSmoothness).asFloat();

    const float stickyMaxThreshold = block.inputValue(s_distanceMaxThreshold).asFloat();
    const float stickyMinThreshold = block.inputValue(s_distanceMinThreshold).asFloat();


    const float cornerAutoRelax = block.inputValue(s_cornerAutoRelax).asFloat(); 
    const float cornerAutoRelaxStartAngle = block.inputValue(s_cornerAutoRelaxStartAngle).asFloat();
    const float cornerAutoRelaxEndAngle = block.inputValue(s_cornerAutoRelaxEndAngle).asFloat(); 
    const float cornerAutoRelaxDistance = block.inputValue(s_cornerAutoRelaxDistance).asFloat(); 

    const float stickyFalloff = block.inputValue(s_stickyFalloff).asFloat();
    const float stickyAmount = block.inputValue(s_stickyAmount).asFloat();

    const MString upperEdgeName = block.inputValue(s_EdgeLoopA).asString();
    const MString lowerEdgeName = block.inputValue(s_EdgeLoopB).asString();

    DEBUG_PRINT(MString("Deforming at input index...") + multiIndex);

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
        MGlobal::displayWarning("[Steel Sticky Lips] Deformer needs a component tag to work on an area of the mesh"); return MS::kFailure;
    } if (state == MFnGeometryData::kInvalidGroup) {
        MGlobal::displayError("[Steel Sticky Lips] Invalid component group provided"); return MS::kFailure;
    } if (state == MFnGeometryData::kEmptyGroup) {
        MGlobal::displayError("[Steel Sticky Lips] Invalid geometry. Unable to deform."); return MS::kFailure;
    }

    // // Get tag names
    // const MString tagA = block.inputValue(s_EdgeLoopA).asString();
    // const MString tagB = block.inputValue(s_EdgeLoopB).asString();

    MFnMesh                 fnMesh(meshObj);
    const MFnGeometryData   meshData(meshObj, &returnStatus);

    if (returnStatus != MS::kSuccess) {
        MGlobal::displayError("[Steel Sticky Lips] Unexpected error when initializing geometry accessor"); return returnStatus;
    }

    // MStringArray keysObj;
    // meshData.componentTags(keysObj);
    // MGlobal::displayInfo("Component tags found...");
    // for (auto& element: keysObj)
    // {
    //     MGlobal::displayInfo(element);
    // }

    // Validate component and types
    const bool hasUpper = meshData.hasComponentTag(upperEdgeName);
    const bool hasLower = meshData.hasComponentTag(lowerEdgeName);
    if (!hasUpper || !hasLower)
    {
        if (hasLower)
            MGlobal::displayWarning(MString("[Steel Sticky Lips] Missing 1 edge component tag named: ") + upperEdgeName);
        if (hasUpper)
            MGlobal::displayWarning(MString("[Steel Sticky Lips] Missing 1 edge component tag named:") + lowerEdgeName);
        else
            MGlobal::displayWarning(MString("[Steel Sticky Lips] Missing 2 edge component tag named:") + upperEdgeName + " and " + lowerEdgeName);

        return MS::kFailure;
    }
    if (meshData.componentTagType(upperEdgeName) != MFn::kMeshEdgeComponent) {
        MGlobal::displayWarning(MString("Component tag:") + upperEdgeName + " must be of type kMeshEdgeComponent");
        return MS::kFailure;
    }
    if (meshData.componentTagType(lowerEdgeName) != MFn::kMeshEdgeComponent) {
        MGlobal::displayWarning(MString("Component tag:") + lowerEdgeName + " must be of type kMeshEdgeComponent");
        return MS::kFailure;
    }

    // Extract component data
    MObject upperEdgeObj = meshData.componentTagContents(upperEdgeName);
    MObject lowerEdgeObj = meshData.componentTagContents(lowerEdgeName);

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
    std::vector<MVector>* orderedPositionsBottomEdge = &pts_b.first;

    std::vector<int> resampleOriginalIndex; // resampled point > original orderedPts pos or originalIndexes mesh index
    std::vector<int> originalToResampleIndex; // from the original point, to the closest point on the resampled. Since we ALWAYS increase, we always have a fairly easy match.

    // Create pointers and numbers for larger and smaller
    auto* larger  = orderedPositionsUpEdge->size() >= orderedPositionsBottomEdge->size() ? &pts_a  : &pts_b;
    auto* smaller = orderedPositionsUpEdge->size() <  orderedPositionsBottomEdge->size() ? &pts_a  : &pts_b;
    const int largerCount  = static_cast<int>(larger->first.size());
    const int smallerCount = static_cast<int>(smaller->first.size());

    resampleOriginalIndex.resize(largerCount);
    originalToResampleIndex.resize(smallerCount, -1);

    for (int largerIdx = 0; largerIdx < largerCount; ++largerIdx)
    {
        double lerpT      = static_cast<double>(largerIdx) * (smallerCount - 1) / (largerCount - 1);
        int smallerIdx    = static_cast<int>(std::floor(lerpT));
        smallerIdx        = std::clamp(smallerIdx, 0, smallerCount - 2);
        double remainder  = lerpT - smallerIdx;

        resampleOriginalIndex[largerIdx]     = smallerIdx;
        originalToResampleIndex[smallerIdx]  = largerIdx;

        MVector resampledSmaller = smaller->first[smallerIdx] + (smaller->first[smallerIdx + 1] - smaller->first[smallerIdx]) * remainder;
    }

    // These are all here, but some should be precomputed only once!!
    std::unordered_map<int, MVector> deformedPositions;

    // the "middle" position each edge vertex is moving toward, needed for growing/blending weights
    std::unordered_map<int, MVector> edgeTargetPos;

    // A temp storage where the edge are "sticking" to do a second blur pass
    std::vector<double> blendVals(largerCount);

    double startAngleInfluence = 1.0;
    double endAngleInfluence = 1.0;

    // precompute cumulative arc length along larger loop
    std::vector<double> arcLength(largerCount, 0.0);
    for (int x = 1; x < largerCount; x++)
    {
        MVector delta = larger->first[x] - larger->first[x - 1];
        arcLength[x] = arcLength[x - 1] + delta.length();
    }
    double totalLength = arcLength[largerCount - 1];
    double cornerDistance = (totalLength / 2.0) * cornerAutoRelaxDistance;


    // Remove stickyness based on the angle between corner top-bottom + a portion of half arclen
    // By computing A-A1 to B-B1 (and opposite) we should get a nice float to auto
    // modulate the corner stickiness, that with distance based approaches alone is not enough
    //
    //     ___ A1 _____ C1 ___
    // A  /                   \  C
    //   *                     *
    // B  \___   ______    ___/  D
    //         B1       D1
    if (cornerAutoRelax > 0.0)
    {
        int leftCornerAngleIndex = -1;
        int rightCornerAngleIndex = -1;

        for (int j = 0; j < largerCount; j++)
        {
            if (arcLength[j] <= cornerDistance)
                leftCornerAngleIndex = j;
            if (arcLength[j] >= totalLength - cornerDistance) {
                rightCornerAngleIndex = j;
                break;
            }
        }

        // Compute slopes using startCornerIdx and endCornerIdx
        MVector startDirA = (larger->first[leftCornerAngleIndex] - larger->first[0]).normal();
        MVector startDirB = (smaller->first[resampleOriginalIndex[leftCornerAngleIndex]] - smaller->first[resampleOriginalIndex[0]]).normal();
        double startSlopeDot = std::clamp(startDirA * startDirB, -1.0, 1.0);
        double startSlopeAngle = std::acos(startSlopeDot) * 90.0 / M_PI; // yes half angle!

        MVector endDirA = (larger->first[largerCount-1] - larger->first[rightCornerAngleIndex]).normal();
        MVector endDirB = (smaller->first[resampleOriginalIndex[largerCount-1]] - smaller->first[resampleOriginalIndex[rightCornerAngleIndex]]).normal();
        double endSlopeDot = std::clamp(endDirA * endDirB, -1.0, 1.0);
        double endSlopeAngle = std::acos(endSlopeDot) * 90.0 / M_PI; // yes half angle!

        // // Create a multiplier
        // double startAngleInfluence = 1.0 - ((startSlopeDot + 1.0) / 2.0);
        // double endAngleInfluence = 1.0 - ((endSlopeDot + 1.0) / 2.0);

        // // Create multipliers based on start-end angle ranges
        startAngleInfluence = std::clamp((startSlopeAngle - cornerAutoRelaxStartAngle) / (cornerAutoRelaxEndAngle - cornerAutoRelaxStartAngle), 0.0, 1.0);
        endAngleInfluence = std::clamp((endSlopeAngle - cornerAutoRelaxStartAngle) / (cornerAutoRelaxEndAngle - cornerAutoRelaxStartAngle), 0.0, 1.0);

        // MGlobal::displayInfo(MString("Start angle: ") + startSlopeAngle + " >" + startAngleInfluence + " End angle: " + endSlopeAngle + " >" + endAngleInfluence);
        // MGlobal::displayInfo(MString("Start angle mult: ") + startAngleInfluence + " End angle mult: " + endAngleInfluence);
        //
        // // Debug output
        // MGlobal::displayInfo(MString("Left idx: ") + leftCornerAngleIndex +
        //                      MString(" (dist: ") + arcLength[leftCornerAngleIndex] +
        //                      MString("), Right idx: ") + rightCornerAngleIndex +
        //                      MString(" (dist: ") + (totalLength - arcLength[rightCornerAngleIndex]) +
        //                      MString(")"));
        //
        // MGlobal::displayInfo(MString("Left B idx: ") + resampleOriginalIndex[leftCornerAngleIndex] +
        //                      MString(", Right B idx: ") + resampleOriginalIndex[rightCornerAngleIndex]);
    }



    // Now for each point A-Mid-B decide the position
    for (int x = 0; x < largerCount; x++)
    {
        const MVector& posA = larger->first[x];

        const int smallerIdx = resampleOriginalIndex[x];
        const MVector& posB  = smaller->first[smallerIdx];

        double cornerRelaxValue = 0;

        // Simple distance base seal or corner relax enanched
        if (cornerAutoRelax > 0.0)
        {
            // distance from nearest end (symmetric)
            double distFromEnd = std::min(arcLength[x], totalLength - arcLength[x]);

            // Normalize it to arc-len and clamp to our corner area
            double cornerAreaInfluence = std::max(0.0, 1.0 - (distFromEnd / cornerDistance));

            // Get normalized position along the curve (0 = start, 1 = end)
            double t = arcLength[x] / totalLength;

            // Blend between start and end angles based on position (angle influence is 0 when closed, towards 1 when closed)
            double targetAngle = (1.0 - t) * startAngleInfluence + t * endAngleInfluence;

            // Now multiply it by the actual angle influence (0-1) and by the angle corner auto relax
            // We will have a gradual value from 0 to 1, guided from the angle between at corners, blending towards the middle
             cornerRelaxValue = cornerAreaInfluence * targetAngle * cornerAutoRelax;

            // MGlobal::displayInfo(MString("Index: ") + x + " distVal " + distFromEnd + " influence: " + cornerAreaInfluence + " relax: " + cornerRelaxValue);
        }

        // Distance between A-B point to control general influence
        double distanceFromMid = std::sqrt(std::pow((posA.x - posB.x), 2) + std::pow((posA.y - posB.y), 2) + std::pow((posA.z - posB.z), 2));

        // ------- seems controllable
        double range = stickyMaxThreshold - stickyMinThreshold;
        double stickyMinRange = stickyMinThreshold - (range + (1.0 - stickyAmount)) - (cornerRelaxValue*2);   // Distance where stickiness starts (min range)
        double stickyMaxRange = stickyMaxThreshold - (range + (1.0 - stickyAmount)) - (cornerRelaxValue*2);   // Distance where stickiness is maxed out (max range)


        // Map distance to a 0-1 signal based on min/max range
        double blendValue = std::clamp(
            (distanceFromMid - stickyMaxRange) / (stickyMinRange - stickyMaxRange),
            0.0,
            1.0
        );

        // Store for blur
        blendVals[x] = blendValue;


    }


    // Sequentially blur the values with a small gaussian like kernel of before and after
    int passes = std::max(1, static_cast<int>(stickyFalloff * 3));  // 0->1, 1->3 passes
    double strength = std::min(1.0, stickyFalloff * 2.0);  // intensity per pass

    for (int p = 0; p < passes; p++)
    {
        std::vector<double> blurred = blendVals;
        for (int x = 0; x < largerCount; x++)
        {
            int left = std::max(x - 1, 0);
            int right = std::min(x + 1, largerCount - 1);

            double smoothed = blendVals[left] * 0.25 +
                              blendVals[x] * 0.5 +
                              blendVals[right] * 0.25;

            blurred[x] = blendVals[x] * (1.0 - strength) + smoothed * strength;
        }

        blendVals = blurred;
    }


    // Compute sticky edge deformed positions and targets for weight propagation
    for (int x = 0; x < largerCount; x++)
    {
        MVector& posA = larger->first[x];
        int posAMeshIndex = larger->second[x];

        int smallerIdx = resampleOriginalIndex[x];
        MVector& posB  = smaller->first[smallerIdx];
        int posBMeshIndex = smaller->second[smallerIdx];

        double blendVal = blendVals[x];

        MVector posM = (posA + posB) * 0.5;
        MVector blendedA = posA + blendVal * (posM - posA);
        MVector blendedB = posB + blendVal * (posM - posB);

        deformedPositions[posAMeshIndex] = blendedA;
        deformedPositions[posBMeshIndex] = blendedB;

        // Store the full 100% target (posM) for neighbor propagation
        edgeTargetPos[posAMeshIndex] = blendedA;
        edgeTargetPos[posBMeshIndex] = blendedB;
    }

    // ------------------ blend weights
    std::unordered_map<int, double> vertexWeights;
    std::unordered_map<int, MVector> vertexTargets;
    std::unordered_set<int> edgeVertices;

    for (int x = 0; x < largerCount; x++)
    {
        int posAMeshIndex = larger->second[x];
        int posBMeshIndex = smaller->second[resampleOriginalIndex[x]];

        vertexWeights[posAMeshIndex] = 1.0;
        vertexWeights[posBMeshIndex] = 1.0;
        vertexTargets[posAMeshIndex] = edgeTargetPos[posAMeshIndex];
        vertexTargets[posBMeshIndex] = edgeTargetPos[posBMeshIndex];
        edgeVertices.insert(posAMeshIndex);
        edgeVertices.insert(posBMeshIndex);
    }

    int propagationPasses = propagateInfluence;
    double propagationFalloff = propagateSmoothness;
    MItMeshVertex neighborIter(meshObj);

    for (int pass = 0; pass < propagationPasses; pass++)
    {
        std::unordered_map<int, double>  newWeights = vertexWeights;
        std::unordered_map<int, MVector> newTargets = vertexTargets;

        for (const auto& [idx, weight] : vertexWeights)
        {
            if (weight <= 0.01)
                continue;

            int prevIdx = 0;
            neighborIter.setIndex(idx, prevIdx);
            MIntArray neighborList;
            neighborIter.getConnectedVertices(neighborList);

            for (int i = 0; i < static_cast<int>(neighborList.length()); i++)
            {
                int neighborIdx = neighborList[i];

                // never overwrite edge vertices!!
                if (edgeVertices.contains(neighborIdx))
                    continue;

                double propagatedWeight = weight * propagationFalloff;
                auto it = newWeights.find(neighborIdx);
                if (it == newWeights.end() || propagatedWeight > it->second)
                {
                    newWeights[neighborIdx] = propagatedWeight;
                    newTargets[neighborIdx] = vertexTargets[idx];
                }
            }
        }
        vertexWeights = std::move(newWeights);
        vertexTargets = std::move(newTargets);
    }

    // Apply
    MItMeshVertex vIt(meshObj);
    for (const auto& [idx, weight] : vertexWeights)
    {
        if (edgeVertices.count(idx)) continue;

        int prevIdx = 0;
        vIt.setIndex(idx, prevIdx);
        MVector originalPos(vIt.position(MSpace::kWorld));

        MVector target = vertexTargets.count(idx) ? vertexTargets[idx] : originalPos;
        deformedPositions[idx] = originalPos + weight * (target - originalPos);
    }

    // ------------------------------------------
    // iterate through each point in the geometry slice provided
    //
    int count = 0;
    for ( ; !iter.isDone(); iter.next())
    {
        auto meshVtxIndex = iter.index();

        // Get in the if and the data with one O(1) query
        if (auto posIter = deformedPositions.find(meshVtxIndex); posIter != deformedPositions.end())
        {
            MPoint pt = iter.position();
            const auto& newP = posIter->second;
            iter.setPosition(pt + (newP - pt) * envelope);
            count++;
        }
    }
    DEBUG_PRINT(MString("Moved ") + count + " points");

    return MS::kSuccess;
}
