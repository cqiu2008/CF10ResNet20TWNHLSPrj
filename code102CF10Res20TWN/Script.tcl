#!/usr/bin/tclsh
############################################################
## This file is generated automatically by Vivado HLS.
## Please DO NOT edit it.
## Copyright (C) 1986-2016 Xilinx, Inc. All Rights Reserved.
############################################################
set iIndex 0
#while {$iIndex < $argc}{
# lindex used to store the list parameter number
    set arg [lindex $argv $iIndex]
    puts $arg
#incr used to add 1
    incr $iIndex 1
#}

#set prjName [lindex $argv 0] 
#set codeName [lindex $argv 1]
set codeName code102CF10Res20TWNGit
set codeBasePath code102CF10Res20TWN
set prjName  ip$codeName
set topFunction Accelerator 
open_project $prjName
set_top $topFunction
set filePath ../../$codeBasePath/$codeName
add_files $filePath/src/fpga_top.cpp
add_files $filePath/src/fpga_top.hpp
add_files -tb $filePath/tb/cpu_top.cpp
add_files -tb $filePath/tb/cpu_top.hpp
add_files -tb $filePath/tb/network.cpp
add_files -tb $filePath/tb/network.hpp
add_files -tb $filePath/tb/layer.cpp
add_files -tb $filePath/tb/layer.hpp
add_files -tb $filePath/tb/logger.cpp
add_files -tb $filePath/tb/logger.hpp
add_files -tb $filePath/tb/myostream.cpp
add_files -tb $filePath/tb/myostream.hpp
add_files -tb $filePath/tb/cf10resnet20.cpp
add_files -tb $filePath/tb/cf10resnet20.hpp
add_files -tb $filePath/tb/cnv_layer.cpp
add_files -tb $filePath/tb/cnv_layer.hpp
add_files -tb $filePath/tb/driver.cpp
add_files -tb $filePath/tb/driver.hpp
add_files -tb $filePath/tb/common.hpp
open_solution "solution1"
#set_part {xc7z045ffg900-2} -tool vivado
set_part {xczu9eg-ffvb1156-2l-e-EVAL}
###set_part {xczu9eg-ffvb1156-1-i-es1} -tool vivado
create_clock -period 10 -name default
#config_dataflow -default_channel fifo -fifo_depth 1
#source "./prjKCF0328a/solution1/directives.tcl"
csim_design
csynth_design
export_design -format ip_catalog
#cosim_design
exit

