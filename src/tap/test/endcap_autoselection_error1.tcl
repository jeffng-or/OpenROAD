source "helpers.tcl"
read_lef Nangate45/Nangate45_tech.lef
read_lef Nangate45/Nangate45_stdcell.lef
read_lef Nangate45_data/endcaps.lef
read_lef Nangate45_data/endcaps_symmetric.lef
read_lef Nangate45/fakeram45_64x7.lef
read_def Nangate45_data/macros.def

set def_file [make_result_file boundary_macros_tapcell.def]

catch {
  tapcell -distance "20" \
    -tapcell_master "TAPCELL_X1" \
    -tap_nwin2_master "ENDCAP_X1_TOPEDGE" \
    -tap_nwin3_master "ENDCAP_X1_TOPEDGE" \
    -tap_nwout2_master "ENDCAP_X1_BOTTOMEDGE" \
    -tap_nwout3_master "ENDCAP_X1_BOTTOMEDGE" \
    -tap_nwintie_master "ENDCAP_X1_TOPEDGE" \
    -tap_nwouttie_master "ENDCAP_X1_BOTTOMEDGE" \
    -cnrcap_nwin_master "ENDCAP_X4_LEFTBOTTOMCORNER_Y" \
    -cnrcap_nwout_master "ENDCAP_X4_LEFTTOPCORNER_Y" \
    -incnrcap_nwin_master "ENDCAP_X4_LEFTTOPEDGE_Y" \
    -incnrcap_nwout_master "ENDCAP_X4_LEFTBOTTOMEDGE_Y"
} error

puts $error
