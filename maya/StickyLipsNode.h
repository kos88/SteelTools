//
// Created by Costantino Fracas on 24/05/2026.
//

#ifndef STICKY_LIPS_NODE_H
#define STICKY_LIPS_NODE_H
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <maya/MPxDeformerNode.h>

// We need to recompute certain data structures, only when something significant of the mesh changed
// obviously point position only is the only consistent factor that will change,
// but what about the mesh or component tags?
struct MeshFingerprint
{
    int vertCount = 0;
    int polyCount = 0;
    unsigned int hashCode = 0;
    int upperTagCount = 0;
    int lowerTagCount = 0;
    int areaTagCount = 0;
};

// Cached per vertex data to know what needs to be moved and how far
struct VertexCache {
    double normalizedWeight;      // topology based weight
    int    sourceIdx;   // which edge vertex drives this one
    int    reachedAtPass = -1; // when our propagate reached this vertex? -1 is just an original one
};

class StickyLipsNode : public MPxDeformerNode
{
    MeshFingerprint m_lastFingerprint;
    int m_lastPropagateHold = -1;
    int m_lastPropagationPasses = -1;

    bool m_computeCaches = true;

    // We use std containers as we might want to abstract the compute part to run also in other DCC..less refactor later?
    std::vector<int> m_upperPoints;
    std::vector<int> m_lowerPoints;

    // Pointers assigned depending on what has more vertices
    std::vector<int>* m_larger;
    std::vector<int>* m_smaller;
    int m_largerCount = -1;

    // Lookups for getting the original index going from small to big or big to small
    std::vector<int> m_resampleOriginalIndex; // resampled point > original orderedPts pos or originalIndexes mesh index
    std::vector<int> m_originalToResampleIndex; // from the original point, to the closest point on the resampled. Since we ALWAYS increase, we always have a fairly easy match.

    std::vector<double> m_arcLength;

    // Propagation caches
    std::unordered_map<int, VertexCache> m_vertexCache; // vertex id > cache where we know who is driving it and how much
    std::unordered_set<int>              m_edgeVertices;

public:
    StickyLipsNode() = default;
    ~StickyLipsNode() override = default;


    static void* creator();
    static MStatus initialize();

    MStatus deform(MDataBlock& block,
                   MItGeometry& iter,
                   const MMatrix& mat,
                   unsigned int multiIndex) override;

    // --- Register data
    static MTypeId id;
    static MString name;

    // --- Attributes
    static MObject s_stickyAmount;
    static MObject s_distanceMinThreshold;
    static MObject s_distanceMaxThreshold;
    static MObject s_stickyFalloff;
    static MObject s_stickySharpness;
    static MObject s_stickySharpnessStrength;
    static MObject s_stickyFalloffSharpness;

    static MObject s_cornerAutoRelax;
    static MObject s_cornerAutoRelaxEndAngle;
    static MObject s_cornerAutoRelaxStartAngle;
    static MObject s_cornerAutoRelaxDistance;

    static MObject s_propagateHoldInfluence;
    static MObject s_propagateInfluence;
    static MObject s_propagateIterations;
    static MObject s_propagateTension;
    static MObject s_propagateHold;
    static MObject s_propagateHoldTension;

    // static MObject s_stickyAutoAnim;

    static MObject s_EdgeLoopA;
    static MObject s_EdgeLoopB;

    static MString s_upperComponentTag;
    static MString s_lowerComponentTag;
};

#endif // STICKY_LIPS_NODE_H
