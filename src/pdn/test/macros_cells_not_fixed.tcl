# test for define_pdn_grid -cells
source "helpers.tcl"

read_lef Nangate45/Nangate45.lef
read_lef nangate_macros/fakeram45_64x32.lef

read_def nangate_macros/floorplan.def

set mem_inst [[ord::get_db_block] findInst dcache.data.data_arrays_0.data_arrays_0_ext.mem]
$mem_inst setPlacementStatus PLACED

add_global_connection -net VDD -pin_pattern {^VDD$} -power
add_global_connection -net VDD -pin_pattern {^VDDPE$}
add_global_connection -net VDD -pin_pattern {^VDDCE$}
add_global_connection -net VSS -pin_pattern {^VSS$} -ground
add_global_connection -net VSS -pin_pattern {^VSSE$}

set_voltage_domain -power VDD -ground VSS

define_pdn_grid -name "Core"
add_pdn_stripe -followpins -layer metal1

add_pdn_stripe -layer metal4 -width 0.48 -spacing 4.0 -pitch 49.0 -offset 2.5
add_pdn_stripe -layer metal7 -width 1.4 -pitch 40.0 -offset 2.5

add_pdn_connect -layers {metal1 metal4}
add_pdn_connect -layers {metal4 metal7}

define_pdn_grid -macro -name "sram" -cells "fakeram45_64x32"
add_pdn_stripe -grid "sram" -layer metal5 -width 0.93 -pitch 10.0 -offset 2.0
add_pdn_stripe -grid "sram" -layer metal6 -width 0.93 -pitch 10.0 -offset 2.0

add_pdn_connect -grid "sram" -layers {metal4 metal5}
add_pdn_connect -grid "sram" -layers {metal5 metal6}
add_pdn_connect -grid "sram" -layers {metal6 metal7}

catch { pdngen } err
puts $err
