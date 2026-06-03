//
// Created by Costantino Fracas on 24/05/2026.
//

#ifndef STICKY_LIPS_NODE_H
#define STICKY_LIPS_NODE_H

#include <maya/MPxDeformerNode.h>

class StickyLipsNode : public MPxDeformerNode {
public:
    StickyLipsNode() = default;
    ~StickyLipsNode() override = default;

    static void* creator();
    static MStatus initialize();

    MStatus deform(MDataBlock& block,
                   MItGeometry& iter,
                   const MMatrix& mat,
                   unsigned int multiIndex) override;

    static MTypeId id;
    static MString name;

    // --- Attributes
    static MObject s_stickyAmount;
    static MObject s_distanceMinThreshold;
    static MObject s_distanceMaxThreshold;
    static MObject s_stickyFalloff;

    static MObject s_cornerAutoRelax;
    static MObject s_cornerAutoRelaxEndAngle;
    static MObject s_cornerAutoRelaxStartAngle;
    static MObject s_cornerAutoRelaxDistance;

    static MObject s_propagateSmoothness;
    static MObject s_propagateInfluence;


    static MObject s_EdgeLoopA;
    static MObject s_EdgeLoopB;

    static MString s_upperComponentTag;
    static MString s_lowerComponentTag;
};

#endif // STICKY_LIPS_NODE_H
