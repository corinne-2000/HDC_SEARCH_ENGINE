proc fGetProcessorCount {} {
  # Windows puts it in an environment variable
  global tcl_platform env
  if {$tcl_platform(platform) eq "windows"} {
    return $env(NUMBER_OF_PROCESSORS)
  }

  # Check for sysctl (OSX, BSD)
  set lSysCtl [auto_execok "sysctl"]
  if {[llength $lSysCtl]} {
    if {![catch {exec {*}$sysctl -n "hw.ncpu"} lCores]} {
      return $lCores
    }
  }

  # Assume Linux, which has /proc/cpuinfo, but be careful
  if {![catch {open "/proc/cpuinfo"} f]} {
    set lCores [regexp -all -line {^processor\s} [read $f]]
    close $f
    if {$lCores > 0} {
      return $lCores
    }
  }

  # On fail, return 1 core (always available, beacause we run on it)
  return 1
}

# Get operating system
set lPlatform "$tcl_platform(os)"

# Get number of available processors
set kCPU_COUNT      [fGetProcessorCount]
if {$kCPU_COUNT > 1} {
  set kCPU_COUNT      [expr {$kCPU_COUNT - 1}]
}

set kSYNTH_NAME       synth_1
set kIMPL_NAME        impl_1
set kPROJECT_NAME     "SearchEngine"
set kBD_NAME          "BlockDesign"

puts "INFO: Using $kCPU_COUNT thread(s)"

puts " "
puts "----------------------------------------------------------------------------------------------------------------------------"
puts "Recreate Vivado project"
puts "----------------------------------------------------------------------------------------------------------------------------"
puts " "

# Recreate Vivado project
source recreate.tcl

# Get project directory
set gProjectPath [get_property directory [current_project]]

puts " "
puts "----------------------------------------------------------------------------------------------------------------------------"
puts " Reset block design IPs"
puts "----------------------------------------------------------------------------------------------------------------------------"
reset_target all [get_files ../IP/$kBD_NAME/$kBD_NAME.bd]
config_ip_cache -clear_output_repo

puts " "
puts "----------------------------------------------------------------------------------------------------------------------------"
puts " Regenerate block design IPs"
puts "----------------------------------------------------------------------------------------------------------------------------"
generate_target all [get_files ../IP/$kBD_NAME/$kBD_NAME.bd]

puts " "
puts "----------------------------------------------------------------------------------------------------------------------------"
puts " Launching synthesis"
puts "----------------------------------------------------------------------------------------------------------------------------"

reset_runs $kSYNTH_NAME
launch_runs $kSYNTH_NAME -jobs $kCPU_COUNT
wait_on_run $kSYNTH_NAME

set lStatus [get_property STATUS [get_runs $kSYNTH_NAME]]
set lSynthStatus [get_property STATUS [get_runs $kSYNTH_NAME]]
if {[string range "$lStatus" 0 20] != "synth_design Complete"} {
  puts "ERROR: Synthesis failed (status: $lStatus)"
  exit 1
} else {
  puts "INFO: Synthesis completed (status: $lStatus)"
}

puts " "
puts "----------------------------------------------------------------------------------------------------------------------------"
puts " Launching implementation"
puts "----------------------------------------------------------------------------------------------------------------------------"

reset_runs $kIMPL_NAME
launch_runs $kIMPL_NAME -jobs $kCPU_COUNT
wait_on_run $kIMPL_NAME

set lStatus [get_property STATUS [get_runs $kIMPL_NAME]]
set lImplStatus [get_property STATUS [get_runs $kIMPL_NAME]]
if {[string range "$lStatus" 0 20] != "route_design Complete"} {
  puts "ERROR: Implementation failed (status: $lStatus)"
  exit 1
} else {
  puts "INFO: Implementation completed (status: $lStatus)"
}

puts " "
puts "----------------------------------------------------------------------------------------------------------------------------"
puts " Launching write bitstream"
puts "----------------------------------------------------------------------------------------------------------------------------"

launch_runs $kIMPL_NAME -to_step write_bitstream -jobs $kCPU_COUNT
wait_on_run $kIMPL_NAME

set lStatus [get_property STATUS [get_runs $kIMPL_NAME]]
if {$lStatus != "write_bitstream Complete!"} {
  puts "ERROR: Write Bitstream Failed (status: $lStatus)"
  exit 1
} else {
  puts "INFO: Bitstream created"
}

puts " "
puts "----------------------------------------------------------------------------------------------------------------------------"
puts " Export hardware"
puts "----------------------------------------------------------------------------------------------------------------------------"

write_hw_platform -fixed -force  -include_bit -file ../SDK/BlockDesign_wrapper.xsa

puts " "
puts "----------------------------------------------------------------------------------------------------------------------------"
puts " "
puts "INFO: Project built succesfully"
puts " "

exit 0
