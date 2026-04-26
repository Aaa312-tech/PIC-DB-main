from yml import LefYmlIn, DefYmlIn, DefYmlOut

if __name__ == "__main__":
    # lef_path ="../build/lef.yml"
    lef_path = "../../benchmarks/pic_placement_lef.yml"
    placed_def = "../../build/clements_placed.yml"
    routed_def = "../../build/clements_routed.yml"

    lef_i = LefYmlIn(lef_path)
    lef_i.register_components()

    lef_i.print_components()
    lef_i.plot_component("crossing")

    placed_def_i = DefYmlIn(placed_def, is_routed=False)
    placed_def_i.patch_component_centering()
    placed_def_i.create_airwires()
    placed_def_i.plot()
    placed_def_i.stream_out_gds("../../build/clements_placed.gds")

    routed_def_i = DefYmlIn(routed_def, is_routed=False)
    routed_def_i.patch_component_centering()
    routed_def_i.create_airwires()
    routed_def_i.plot()
    routed_def_i.stream_out_gds("../../build/clements_routed.gds")
