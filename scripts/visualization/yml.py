import gdsfactory as gf
from gdsfactory.typings import LayerSpec
from IPython.display import Code
import matplotlib.pyplot as plt
import yaml
import time
from gdsfactory.generic_tech import get_generic_pdk
import numpy as np
from copy import deepcopy
import math


class LefYmlIn:
    def __init__(self, filepath=None):
        self.filepath = filepath
        self.data = None
        try:
            # generic_pdk needs to be activated in the new version of gdsfactory
            gf.gpdk.PDK.activate()
        except:
            pass
        self.generic_pdk = get_generic_pdk()
        if filepath:
            self.read(filepath)

    def read(self, filepath):
        with open(filepath, "r") as file:
            self.data = yaml.safe_load(file)

    def print(self):
        print(self.data)

    def load(self, data):
        self.data = data

    def print_components(self):
        print(self.generic_pdk.cells.keys())

    def _create_component_class(
        self,
        comp_name,
        comp_info,
        comp_type,
        polygon_points=None,
        comp_layer: LayerSpec = "WG",
    ):
        # Extract bounding box and calculate dimensions
        x_coords = [point[0] for point in comp_info["bbox"]]
        y_coords = [point[1] for point in comp_info["bbox"]]
        w, h = max(x_coords) - min(x_coords), max(y_coords) - min(y_coords)
        center_x, center_y = (max(x_coords) + min(x_coords)) / 2, (
            max(y_coords) + min(y_coords)
        ) / 2
        if polygon_points:
            polygon_points = [
                [point[0] - center_x, point[1] - center_y] for point in polygon_points
            ]

        def create_segment_component(layer, length):
            if length:
                return gf.components.rectangle(
                    size=(length, h), layer=layer, centered=True
                )
            else:
                return gf.components.rectangle(size=(w, h), layer=layer, centered=True)

        def create_default_component(layer):
            return gf.components.rectangle(size=(w, h), layer=layer, centered=True)

        def create_polygon_component(layer):
            c = gf.Component()
            c.add_polygon(polygon_points, layer=layer)
            return c

        def component(layer: LayerSpec = comp_layer, length=None):
            c = gf.Component()
            if comp_type == "straight_segment":
                comp = create_segment_component(layer, length)
            elif comp_type == "bend_segment" or polygon_points is not None:
                comp = create_polygon_component(layer)
            else:
                comp = create_default_component(layer)
            ref = c.add_ref(comp)
            for pin_name, pin_info in comp_info["pins"].items():
                if comp_type == "straight_segment" and length:
                    center = (
                        np.sign(pin_info["xya"][0] - center_x) * length / 2,
                        pin_info["xya"][1] - center_y,
                    )
                else:
                    center = (
                        pin_info["xya"][0] - center_x,
                        pin_info["xya"][1] - center_y,
                    )
                width = pin_info["width"]
                orientation = pin_info["xya"][2]
                c.add_port(
                    name=pin_name,
                    center=center,
                    width=width,
                    orientation=orientation,
                    layer=layer,
                )
            return c

        component.__name__ = comp_name
        return gf.cell(component)

    def register_components(self):
        # register cells
        for comp, comp_info in self.data["blocks"].items():
            if len(comp_info["bbox"]) != 4:
                raise ValueError("bbox must have 4 points")
            comp_type = "cell"
            if "type" in comp_info:
                comp_type = comp_info["type"]
            polygon_points = None
            if "polygon" in comp_info:
                polygon_points = comp_info["polygon"]
            component_func = self._create_component_class(
                comp, comp_info, comp_type, polygon_points
            )
            if comp not in self.generic_pdk.cells:
                gf.get_active_pdk().register_cells(**{comp: component_func})
                print(f"Component {comp} registered successfully.")

    def plot_component(self, comp_name):
        c = gf.get_component(comp_name).copy()
        c.draw_ports()
        c.plot()
        plt.show()


class DefYmlIn:
    def __init__(self, filepath, is_routed=True):
        self.filepath = filepath
        self.data = None
        self.netlist = None
        self.is_routed = is_routed
        self.raw_yaml = None
        with open(self.filepath, "r") as file:
            self.raw_yaml = yaml.safe_load(file)
            new_yaml = deepcopy(self.raw_yaml)
            timestamp = int(time.time())
            new_yaml["name"] += f"_{timestamp}"
            if "bbox" in new_yaml:
                del new_yaml["bbox"]
            if self.is_routed:
                self.data = gf.read.from_yaml(self.raw_yaml)
            else:
                if "connections" in new_yaml:
                    del new_yaml["connections"]
                self.data = gf.read.from_yaml(new_yaml)
        self.netlist = self.data.get_netlist()

    def patch_component_centering(self):
        # print(inst.cplx_trans.mirror)

        for inst in self.data.insts:
            rotation = inst.cplx_trans.angle
            mirror = inst.cplx_trans.mirror
            cx, cy = inst.cell.center
            # if mirror, then the center y should be negated
            if mirror:
                cy = -cy
            # rotation is in degree, the cx and cy should shift accordingly
            cx, cy = cx * math.cos(math.radians(rotation)) - cy * math.sin(
                math.radians(rotation)
            ), cx * math.sin(math.radians(rotation)) + cy * math.cos(
                math.radians(rotation)
            )
            inst.move((-cx, -cy))
        self.netlist = self.data.get_netlist()

    def create_airwires(self):
        for k, v in self.raw_yaml["connections"].items():
            cell_name_0, port_name_0 = k.split(",")
            cell_name_1, port_name_1 = v.split(",")
            port_0 = self.data.insts[cell_name_0].ports[port_name_0]
            port_1 = self.data.insts[cell_name_1].ports[port_name_1]
            port_position_0 = (port_0.x, port_0.y)
            port_position_1 = (port_1.x, port_1.y)
            dx = port_0.x - port_1.x
            dy = port_0.y - port_1.y
            if math.hypot(dx, dy) > 1e-6:
                print(
                    f"{k} {port_position_0} and {v} {port_position_1} is not connected"
                )
                # P = gf.Path()
                # P.points = np.array([port_position_0, port_position_1])
                P = gf.Path([port_position_0, port_position_1])
                airwire = gf.path.extrude(P, width=0.1, layer="DRC_MARKER")
                self.data.add_ref(airwire)

    def plot(self, filepath=None):
        self.data.draw_ports()
        self.data.plot()
        if filepath:
            plt.savefig(filepath)
        else:
            plt.show()

    def stream_out_gds(self, filepath):
        self.data.write_gds(filepath)


class DefYmlOut:
    def __init__(self, netlist=None, name="design", filepath="output"):
        self.name = name
        self.filepath = filepath
        self.netlist = netlist

    def load(self, netlist):
        self.netlist = netlist

    def get_yml(self):
        yml_str = ""
        yml_str += self.print_name()
        yml_str += self.print_instances()
        yml_str += self.print_placements()
        yml_str += self.print_connections()
        return yml_str

    def write_yml(self):
        yml_str = self.get_yml()
        with open(self.filepath, "w") as f:
            f.write(yml_str)

    def print_name(self):
        name_str = ""
        name_str += f"name: {self.name}\n"
        return name_str

    def print_instances(self):
        instances_str = ""
        instances_str += "instances:\n"
        for instance in self.netlist["instances"]:
            print(self.netlist["instances"][instance])
            instance_component = self.netlist["instances"][instance]["component"]
            instance_settings = self.netlist["instances"][instance]["settings"]
            instances_str += f"  {instance}:\n"
            instances_str += f"    component: {instance_component}\n"
            # instances_str+=f"    settings:\n"
            # for setting in instance_settings:
            #     if setting!="layer" and instance_settings[setting]!=None:
            #         instances_str+=f"      {setting}: {instance_settings[setting]}\n"
            instances_str += (
                "    settings:\n"
                if any(
                    setting != "layer" and instance_settings[setting] is not None
                    for setting in instance_settings
                )
                else ""
            )
            for setting in instance_settings:
                if setting != "layer" and instance_settings[setting] is not None:
                    instances_str += f"      {setting}: {instance_settings[setting]}\n"
        return instances_str

    def print_placements(self):
        placements_str = ""
        placements_str += "placements:\n"
        for instance in self.netlist["placements"]:
            print(self.netlist["placements"][instance])
            placement_settings = self.netlist["placements"][instance]
            placements_str += f"  {instance}:\n"
            for setting in placement_settings:
                if setting == "mirror":
                    if placement_settings[setting]:
                        placements_str += f"    {setting}: true\n"
                    else:
                        placements_str += f"    {setting}: false\n"
                else:
                    placements_str += f"    {setting}: {placement_settings[setting]}\n"
        return placements_str

    def print_connections(self):
        connections_str = ""
        connections_str += "connections:\n"
        for net in self.netlist["nets"]:
            connections_str += f"  {net['p1']}: {net['p2']}\n"
        return connections_str
