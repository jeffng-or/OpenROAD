# repair_timing -recover_power 100
source "helpers.tcl"
define_corners fast slow
read_liberty -corner slow Nangate45/Nangate45_slow.lib
read_liberty -corner fast Nangate45/Nangate45_fast.lib
read_lef Nangate45/Nangate45.lef
read_def recover_power1.def
create_clock -period 2.0 clk

source Nangate45/Nangate45.rc
set_wire_rc -layer metal3
estimate_parasitics -placement

report_worst_slack -max -digits 3
write_verilog_for_eqy repair_setup4_verbose before "None"
repair_timing -verbose -recover_power 100
run_equivalence_test repair_setup4_verbose ./Nangate45/work_around_yosys/ "None"
report_worst_slack -max -digits 3
