# Testing buffer inferece based on port
# LEF USE CLOCK

source "helpers.tcl"

read_liberty Nangate45/Nangate45_typ.lib
read_lef ModNangate45/ModNangate45.lef
read_def check_buffers.def

create_clock -period 5 clk
set_wire_rc -clock -layer metal5

set_debug CTS "buffering" 1

clock_tree_synthesis -root_buf CLKBUF_X3 \
  -wire_unit 20 \
  -sink_clustering_enable \
  -distance_between_buffers 100 \
  -sink_clustering_size 10 \
  -sink_clustering_max_diameter 60 \
  -num_static_layers 1

set unconnected_buffers 0
foreach buf [get_cells clkbuf_*_clk] {
  set buf_name [get_name $buf]
  set input_pin [get_pin $buf_name/A]
  set input_net [get_net -of $input_pin]
  if { $input_net == "NULL" } {
    incr unconnected_buffers
  }
}

puts "Found $unconnected_buffers unconnected buffers."
