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


MObject StickyLipsNode::s_stickyAmount;
MObject StickyLipsNode::s_stickyFalloff;

MObject StickyLipsNode::s_distanceMinThreshold;
MObject StickyLipsNode::s_distanceMaxThreshold;

MObject StickyLipsNode::s_angleThreshold;
MObject StickyLipsNode::s_angleInfluence;

MObject StickyLipsNode::s_propagateSmoothness;
MObject StickyLipsNode::s_propagateInfluence;

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

    s_stickyAmount = nAttr.create("stickyAmount", "sta", MFnNumericData::kFloat, 1.0);
    nAttr.setMin(0.0);
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    addAttribute(s_stickyAmount);
    attributeAffects(s_stickyAmount, outputGeom);

    s_distanceMaxThreshold = nAttr.create("stickyMaxThreshold", "smxth", MFnNumericData::kFloat, 1.0);
    nAttr.setMin(0.0);
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    addAttribute(s_distanceMaxThreshold);
    attributeAffects(s_distanceMaxThreshold, outputGeom);


    s_distanceMinThreshold = nAttr.create("stickyMinThreshold", "smith", MFnNumericData::kFloat, 1.0);
    nAttr.setMin(0.0);
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    addAttribute(s_distanceMinThreshold);
    attributeAffects(s_distanceMinThreshold, outputGeom);

    s_stickyFalloff = nAttr.create("stickyFalloff", "sfl", MFnNumericData::kFloat, 1.0);
    nAttr.setMin(0.0);
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    addAttribute(s_stickyFalloff);
    attributeAffects(s_stickyFalloff, outputGeom);
    //
    //
    // s_angleThreshold = nAttr.create("angleThreshold", "ath", MFnNumericData::kFloat, 40.0);
    // nAttr.setMin(0.0);
    // nAttr.setMax(180.0);
    // nAttr.setKeyable(true);
    // nAttr.setStorable(true);
    // addAttribute(s_angleThreshold);
    // attributeAffects(s_angleThreshold, outputGeom);

    // s_angleInfluence = nAttr.create("angleInfluence", "aif", MFnNumericData::kFloat, 1.0);
    // // nAttr.setMin(0.0);
    // // nAttr.setMax(1.0);
    // nAttr.setKeyable(true);
    // nAttr.setStorable(true);
    // addAttribute(s_angleInfluence);
    // attributeAffects(s_angleInfluence, outputGeom);

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

    int propagateInfluence = block.inputValue(s_propagateInfluence).asInt();
    float propagateSmoothness = block.inputValue(s_propagateSmoothness).asFloat();
    // float bwd = block.inputValue(s_BackwardInfluence).asFloat();
    // float dir = block.inputValue(s_Direction).asFloat();

    float stickyMaxThreshold = block.inputValue(s_distanceMaxThreshold).asFloat();
    float stickyMinThreshold = block.inputValue(s_distanceMinThreshold).asFloat();
    // float angleThreshold = block.inputValue(s_angleThreshold).asFloat();
    // float angleInfluence = block.inputValue(s_angleInfluence).asFloat();

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
    std::vector<double> slopes(largerCount);

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

        //
        MVector& posA = larger->first[largerIdx];
        MVector& posB = smaller->first[resampleOriginalIndex[largerIdx]];
        MVector& posM = averagedPositions[largerIdx];

        MVector dirA = (posA - posM).normal();
        MVector dirB = (posB - posM).normal();

        // 1 = perfectly opposing (straight open/close, full sticky)
        // 0 = perpendicular (corner, no sticky)
        // negative = crossing (shouldn't happen but safe)
        double alignment = std::max(-(dirA * dirB), 0.0);
        // MGlobal::displayInfo(MString("SlopeAt idx ") + largerIdx + " -  " + alignment);
        slopes[largerIdx] = alignment;
    }

    if (__build_debug_curves)
        DebugUtils::createDebugCurve("middle", averagedPositions, true);

    std::unordered_map<int, MVector> mid_pos; // we need to rethink this as we can probably generate some weights directly?

    // std::vector<double> weights;

    // // Now for each point A-Mid-B decide the position
    // for (int x = 0; x < largerCount; x++)
    // {
    //     MVector& posA = larger->first[x];
    //     int posAMeshIndex = larger->second[x];
    //
    //     int smallerIdx = resampleOriginalIndex[x];
    //     MVector& posB  = smaller->first[smallerIdx];
    //     int posBMeshIndex = smaller->second[smallerIdx];
    //
    //     MVector& posM = averagedPositions[x];
    //
    //     // Slope proved to be hard to control in combination with distance
    //     // // edge direction along A's loop
    //     // int nextA = std::min(x + 1, largerCount - 1);
    //     // MVector dirA = (larger->first[nextA] - posA).normal();
    //     //
    //     // // edge direction along B's loop
    //     // int nextB = std::min(smallerIdx + 1, smallerCount - 1);
    //     // MVector dirB = (smaller->first[nextB] - posB).normal();
    //     //
    //     // // 1 = edges are parallel (center, full sticky)
    //     // // 0 = edges diverge (corner, sticky fades)
    //     // double slopeAtPoint = std::clamp(dirA * dirB, -1.0, 1.0); // std::max(dirA * dirB, 0.0);
    //
    //     // MGlobal::displayInfo(MString("SlopeAt idx ") + posAMeshIndex + " -  " + slopeAtPoint);
    //
    //     // We'll likely not need the real distance.. or maybe we can just have the distance2?
    //     double distance = std::sqrt(std::pow((posA.x - posB.x), 2) + std::pow((posA.y - posB.y), 2) + std::pow((posA.z - posB.z), 2));
    //
    //
    //     // Gives nice controllable/sealable slider
    //     double thresholdSeal = std::clamp((distance - stickyThreshold) / stickyThreshold, 0.0, 1.0);
    //
    //
    //     // Angle based seems like a nice idea, but then fights with distance based ffs...
    //     // // Get a blend to release when the angle is bigger than the threshold
    //     // double angleRad = std::acos(slopeAtPoint);
    //     // double angleDeg = angleRad * (180.0 / M_PI);
    //     // double thresholdAngle = std::clamp((angleDeg - angleThreshold) / angleThreshold, 0.0, 1.0);
    //     //
    //     // double t = thresholdSeal + (thresholdAngle * angleInfluence * (1.0 - thresholdSeal));
    //
    //     // Maybe after a min or max dist has been reached, is literally a linear blend back to 0?? like anim the envelope
    //     double t = thresholdSeal;
    //
    //     double blendVal = t;                          // linear
    //     // double blendVal = t * t;                          // quadratic
    //     // double blendVal = t * t * (3.0 - 2.0 * t);       // smoothstep
    //     // double blendVal = t * t * (3.0 - 2.0 * t);       // smoothstep
    //     // smootherstep (quintic) - Ken Perlin, flatter at both ends
    //     // double blendVal = t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
    //     // power blend - tension controls how flat the ends are
    //     // tension > 1 = more aggressive snap, flat shoulders
    //     // tension < 1 = more linear
    //     // double blendVal = 0.5 * (1.0 - std::cos(t * M_PI)); // cosine, very smooth
    //     // double blendVal = std::pow(t, stickyFalloff);     // user-controlled
    //
    //     // auto easeInOut =[](double t, double easeIn, double easeOut)
    //     // {
    //     //     // if (t < 0.5)
    //     //     return 0.5 * std::pow(2.0 * t, easeIn);
    //     //     // else
    //     //     // return 1.0 - 0.5 * std::pow(2.0 * (1.0 - t), easeOut);
    //     // };
    //
    //     // double easeIn  = stickyFalloff;
    //     // double easeOut = stickyAmount;
    //     //
    //     // double blendVal = easeInOut(t, easeIn, easeOut);
    //
    //     // if (posAMeshIndex == 4305)
    //     //     MGlobal::displayInfo(MString("dist: ") + distance + " thresholdSeal " + thresholdSeal);
    //
    //
    //     MVector blendedA = posM + blendVal * (posA - posM);
    //     MVector blendedB = posM + blendVal * (posB - posM);
    //
    //     mid_pos[posAMeshIndex] = blendedA;
    //     mid_pos[posBMeshIndex] = blendedB;
    //
    //
    //
    // }

    // precompute cumulative arc length along larger loop
    std::vector<double> arcLength(largerCount, 0.0);
    for (int x = 1; x < largerCount; x++)
    {
        MVector delta = larger->first[x] - larger->first[x - 1];
        arcLength[x] = arcLength[x - 1] + delta.length();
    }
    double totalLength = arcLength[largerCount - 1];

    double sealSkip = stickyFalloff;

    std::vector<double> blendVals(largerCount);
    std::vector<double> slopeAtPoints(largerCount);

    // seal=0 → no sticky, seal=1 → sticky up to middle
    // double sealLength = stickyAmount * totalLength * 0.5; // symmetric, so half each side

    double sealLength = stickyAmount * (totalLength * 0.5 + sealSkip) - sealSkip;
    // seal=0 → sealLength = -sealSkip → fully open
    // seal=1 → sealLength = totalLength*0.5 → ramp ends exactly at middle, fully sealed


    // Now for each point A-Mid-B decide the position
    for (int x = 0; x < largerCount; x++)
    {
        MVector& posA = larger->first[x];
        int posAMeshIndex = larger->second[x];

        int smallerIdx = resampleOriginalIndex[x];
        MVector& posB  = smaller->first[smallerIdx];
        int posBMeshIndex = smaller->second[smallerIdx];

        MVector& posM = averagedPositions[x];

        // Slope proved to be hard to control in combination with distance
        // edge direction along A's loop
        int nextA = std::min(x + 1, largerCount - 1);
        MVector dirA = (larger->first[nextA] - posA).normal();

        // edge direction along B's loop
        int nextB = std::min(smallerIdx + 1, smallerCount - 1);
        MVector dirB = (smaller->first[nextB] - posB).normal();

        // 1 = edges are parallel (center, full sticky)
        // 0 = edges diverge (corner, sticky fades)
        double slopeAtPoint =  std::clamp(dirA * dirB, -1.0, 1.0); // std::max(dirA * dirB, 0.0);


        // // use prev and next on the SAME loop
        // int prevA = std::max(x - 1, 0);
        // int nextA = std::min(x + 1, largerCount - 1);
        //
        // MVector dirIn  = (larger->first[x]     - larger->first[prevA]).normal();
        // MVector dirOut = (larger->first[nextA] - larger->first[x]).normal();
        //
        // // 1 = straight (center), 0 = sharp turn (corner)
        // double slopeAtPoint = std::max(dirIn * dirOut, 0.0);
        //
        // MGlobal::displayInfo(MString("SlopeAt idx ") + posAMeshIndex + " -  " + slopeAtPoint);

        // distance from nearest end (symmetric)
        double distFromEnd = std::min(arcLength[x], totalLength - arcLength[x]);

        // Distance from the middle point
        double distanceFromMid = std::sqrt(std::pow((posA.x - posB.x), 2) + std::pow((posA.y - posB.y), 2) + std::pow((posA.z - posB.z), 2));
        // double distanceFromMid = std::sqrt(std::pow((posA.x - posB.x), 2) + std::pow((posA.y - posB.y), 2) + std::pow((posA.z - posB.z), 2)) * (1.0 - stickyAmount);

        // // inside seal zone → 0 (sticky)
        // // inside ramp zone → linear 0→1
        // // outside → 1 (open)
        // double sealMask;
        // if (distFromEnd < sealLength)
        //     sealMask = 0.0;
        // else if (distFromEnd < sealLength + sealSkip)
        //     sealMask = (distFromEnd - sealLength) / sealSkip;
        // else
        //     sealMask = 1.0;

        // double thresholdSeal = std::clamp((distanceFromMid - stickyThreshold) / stickyThreshold, 0.0, 1.0);
        //
        // // double blendVal = sealMask * thresholdSeal;
        //
        // // double blendVal = std::min(thresholdSeal, sealMask);
        //
        // blendVals[x] = thresholdSeal;
        // slopeAtPoints[x] = slopeAtPoint;
        // blendVals[x] = std::clamp(angleInfluence - slopeAtPoint, 0.0, 1.0); // kind of bueno

        // double distSignal  = std::clamp((distanceFromMid - stickyThreshold) / stickyThreshold, 0.0, 1.0);
        // double angleSignal = std::clamp(angleInfluence - slopeAtPoint, 0.0, 1.0);
        //
        // blendVals[x] = std::clamp(distSignal + angleSignal, 0.0, 1.0);

        // double distSignal  = std::clamp((distanceFromMid - stickyThreshold) / stickyThreshold, 0.0, 1.0);
        // double rawSignal = distSignal;
        // double distSignal  = std::clamp((distanceFromMid - (stickyThreshold*stickyAmount)) / (stickyThreshold*stickyAmount), 0.0, 1.0);


        // double dx = posA.x - posB.x;
        // double dy = posA.y - posB.y;
        // double dz = posA.z - posB.z;
        // double actualDistance = std::sqrt(dx*dx + dy*dy + dz*dz);
        // double stickyFactor = stickyThreshold * stickyAmount;
        // double distanceFromMid = actualDistance * (1.0 - stickyAmount);
        // double distSignal = std::clamp((distanceFromMid - stickyFactor) / stickyFactor, 0.0, 1.0);
        //
        // double angleSignal = std::clamp(angleInfluence - slopeAtPoint, 0.0, 1.0);
        // double rawSignal   = std::clamp(distSignal + angleSignal, 0.0, 1.0);


        // blendVals[x] = std::clamp(rawSignal * stickyAmount, 0.0, 1.0);
        // blendVals[x] = rawSignal * stickyAmount + (1.0 - stickyAmount);

        // double interpolated = rawSignal * rawSignal * (3.0 - 2.0 * rawSignal);       // smoothstep

        // ------- seems controllable
        double range = stickyMaxThreshold - stickyMinThreshold;
        double stickyMinRange = stickyMinThreshold - (range + (1.0 - stickyAmount));   // Distance where stickiness starts (min range)
        double stickyMaxRange = stickyMaxThreshold - (range + (1.0 - stickyAmount));   // Distance where stickiness is maxed out (max range)


        // Map distance to a 0-1 signal based on min/max range
        double distanceSignal = std::clamp(
            (distanceFromMid - stickyMaxRange) / (stickyMinRange - stickyMaxRange),
            0.0,
            1.0
        );


        // // Apply corner influence - slopeAtPoint already does what you need!
        // // At center: slopeAtPoint=1 → full rawSignal
        // // At corners: slopeAtPoint=0 → zero stickiness
        // // float withCorners = distanceSignal * slopeAtPoint;
        //
        // // Or if you want angleInfluence to control how strong the effect is:
        // double withCorners = distanceSignal * (1.0 - (angleInfluence * slopeAtPoint));
        //
        // // Final blend
        // double finalStickiness = std::lerp(0.0, withCorners, stickyAmount);

        // double cornerFactor = 1.0 - slopeAtPoint;  // 0 at center, 1 at corners
        // blendVals[x] = std::clamp(distanceSignal - (cornerFactor * angleInfluence), 0.0, 1.0);

        // Actually nice ease in ease out
        // blendVals[x] = std::pow(distanceSignal, stickyFalloff) /
        //               (std::pow(distanceSignal, stickyFalloff) + std::pow(1.0 - distanceSignal, stickyFalloff));


        blendVals[x] = distanceSignal;

        // double distSignal  = std::clamp((distanceFromMid - stickyThreshold) / stickyThreshold, 0.0, 1.0);
        // double angleSignal = std::clamp(angleInfluence - slopeAtPoint, 0.0, 1.0);
        //
        // double gate = distSignal >= 0.0 ? 1.0 : 0.0;
        //
        // blendVals[x] = std::clamp(distSignal + angleSignal * gate, 0.0, 1.0);


        // // corners get a smaller effective threshold → release sooner
        // double cornerMin      = 0.5;
        // double cornerness     = 1.0 - std::clamp((slopeAtPoint - cornerMin) / (1.0 - cornerMin), 0.0, 1.0);
        // double effectiveThreshold = stickyThreshold * (1.0 - cornerness * angleInfluence);
        //
        // blendVals[x]     = std::clamp((distanceFromMid - effectiveThreshold) / effectiveThreshold, 0.0, 1.0);
        // slopeAtPoints[x] = slopeAtPoint;


        //
        // MVector blendedA = posM + blendVal * (posA - posM);
        // MVector blendedB = posM + blendVal * (posB - posM);
        //
        // mid_pos[posAMeshIndex] = blendedA;
        // mid_pos[posBMeshIndex] = blendedB;
    }

    // Preddy decent "smooth/relax" of the whole edge
    int passes = std::max(1, int(stickyFalloff * 3));  // 0->1, 1->3 passes
    double strength = std::min(1.0, stickyFalloff * 2.0);  // intensity per pass

    for (int p = 0; p < passes; p++) {
        std::vector<double> blurred = blendVals;
        for (int x = 0; x < largerCount; x++) {
            int left = std::max(x - 1, 0);
            int right = std::min(x + 1, largerCount - 1);

            double smoothed = blendVals[left] * 0.25 +
                              blendVals[x] * 0.5 +
                              blendVals[right] * 0.25;

            blurred[x] = blendVals[x] * (1.0 - strength) + smoothed * strength;
        }
        blendVals = blurred;
    }

    // backward pass first
    // forward pass
    // for (int x = 1; x < largerCount; x++)
    // {
    //     double worldDist = (larger->first[x] - larger->first[x - 1]).length();
    //     double maxSlope  = 1.0 / (sealSkip * std::max(slopeAtPoints[x], 0.1));
    //     double maxDelta  = maxSlope * worldDist;
    //     blendVals[x]     = std::min(blendVals[x], blendVals[x - 1] + maxDelta);
    // }
    //
    // // backward pass
    // for (int x = largerCount - 2; x >= 0; x--)
    // {
    //     double worldDist = (larger->first[x + 1] - larger->first[x]).length();
    //     double maxSlope  = 1.0 / (sealSkip * std::max(slopeAtPoints[x], 0.1));
    //     double maxDelta  = maxSlope * worldDist;
    //     blendVals[x]     = std::min(blendVals[x], blendVals[x + 1] + maxDelta);
    // }


    // clamp max slope between neighbors in world space
    // maxSlope = max change in blendVal per unit of arc length
    // double maxSlope = 1.0 / sealSkip; // sealSkip = world distance over which blend can go 0→1

    // double cornerRelax = angleInfluence;

    // // 1. corner relaxation FIRST - set the floor at corners
    // for (int x = 0; x < largerCount; x++)
    // {
    //     double cornerMin   = 0.5;
    //     double cornerness  = 1.0 - std::clamp((slopeAtPoints[x] - cornerMin) / (1.0 - cornerMin), 0.0, 1.0);
    //     double cornerFloor = std::pow(cornerness, cornerRelax);
    //     blendVals[x]       = std::max(blendVals[x], cornerFloor);
    // }
    //
    // // 2. forward pass - spread values, corners now seed high values outward
    // for (int x = 1; x < largerCount; x++)
    // {
    //     double worldDist = (larger->first[x] - larger->first[x - 1]).length();
    //     double maxDelta  = worldDist / sealSkip;
    //     blendVals[x]     = std::max(blendVals[x], blendVals[x - 1] - maxDelta); // spread high values
    //     blendVals[x]     = std::min(blendVals[x], blendVals[x - 1] + maxDelta); // limit steep rises
    // }
    //
    // // 3. backward pass
    // for (int x = largerCount - 2; x >= 0; x--)
    // {
    //     double worldDist = (larger->first[x + 1] - larger->first[x]).length();
    //     double maxDelta  = worldDist / sealSkip;
    //     blendVals[x]     = std::max(blendVals[x], blendVals[x + 1] - maxDelta);
    //     blendVals[x]     = std::min(blendVals[x], blendVals[x + 1] + maxDelta);
    // }

    std::unordered_map<int, MVector> edgeTargetPos; // the "middle" position each edge vertex is moving toward

    for (int x = 0; x < largerCount; x++)
    {
        MVector& posA = larger->first[x];
        int posAMeshIndex = larger->second[x];

        int smallerIdx = resampleOriginalIndex[x];
        MVector& posB  = smaller->first[smallerIdx];
        int posBMeshIndex = smaller->second[smallerIdx];

        MVector& posM = averagedPositions[x];

        double blendVal = blendVals[x];


        MVector blendedA = posA + blendVal * (posM - posA);
        MVector blendedB = posB + blendVal * (posM - posB);

        mid_pos[posAMeshIndex] = blendedA;
        mid_pos[posBMeshIndex] = blendedB;

        // Store the full 100% target (posM) for neighbor propagation
        edgeTargetPos[posAMeshIndex] = blendedA;
        edgeTargetPos[posBMeshIndex] = blendedB;
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


    // ------------------ blend weights ??
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
        if (weight <= 0.01) continue;

        int prevIdx = 0;
        neighborIter.setIndex(idx, prevIdx);
        MIntArray neighborList;
        neighborIter.getConnectedVertices(neighborList);

        for (int i = 0; i < (int)neighborList.length(); i++)
        {
            int neighborIdx = neighborList[i];

            if (edgeVertices.count(neighborIdx)) continue;  // never overwrite edge verts

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
    mid_pos[idx] = originalPos + weight * (target - originalPos);
}

    // ------------------------------------------



    // iterate through each point in the geometry
    //
    int count = 0;
    for ( ; !iter.isDone(); iter.next())
    {
        auto meshVtxIndex = iter.index();
        if (mid_pos.contains(meshVtxIndex))
        {
            MPoint pt = iter.position();
            const auto& newP = mid_pos[meshVtxIndex];
            iter.setPosition(pt + (newP - pt) * envelope);
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
