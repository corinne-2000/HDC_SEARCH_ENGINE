# Set workspace
setws .

# Create and build platform
platform create -name BlockDesign_wrapper -hw BlockDesign_wrapper.xsa -os standalone -proc psu_cortexa53_0 -no-boot-bsp
platform generate

# Createhelp  project
app create -name TestBench -platform BlockDesign_wrapper -proc psu_cortexa53_0 -template "Empty Application" -os standalone -lang c

# Change UART for Alinx
bsp config stdin psu_uart_1
bsp config stdout psu_uart_1

# Build project
app build -name TestBench