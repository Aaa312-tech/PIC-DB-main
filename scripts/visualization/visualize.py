from yml import LefYmlIn, DefYmlIn

if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser(description="Visualize PIC")
    parser.add_argument("--lef_path", type=str)
    parser.add_argument("--def_path", type=str)
    parser.add_argument("--file_path", type=str)
    parser.add_argument("--output_gds", type=bool, default=False)

    args = parser.parse_args()

    lef_path = args.lef_path
    def_path = args.def_path
    file_path = args.file_path
    output_gds = args.output_gds

    lef_i = LefYmlIn(lef_path)
    lef_i.register_components()

    def_i = DefYmlIn(def_path, is_routed=False)
    def_i.patch_component_centering()
    def_i.create_airwires()
    def_i.plot(file_path + ".png")
    if output_gds:
        def_i.stream_out_gds(file_path + ".gds")
