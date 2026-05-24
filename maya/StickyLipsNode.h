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
    static MObject aForwardInfluence;
    static MObject aBackwardInfluence;
    static MObject aDirection;
    static MObject aEdgeLoop; 
};

#endif // STICKY_LIPS_NODE_H
