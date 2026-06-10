from typing import List
import maya.cmds as cmds

def create_sticky_lips(mesh_name: str,

                       area_component_tag: str ="stickyLipsArea",
                       upper_edge_component_tag: str ="stickyUpperEdge",
                       lower_edge_component_tag: str ="stickyLowerEdge",

                       sticky_seal: float = 2.0,
                       max_threshold: float = 1.0,
                       min_threshold: float = 0.5,
                       edge_smooth: float = 5.0,

                       corner_auto_relax: float = 1.0,
                       auto_relax_start_angle: float = 20.0,
                       auto_relax_end_angle: float = 45.0,

                       influence_propagate=4,
                       influence_smooth=1.0):
    ...
    cmds.undoInfo(openChunk=True)
    try:
        created_name = cmds.deformer(mesh_name, type="steelStickyLips", useComponentTags=True)[0]
        cmds.setAttr(f"{created_name}.sealDistance", sticky_seal)
        cmds.setAttr(f"{created_name}.maxThreshold", max_threshold)
        cmds.setAttr(f"{created_name}.minThreshold", min_threshold)
        cmds.setAttr(f"{created_name}.edgeSmooth", edge_smooth)

        cmds.setAttr(f"{created_name}.cornerAutoRelax", corner_auto_relax)
        cmds.setAttr(f"{created_name}.autoRelaxStartAngle", auto_relax_start_angle)
        cmds.setAttr(f"{created_name}.autoRelaxEndAngle", auto_relax_end_angle)

        cmds.setAttr(f"{created_name}.propagateInfluence", influence_propagate)
        cmds.setAttr(f"{created_name}.propagateSmoothness", influence_smooth)

        cmds.setAttr(f"{created_name}.input[0].componentTagExpression", area_component_tag, type='string')
        cmds.setAttr(f"{created_name}.edgeLoopNameA", upper_edge_component_tag, type='string')
        cmds.setAttr(f"{created_name}.edgeLoopNameB", lower_edge_component_tag, type='string')

    except Exception as e:
        print(f"Error while creating Sticky Lips Deformer: {e}")
    finally:
        cmds.undoInfo(closeChunk=True)



def create_cornea_push(affected_mesh_name: str,
                       push_meshes_names: List[str] = None):
    ...