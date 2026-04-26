# pic_pr: Photonic Integrated Circuit Placement & Routing Tool

**pic_pr** is a C++ based tool designed for the Placement and Routing (P&R) of Photonic Integrated Circuits (PICs).

# 📁 File Structure

```
.
├── benchmarks/                 # Benchmark designs and evaluation cases
│   ├── clements/               # Clements MZI mesh architecture benchmarks
│   ├── light_router_mrr/       # Microring-resonator-based optical router benchmarks
│   ├── mrr_weight_bank/        # MRR weight bank (e.g., for photonic neural networks)
│   ├── opa1/                   # Optical Phased Array benchmarks (Type 1)
│   ├── opa2/                   # Optical Phased Array benchmarks (Type 2)
│   ├── optical_switch/         # Optical switch benchmark designs
│   ├── self_calibrating_PIC/   # Self-calibrating photonic integrated circuits
│   ├── structured_light/       # Structured-light photonic system benchmarks
│   └── TeMPO/                  # TeMPO architecture benchmark designs
├── configs/                    # Configuration files for algorithms and tools
│   ├── pr/                     # Placement & Routing configuration files
│   └── pr_dreamplace/          # DreamPlace-based P&R configuration files
├── scripts/                    # Auxiliary scripts and automation tools
│   └── visualization/          # Scripts for layout and routing visualization
├── src/                        # Core source code
│   ├── algorithm/              # Core algorithms (placement, routing, optimization)
│   ├── db/                     # Database and data models for layouts, netlists, components
│   ├── io/                     # Input/output handling (parsing configs, benchmarks, results)
│   └── utility/                # Shared utilities (logging, math helpers, common functions)
├── tests/                      # Test suites
│   └── unit/                   # C++ unit tests for core modules
├── third-party/                # External dependencies and third-party libraries
│   └── DREAMPlace/             # DREAMPlace (external placer) as a Git submodule
└── tools/                      # Executable tools and pipelines
    ├── pr/                     # Placement & Routing toolchain
    └── pr_dreamplace/          # DreamPlace-based P&R tools

```
# 🛠️ Build & Installation
## How to Build
```
# Clone the repository
git clone https://github.com/O-EDA/PIC-DB.git
cd PIC-DB
```

### Option 1: Using Docker
Install Docker and Navigate to the repository
#### 1. Build the Image

Build with Linux.

```
export DOCKER_BUILDKIT=0
docker build . --file Dockerfile --tag pic_pr
```

Build with Windows.

```
$env:DOCKER_BUILDKIT=0
docker build . --file Dockerfile --tag pic_pr
```
#### 2. Run the Docker

Run with GPU on Linux.
```
docker run --gpus 1 -it -v $(pwd):/PIC-DB pic_pr bash
```

Run with GPU on Windows.
```
docker run --gpus 1 -it -v %cd%:/PIC-DB pic_pr bash
```

Run without GPU on Linux.
```
docker run -it -v $(pwd):/PIC-DB pic_pr bash
```

Run without GPU on Windows.
```
docker run -it -v %cd%:/PIC-DB pic_pr bash
```

### Option 2: Native Build
Prerequisites:
- Compiler: GCC 9.0+ or Clang (C++17 support required)
- Build System: CMake >= 3.10
- Libraries: yaml-cpp, spdlog, fmt, boost
#### Build Steps:
```
# 1. Install dependencies (Ubuntu example)
sudo apt-get update && sudo apt-get install -y build-essential cmake libyaml-cpp-dev libspdlog-dev libfmt-dev libboost-all-dev
pip311 install \
        gdsfactory>=9.23.1 \
        matplotlib>=3.9.2 \
        ipython>=8.37.0 \
        multimethod==1.12 \
        ryaml==0.4.0 \
        numba==0.60.0 \

# 2. Build the project
cd PIC-DB
mkdir build && cd build
cmake ..
make
cd ../build/third-party/DREAMPlace
mkdir build && cd build
cmake .. -DCMAKE_CXX_STANDARD=11 -DCMAKE_C_FLAGS="-fpermissive" -DCMAKE_CXX_FLAGS="-fpermissive"
make
make install
```

# 🚀 How to Run
## Basic Workflow

```
./pr <fc> <lef_in_path> <def_in_path> <def_out_path> <place_config> <route_config>
```
| Index | Argument | Type | Description |
| :---: | :--- | :--- | :--- |
| 1 | `fc` | `int` | **Flow Control**<br>Integer identifier controlling the operational mode or specific execution flow.<br>(e.g., `0` for P&R, `1` Placement Only, `2` Routing Only). |
|   2   | `lef_in_path`  | `string` | **LEF Input Path** Path to the source Library Exchange Format (.lef) file. |
|   3   | `def_in_path`  | `string` | **DEF Input Path**<br>Path to the source Design Exchange Format (.def) file. |
|   4   | `def_out_path` | `string` | **DEF Output Path**<br>Destination path for the DEF file.    |
| 5 | `place_config` | `string` | **Place Config Path**<br/>Destination path for the Place Config file. |
| 6 | `route_config` | `string` | **Route Config Path**<br/>Destination path for the Route Config file. |

**Usage Example**

Run the following command to execute the workflow (using Flow Control `0`):
```
./pr 0  ../benchmarks/pic_placement_lef.yml ../benchmarks/clements/clements_8x8.yml ./clements ../configs/pr/place_config/clements_8x8.yml ../configs/pr/route_config/clements_8x8.yml
```
Output: After execution, the following result files will be generated in the specified directory:
- `clements_placed.yml`: Contains the final component placement coordinates.
- `clements_routed.yml`: Contains the generated waveguide routing paths.

## DREAMPlace Workflow

```
./pr_dreamplace <fc> <lef_in_path> <def_in_path> <def_out_path> <place_config> <route_config>
```

**Usage Example**

Run the following command to execute the workflow (using Flow Control `0`):

```
./pr_dreamplace 0 ../benchmarks/pic_placement_lef.yml ../benchmarks/clements/clements_8x8.yml ./clements ../configs/pr_dreamplace/place_config/clements_8x8 ../configs/pr_dreamplace/route_config/clements_8x8.yml
```

Output: After execution, the following result files will be generated in the specified directory:

- `clements_placed.yml`: Contains the final component placement coordinates.
- `clements_routed.yml`: Contains the generated waveguide routing paths.

## LiDAR Workflow

```
./pr_lidar <fc> <lef_in_path> <def_in_path> <def_out_path> <place_config> <route_config>
```

**Usage Example**

Run the following command to execute the workflow (using Flow Control `2`):

```
./pr_lidar 0 ../benchmarks/pic_placement_lef.yml ../benchmarks/clements/clements_8x8.yml ./clements ../configs/pr_lidar/place_config/clements_8x8 ../configs/pr_lidar/route_config/comp_LiDAR.yml
```

Output: After execution, the following result files will be generated in the specified directory:

- `clements_placed.yml`: Contains the final component placement coordinates.
- `clements_routed.gds`: Contains the generated waveguide routing paths in GDS format.


# 📊 Visualization
We provide a Python-based visualization tool to inspect the generated placement and routing results. This script parses the output YAML files and renders the layout of the Photonic Integrated Circuit.

## Usage
Use the main.py script located in scripts/visualization/ to visualize the input architecture or the processing results (placed/routed files).

```
python311 scripts/visualization/main.py <lef_yaml_file_path> <def_yaml_file_path>
```

## Example
To visualize the routing results generated from the Basic Workflow step above:
```
python311 scripts/visualization/main.py ../routed.yml
```