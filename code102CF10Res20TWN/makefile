#**************************************************************************
# This is the makeflie to create the hls project 
#
# Author: Qiu Chao 
# Date  : 2017/03/26
#
#**************************************************************************
SHELL=/bin/bash
export PRJ_ROOT

#SCRIPT_NAME = kcfScriptTclTemplate
prj_cfg_org :=$(shell cat prj_cfg.dat)
prj_root_filter =$(filter prj_root_name=%,$(prj_cfg_org))
script_filter=$(filter script_name=%,$(prj_cfg_org))
folder_filter =$(filter folder_name=%,$(prj_cfg_org))
code_base_path_filter =$(filter code_base_path_name=%,$(prj_cfg_org))
code_filter =$(filter code_name=%,$(prj_cfg_org))
version_filter =$(filter version_name=%,$(prj_cfg_org))
PRJ_ROOT_NAME:=$(patsubst prj_root_name=%,%,$(prj_root_filter))
SCRIPT_NAME :=$(patsubst script_name=%,%,$(script_filter))
FOLDER_NAME :=$(patsubst folder_name=%,%,$(folder_filter))
CODE_BASE_PATH_NAME :=$(patsubst code_base_path_name=%,%,$(code_base_path_filter))
CODE_NAME :=$(patsubst code_name=%,%,$(code_filter))
VERSION_NAME :=$(patsubst version_name=%,%,$(version_filter))
FOLDER_NAME := $(FOLDER_NAME)$(VERSION_NAME)  
CODE_NAME  := $(CODE_NAME)$(VERSION_NAME)  
PRJ_ROOT = $(shell cd ../$(PRJ_ROOT_NAME);pwd)
##HLS_PATH =/opt/tool/xilinx/vivado2016_04/Vivado_HLS/2016.4/bin
HLS_PATH =/opt/tool/xilinx/vivado2016_02/Vivado_HLS/2016.2/bin
default:
	@echo -e "\033[40;32m ******************************************\033[0m"
	@echo -e "\033[40;32m EXAMPLES:                                 \033[0m"
	@echo -e "\033[40;32m make build                                \033[0m"
	@echo -e "\033[40;32m make moveto_tc0_golden tc=tc1             \033[0m"
	@echo -e "\033[40;32m make clean                                \033[0m"
	@echo -e "\033[40;32m ******************************************\033[0m"
	@echo -e "\033[40;32m                        Qiu Chao           \033[0m"
	@echo -e "\033[40;32m ******************************************\033[0m"

build:
	@if [[ ! -d $(PRJ_ROOT)/$(FOLDER_NAME) ]]; then \
	   echo -e "\033[48;34m Building the project folder...\033[0m" ; \
	   mkdir $(PRJ_ROOT)/$(FOLDER_NAME) ; \
	else \
	   echo -e "\033[48;34m Rebuilding the project folder...\033[0m" ; \
	fi ; \
    chmod 777 genScript.pl ; \
    ./genScript.pl  $(CODE_BASE_PATH_NAME) $(CODE_NAME) $(SCRIPT_NAME) ; \
    cp -rf Script.tcl $(PRJ_ROOT)/$(FOLDER_NAME) ; \
    cd $(PRJ_ROOT)/$(FOLDER_NAME) ; \
    $(HLS_PATH)/vivado_hls -f Script.tcl $(FOLDER_NAME) ; \

clean:
	rm -rf vivado.log ; \
    rm -rf $(PRJ_ROOT)/$(FOLDER_NAME)
