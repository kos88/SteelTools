import logging as log
from collections import defaultdict
from typing import Callable
import math

import maya.cmds as cmds
import maya.api.OpenMaya as om
import maya.api.OpenMayaUI as omui
import maya.api.OpenMayaRender as omr

from steelTools import mesh_utils
from steelTools import commands as steel_cmd
from steelTools.interface.settingsHelper import SettingsHelper

logger = log.getLogger(__name__)
logger.setLevel(log.DEBUG)

class StickyMeshHelper:

    @staticmethod
    def clearPreSelection():
        empty_list = om.MSelectionList()
        om.MGlobal.setPreselectionHiliteList(empty_list)

    def __init__(self):
        self.grow_amount = 3
        self.mesh_shape_name: str = ""
        self.current_edges = []
        self.corner_vertices = []

        # Output for deformer creation
        self.current_faces = []
        self.upper_edge = []
        self.lower_edge = []

        self._sel_callback = None
        self._selection_changed_ui_callback = None

    def start(self, selection_changed_ui_callback: Callable = None):
        self._setup_callback()
        self.mesh_shape_name = mesh_utils.get_mesh_from_selection()
        log.debug(f"Started pre-sel util with mesh shape {self.mesh_shape_name}")
        if selection_changed_ui_callback:
            self._selection_changed_ui_callback = selection_changed_ui_callback
            selection_changed_ui_callback()
        self._on_selection_changed()

    def stop(self):
        self.clearPreSelection()
        self._remove_callback()
        self.mesh_shape_name = ""
        self.current_faces = []
        log.debug("Stopped presel util")

    def set_grow_amount(self, amount):
        logger.debug(f"set_grow_amount() called with amount: {amount}")

        if not self.mesh_shape_name:
            logger.debug("No mesh_shape_name set, returning")
            return

        sel_list = om.MGlobal.getActiveSelectionList()
        if sel_list.length() == 0:
            logger.debug("No active selection, returning")
            return

        dag_path, component = sel_list.getComponent(0)
        mesh_name = dag_path.fullPathName().split('|')[-1]

        logger.debug(f"Selected mesh: {mesh_name}, expected: {self.mesh_shape_name}")
        logger.debug(f"Component type: {component.apiTypeStr}")

        if mesh_name == self.mesh_shape_name and component.apiTypeStr == 'kMeshEdgeComponent':
            logger.debug("Proceed to save face area")
            self.grow_amount = amount
            self.store_full_area()
            self._draw_faces_highlight()
        else:
            logger.debug("Selection doesn't match mesh or isn't edges, skipping")

    def find_corner_vertices(self):
        """Find 2 opposite corner vertices with sharpest angles in the edge loop"""
        if not self.mesh_shape_name or not self.current_edges:
            return

        dag_path = self._get_dag_path()
        mesh_fn = om.MFnMesh(dag_path)
        edge_it = om.MItMeshEdge(dag_path)
        vert_it = om.MItMeshVertex(dag_path)

        # Collect unique vertex positions
        verts = {}
        for edge_id in self.current_edges:
            edge_it.setIndex(edge_id)
            for i in range(2):
                v = edge_it.vertexId(i)
                if v not in verts:
                    verts[v] = mesh_fn.getPoint(v, om.MSpace.kWorld)

        # Find longest axis and midpoint
        axis = max('xyz', key=lambda a: max(p.__getattribute__(a) for p in verts.values()) -
                                        min(p.__getattribute__(a) for p in verts.values()))
        mid = (min(p.__getattribute__(axis) for p in verts.values()) +
               max(p.__getattribute__(axis) for p in verts.values())) / 2

        # Find sharpest angle on each side
        best = {'left': [None, float('inf')], 'right': [None, float('inf')]}

        def edge_dir(e_id, from_v):
            edge_it.setIndex(e_id)
            p1, p2 = edge_it.point(0, om.MSpace.kWorld), edge_it.point(1, om.MSpace.kWorld)
            return (p2 - p1).normal() if edge_it.vertexId(0) == from_v else (p1 - p2).normal()

        for v_id, pt in verts.items():
            vert_it.setIndex(v_id)
            loop_edges = [e for e in vert_it.getConnectedEdges() if e in self.current_edges]
            if len(loop_edges) != 2:
                continue

            angle = edge_dir(loop_edges[0], v_id).angle(edge_dir(loop_edges[1], v_id))
            side = 'left' if pt.__getattribute__(axis) < mid else 'right'

            if angle < best[side][1]:
                best[side] = [v_id, angle]

        if best['left'][0] is not None and best['right'][0] is not None:
            self._compute_upper_lower_edges(best['left'][0], best['right'][0])

    def store_full_area(self):
        """Store the affected area from the initial edge loop input"""
        if not self.mesh_shape_name or not self.current_edges:
            return

        sel_list = om.MSelectionList()
        sel_list.add(self.mesh_shape_name)
        dag_path = sel_list.getDagPath(0)

        # Build edge iteration from edge indices
        edge_it = om.MItMeshEdge(dag_path)
        face_ids = set()

        for edge_id in self.current_edges:
            edge_it.setIndex(edge_id)
            connected_faces = edge_it.getConnectedFaces()
            face_ids.update(connected_faces)

        # Grow selection
        for _ in range(self.grow_amount):
            new_faces = set(face_ids)
            for face_id in face_ids:
                face_it = om.MItMeshPolygon(dag_path)
                face_it.setIndex(face_id)
                connected_faces = face_it.getConnectedFaces()
                new_faces.update(connected_faces)
            face_ids = new_faces

        self.current_faces = list(face_ids)
        logger.debug(f"Stored faces {len(self.current_faces)}")

    def auto_compute_thresholds(self):
        """Walk both edge loops in the same direction and find some decent initial thresholds"""
        if not self.mesh_shape_name or not self.current_edges or not all([self.upper_edge, self.lower_edge]):
            return None

        ab_up = self.upper_edge[0]
        ab_down = self.lower_edge[0]

        dag_path = self._get_dag_path()
        mesh_fn = om.MFnMesh(dag_path)

        up_ids = mesh_fn.getEdgeVertices(ab_up)
        up_pt_a: om.MPoint = mesh_fn.getPoint(up_ids[0])
        up_pt_b: om.MPoint = mesh_fn.getPoint(up_ids[1])
        down_ids = mesh_fn.getEdgeVertices(ab_down)
        down_pt_a: om.MPoint = mesh_fn.getPoint(down_ids[0])
        down_pt_b: om.MPoint = mesh_fn.getPoint(down_ids[1])

        tolerance = 0.0001
        reverse_needed = False
        unmatching = []
        for up_pt in (up_pt_a, up_pt_b):
            for down_pt in (down_pt_a, down_pt_b):
                if up_pt.distanceTo(down_pt) >= tolerance:
                    unmatching.append(True)
                else:
                    unmatching.append(False)


        if all(unmatching): # We need to reverse one...
            reverse_needed = True
            logger.debug("Edges are going in opposite directions...")
        else:
            logger.debug("At least two start points below tolerance... same direction")

        max_distance: float = 0
        safe_count = min(len(self.upper_edge), len(self.lower_edge))

        # Try to understand how curved is the shape.
        # very linear edges will need a higher threshold to look good as the overall average distance
        # is the same while very concave curves will need less as there is a big difference
        mids = []

        for x in range(safe_count):
            index_up = x
            index_down = -x if reverse_needed else x
            curr_up_ids = mesh_fn.getEdgeVertices(self.upper_edge[index_up])
            curr_down_ids = mesh_fn.getEdgeVertices(self.lower_edge[index_down])
            # We check just one point..
            pt_up = mesh_fn.getPoint(curr_up_ids[0], space=om.MSpace.kObject)
            pt_down = mesh_fn.getPoint(curr_down_ids[0], space=om.MSpace.kObject)
            dist = pt_up.distanceTo(pt_down)
            logger.debug(f"Dist between {curr_up_ids[0]} - {curr_down_ids[0]}: {dist}")
            if dist > max_distance:
                max_distance = dist

            # Curve variance
            mids.append((om.MVector(pt_up) + om.MVector(pt_down)) / 2)

        curve_mult = 1.0

        if len(mids) >= 3:
            p1 = mids[0]
            p2 = mids[len(mids) // 4]
            p3 = mids[len(mids) // 2]

            chord = (p3 - p1).length()
            a = (p2 - p1).length()
            b = (p3 - p2).length()
            c = (p3 - p1).length()
            area = ((p2 - p1) ^ (p3 - p1)).length() / 2
            radius = (a * b * c) / (4 * area) if area > 1e-9 else float('inf')

            # normalized: tight circle → small, flat arc → large
            curvature = chord / radius if radius < float('inf') else 0.0
            logger.debug(f"Curve radius={radius:.3f} chord={chord:.3f} curvature={curvature:.3f}")

            # curvature ~1.1 = flat → 1.5, curvature ~1.3 = curved → 0.9
            curve_mult = 1.5 - (curvature - 1.0) * 3.0
            curve_mult = max(0.9, min(2.0, curve_mult))
            logger.debug(f"Curve curvature={curvature:.3f} mult={curve_mult:.3f}")

        max_t = max_distance * curve_mult
        min_t = max_distance / (2.5 / curve_mult)
        logger.debug(f"Found thresholds {max_t} {min_t}")
        return max_t, min_t

    # ---------------------------------------------------------------------------------------------------------------- #
    def _get_dag_path(self) -> om.MDagPath:
        sel_list = om.MSelectionList()
        sel_list.add(self.mesh_shape_name)
        return sel_list.getDagPath(0)

    def _setup_callback(self):
        self._remove_callback()
        self._sel_callback = om.MEventMessage.addEventCallback("SelectionChanged",
                                                               self._on_selection_changed)
        logger.debug("Setup selection callback")

    def _remove_callback(self):
        if self._sel_callback:
            om.MMessage.removeCallback(self._sel_callback)
            self._sel_callback = None
            logger.debug("Removed selection callback")

    def _compute_upper_lower_edges(self, left_idx: int, right_idx: int):
        """Split edge loop into upper/lower by walking from corner to corner"""
        if not self.current_edges:
            return

        self.corner_vertices = [left_idx, right_idx]

        dag_path = self._get_dag_path()
        edge_it = om.MItMeshEdge(dag_path)

        # Create two lookups one direct and one reverse to "efficiently" walk on edges
        vertex_to_edges = defaultdict(list)
        edge_to_vertices = dict()

        for edge_id in self.current_edges:
            edge_it.setIndex(edge_id)
            vertex_0 = edge_it.vertexId(0)
            vertex_1 = edge_it.vertexId(1)

            # Avoid all the dance for adding data if key not there with default dict
            vertex_to_edges[vertex_0].append(edge_id)
            vertex_to_edges[vertex_1].append(edge_id)
            edge_to_vertices[edge_id] = (vertex_0, vertex_1)

        def walk_edge_loop(start_edge, start_vertex, end_vertex):
            """Walk from start_vertex to end_vertex along the edge loop"""
            path = []
            current_edge = start_edge
            current_vertex = start_vertex
            visited_edges = set()

            while current_edge and current_edge not in visited_edges:
                visited_edges.add(current_edge)
                path.append(current_edge)

                # Get the other vertex of this edge
                vertex_0, vertex_1 = edge_to_vertices[current_edge]
                current_vertex = vertex_1 if current_vertex == vertex_0 else vertex_0

                if current_vertex == end_vertex:
                    break

                # Find next unvisited edge from this vertex
                edges = vertex_to_edges.get(current_vertex, [])
                unvisited = [e for e in edges if e not in visited_edges]
                current_edge = unvisited[0] if unvisited else None

            return path

        self.upper_edge = walk_edge_loop(vertex_to_edges[left_idx][0], left_idx, right_idx)
        self.lower_edge = walk_edge_loop(vertex_to_edges[left_idx][1], left_idx, right_idx)
        self._draw_vertices_highlight()

    def _on_selection_changed(self, *args):
        self.mesh_shape_name = mesh_utils.get_mesh_from_selection()

        if self._selection_changed_ui_callback:
            self._selection_changed_ui_callback()

        if not self.mesh_shape_name:
            return

        sel_list = om.MGlobal.getActiveSelectionList()
        if sel_list.length() == 0:
            self.clearPreSelection()
            return

        dag_path, component = sel_list.getComponent(0)
        mesh_name = dag_path.fullPathName().split('|')[-1]

        # Store the initial edges if any...
        if mesh_name != self.mesh_shape_name or component.apiTypeStr != 'kMeshEdgeComponent':
            self.clearPreSelection()
            return

        edge_comp = om.MFnSingleIndexedComponent(component)
        self.current_edges = list(edge_comp.getElements())
        logger.debug(f"Saved num initial edges {len(self.current_edges)}")

    def _highlightMeshComponents(self, selection: om.MSelectionList):
        om.MGlobal.setPreselectionHiliteList(selection)
        logger.debug(f"Preselection faces: {self.current_faces}")
        logger.debug(f"Preselection list length: {selection.length()}")

        # Force complete viewport refresh
        view = omui.M3dView.active3dView()
        view.refresh(True, True, True)  # force all
        view.scheduleRefresh()  # schedule next refresh cycle

    def _draw_vertices_highlight(self):
        """Highlight the break vertices"""
        if not self.mesh_shape_name or not self.corner_vertices:
            self.clearPreSelection()
            return

        sel_list = om.MSelectionList()
        sel_list.add(self.mesh_shape_name)
        dag_path = sel_list.getDagPath(0)

        highlight_list = om.MSelectionList()
        vert_comp = om.MFnSingleIndexedComponent()
        vert_obj = vert_comp.create(om.MFn.kMeshVertComponent)
        for vert_id in self.corner_vertices:
            vert_comp.addElement(vert_id)

        highlight_list.add((dag_path, vert_obj))
        self._highlightMeshComponents(highlight_list)

    def _draw_faces_highlight(self):
        if not self.mesh_shape_name or not self.current_edges:
            self.clearPreSelection()
            return

        sel_list = om.MSelectionList()
        sel_list.add(self.mesh_shape_name)
        dag_path = sel_list.getDagPath(0)

        highlight_list = om.MSelectionList()
        face_comp = om.MFnSingleIndexedComponent()
        face_obj = face_comp.create(om.MFn.kMeshPolygonComponent)
        for face_id in self.current_faces:
            face_comp.addElement(face_id)

        highlight_list.add((dag_path, face_obj))
        self._highlightMeshComponents(highlight_list)


class StickyLipsValues:

    def __init__(self):
        self.settings_helper = SettingsHelper("StickyLips")

    @property
    def area_tag_name(self) -> str:
        return self.settings_helper.get_value("area_tag_name", "stickyLipsGeo")

    @area_tag_name.setter
    def area_tag_name(self, value: str):
        self.settings_helper.set_value("area_tag_name", value)

    @property
    def upper_tag_name(self) -> str:
        return self.settings_helper.get_value("upper_tag_name", "stickyLipsUpperEdge")

    @upper_tag_name.setter
    def upper_tag_name(self, value: str):
        self.settings_helper.set_value("upper_tag_name", value)

    @property
    def lower_tag_name(self) -> str:
        return self.settings_helper.get_value("lower_tag_name", "stickyLipsLowerEdge")

    @lower_tag_name.setter
    def lower_tag_name(self, value: str):
        self.settings_helper.set_value("lower_tag_name", value)

    @property
    def propagate_amount(self) -> int:
        return self.settings_helper.get_value("propagate_amount", 5)

    @propagate_amount.setter
    def propagate_amount(self, value: int):
        self.settings_helper.set_value("propagate_amount", value)

    @property
    def propagate_influence(self) -> float:
        return self.settings_helper.get_value("propagate_influence", 0.9)

    @propagate_influence.setter
    def propagate_influence(self, value: float):
        self.settings_helper.set_value("propagate_influence", value)

    @property
    def propagate_tension(self) -> int:
        return self.settings_helper.get_value("propagate_tension", 1)

    @propagate_tension.setter
    def propagate_tension(self, value: int):
        self.settings_helper.set_value("propagate_tension", value)

    @property
    def propagate_edge_tension(self) -> float:
        return self.settings_helper.get_value("propagate_edge_tension", 0.5)

    @propagate_edge_tension.setter
    def propagate_edge_tension(self, value: float):
        self.settings_helper.set_value("propagate_edge_tension", value)

    @property
    def max_threshold(self) -> float:
        return self.settings_helper.get_value("max_threshold", 1.0)

    @max_threshold.setter
    def max_threshold(self, value: float):
        self.settings_helper.set_value("max_threshold", value)

    @property
    def min_threshold(self) -> float:
        return self.settings_helper.get_value("min_threshold", 0.5)

    @min_threshold.setter
    def min_threshold(self, value: float):
        self.settings_helper.set_value("min_threshold", value)

    @property
    def edge_smooth(self) -> int:
        return self.settings_helper.get_value("edge_smooth", 1)

    @edge_smooth.setter
    def edge_smooth(self, value: int):
        self.settings_helper.set_value("edge_smooth", value)

    @property
    def edge_sharpness(self) -> float:
        return self.settings_helper.get_value("edge_sharpness", 1.0)

    @edge_sharpness.setter
    def edge_sharpness(self, value: float):
        self.settings_helper.set_value("edge_sharpness", value)

    @property
    def corner_auto_relax(self) -> float:
        return self.settings_helper.get_value("corner_auto_relax", 1.0)

    @corner_auto_relax.setter
    def corner_auto_relax(self, value: float):
        self.settings_helper.set_value("corner_auto_relax", value)

    @property
    def corner_auto_relax_start_angle(self) -> float:
        return self.settings_helper.get_value("corner_auto_relax_start_angle", 20.0)

    @corner_auto_relax_start_angle.setter
    def corner_auto_relax_start_angle(self, value: float):
        self.settings_helper.set_value("corner_auto_relax_start_angle", value)

    @property
    def corner_auto_relax_end_angle(self) -> float:
        return self.settings_helper.get_value("corner_auto_relax_end_angle", 45.0)

    @corner_auto_relax_end_angle.setter
    def corner_auto_relax_end_angle(self, value: float):
        self.settings_helper.set_value("corner_auto_relax_end_angle", value)


class StickyLipsCreator:

    WINDOW_NAME = "SteelStickyLipsWindow"
    WINDOW_TITLE = "Steel Sticky Lips"

    def __init__(self):
        self.mesh_helper = StickyMeshHelper()
        self.values_helper = StickyLipsValues()

        # Widgets
        self.setup_text_hint = None
        self.grow_amount_slider = None
        self.corner_vertices_text = None

        # tag names
        self.lip_area_comp_text_field = None
        self.up_edge_comp_text_field = None
        self.low_edge_comp_text_field = None

        # deformer attributes
        self.sticky_max_slider = None
        self.sticky_min_slider = None
        self.sticky_smooth_slider = None

        self.use_corner_relax_check_box = None
        self.corner_auto_relax_slider = None
        self.auto_relax_start_slider = None
        self.auto_relax_end_slider = None

        self.influence_amount_slider = None
        self.tension_amount_slider = None
        self.edge_tension_amount_slider = None

    @property
    def has_window(self) -> bool:
        return cmds.window(self.WINDOW_NAME, exists=True)

    def __del__(self):
        self.mesh_helper.stop()

    def _build_ui(self):
        if cmds.window(self.WINDOW_NAME, exists=True):
            cmds.deleteUI(self.WINDOW_NAME)

        window = cmds.window(
            self.WINDOW_NAME,
            title=self.WINDOW_TITLE,
            widthHeight=(400, 300),
            sizeable=True,
            resizeToFitChildren=False
        )

        main_form = cmds.formLayout()

        # Scrollable content
        scroll = cmds.scrollLayout(childResizable=True)
        cmds.columnLayout(adjustableColumn=True, rowSpacing=5, margins=10)

        self._build_selection_area()
        self._build_deformer_options()
        self._build_tag_names_options()
        self._build_buttons(main_form, scroll)

        cmds.showWindow(window)
        self.mesh_helper.start(self._on_selection_changed)

    def _on_selection_changed(self):
        if not self.mesh_helper.mesh_shape_name:
            cmds.text(self.setup_text_hint, edit=True, label="Select the lips edge loop to apply the deformer to")
        elif self.mesh_helper.current_edges:
            cmds.text(self.setup_text_hint, edit=True, label="Drag the slider to define the influence area.")
        else:
            cmds.text(self.setup_text_hint, edit=True, label="Select an edge loop to apply the deformer to")

    def _save_settings(self):
        self.values_helper.max_threshold = cmds.floatSliderGrp(self.sticky_max_slider, query=True, value=True)
        self.values_helper.min_threshold = cmds.floatSliderGrp(self.sticky_min_slider, query=True, value=True)
        self.values_helper.edge_smooth = cmds.floatSliderGrp(self.sticky_smooth_slider, query=True, value=True)
        self.values_helper.edge_sharpness = cmds.floatSliderGrp(self.sticky_sharp_slider, query=True, value=True)

        corner_relax_active = cmds.checkBoxGrp(self.use_corner_relax_check_box, query=True, value1=True)
        self.values_helper.corner_auto_relax = cmds.floatSliderGrp(self.corner_auto_relax_slider, query=True, value=True) if corner_relax_active else 0.0
        self.values_helper.corner_auto_relax_start_angle = cmds.floatSliderGrp(self.auto_relax_start_slider, query=True, value=True)
        self.values_helper.corner_auto_relax_end_angle = cmds.floatSliderGrp(self.auto_relax_end_slider, query=True, value=True)

        self.values_helper.propagate_amount = cmds.intSliderGrp(self.grow_amount_slider, query=True, value=True)
        self.values_helper.propagate_hold = cmds.intSliderGrp(self.tension_amount_slider, query=True, value=True)
        self.values_helper.propagate_influence = cmds.floatSliderGrp(self.influence_amount_slider, query=True, value=True)
        self.values_helper.propagate_hold_influence = cmds.floatSliderGrp(self.influence_amount_slider, query=True, value=True)

    def _build_selection_area(self):
        cmds.text(label="Setup Area", font="boldLabelFont", align="left")

        cmds.separator(height=10, style="none")
        self.setup_text_hint = cmds.text(label="Select an edge loop to apply the deformer to")
        cmds.separator(height=10, style="none")

        self.grow_amount_slider = cmds.intSliderGrp(
            label="Lips Area Grow: ",
            field=True,
            minValue=0,
            maxValue=50,
            value=self.values_helper.propagate_amount,
            step=1,
            columnWidth=[(1, 140), (2, 60), (3, 180)],
            changeCommand=self._on_grow_amount_change,
            dragCommand=self._on_grow_amount_change
        )

        cmds.separator(height=10, style="none")

        cmds.rowLayout(numberOfColumns=3,
                       columnWidth3=(140, 120, 120),
                       columnAttach=[(1, "left", 0), (2, "both", 5), (3, "both", 5)])

        cmds.text(label="Corner Vertices:", align="right", width=140)
        cmds.button(label="Detect", command=self._on_auto_detect_corners)
        self.corner_vertices_text = cmds.text(label="...")
        cmds.setParent("..")
        # cmds.intFieldGrp( numberOfFields=2, label='Index', extraLabel='cm', value1=3, value2=5)

        cmds.separator(height=10, style="none")

        cmds.separator(height=10, style="none")
        cmds.text(label="Set the pose where the sticky effect should be completely gone, then set the thresholds")
        cmds.separator(height=10, style="none")

        self.sticky_max_slider = cmds.floatSliderGrp(
            label="Sticky Max Threshold: ",
            field=True,
            minValue=0.0,
            fieldMinValue=0.0,
            fieldMaxValue=5.0,
            value=self.values_helper.max_threshold,
            step=0.1,
            columnWidth=[(1, 140), (2, 60), (3, 180)]
        )

        self.sticky_min_slider = cmds.floatSliderGrp(
            label="Sticky Min Threshold: ",
            field=True,
            minValue=0.0,
            fieldMinValue=0.0,
            fieldMaxValue=self.values_helper.min_threshold,
            value=0.5,
            step=0.1,
            columnWidth=[(1, 140), (2, 60), (3, 180)]
        )

        cmds.rowLayout(numberOfColumns=3,
                       columnWidth3=(140, 120, 120),
                       columnAttach=[(1, "left", 0), (2, "both", 5), (3, "both", 5)])

        cmds.text(label="Auto Detect Thresholds:", align="right", width=140)
        cmds.button(label="From Current", command=self._on_auto_compute_thresholds)
        cmds.setParent("..")


    def _build_deformer_options(self):
        cmds.frameLayout(
            "advancedGroup",
            label="More Options",
            collapsable=True,
            collapse=False
        )

        # cmds.text(label="Main Parameters", font="boldLabelFont", align="left")


        cmds.separator(height=15, style="in")
        self.sticky_smooth_slider = cmds.floatSliderGrp(
            label="Sticky Edge Smoothness: ",
            field=True,
            minValue=self.values_helper.edge_smooth,
            fieldMinValue=0.0,
            fieldMaxValue=50.0,
            value=5.0,
            step=0.1,
            columnWidth=[(1, 140), (2, 60), (3, 180)]
        )

        self.sticky_sharp_slider = cmds.floatSliderGrp(
            label="Sticky Edge Sharpness: ",
            field=True,
            minValue=self.values_helper.edge_sharpness,
            fieldMinValue=0.0,
            fieldMaxValue=50.0,
            value=5.0,
            step=0.1,
            columnWidth=[(1, 140), (2, 60), (3, 180)]
        )

        cmds.separator(height=15, style="in")


        cmds.columnLayout(adjustableColumn=True, rowSpacing=3)

        cmds.separator(height=5, style="none")
        self.use_corner_relax_check_box = cmds.checkBoxGrp(
            label="Corner Auto Relax",
            value1=True,
            columnWidth=[(1, 150)],
            changeCommand=self._on_advanced_toggle
        )

        cmds.separator(height=5, style="none")

        self.corner_auto_relax_slider = cmds.floatSliderGrp(
            label="Corner Auto Relax: ",
            field=True,
            minValue=0,
            maxValue=1.0,
            value=self.values_helper.corner_auto_relax,
            step=0.1,
            columnWidth=[(1, 140), (2, 60), (3, 180)]
        )

        self.auto_relax_start_slider = cmds.floatSliderGrp(
            label="Relax Start Angle: ",
            field=True,
            minValue=0.0,
            maxValue=90.0,
            value=self.values_helper.corner_auto_relax_start_angle,
            columnWidth=[(1, 140), (2, 60), (3, 180)]
        )

        self.auto_relax_end_slider = cmds.floatSliderGrp(
            label="Relax End Angle: ",
            field=True,
            minValue=0.0,
            maxValue=90.0,
            value=self.values_helper.corner_auto_relax_end_angle,
            columnWidth=[(1, 140), (2, 60), (3, 180)]
        )

        cmds.separator(height=15, style="in")
        self.influence_amount_slider = cmds.floatSliderGrp(
            label="Propagate Influence: ",
            field=True,
            minValue=0,
            maxValue=1.0,
            value=self.values_helper.propagate_influence,
            step=0.1,
            columnWidth=[(1, 140), (2, 60), (3, 180)]
        )

        self.tension_amount_slider = cmds.intSliderGrp(
            label="Propagate Tension: ",
            field=True,
            minValue=0,
            maxValue=10,
            value=self.values_helper.propagate_tension,
            columnWidth=[(1, 140), (2, 60), (3, 180)]
        )

        self.edge_tension_amount_slider = cmds.floatSliderGrp(
            label="Propagate Edge Tension: ",
            field=True,
            minValue=0.0,
            maxValue=1.0,
            value=self.values_helper.propagate_edge_tension,
            columnWidth=[(1, 140), (2, 60), (3, 180)]
        )

        cmds.separator(height=15, style="in")
        cmds.setParent("..")
        cmds.setParent("..")


    def _build_tag_names_options(self):
        cmds.frameLayout(label="Component Tag Names", collapsable=True)
        # cmds.separator(height=20, style="in")
        # cmds.text(label="Component Tag Names", font="boldLabelFont", align="left")
        # cmds.separator(height=10, style="none")
        self.lip_area_comp_text_field = cmds.textFieldGrp(label="Lip Area", text=self.values_helper.area_tag_name)
        self.up_edge_comp_text_field = cmds.textFieldGrp(label="Upper Edge", text=self.values_helper.upper_tag_name)
        self.low_edge_comp_text_field = cmds.textFieldGrp(label="Lower Edge", text=self.values_helper.lower_tag_name)

        cmds.setParent("..")
        cmds.separator(height=20, style="in")

    def _build_buttons(self, main_form, scroll):
        # Buttons on top, outside scroll!
        cmds.setParent(main_form)
        create_btn = cmds.button(label="Create", command=self.create_deformer)
        close_btn = cmds.button(label="Close", command=self._on_close)
        cmds.formLayout(main_form, edit=True,
                        attachForm=[
                            (scroll, 'left', 0), (scroll, 'right', 0), (scroll, 'top', 0),
                            (create_btn, 'left', 5), (create_btn, 'bottom', 5),
                            (close_btn, 'right', 5), (close_btn, 'bottom', 5)
                        ],
                        attachControl=[
                            (scroll, 'bottom', 5, create_btn)
                        ],
                        attachPosition=[
                            (create_btn, 'right', 5, 50),
                            (close_btn, 'left', 5, 50)
                        ]
                        )
        cmds.setParent("..")

    # ------------------------------------------------------------------------
    # Callbacks
    # ------------------------------------------------------------------------
    def _on_advanced_toggle(self, checked):
        state = cmds.checkBoxGrp(self.use_corner_relax_check_box, query=True, value1=True)
        cmds.floatSliderGrp(self.auto_relax_start_slider, edit=True, enable=state)
        cmds.floatSliderGrp(self.auto_relax_end_slider, edit=True, enable=state)
        cmds.floatSliderGrp(self.corner_auto_relax_slider, edit=True, enable=state)

    def _on_grow_amount_change(self, *args):
        amount = cmds.intSliderGrp(self.grow_amount_slider, query=True, value=True)
        self.mesh_helper.set_grow_amount(amount)

    def _on_auto_detect_corners(self, *args):
        self.mesh_helper.find_corner_vertices()
        if self.mesh_helper.corner_vertices:
            vtx_a = self.mesh_helper.corner_vertices[0]
            vtx_b = self.mesh_helper.corner_vertices[1]
            cmds.text(self.corner_vertices_text, edit=True, label=f"Stored: {vtx_a} {vtx_b}")

    def _on_auto_compute_thresholds(self, *args):
        if not self.mesh_helper.corner_vertices:
            self._on_auto_detect_corners()

        thresholds = self.mesh_helper.auto_compute_thresholds()
        if thresholds:
            cmds.floatSliderGrp(self.sticky_max_slider, e=True, v=thresholds[0])
            cmds.floatSliderGrp(self.sticky_min_slider, e=True, v=thresholds[1])


    def _on_close(self, *args):
        self._save_settings()
        self.mesh_helper.stop()
        cmds.deleteUI(self.WINDOW_NAME)

    # ------------------------------------------------------------------------
    # Public methods
    # ------------------------------------------------------------------------
    def show_ui(self):
        self._build_ui()

    def create_deformer(self, *args):
        # Make sure we have data
        if not self.mesh_helper.mesh_shape_name:
            self.mesh_helper.start()

        if not self.mesh_helper.mesh_shape_name:
            cmds.error("Select a mesh edge loop to proceed")
            return

        if not self.mesh_helper.current_faces:
            self.mesh_helper.store_full_area()
        if not self.mesh_helper.upper_edge or not self.mesh_helper.lower_edge:
            self.mesh_helper.find_corner_vertices()

        # Store settings from UI to persistent data, if we have shown a window or use last
        if self.has_window:
            self._save_settings()

        lip_area_component_tag_name = self.values_helper.area_tag_name
        upper_edge_component_tag_name = self.values_helper.upper_tag_name
        lower_edge_component_tag_name = self.values_helper.lower_tag_name
        logger.debug(f"Affected vertices \n{self.mesh_helper.current_faces} face component tag {lip_area_component_tag_name}")
        logger.debug(f"Upper Edges: \n{self.mesh_helper.upper_edge} upper edge component tag {upper_edge_component_tag_name}")
        logger.debug(f"Lower Edges: \n{self.mesh_helper.lower_edge} lower edge component tag {lower_edge_component_tag_name}")

        # ------------------------------------------------------------------------------------------------------------ #
        # Create the component tags on the mesh for each group, convert to vertices the face area (as deformers wants vertices)
        mesh_shape_name = self.mesh_helper.mesh_shape_name

        # Convert faces to vertices
        sel_list = om.MSelectionList()
        sel_list.add(mesh_shape_name)
        dag_path = sel_list.getDagPath(0)
        mesh_fn = om.MFnMesh(dag_path)

        vert_ids = set()
        for face_id in self.mesh_helper.current_faces:
            verts = mesh_fn.getPolygonVertices(face_id)
            vert_ids.update(verts)

        # ... we can optimize this if
        # Build vertex selection string
        vert_sel = [f"{dag_path.fullPathName()}.vtx[{v}]" for v in sorted(vert_ids)]
        cmds.componentTag(vert_sel, cr=True, ntn=lip_area_component_tag_name)

        # Build upper edge selection string
        upper_edges = [f"{dag_path.fullPathName()}.e[{e}]" for e in self.mesh_helper.upper_edge]
        cmds.componentTag(upper_edges, cr=True, ntn=upper_edge_component_tag_name)

        # Build lower edge selection string
        lower_edges = [f"{dag_path.fullPathName()}.e[{e}]" for e in self.mesh_helper.lower_edge]
        cmds.componentTag(lower_edges, cr=True, ntn=lower_edge_component_tag_name)

        # Create the deformer
        steel_cmd.create_sticky_lips(mesh_shape_name,
                                     area_component_tag=self.values_helper.area_tag_name,
                                     upper_edge_component_tag=self.values_helper.upper_tag_name,
                                     lower_edge_component_tag=self.values_helper.lower_tag_name,
                                     sticky_seal=self.values_helper.max_threshold,
                                     max_threshold=self.values_helper.max_threshold,
                                     min_threshold=self.values_helper.min_threshold,
                                     edge_smooth=self.values_helper.edge_smooth,
                                     edge_sharpness=self.values_helper.edge_sharpness,

                                     corner_auto_relax=self.values_helper.corner_auto_relax,
                                     auto_relax_start_angle=self.values_helper.corner_auto_relax_start_angle,
                                     auto_relax_end_angle=self.values_helper.corner_auto_relax_end_angle,

                                     propagate_iterations=self.values_helper.propagate_amount,
                                     propagate_influence=self.values_helper.propagate_influence,
                                     propagate_tension=self.values_helper.propagate_tension,
                                     propagate_edge_tension=self.values_helper.propagate_edge_tension,
                                     )

        if self.has_window:
            self._on_close()
