import os
import glob
import re
from yml import LefYmlIn, DefYmlIn, DefYmlOut

if __name__ == "__main__":
    lef_path = "../../benchmarks/pic_placement_lef.yml"
    build_dir = "../../build"

    lef_i = LefYmlIn(lef_path)
    lef_i.register_components()

    def_files = glob.glob(os.path.join(build_dir, "placement_iter_*.yml"))

    def get_iteration_number(filename):
        match = re.search(r"placement_iter_(\d+)\.yml", filename)
        if match:
            return int(match.group(1))
        return 0

    def_files_sorted = sorted(def_files, key=get_iteration_number)

    for def_file in def_files_sorted:
        print(f"Processing {def_file}...")

        placed_def_i = DefYmlIn(def_file, is_routed=False)
        placed_def_i.patch_component_centering()
        placed_def_i.create_airwires()
        placed_def_i.plot()
        # gds_file = def_file.replace(".yml", ".gds")
        # placed_def_i.stream_out_gds(gds_file)
        # print(f"Generated GDS: {gds_file}")
