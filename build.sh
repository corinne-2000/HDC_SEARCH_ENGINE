# Launch Vivado HLS
echo "Build HLS module"
pushd HLS/SearchEngine
source /tools/Xilinx/Vivado/2019.2/settings64.sh
/tools/Xilinx/Vivado/2019.2/bin/vivado_hls build.tcl
popd

# echo "Building block design"
pushd Project
/tools/Xilinx/Vivado/2019.2/bin/vivado -mode batch -source build.tcl -notrace
popd

echo "Building SDK project"
pushd SDK
/tools/Xilinx/Vitis/2019.2/bin/xsct -eval "source build.tcl"
popd