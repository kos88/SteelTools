//
// Created by Costantino Fracas on 24/05/2026.
//

#include "StickyLipsNode.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "SteelMayaCommon.h"

#include <maya/MGlobal.h>
#include <maya/MObjectHandle.h>
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
#include "MeshUtils.h"

MTypeId StickyLipsNode::id = SteelMaya::kStickyLipsNodeId;
MString StickyLipsNode::name = SteelMaya::kStickyLipsNodeName;


MObject StickyLipsNode::s_stickyAmount; // Seal the lips if below this distance, this is the parameter suitable for anim
MObject StickyLipsNode::s_stickyFalloff;  // The smooth amount on the sticky edge
MObject StickyLipsNode::s_stickySharpness;  //
MObject StickyLipsNode::s_stickySharpnessStrength;  //
MObject StickyLipsNode::s_stickyFalloffSharpness;  //

MObject StickyLipsNode::s_distanceMinThreshold;  // Define the min range where the sticky starts
MObject StickyLipsNode::s_distanceMaxThreshold;  // Define the min range where the sticky ends
//
// MObject StickyLipsNode::s_cornerAutoRelax;  // main influence of corner relax
// MObject StickyLipsNode::s_cornerAutoRelaxStartAngle;  // the angle between where the relax starts
// MObject StickyLipsNode::s_cornerAutoRelaxEndAngle;  // the angle between where the relax is at it's peak
// MObject StickyLipsNode::s_cornerAutoRelaxDistance;  // the percent of half segment the corner relax propagates


MObject StickyLipsNode::s_propagateInfluence;  // How much the edge loop surrounding area is affected
MObject StickyLipsNode::s_propagateTension;  // Blend between direction vector and target position, giving different angle tension on the surrounding loops
MObject StickyLipsNode::s_propagateIterations; // How many loops we expand the weights from the main edge

MObject StickyLipsNode::s_propagateHold;  // The amount of edge loops that are "holding" the same influence as the main
MObject StickyLipsNode::s_propagateHoldTension;  // The amount of tension for the old loops
MObject StickyLipsNode::s_propagateHoldInfluence;


MObject StickyLipsNode::s_stickyAutoAnim;
MObject StickyLipsNode::s_currentTime;
MObject StickyLipsNode::s_releaseDurationFrames;
MObject StickyLipsNode::s_engageDurationFrames;

MObject StickyLipsNode::s_animRelaxEndAngle;
MObject StickyLipsNode::s_animRelaxStartAngle;
MObject StickyLipsNode::s_animRelaxCurveDistance;

MObject StickyLipsNode::s_closeDistance; // Tolerance for what is considered "closed" (point needs to ble closer than this

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

    s_stickyAmount = nAttr.create("stickyAmount", "sta", MFnNumericData::kFloat, 1.0);
    nAttr.setMin(0.0);
    nAttr.setMax(1.0);
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


    s_stickySharpness = nAttr.create("edgeRetract", "shrp", MFnNumericData::kFloat, 1.0);
    nAttr.setMin(0.0);
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    addAttribute(s_stickySharpness);
    attributeAffects(s_stickySharpness, outputGeom);

    // s_stickySharpnessStrength = nAttr.create("sharpnessStrength", "shrpst", MFnNumericData::kFloat, 1.0);
    // nAttr.setMin(0.0);
    // nAttr.setKeyable(true);
    // nAttr.setStorable(true);
    // addAttribute(s_stickySharpnessStrength);
    // attributeAffects(s_stickySharpnessStrength, outputGeom);

    s_stickyFalloff = nAttr.create("edgeSmooth", "esm", MFnNumericData::kFloat, 1.0);
    nAttr.setMin(0.0);
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    addAttribute(s_stickyFalloff);
    attributeAffects(s_stickyFalloff, outputGeom);

    // s_stickyFalloffSharpness = nAttr.create("edgeCornerSmooth", "ecosm", MFnNumericData::kFloat, 1.0);
    // nAttr.setMin(0.0);
    // nAttr.setMax(1.0);
    // nAttr.setKeyable(true);
    // nAttr.setStorable(true);
    // addAttribute(s_stickyFalloffSharpness);
    // attributeAffects(s_stickyFalloffSharpness, outputGeom);

    // s_cornerAutoRelax = nAttr.create("cornerAutoRelax", "carx", MFnNumericData::kFloat, 1.0);
    // nAttr.setMin(0.0);
    // nAttr.setMax(1.0);
    // nAttr.setKeyable(true);
    // nAttr.setStorable(true);
    // addAttribute(s_cornerAutoRelax);
    // attributeAffects(s_cornerAutoRelax, outputGeom);
    //
    // s_cornerAutoRelaxStartAngle = nAttr.create("autoRelaxStartAngle", "crsa", MFnNumericData::kFloat, 20.0);
    // nAttr.setMin(0.0);
    // nAttr.setMax(90.0);
    // nAttr.setKeyable(true);
    // nAttr.setStorable(true);
    // addAttribute(s_cornerAutoRelaxStartAngle);
    // attributeAffects(s_cornerAutoRelaxStartAngle, outputGeom);
    //
    // s_cornerAutoRelaxEndAngle = nAttr.create("autoRelaxEndAngle", "crea", MFnNumericData::kFloat, 45.0);
    // nAttr.setMin(0.0);
    // nAttr.setMax(90.0);
    // nAttr.setKeyable(true);
    // nAttr.setStorable(true);
    // addAttribute(s_cornerAutoRelaxEndAngle);
    // attributeAffects(s_cornerAutoRelaxEndAngle, outputGeom);
    //
    // s_cornerAutoRelaxDistance = nAttr.create("autoRelaxSegmentPortion", "crd", MFnNumericData::kFloat, 0.3);
    // nAttr.setMin(0.0);
    // nAttr.setMax(1.0);
    // nAttr.setKeyable(true);
    // nAttr.setStorable(true);
    // addAttribute(s_cornerAutoRelaxDistance);
    // attributeAffects(s_cornerAutoRelaxDistance, outputGeom);

    s_propagateIterations = nAttr.create("propagateIterations", "pin", MFnNumericData::kInt, 3);
    nAttr.setMin(0);
    // nAttr.setMax(10);
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    addAttribute(s_propagateIterations);
    attributeAffects(s_propagateIterations, outputGeom);

    s_propagateInfluence = nAttr.create("propagateInfluence", "psm", MFnNumericData::kFloat, 1.0);
    nAttr.setMin(0.0);
    nAttr.setMax(1.0);
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    addAttribute(s_propagateInfluence);
    attributeAffects(s_propagateInfluence, outputGeom);


    // s_propagateHold = nAttr.create("holdLoops", "hlop", MFnNumericData::kInt, 1);
    // nAttr.setMin(0);
    // // nAttr.setMax(10);
    // nAttr.setKeyable(true);
    // nAttr.setStorable(true);
    // addAttribute(s_propagateHold);
    // attributeAffects(s_propagateHold, outputGeom);
    //
    s_propagateHoldInfluence = nAttr.create("propagateEdgeTension", "eten", MFnNumericData::kFloat, 0.9);
    nAttr.setMin(0.0);
    nAttr.setMax(1.0);
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    addAttribute(s_propagateHoldInfluence);
    attributeAffects(s_propagateHoldInfluence, outputGeom);

    s_propagateTension = nAttr.create("propagateTension", "ptes", MFnNumericData::kFloat, 0.5);
    // nAttr.setMin(0.0);
    // nAttr.setMax(2.0);
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    addAttribute(s_propagateTension);
    attributeAffects(s_propagateTension, outputGeom);

    s_currentTime = nAttr.create("currentTime", "curtm", MFnNumericData::kInt, 0);
    nAttr.setKeyable(true);
    addAttribute(s_currentTime);
    attributeAffects(s_currentTime, outputGeom);

    s_stickyAutoAnim = nAttr.create("autoAnim", "auan", MFnNumericData::kBoolean, 1);
    nAttr.setKeyable(true);
    addAttribute(s_stickyAutoAnim);
    attributeAffects(s_stickyAutoAnim, outputGeom);

    s_releaseDurationFrames = nAttr.create("s_releaseDurationFrames", "rdf", MFnNumericData::kFloat, 3);
    nAttr.setKeyable(true);
    addAttribute(s_releaseDurationFrames);
    attributeAffects(s_releaseDurationFrames, outputGeom);

    s_engageDurationFrames = nAttr.create("engageDurationFrames", "edf", MFnNumericData::kFloat, 3);
    nAttr.setKeyable(true);
    addAttribute(s_engageDurationFrames);
    attributeAffects(s_engageDurationFrames, outputGeom);

    s_animRelaxStartAngle = nAttr.create("animRelaxStartAngle", "arsa", MFnNumericData::kFloat, 20.0);
    nAttr.setMin(0.0);
    nAttr.setMax(90.0);
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    addAttribute(s_animRelaxStartAngle);
    attributeAffects(s_animRelaxStartAngle, outputGeom);

    s_animRelaxEndAngle = nAttr.create("animRelaxEndAngle", "area", MFnNumericData::kFloat, 45.0);
    nAttr.setMin(0.0);
    nAttr.setMax(90.0);
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    addAttribute(s_animRelaxEndAngle);
    attributeAffects(s_animRelaxEndAngle, outputGeom);

    s_animRelaxCurveDistance = nAttr.create("animRelaxSegmentPortion", "arsp", MFnNumericData::kFloat, 0.3);
    nAttr.setMin(0.0);
    nAttr.setMax(1.0);
    nAttr.setKeyable(true);
    nAttr.setStorable(true);
    addAttribute(s_animRelaxCurveDistance);
    attributeAffects(s_animRelaxCurveDistance, outputGeom);

    s_closeDistance = nAttr.create("closeDistance", "clodis", MFnNumericData::kFloat, 0.1);
    nAttr.setKeyable(true);
    addAttribute(s_closeDistance);
    attributeAffects(s_closeDistance, outputGeom);

    // s_propagateHoldTension = nAttr.create("holdPropagateLoopsTension", "phlt", MFnNumericData::kFloat, 0.5);
    // nAttr.setMin(0.0);
    // nAttr.setMax(1.0);
    // nAttr.setKeyable(true);
    // nAttr.setStorable(true);
    // addAttribute(s_propagateHoldTension);
    // attributeAffects(s_propagateHoldTension, outputGeom);


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

    // AE templates are picked up from folders etc.. but, to keep things in a sane way we store it here
    MGlobal::executeCommand(R"my_delimiter(
global proc AEsteelStickyLipsTemplate( string $nodeName )
{
    editorTemplate -beginScrollLayout;

    // Main Sticky Controls
    editorTemplate -beginLayout "Sticky Controls" -collapse 0;
        editorTemplate -annotation "0-1 value where lips seal together. Animate this for sticky behavior"
            -addControl "stickyAmount";
        editorTemplate -annotation "Maximum distance where sticky effect begins"
            -addControl "maxThreshold";
        editorTemplate -annotation "Minimum distance where sticky effect is fully applied"
            -addControl "minThreshold";
        editorTemplate -annotation "Smoothness passes of the sticky edge. Similar to a gaussian blur."
            -addControl "edgeSmooth";
        editorTemplate -annotation "Increase the decay influence between the sealed and unsealed areas"
            -addControl "sharpness";
    editorTemplate -endLayout;

    // Corner Auto Relax
    editorTemplate -beginLayout "Corner Auto Relax" -collapse 0;
        editorTemplate -annotation "Main influence strength of corner relaxation (0-1)"
            -addControl "cornerAutoRelax";
        editorTemplate -annotation "Angle in degrees where corner relax begins (0-90)"
            -addControl "autoRelaxStartAngle";
        editorTemplate -annotation "Angle in degrees where corner relax reaches peak effect (0-90)"
            -addControl "autoRelaxEndAngle";
        editorTemplate -annotation "Percentage of half segment distance that corner relax propagates (0-1)"
            -addControl "autoRelaxSegmentPortion";
    editorTemplate -endLayout;

    // Propagation Settings
    editorTemplate -beginLayout "Propagation" -collapse 0;
        editorTemplate -annotation "Number of edge loop where the influence expands from the main edge"
            -addControl "propagateIterations";
        editorTemplate -annotation "Overall influence strength on surrounding edge loops (0-1)"
            -addControl "propagateInfluence";
        editorTemplate -annotation "Number of edge loops around the main one to follow completely"
            -addControl "holdLoops";
        editorTemplate -annotation "Hold loops influence strength"
            -addControl "holdInfluence";

    editorTemplate -endLayout;

    // Edge Loop Setup
    editorTemplate -beginLayout "Component Tags Names" -collapse 1;
        editorTemplate -annotation "Component tag name for upper lip edge loop"
            -addControl "edgeLoopNameA";
        editorTemplate -annotation "Component tag name for lower lip edge loop"
            -addControl "edgeLoopNameB";
    editorTemplate -endLayout;

    // Standard deformer attributes
    editorTemplate -beginLayout "Deformer Attributes" -collapse 1;
        editorTemplate -addControl "envelope";
    editorTemplate -endLayout;

    // Add any extra attributes
    editorTemplate -addExtraControls;

    editorTemplate -endScrollLayout;

    editorTemplate -suppress "input";
    editorTemplate -suppress "outputGeom";
}

)my_delimiter");

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

    if (multiIndex != 0)
    {
        MGlobal::displayWarning(MString("") + name + " only operates on one mesh, skipping index " + multiIndex);
        return MS::kFailure;
    }

    const float envelope = envData.asFloat();

    if (envelope <= 0.0)
        return MS::kSuccess;

    // --------------------------------------------- Geo validation ------------------------------------------------- //
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
        MGlobal::displayWarning("[Steel Sticky Lips] Deformer needs a component tag to work on an area of the mesh");
        return MS::kFailure;
    } if (state == MFnGeometryData::kInvalidGroup) {
        MGlobal::displayError("[Steel Sticky Lips] Invalid component group provided");
        return MS::kFailure;
    } if (state == MFnGeometryData::kEmptyGroup) {
        MGlobal::displayError("[Steel Sticky Lips] Invalid geometry. Unable to deform.");
        return MS::kFailure;
    }

    MFnMesh                 fnMesh(meshObj);
    const MFnGeometryData   meshData(meshObj, &returnStatus);
    MObjectHandle           meshMHandle(meshObj);

    if (returnStatus != MS::kSuccess) {
        MGlobal::displayError("[Steel Sticky Lips] Unexpected error when initializing geometry accessor");
        return returnStatus;
    }

    // Validate component and types
    const MString upperEdgeName = block.inputValue(s_EdgeLoopA).asString();
    const MString lowerEdgeName = block.inputValue(s_EdgeLoopB).asString();

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

    // Base our logic on component number only
    MFnSingleIndexedComponent fnUpper(upperEdgeObj);
    MFnSingleIndexedComponent fnLower(lowerEdgeObj);
    int upperCount = fnUpper.elementCount();
    int lowerCount = fnLower.elementCount();
    int areaCount = subIter.count();

    // Now decide if we need to compute the caches, or we can reuse them
    MeshFingerprint currentPrint{
        fnMesh.numVertices(),
        fnMesh.numPolygons(),
        meshMHandle.hashCode(),
        upperCount,
        lowerCount,
        areaCount
    };

    if (currentPrint.hashCode != m_lastFingerprint.hashCode)
    {
        DEBUG_PRINT("Different MESH request recompute of each cache");
        m_computeCaches = true;
        m_lastFingerprint = currentPrint;
    }

    if (currentPrint.polyCount != m_lastFingerprint.polyCount || currentPrint.vertCount != m_lastFingerprint.vertCount)
    {
        DEBUG_PRINT("Different mesh TOPOLOGY request recompute of each cache");
        m_computeCaches = true;
        m_lastFingerprint = currentPrint;
    }

    if (currentPrint.upperTagCount != m_lastFingerprint.upperTagCount ||
        currentPrint.upperTagCount != m_lastFingerprint.upperTagCount ||
        currentPrint.lowerTagCount != m_lastFingerprint.lowerTagCount
        )
    {
        DEBUG_PRINT("Different mesh TAG request recompute of each cache");
        m_computeCaches = true;
        m_lastFingerprint = currentPrint;
    }


    // ---------------- From here, we can probably move it outside of maya, if we want it generic and portable to other DCC
    double startAngleInfluence = 1.0;
    double endAngleInfluence = 1.0;

    // maybe not a great idea since we access often in a loop... probably better to rebake it on reordered data?
    auto ptAsVec = [&](const int index) -> MVector
    {
        MPoint pt;
        fnMesh.getPoint(index, pt, MSpace::kObject);
        return pt; // implicit conversion..
    };

    if (m_computeCaches)
    {

        m_upperPoints = SteelMeshUtils::createOrderedVertices(upperEdgeObj, meshObj);
        m_lowerPoints = SteelMeshUtils::createOrderedVertices(lowerEdgeObj, meshObj);
        // DEBUG_PRINT(MString("Upper curve points") + std::to_string(m_upperPoints.first.size()).c_str());

        // Detect if edges are going in opposite directions
        bool reverseNeeded = false;
        const double tolerance = 0.0001;

        // Get the start and end points of both edges
        MVector up_start = ptAsVec(m_upperPoints[0]);
        MVector up_end = ptAsVec(m_upperPoints[m_upperPoints.size() - 1]);
        MVector down_start = ptAsVec(m_lowerPoints[0]);
        MVector down_end = ptAsVec(m_lowerPoints[m_lowerPoints.size() - 1]);

        // Check if start points match or end points match
        bool startMatch = (up_start - down_start).length() < tolerance;
        bool endMatch = (up_end - down_end).length() < tolerance;

        // If neither start nor end points match, edges are going in opposite directions
        if (!startMatch && !endMatch)
        {
            reverseNeeded = true;
            // DEBUG_PRINT("Edges are going in opposite directions... reversing one");
        }
        else
        {
            // DEBUG_PRINT("Edges are going in same direction");
        }

        // If reverse is needed, reverse the larger segment
        if (reverseNeeded)
        {
            if (m_upperPoints.size() >= m_lowerPoints.size())
            {
                std::ranges::reverse(m_upperPoints);
            }
            else
            {
                std::ranges::reverse(m_lowerPoints);
            }
        }

        // We now resample the segment with LESS points to have a even point number on both sides.
        // the side without resample, will simply "snap" on the middle line.
        // the side with the resample, will snap to the closest point of the middle line.

        // *-------*-------*-------*-------* << Original A, more points
        // |       |       |       |       |
        // *-------*-------*-------*-------* Original B, less points
        // |        \       \             /
        // *---------*--------*----------* Original B, less points


        // Create pointers and numbers for larger and smaller
        m_larger  = m_upperPoints.size() >= m_lowerPoints.size() ? &m_upperPoints  : &m_lowerPoints;
        m_smaller = m_upperPoints.size() <  m_lowerPoints.size() ? &m_upperPoints  : &m_lowerPoints;
        m_largerCount  = static_cast<int>(m_larger->size());
        const int smallerCount = static_cast<int>(m_smaller->size());

        m_resampleOriginalIndex.resize(m_largerCount);
        m_originalToResampleIndex.resize(smallerCount, -1);



        for (int largerIdx = 0; largerIdx < m_largerCount; ++largerIdx)
        {
            double lerpT      = static_cast<double>(largerIdx) * (smallerCount - 1) / (m_largerCount - 1);
            int smallerIdx    = static_cast<int>(std::floor(lerpT));
            smallerIdx        = std::clamp(smallerIdx, 0, smallerCount - 2);
            double remainder  = lerpT - smallerIdx;

            m_resampleOriginalIndex[largerIdx]     = smallerIdx;
            m_originalToResampleIndex[smallerIdx]  = largerIdx;

            auto& smaller_ids = *m_smaller;
            MVector resampledSmaller = ptAsVec(smaller_ids[smallerIdx]) +
                                       (ptAsVec(smaller_ids[smallerIdx + 1]) - ptAsVec(smaller_ids[smallerIdx])) *
                                       remainder;
        }


        // precompute cumulative arc length along larger loop
        m_arcLength = std::vector(m_largerCount, 0.0);

        for (int x = 1; x < m_largerCount; x++)
        {
            auto& larger_ids = *m_larger;
            MVector delta = ptAsVec(larger_ids[x]) - ptAsVec(larger_ids[x - 1]);
            m_arcLength[x] = m_arcLength[x - 1] + delta.length();
        }

    }

    std::vector<MVector> largerPoints;
    std::vector<MVector> smallerPoints;
    largerPoints.reserve(m_larger->size());
    smallerPoints.reserve(m_smaller->size());
    //
    for (const int index: *m_larger)
    {
        MPoint pt;
        fnMesh.getPoint(index, pt, MSpace::kObject);
        largerPoints.emplace_back(pt); // convert to MVector in place
    }

    for (const int index: *m_smaller)
    {
        MPoint pt;
        fnMesh.getPoint(index, pt, MSpace::kObject);
        smallerPoints.emplace_back(pt); // convert to MVector in place
    }

    // DebugUtils::createDebugCurve("upper", largerPoints, true);
    // DebugUtils::createDebugCurve("lower", smallerPoints, true);

    // ---------------------------------------- Values extraction --------------------------------------------------- //
    const int propagationPasses = block.inputValue(s_propagateIterations).asInt();
    const float propagateInfluence = block.inputValue(s_propagateInfluence).asFloat();
    const float propagateTension = block.inputValue(s_propagateTension).asFloat();

    // const int propagateHold = block.inputValue(s_propagateHold).asInt();
    // const float propagateHoldTension = block.inputValue(s_propagateHoldTension).asFloat();
    const float propagateHoldInfluence = block.inputValue(s_propagateHoldInfluence).asFloat();


    const float stickyMaxThreshold = block.inputValue(s_distanceMaxThreshold).asFloat();
    const float stickyMinThreshold = block.inputValue(s_distanceMinThreshold).asFloat();


    // const float cornerAutoRelax = block.inputValue(s_cornerAutoRelax).asFloat();
    // const float cornerAutoRelaxStartAngle = block.inputValue(s_cornerAutoRelaxStartAngle).asFloat();
    // const float cornerAutoRelaxEndAngle = block.inputValue(s_cornerAutoRelaxEndAngle).asFloat();
    // const float cornerAutoRelaxDistance = block.inputValue(s_cornerAutoRelaxDistance).asFloat();

    const float cornerAutoRelaxStartAngle = block.inputValue(s_animRelaxStartAngle).asFloat();
    const float cornerAutoRelaxEndAngle = block.inputValue(s_animRelaxEndAngle).asFloat();
    const float cornerAutoRelaxDistance = block.inputValue(s_animRelaxCurveDistance).asFloat();

    const float stickyFalloff = block.inputValue(s_stickyFalloff).asFloat();
    // const float stickyFalloffSmooth = block.inputValue(s_stickyFalloffSharpness).asFloat();
    const float stickySharpness = block.inputValue(s_stickySharpness).asFloat();
    // const float stickySharpnessStrength = block.inputValue(s_stickySharpnessStrength).asFloat();
    float stickyAmount = block.inputValue(s_stickyAmount).asFloat();

    const int currentTime = block.inputValue(s_currentTime).asInt();
    const float releaseDurationFrames = block.inputValue(s_releaseDurationFrames).asFloat();
    const float engageDurationFrames = block.inputValue(s_engageDurationFrames).asFloat();
    const float closeDistance = block.inputValue(s_closeDistance).asFloat();
    const bool autoAnim = block.inputValue(s_stickyAutoAnim).asBool();



    // Dirty propagation caches based on expansion parameters
    // if (m_lastPropagateHold != propagateHold || m_lastPropagationPasses != propagationPasses)
    // {
    //     m_lastPropagateHold = propagateHold;
    //     m_lastPropagationPasses = propagationPasses;
    //     DEBUG_PRINT("Parameter change requested cache recompute");
    //     m_computeCaches = true;
    // }

    if (m_lastPropagationPasses != propagationPasses)
    {
        m_lastPropagationPasses = propagationPasses;
        DEBUG_PRINT("Parameter change requested cache recompute");
        m_computeCaches = true;
    }

    // DEBUG_PRINT(MString("Deforming at input index...") + multiIndex);


    // MStringArray keysObj;
    // meshData.componentTags(keysObj);
    // MGlobal::displayInfo("Component tags found...");
    // for (auto& element: keysObj)
    // {
    //     MGlobal::displayInfo(element);
    // }

    // --- Time handling ---
    bool frameChanged = false;
    if (currentTime > m_lastFrame)
    {
        frameChanged = true;
        m_lastFrame  = currentTime;
    }
    else if (currentTime < m_lastFrame)
    {
        DEBUG_PRINT("Time moved backward...reset");
        m_lastFrame        = currentTime;
        m_lastDistance     = -1.0;
        m_smoothedVelocity = 0.0f;
    }

    // {
    //
    //     // We sample the top and bottom mid points here, this is usually the area with bigger movement
    //     const int halfIdx    = std::max(1, m_largerCount / 2);
    //     const MVector& posA  = largerPoints[halfIdx];
    //     const int smallerIdx = m_resampleOriginalIndex[halfIdx];
    //     const MVector& posB  = smallerPoints[smallerIdx];
    //
    //     const double dx = posA.x - posB.x;
    //     const double dy = posA.y - posB.y;
    //     const double dz = posA.z - posB.z;
    //     const double distance = std::sqrt(dx*dx + dy*dy + dz*dz);
    //
    //     if (frameChanged)
    //     {
    //         // // --- User-facing parameters (all in frames or scene units) ---
    //         // const float releaseDuration = std::max(1.0f, releaseDurationFrames); // e.g. 4
    //         // const float engageDuration  = std::max(1.0f, engageDurationFrames);  // e.g. 2
    //         // const float velThreshold    = std::max(0.0f, 5e-4f);       // e.g. 5e-4
    //         // const float closeDist       = std::max(1e-6f, closeDistance);        // e.g. 0.05
    //         //
    //         // // Velocity smoothing is now a fixed internal constant. It only needs
    //         // // to filter numerical noise, not to be an animator-facing knob.
    //         // constexpr float kVelSmoothing = 0.4f;
    //         //
    //         // // Convert frame durations to per-frame filter rates.
    //         // // These reach ~95% of the target after `duration` frames.
    //         // auto framesToAlpha = [](float frames) {
    //         //     return 1.0f - std::exp(-3.0f / frames);
    //         // };
    //         // const float alphaRelease = framesToAlpha(releaseDuration);
    //         // const float alphaEngage  = framesToAlpha(engageDuration);
    //         //
    //         // if (m_lastDistance < 0.0)
    //         // {
    //         //     m_lastDistance     = distance;
    //         //     m_smoothedVelocity = 0.0f;
    //         //     m_internalAnimVal  = (distance <= closeDist) ? 1.0f : 0.0f;
    //         // }
    //
    //         // const float rawVelocity = static_cast<float>(distance - m_lastDistance);
    //         // m_smoothedVelocity += (rawVelocity - m_smoothedVelocity) * kVelSmoothing;
    //         //
    //         // const bool isClosing = (m_smoothedVelocity < -velThreshold);
    //         // const bool isClosed  = (distance <= closeDist);
    //         //
    //         // // Target: 0 while lips are actively closing in mid-air, 1 otherwise.
    //         // const float target = (isClosing && !isClosed) ? 0.0f : 1.0f;
    //         //
    //         // // Pick the rate based on which direction we're moving in the sticky value.
    //         // // Going down (releasing sticky as lips close)  -> alphaRelease  (gradual)
    //         // // Going up   (re-engaging on contact / at rest) -> alphaEngage  (snappy)
    //         // const float rate = (target < m_internalAnimVal) ? alphaRelease : alphaEngage;
    //         //
    //         // m_internalAnimVal += (target - m_internalAnimVal) * rate;
    //         // m_internalAnimVal = std::clamp(m_internalAnimVal, 0.0f, 1.0f);
    //
    //         //
    //         // const float closeDist       = std::max(1e-6f, closeDistance);        // e.g. 0.05
    //         // const bool isClosed  = (distance <= closeDist);
    //         // float direction = (m_lastDistance <= distance) ? 1.0f : -1.0f;
    //         // bool isDeadZone = ((m_lastDistance - distance) <= 0.1f); // We are in a "slow down" steady
    //         //
    //         // if (isClosed) // Quickly bump up the value to 1
    //         // {
    //         //     const float engageStep = 1.0f / std::max(1.0f, engageDurationFrames);
    //         //     m_internalAnimVal = std::min(1.0f, m_internalAnimVal + engageStep);
    //         // }
    //         //
    //         // if (direction == 1.0f) // We are opening
    //         // {
    //         //     // Do nothing, default weighting works, we must be at 1 here
    //         // }
    //         //
    //         // if (direction == -1.0f) // we are closing!
    //         // {
    //         //     // Relax in N frames moving value to 0
    //         //     const float releaseStep = 1.0f / std::max(1.0f, releaseDurationFrames);
    //         //     m_internalAnimVal = std::max(0.0f, m_internalAnimVal - releaseStep);
    //         // }
    //         //
    //         // // m_internalAnimVal = direction; // Pass the final animated value
    //         // m_internalAnimVal = std::clamp(m_internalAnimVal, 0.0f, 1.0f); // Pass the final animated value
    //         //
    //         //
    //         // m_lastDistance = distance;
    //
    //         // DEBUG_PRINT(MString("d: ") + distance +
    //         //             " vel: " + m_smoothedVelocity +
    //         //             " tgt: " + target +
    //         //             " rate: " + rate +
    //         //             " out: " + m_internalAnimVal);
    //
    //
    //         const float closeDist = std::max(1e-6f, closeDistance);        // e.g. 0.05
    //         const bool isClosed = (distance <= closeDist);
    //
    //         // Positive delta = opening, negative delta = closing
    //         const float distanceDelta = static_cast<float>(m_lastDistance - distance);
    //         constexpr float kDeadZoneThreshold = 0.1f;
    //         const bool isDeadZone = (std::abs(distanceDelta) <= kDeadZoneThreshold);
    //
    //         // Determine direction, but only when we're outside the dead zone to avoid jitter
    //         float direction = 0.0f; // 0 = steady/dead-zone, 1.0 = opening, -1.0 = closing
    //         if (!isDeadZone)
    //         {
    //             direction = (distanceDelta < 0.0f) ? 1.0f : -1.0f;
    //             // distanceDelta < 0  →  m_lastDistance < distance  →  opening  →  1.0
    //             // distanceDelta > 0  →  m_lastDistance > distance  →  closing  → -1.0
    //         }
    //
    //         if (isClosed)
    //         {
    //             // Closed: quickly bump up to 1 (engaged) — takes priority over direction
    //             const float engageStep = 1.0f / std::max(1.0f, engageDurationFrames);
    //             m_internalAnimVal = std::min(1.0f, m_internalAnimVal + engageStep);
    //             m_closingHoldCounter = 0.0f;
    //         }
    //         else if (direction == -1.0f)
    //         {
    //             // Closing and not yet closed: count frames before releasing
    //             m_closingHoldCounter += 1.0f;
    //             if (m_closingHoldCounter >= releaseDurationFrames)
    //             {
    //                 const float releaseStep = 1.0f / std::max(1.0f, releaseDurationFrames);
    //                 m_internalAnimVal = std::max(0.0f, m_internalAnimVal - releaseStep);
    //             }
    //             // else: preserve current value — not holding long enough yet
    //         }
    //         else
    //         {
    //             // Opening, steady, or in dead zone (and not closed): ramp back to 1 (engaged)
    //             const float engageStep = 1.0f / std::max(1.0f, engageDurationFrames);
    //             m_internalAnimVal = std::min(1.0f, m_internalAnimVal + engageStep);
    //             m_closingHoldCounter = 0.0f;
    //         }
    //
    //         m_internalAnimVal = std::clamp(m_internalAnimVal, 0.0f, 1.0f);
    //         m_lastDistance = distance;
    //
    //     }
    //
    //     if (autoAnim)
    //         stickyAmount = m_internalAnimVal;
    //
    // }



    // A temp storage where the edge are "sticking" to do a second blur pass
    std::vector<double> blendVals(m_largerCount);
    double totalLength = m_arcLength[m_largerCount - 1];
    double cornerDistance = (totalLength / 2.0) * cornerAutoRelaxDistance;


    // Remove stickyness based on the angle between corner top-bottom + a portion of half arclen
    // By computing the angle between A-A1 to B-B1 (and opposite)
    // We wre initially using this as deformer parameter to modulate the stickiness, but it revealed to be better
    // for the internal aniation system
    //
    //     ___ A1 _____ C1 ___
    // A  /                   \  C
    //   *                     *
    // B  \___   ______    ___/  D
    //         B1       D1
    // if (cornerAutoRelax) // old!
    if (autoAnim)
    {

        // Initialize the caches
        m_lastDistances.resize(m_largerCount);
        m_lastBlend.resize(m_largerCount);
        m_isClosing.resize(m_largerCount);
        m_perPointSticky.resize(m_largerCount);
        m_isReleasing.resize(m_largerCount);
        m_lastPeakAngle.resize(m_largerCount);

        // Create a 0-1 multiplier whithin the angle to drive the animation relaxation
        int leftCornerAngleIndex = -1;
        int rightCornerAngleIndex = -1;

        for (int j = 0; j < m_largerCount; j++)
        {
            if (m_arcLength[j] <= cornerDistance)
                leftCornerAngleIndex = j;
            if (m_arcLength[j] >= totalLength - cornerDistance) {
                rightCornerAngleIndex = j;
                break;
            }
        }

        // Compute slopes using startCornerIdx and endCornerIdx
        MVector startDirA = (largerPoints[leftCornerAngleIndex] - largerPoints[0]).normal();
        MVector startDirB = (smallerPoints[m_resampleOriginalIndex[leftCornerAngleIndex]] - smallerPoints[m_resampleOriginalIndex[0]]).normal();
        double startSlopeDot = std::clamp(startDirA * startDirB, -1.0, 1.0);
        double startSlopeAngle = std::acos(startSlopeDot) * 90.0 / M_PI; // yes half angle!

        MVector endDirA = (largerPoints[m_largerCount-1] - largerPoints[rightCornerAngleIndex]).normal();
        MVector endDirB = (smallerPoints[m_resampleOriginalIndex[m_largerCount-1]] - smallerPoints[m_resampleOriginalIndex[rightCornerAngleIndex]]).normal();
        double endSlopeDot = std::clamp(endDirA * endDirB, -1.0, 1.0);
        double endSlopeAngle = std::acos(endSlopeDot) * 90.0 / M_PI; // yes half angle!

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
    for (int x = 0; x < m_largerCount; x++)
    {
        const MVector& posA = largerPoints[x];

        const int smallerIdx = m_resampleOriginalIndex[x];
        const MVector& posB  = smallerPoints[smallerIdx];

        // Distance between A-B point to control general influence.. (maybe we can avoid real distance ans skip sqrt)
        double distanceFromMid = std::sqrt(std::pow((posA.x - posB.x), 2) +
                                           std::pow((posA.y - posB.y), 2) +
                                           std::pow((posA.z - posB.z), 2)
                                           );
        double localStickyAmount = stickyAmount;

        double cornerRelaxValue = 0;
        // // Simple distance base seal or corner relax enhanced
        // if (cornerAutoRelax > 0.0)
        // {
        //     // distance from nearest end (symmetric)
        //     double distFromEnd = std::min(m_arcLength[x], totalLength - m_arcLength[x]);
        //
        //     // Normalize it to arc-len and clamp to our corner area
        //     double cornerAreaInfluence = std::max(0.0, 1.0 - (distFromEnd / cornerDistance));
        //
        //     // Get normalized position along the curve (0 = start, 1 = end)
        //     double t = m_arcLength[x] / totalLength;
        //
        //     // Blend between start and end angles based on position (angle influence is 0 when closed, towards 1 when closed)
        //     double targetAngle = (1.0 - t) * startAngleInfluence + t * endAngleInfluence;
        //
        //     // Now multiply it by the actual angle influence (0-1) and by the angle corner auto relax
        //     // We will have a gradual value from 0 to 1, guided from the angle between at corners, blending towards the middle
        //      cornerRelaxValue = std::pow(cornerAreaInfluence * targetAngle * cornerAutoRelax, 2);
        //
        //     // MGlobal::displayInfo(MString("Index: ") + x + " distVal " + distFromEnd + " influence: " + cornerAreaInfluence + " relax: " + cornerRelaxValue);
        // }

        if (frameChanged && autoAnim)
        {
            // Store the data of the direction std::vector<bool> m_isOpening;
            // Store the last distance if std::vector<float> m_lastDistance;
            // use std::vector<float> m_lastBlend;
            // if last distance was closed, we can use full interpolation, otherwise, we use the last!
            // const float closeDist = std::max(1e-6f, closeDistance);        // e.g. 0.05
            //         const bool isClosed = (distance <= closeDist);

            // On a new frame, record whether we're actively closing (not yet closed)
            const float closeDist = std::max(1e-6f, closeDistance);
            m_isClosing[x] = (distanceFromMid < m_lastDistances[x] && distanceFromMid > closeDist);


            // Per-point sticky: gradually fades to 0 when lingering within stickyMaxThreshold, snaps up on close
            const float engageStep = 1.0f / std::max(1.0f, engageDurationFrames);
            const float releaseStep = 1.0f / std::max(1.0f, releaseDurationFrames);


            // Get normalized position along the curve (0 = start, 1 = end)
            double t = m_arcLength[x] / totalLength;

            // Blend between start and end angles based on position (angle influence is 0 when closed, towards 1 when closed)
            double targetAngle = (1.0 - t) * startAngleInfluence + t * endAngleInfluence;
            if (x == 3) DEBUG_PRINT(MString("ANGLE IS...") + targetAngle + " start at " + startAngleInfluence);


            // Per point to resticky it
            if (distanceFromMid <= closeDist)
            {
                // Closed: bump back to 1
                m_isReleasing[x] = false;
                m_lastPeakAngle[x] = 0.0f;
                m_perPointSticky[x] = std::min(1.0f, m_perPointSticky[x] + engageStep * 3.0f); // extra snappy
                if (x == 3) DEBUG_PRINT(MString("WE ARE CLOSING!..."));
            }

            else if (targetAngle > 0.0f || m_isReleasing[x]) // We are within the range
            {
                if (targetAngle > 0.0f)
                {
                    m_isReleasing[x] = true;
                    // Track peak angle as it rises
                    if (targetAngle > m_lastPeakAngle[x])
                        m_lastPeakAngle[x] = targetAngle;
                }
                float stickyReleaseCurve = 1.0;
                // Angle-based trigger: targetAngle is already 0-1 (influence, not degrees)
                // float animVal = std::clamp(static_cast<float>(targetAngle), 0.0f, 1.0f);
                // animVal = std::pow(animVal, 1.0f + stickyReleaseCurve * 10.0f);
                // m_perPointSticky[x] = std::max(0.0f, m_perPointSticky[x] - releaseStep * animVal);

                // Use peak angle
                float rawAngle = std::clamp(static_cast<float>(m_lastPeakAngle[x]), 0.0f, 1.0f);
                float animVal = std::max(rawAngle, 0.5f);
                animVal = std::pow(animVal, 1.0f + stickyReleaseCurve * 10.0f);
                m_perPointSticky[x] = std::max(0.0f, m_perPointSticky[x] - releaseStep * animVal);
                if (x == 3) DEBUG_PRINT(MString("WE ARE RELEASING!...") + animVal);

                // Reset release state if fully released and angle is gone
                if (m_perPointSticky[x] <= 0.001f && targetAngle <= 0.0f)
                {
                    m_isReleasing[x] = false;
                    m_lastPeakAngle[x] = 0.0f;
                }
            }
            else
            {
                // Low/zero angle: ramp back to 1
                m_perPointSticky[x] = std::min(1.0f, m_perPointSticky[x] + engageStep);
                if (x == 3) DEBUG_PRINT(MString("WE ARE OPENING"));
            }


            // else if (middleDistance >= stickyMinThreshold && middleDistance > closeDist)
            // {
            //     // Within sticky range: stronger fade near stickyMaxThreshold, weaker near stickyMinThreshold
            //     float t = std::clamp(static_cast<float>((middleDistance - stickyMinThreshold) /
            //         std::max(1e-6, static_cast<double>(stickyMaxThreshold - stickyMinThreshold))), 0.0f, 1.0f);
            //     m_perPointSticky[x] = std::max(0.0f, m_perPointSticky[x] - releaseStep * t);
            //     if (x == 3) DEBUG_PRINT(MString("WE ARE RELEASING...") + m_perPointSticky[x]);
            // }
            // else
            // {
            //     // Outside sticky range: ramp back to 1
            //     m_perPointSticky[x] = std::min(1.0f, m_perPointSticky[x] + engageStep);
            //     if (x == 3) DEBUG_PRINT(MString("RE ENGAGE..."));
            // }

            localStickyAmount *= m_perPointSticky[x];
        }


        // ------- seems controllable
        double range = stickyMaxThreshold - stickyMinThreshold;
        double stickyFullAmount = localStickyAmount * stickyMaxThreshold; // Make sure we reach max to override the dist vals!
        double stickyMinRange = stickyMinThreshold - (range + (1.0 - stickyFullAmount)) - (cornerRelaxValue*2);   // Distance where stickiness starts (min range)
        double stickyMaxRange = stickyMaxThreshold - (range + (1.0 - stickyFullAmount)) - (cornerRelaxValue*2);   // Distance where stickiness is maxed out (max range)

        // Map distance to a 0-1 signal based on min/max range
        double distanceWeight = std::clamp(
            (distanceFromMid - stickyMaxRange) / (stickyMinRange - stickyMaxRange),
            0.0,
            1.0
        );

        double blendValue = std::pow(distanceWeight, stickySharpness + 1.0); // controls the curve make it up down
        blendVals[x] = blendValue;  // Store for blur pass in later stage




        if (autoAnim)
        {
            if (m_isClosing[x])
                blendVals[x] = m_lastBlend[x];
            m_lastDistances[x] = distanceFromMid;
            m_lastBlend[x] = blendVals[x];
            // blendVals[x] *= m_perPointSticky[x]; // apply per-point sticky fade
            // blendVals[x] -= m_perPointSticky[x]; // apply per-point sticky fade
        }

    }

    // Sequentially blur the values with a small gaussian like kernel of before and after
    int passes = std::max(1, static_cast<int>(stickyFalloff * 3));  // 0->1, 1->3 passes
    double strength = std::min(1.0, stickyFalloff * 2.0);  // intensity per pass

    for (int p = 0; p < passes; p++)
    {
        std::vector<double> blurred = blendVals;
        for (int x = 0; x < m_largerCount; x++)
        {
            int left = std::max(x - 1, 0);
            int right = std::min(x + 1, m_largerCount - 1);
            double smoothed = blendVals[left] * 0.25 +
                              blendVals[x] * 0.5 +
                              blendVals[right] * 0.25;

            blurred[x] = blendVals[x] * (1.0 - strength) + smoothed * strength;
        }

        blendVals = blurred;
    }


    // ------------------ blend weights computation and cache reset
    if (m_computeCaches)
    {
        m_vertexCache.clear();
        m_edgeVertices.clear();

        // We need to create the initial map with the actively deformed points
        for (int x = 0; x < m_largerCount; x++)
        {
            int posAMeshIndex = (*m_larger)[x];
            int posBMeshIndex = (*m_smaller)[m_resampleOriginalIndex[x]];

            // The main vertices have 100% of the weight...
            m_vertexCache[posAMeshIndex] = { 1.0, posAMeshIndex };
            m_vertexCache[posBMeshIndex] = { 1.0, posBMeshIndex };
            m_edgeVertices.insert(posAMeshIndex);
            m_edgeVertices.insert(posBMeshIndex);
        }

        // Now we propagate the weights...
        MItMeshVertex neighborIter(meshObj);

        for (int pass = 0; pass < propagationPasses; pass++)
        {
            std::unordered_map<int, VertexCache> newCache = m_vertexCache;

            // double t             = (propagationPasses > 1) ? static_cast<double>(pass) / (propagationPasses - 1) : 0.0;
            // double cosine        = (std::cos(t * M_PI) + 1.0) / 2.0;
            // double passFactor    = cosine;
            // double passFactor    = t;
            // double t             = (propagationPasses > 1) ? static_cast<double>(pass) / (propagationPasses) : 0.0;
            // double cosine        = (std::cos(t * M_PI) + 1.0) / 2.0;
            // double passFactor = t;
            // double passFactor    = std::pow(cosine, std::pow(2.0, propagateTension));
            // double passFactor    = std::pow(cosine, std::pow(2.0, propagateTension));

            for (const auto& [idx, cache] : m_vertexCache)
            {
                if (cache.normalizedWeight <= 0.01)
                    continue;

                int prevIdx = 0;
                neighborIter.setIndex(idx, prevIdx);
                MIntArray neighborList;
                neighborIter.getConnectedVertices(neighborList);

                double propagatedWeight = cache.normalizedWeight;//cache.normalizedWeight * passFactor;

                for (unsigned int i = 0; i < neighborList.length(); i++)
                {
                    int neighborIdx = neighborList[i];
                    if (m_edgeVertices.contains(neighborIdx)) continue;

                    auto it = newCache.find(neighborIdx);
                    if (it == newCache.end() || propagatedWeight > it->second.normalizedWeight)
                    {
                        // inherit source from whoever is propagating to this neighbor
                        newCache[neighborIdx] = { propagatedWeight, cache.sourceIdx, pass };
                    }
                }
            }

            m_vertexCache = std::move(newCache);
        }

        // Turn off computes for both cache blocks until somethings dirties it
        m_computeCaches = false;
        DEBUG_PRINT("Cache clean, set compute to false...");

    }

    // Apply the computed blend positions to the actively deformed points,
    // these actual points will then be used to propagate the deformations!
    // bool collision_only = true;
    // MVector forward{0, 0, 1}; // for collision mode.. need to guess it


    std::unordered_map<int, MVector> edgeOriginalPos;
    std::unordered_map<int, MVector> edgeTargetPos;

    for (int x = 0; x < m_largerCount; x++)
    {
        const MVector& posA = largerPoints[x];
        int posAMeshIndex = (*m_larger)[x];

        int smallerIdx = m_resampleOriginalIndex[x];
        const MVector& posB = smallerPoints[smallerIdx];
        int posBMeshIndex = (*m_smaller)[smallerIdx];


        double blendVal = blendVals[x];
        MVector posM = (posA + posB) * 0.5;

        // Delta from A toward B
        MVector delta = posB - posA;


        edgeOriginalPos[posAMeshIndex] = posA;
        edgeOriginalPos[posBMeshIndex] = posB;
        edgeTargetPos[posAMeshIndex] = posA + blendVal * (posM - posA);
        edgeTargetPos[posBMeshIndex] = posB + blendVal * (posM - posB);

    }

    // Now we can deform the mesh in one single pass
    int count = 0;

    // for (; !iter.isDone(); iter.next())
    // {
    //     const int idx = iter.index();
    //
    //     auto targetIt = edgeTargetPos.find(idx);
    //     if (targetIt == edgeTargetPos.end()) continue;
    //
    //     MPoint pos = iter.position(MSpace::kObject);
    //     MPoint target(targetIt->second);
    //
    //     iter.setPosition(target, MSpace::kObject);
    // }

    for (; !iter.isDone(); iter.next())
    {
        const int idx = iter.index();

        auto cacheIt = m_vertexCache.find(idx);
        if (cacheIt == m_vertexCache.end()) continue;

        const VertexCache& cache = cacheIt->second;

        auto targetIt   = edgeTargetPos.find(cache.sourceIdx);
        auto originalIt = edgeOriginalPos.find(cache.sourceIdx);
        if (targetIt == edgeTargetPos.end() || originalIt == edgeOriginalPos.end())
            continue;

        MPoint pos = iter.position(MSpace::kObject);

        MVector sourceDisplacement = targetIt->second - originalIt->second;
        MPoint  deformed;
        if (cache.reachedAtPass == -1)
            deformed = pos + cache.normalizedWeight * sourceDisplacement;
        else
        {
            double t = static_cast<double>(cache.reachedAtPass + 1) / (propagationPasses + 1);

            // holdInfluence stretches/squishes the cosine width
            // double tScaled = std::min(1.0, t / std::max(static_cast<double>(propagateHoldInfluence), 0.001));
            // double cosine = (std::cos(tScaled * M_PI) + 1.0) / 2.0;

            // double s = tScaled * tScaled * tScaled * (tScaled * (tScaled * 6.0 - 15.0) + 10.0);
            // double cosine = (std::cos(s * M_PI) + 1.0) / 2.0;


            double sCurveStrength = propagateHoldInfluence * 2; // 0.0 = no effect, 1.0 = full smoothstep
            double s = t * t * (3.0 - 2.0 * t);
            double blended = t + (s - t) * sCurveStrength;
            double cosine = (std::cos(blended * M_PI) + 1.0) / 2.0;

            double shaped;
            if (propagateTension >= 0.0)
            {
                double exp = std::pow(4.0, propagateTension);
                shaped = exp == 0.0 ? 1.0 : std::pow(cosine, exp);
            }
            else
            {
                // double bump = std::sin(tScaled * M_PI) * (-propagateTension);
                double bump = std::sin(t * M_PI) * (-propagateTension);
                shaped = std::clamp(cosine + bump, 0.0, 1.0);
            }

            deformed = pos + shaped * propagateInfluence * cache.normalizedWeight * sourceDisplacement;
        }


        iter.setPosition(pos + (deformed - pos) * envelope, MSpace::kObject);
        count++;
    }

    DEBUG_PRINT(MString("Moved ") + count + " points");

    return MS::kSuccess;
}
