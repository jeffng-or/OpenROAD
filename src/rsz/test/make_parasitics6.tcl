# 2 corners with set_layer_rc
source "helpers.tcl"
define_corners ss ff
read_liberty -corner ss Nangate45/Nangate45_slow.lib
read_liberty -corner ff Nangate45/Nangate45_fast.lib
read_lef Nangate45/Nangate45.lef
read_def reg6.def

create_clock -period 10 clk
set_input_delay -clock clk 0 in1

# kohm/micron
set r 5.43e-3
# fF/micron
set c 6.013e-2

source Nangate45/Nangate45.rc
set_layer_rc -layer metal1 -corner ff \
  -resistance [expr { $r * 0.8 }] -capacitance [expr { $c * 0.8 }]
set_layer_rc -layer metal1 -corner ss \
  -resistance [expr { $r * 1.2 }] -capacitance [expr { $c * 1.2 }]
set_wire_rc -corner ff -layer metal1
set_wire_rc -corner ss -layer metal1

puts "Corner: ff"
report_layer_rc -corner ff

puts "Corner: ss"
report_layer_rc -corner ss

estimate_parasitics -placement -spef_file results/make_parasitics6.spef

report_net r1/Q -corner ff
report_net r1/Q -corner ss

diff_file make_parasitics6_ff.spefok results/make_parasitics6_ff.spef
diff_file make_parasitics6_ss.spefok results/make_parasitics6_ss.spef
