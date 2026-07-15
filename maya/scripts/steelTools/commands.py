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

                       auto_anim: bool = False,
                       release_duration_frames: float = 10.0,
                       engage_duration_frames: float = 10.0,
                       anim_relax_start_angle: float = 20.0,
                       anim_relax_end_angle: float = 45.0,
                       anim_relax_segment_portion: float = 0.5,
                       close_distance: float = 0.5,

                       propagate_iterations=4,
                       propagate_influence=1.0,
                       propagate_tension=1,
                       propagate_edge_tension=1.0,
                       ):
    ...
    cmds.undoInfo(openChunk=True)
    try:
        created_name = cmds.deformer(mesh_name, type="steelStickyLips", useComponentTags=True)[0]

        # Main Sticky Controls
        cmds.setAttr(f"{created_name}.stickyAmount", sticky_amount)
        cmds.setAttr(f"{created_name}.maxThreshold", max_threshold)
        cmds.setAttr(f"{created_name}.minThreshold", min_threshold)

        cmds.setAttr(f"{created_name}.edgeRetract", edge_sharpness)
        cmds.setAttr(f"{created_name}.edgeSmooth", edge_smooth)


        # Propagation Settings
        cmds.setAttr(f"{created_name}.propagateIterations", propagate_iterations)
        cmds.setAttr(f"{created_name}.propagateInfluence", propagate_influence)
        cmds.setAttr(f"{created_name}.propagateTension", propagate_tension)
        cmds.setAttr(f"{created_name}.propagateEdgeTension", propagate_edge_tension)

        # Animation Controls
        cmds.setAttr(f"{created_name}.autoAnim", auto_anim)
        cmds.setAttr(f"{created_name}.releaseDurationFrames", release_duration_frames)
        cmds.setAttr(f"{created_name}.engageDurationFrames", engage_duration_frames)
        cmds.setAttr(f"{created_name}.animRelaxStartAngle", anim_relax_start_angle)
        cmds.setAttr(f"{created_name}.animRelaxEndAngle", anim_relax_end_angle)
        cmds.setAttr(f"{created_name}.animRelaxSegmentPortion", anim_relax_segment_portion)
        cmds.setAttr(f"{created_name}.closeDistance", close_distance)

        # Component Tags
        cmds.setAttr(f"{created_name}.input[0].componentTagExpression", area_component_tag, type='string')
        cmds.setAttr(f"{created_name}.edgeLoopNameA", upper_edge_component_tag, type='string')
        cmds.setAttr(f"{created_name}.edgeLoopNameB", lower_edge_component_tag, type='string')

        cmds.connectAttr("time1.outTime", f"{created_name}.currentTime")

    except Exception as e:
        print(f"Error while creating Sticky Lips Deformer: {e}")
    finally:
        cmds.undoInfo(closeChunk=True)



def create_cornea_push(affected_mesh_name: str,
                       push_meshes_names: List[str] = None):
    ...