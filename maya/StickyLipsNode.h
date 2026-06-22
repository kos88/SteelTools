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

    static MObject s_propagateTension;

    static MObject s_propagateHoldInfluence;

    static void* creator();
    static MStatus initialize();

    MStatus deform(MDataBlock& block,
                   MItGeometry& iter,
                   const MMatrix& mat,
                   unsigned int multiIndex) override;

    // Schlick's bias: pushes the curve toward 0 or 1 around a pivot
    inline double bias(double t, double b) {
        return t / ((1.0 / b - 2.0) * (1.0 - t) + 1.0);
    }

    // Schlick's gain: S-curve with a controllable steepness at the midpoint
    inline double gain(double t, double g) {
        if (t < 0.5)
            return bias(t * 2.0, g) * 0.5;
        return bias(t * 2.0 - 1.0, 1.0 - g) * 0.5 + 0.5;
    }

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

    static MObject s_propagateInfluence;
    static MObject s_propagateIterations;
    static MObject s_propagateHold;
    static MObject s_propagateHoldTension;

    // static MObject s_stickyAutoAnim;

    static MObject s_EdgeLoopA;
    static MObject s_EdgeLoopB;

    static MString s_upperComponentTag;
    static MString s_lowerComponentTag;
};

#endif // STICKY_LIPS_NODE_H
