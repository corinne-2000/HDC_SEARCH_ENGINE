# Create project and open it
open_project -reset hls_project

# Set top-level function
set_top SearchEngine

# Add source files
add_files main.cpp

# Add test benches
add_files -tb main_tb.cpp

# Create solution and open it
open_solution "alinx"

# Set part number to be used
set_part {xczu2cg-sfvc784-1-e} -tool vivado

# Create clock
create_clock -period 3.333 -name default

# Config export options
config_export -format ip_catalog -rtl vhdl -display_name "SEARCH_ENGINE" -ipname "SearchEngine" -version "1.3" -vendor "CorinneBP" -description "HDC search engine"

# Build the design
csim_design
csynth_design
cosim_design

# Export the module
export_design -rtl vhdl -format ip_catalog

# Close project
close_project