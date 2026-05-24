//
// Created by Costantino Fracas on 24/05/2026.
//

#ifndef CORNEA_PUSH_NODE_H
#define CORNEA_PUSH_NODE_H

#include <maya/MPxDeformerNode.h>

class CorneaPushNode : public MPxDeformerNode {
public:
    CorneaPushNode() = default;
    ~CorneaPushNode() override = default;

    static void* creator();
    static MStatus initialize();

    MStatus deform(MDataBlock& block,
                   MItGeometry& iter,
                   const MMatrix& mat,
                   unsigned int multiIndex) override;

    static MTypeId id;
    static MString name;

    // Attributes
    static MObject aUndeformedEyeball;
    static MObject aDeformedEyeball;
};

#endif // CORNEA_PUSH_NODE_H
