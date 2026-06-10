#include "StickyLipsNode.h"
#include "CorneaPushNode.h"

#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>

MStatus initializePlugin(MObject obj) {
    MStatus status;
    MFnPlugin plugin(obj, "Steel - Developed By Costantino Fracas", "0.1.0", "Any");

    status = plugin.registerNode(StickyLipsNode::name,
                                 StickyLipsNode::id,
                                 StickyLipsNode::creator,
                                 StickyLipsNode::initialize,
                                 MPxNode::kDeformerNode);
    if (!status) {
        status.perror("registerNode StickyLipsNode");
        return status;
    }

    status = plugin.registerNode(CorneaPushNode::name,
                                 CorneaPushNode::id,
                                 CorneaPushNode::creator,
                                 CorneaPushNode::initialize,
                                 MPxNode::kDeformerNode);
    if (!status) {
        status.perror("registerNode CorneaPushNode");
        return status;
    }

    // Add python UI
    MGlobal::executePythonCommandOnIdle("from steelTools.interface.menu import steel_menu;steel_menu()");

    return status;
}

MStatus uninitializePlugin(MObject obj) {
    MStatus status;
    MFnPlugin plugin(obj);

    status = plugin.deregisterNode(StickyLipsNode::id);
    if (!status) {
        status.perror("deregisterNode StickyLipsNode");
        return status;
    }

    status = plugin.deregisterNode(CorneaPushNode::id);
    if (!status) {
        status.perror("deregisterNode CorneaPushNode");
        return status;
    }

    // Remove Python UI
    //
    MGlobal::executePythonCommandOnIdle("from steelTools.interface.menu import remove_menu;remove_menu()");

    return status;
}
