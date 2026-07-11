import maya.api.OpenMaya as om

def get_mesh_from_selection():
    """Get mesh shape name from any selection using API"""
    sel_list = om.MGlobal.getActiveSelectionList()
    if sel_list.length() == 0:
        return None

    try:
        dag_path = sel_list.getDagPath(0)
    except TypeError:
        return None

    # If we have a transform, extend to the shape
    if dag_path.apiType() == om.MFn.kTransform:
        dag_path.extendToShape()

    # Return the shape name if it's a mesh
    if dag_path.apiType() == om.MFn.kMesh:
        return dag_path.fullPathName().split('|')[-1]

    return None