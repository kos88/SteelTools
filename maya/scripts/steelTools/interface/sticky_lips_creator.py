import logging as log
from collections import defaultdict
from typing import Callable

import maya.cmds as cmds
import maya.api.OpenMaya as om
import maya.api.OpenMayaUI as omui
import maya.api.OpenMayaRender as omr

from steelTools import mesh_utils
from steelTools import commands as steel_cmd

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

        sel_list = om.MSelectionList()
        sel_list.add(self.mesh_shape_name)
        dag_path = sel_list.getDagPath(0)
        edge_it = om.MItMeshEdge(dag_path)
        vert_it = om.MItMeshVertex(dag_path)
        mesh_fn = om.MFnMesh(dag_path)

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

    # ---------------------------------------------------------------------------------------------------------------- #
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

        sel_list = om.MSelectionList()
        sel_list.add(self.mesh_shape_name)
        dag_path = sel_list.getDagPath(0)
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


class StickyLipsWindow:

    WINDOW_NAME = "SteelStickyLipsWindow"
    WINDOW_TITLE = "Steel Sticky Lips"

    def __init__(self):
        self.mesh_helper = StickyMeshHelper()

        # Widgets
        self.setup_text_hint = None
        self.grow_amount_slider = None

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

        self._build_ui()

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
            value=5,
            step=1,
            columnWidth=[(1, 140), (2, 60), (3, 180)],
            changeCommand=self._on_grow_amount_change,
            dragCommand=self._on_grow_amount_change
        )

        cmds.separator(height=10, style="none")

        cmds.rowLayout(numberOfColumns=3,
                       columnWidth3=(140, 120, 120),
                       columnAttach=[(1, "left", 0), (2, "both", 5), (3, "both", 5)])

        cmds.text(label="Corner Edges:", align="right", width=140)
        cmds.button(label="Highlight detected", command=self._on_auto_detect_corners)
        cmds.setParent("..")


        # cmds.frameLayout(label="Component Tag Names", collapsable=False)
        cmds.separator(height=20, style="in")
        cmds.text(label="Component Tag Names", font="boldLabelFont", align="left")
        cmds.separator(height=10, style="none")
        self.lip_area_comp_text_field = cmds.textFieldGrp(label="Lip Area", text="stickyLipsGeo")
        self.up_edge_comp_text_field = cmds.textFieldGrp(label="Upper Edge", text="stickyLipsUpperEdge")
        self.low_edge_comp_text_field = cmds.textFieldGrp(label="Lower Edge", text="stickyLipsLowerEdge")

        cmds.setParent("..")
        cmds.separator(height=20, style="in")


    def _build_deformer_options(self):
        cmds.frameLayout(
            "advancedGroup",
            label="Deformer Options",
            collapsable=True,
            collapse=True
        )

        # cmds.text(label="Main Parameters", font="boldLabelFont", align="left")
        cmds.separator(height=10, style="none")

        self.sticky_max_slider = cmds.floatSliderGrp(
            label="Sticky Max Threshold: ",
            field=True,
            minValue=0.0,
            fieldMinValue=0.0,
            fieldMaxValue=5.0,
            value=1.0,
            step=0.1,
            columnWidth=[(1, 140), (2, 60), (3, 180)]
        )

        self.sticky_min_slider = cmds.floatSliderGrp(
            label="Sticky Min Threshold: ",
            field=True,
            minValue=0.0,
            fieldMinValue=0.0,
            fieldMaxValue=5.0,
            value=0.5,
            step=0.1,
            columnWidth=[(1, 140), (2, 60), (3, 180)]
        )

        self.sticky_smooth_slider = cmds.floatSliderGrp(
            label="Sticky Edge Smoothness: ",
            field=True,
            minValue=0.0,
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
            maxValue=1,
            value=1.0,
            step=0.1,
            columnWidth=[(1, 140), (2, 60), (3, 180)]
        )

        self.auto_relax_start_slider = cmds.floatSliderGrp(
            label="Relax Start Angle: ",
            field=True,
            minValue=0.0,
            maxValue=90.0,
            value=20.0,
            columnWidth=[(1, 140), (2, 60), (3, 180)]
        )

        self.auto_relax_end_slider = cmds.floatSliderGrp(
            label="Relax End Angle: ",
            field=True,
            minValue=0.0,
            maxValue=90.0,
            value=45.0,
            columnWidth=[(1, 140), (2, 60), (3, 180)]
        )

        cmds.setParent("..")
        cmds.setParent("..")

        cmds.separator(height=15, style="in")

    def _build_buttons(self, main_form, scroll):
        # Buttons on top, outside scroll!
        cmds.setParent(main_form)
        create_btn = cmds.button(label="Create", command=self._on_create)
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

    def _on_create(self, *args):

        # Make sure we have data
        if not self.mesh_helper.current_faces:
            self.mesh_helper.store_full_area()
        if not self.mesh_helper.upper_edge or not self.mesh_helper.lower_edge:
            self.mesh_helper.find_corner_vertices()

        lip_area_component_tag_name = cmds.textFieldGrp(self.lip_area_comp_text_field, q=True, text=True)
        upper_edge_component_tag_name = cmds.textFieldGrp(self.up_edge_comp_text_field, q=True, text=True)
        lower_edge_component_tag_name = cmds.textFieldGrp(self.low_edge_comp_text_field, q=True, text=True)
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

        # ----------------------------------- Pull deformer settings from UI ----------------------------------------- #
        sticky_max_threshold = cmds.floatSliderGrp(self.sticky_max_slider, query=True, value=True)
        sticky_min_threshold = cmds.floatSliderGrp(self.sticky_min_slider, query=True, value=True)
        sticky_smooth_val = cmds.floatSliderGrp(self.sticky_smooth_slider, query=True, value=True)

        corner_relax_active = cmds.checkBoxGrp(self.use_corner_relax_check_box, query=True, value1=True)
        corner_relax_val = cmds.floatSliderGrp(self.corner_auto_relax_slider, query=True, value=True) if corner_relax_active else 0.0
        corner_relax_start_val = cmds.floatSliderGrp(self.auto_relax_start_slider, query=True, value=True)
        corner_relax_end_val = cmds.floatSliderGrp(self.auto_relax_end_slider, query=True, value=True)


        grow_amount_value = cmds.intSliderGrp(self.grow_amount_slider, query=True, value=True)

        # Create the deformer
        steel_cmd.create_sticky_lips(mesh_shape_name,
                                     area_component_tag=lip_area_component_tag_name,
                                     upper_edge_component_tag=upper_edge_component_tag_name,
                                     lower_edge_component_tag=lower_edge_component_tag_name,

                                     max_threshold=sticky_max_threshold,
                                     min_threshold=sticky_min_threshold,
                                     edge_smooth=sticky_smooth_val,

                                     corner_auto_relax=corner_relax_val,
                                     auto_relax_start_angle=corner_relax_start_val,
                                     auto_relax_end_angle=corner_relax_end_val,

                                     propagate_iterations=grow_amount_value)

    def _on_close(self, *args):
        self.mesh_helper.stop()
        cmds.deleteUI(self.WINDOW_NAME)