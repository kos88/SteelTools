from typing import List
import maya.cmds as cmds

def create_sticky_lips(mesh_name: str,

                       area_component_tag: str ="stickyLipsArea",
                       upper_edge_component_tag: str ="stickyUpperEdge",
                       lower_edge_component_tag: str ="stickyLowerEdge",

                       sticky_amount: float = 1.0,
                       max_threshold: float = 1.0,
                       min_threshold: float = 0.5,
                       edge_smooth: float = 5.0,
                       edge_sharpness: float = 1.0,

                       corner_auto_relax: float = 1.0,
                       auto_relax_start_angle: float = 20.0,
                       auto_relax_end_angle: float = 45.0,

                       propagate_iterations=4,
                       propagate_influence=1.0,
                       propagate_tension=1,
                       propagate_edge_tension=1.0,
                       ):
    ...
    cmds.undoInfo(openChunk=True)
    try:
        created_name = cmds.deformer(mesh_name, type="steelStickyLips", useComponentTags=True)[0]
        cmds.setAttr(f"{created_name}.stickyAmount", sticky_amount)
        cmds.setAttr(f"{created_name}.maxThreshold", max_threshold)
        cmds.setAttr(f"{created_name}.minThreshold", min_threshold)

        cmds.setAttr(f"{created_name}.edgeRetract", edge_sharpness)
        cmds.setAttr(f"{created_name}.edgeSmooth", edge_smooth)

        # cmds.setAttr(f"{created_name}.cornerAutoRelax", corner_auto_relax)
        # cmds.setAttr(f"{created_name}.autoRelaxStartAngle", auto_relax_start_angle)
        # cmds.setAttr(f"{created_name}.autoRelaxEndAngle", auto_relax_end_angle)

        cmds.setAttr(f"{created_name}.propagateIterations", propagate_iterations)
        cmds.setAttr(f"{created_name}.propagateInfluence", propagate_influence)
        cmds.setAttr(f"{created_name}.propagateTension", propagate_tension)
        cmds.setAttr(f"{created_name}.propagateEdgeTension", propagate_edge_tension)

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