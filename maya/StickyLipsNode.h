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

    // Attributes
    static MObject s_ForwardInfluence;
    static MObject s_BackwardInfluence;
    static MObject s_Direction;
    static MObject s_EdgeLoopA;
    static MObject s_EdgeLoopB;
};

#endif // STICKY_LIPS_NODE_H
