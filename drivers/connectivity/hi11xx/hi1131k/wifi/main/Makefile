#
# Main Makefile for building the corresponding module functioned target
#
#
#
# set the coorespondig path to the project root directory
# DO NOT change the variable value, or else if YOU know what you are changing
export ROOT_DEPTH_MAIN=../../..
export KERNEL_PATH_MAIN=$(PWD)/$(ROOT_DEPTH_MAIN)/$(KERNEL_PATH)

# set the default make target
# NOTICE: make sure that the target name here is same with the module
# build target defined at the bottom
.PHONY:module_build

# include the config file to setup the compiling environment
include $(PWD)/$(ROOT_DEPTH_MAIN)/build/1151_host_wifi/env.config

# set the module objects files

#---------- main -------------------
obj-m := main_host.o
main-objs := main_host.o


# set the feature options
include $(PWD)/$(ROOT_DEPTH_MAIN)/build/1151_host_wifi/caps.config

# for example:
# ifeq ($(SUPPORT_VIDEO_OPTIMIZE),1)
#	alg-objs += alg_video.o
# endif

# set the module name
MODULE_NAME=$(subst .o,.ko,$(obj-m))

BACKUP_OBJFILE_FOLDER=$(strip $(subst .o,,$(obj-m)))_objfile

BACKUP_OBJFILE_DIR=$(PWD)/$(BACKUP_OBJFILE_FOLDER)

OBJDUMP_FILE=$(subst .o,.objdump,$(obj-m))
OBJDUMP_TXT_FILE=$(subst .o,.txt,$(obj-m))

# module's elf header file name 
MODULE_ELF_HEADER_FILE=$(subst .o,.elf_header,$(obj-m))

# build the module
all:
	@echo Building module $(MODULE_NAME)
	cp -f $(TOPDIR)/Prj/HSIMU/hsimu_reg.* ./
	make -j $(NJOBS) -C $(KERNEL_PATH_MAIN) M=$(PWD) modules
	$(shell [ -d $(IMAGE_PATH_KO) ] || mkdir -p $(IMAGE_PATH_KO))
	cp ./*.ko $(IMAGE_PATH_KO) -fv
	$(STRIP) $(IMAGE_PATH_KO)/*.ko --strip-unneeded
	make backup_objfile
	make create_objdump
	make create_objdump_txt
	make create_original_ko
	make create_readelf_header
	
# clean the module
clean:
	@echo cleaning module $(MODULE_NAME)
	-find . -type f -name "*.o" -exec rm -f {} \;
	-find . -type f -name "*.o.uu" -exec rm -f {} \;
	-find . -type f -name "*.cmd" -exec rm -f {} \;
	-find . -type f -name "*.ko" -exec rm -f {} \;
	-find . -type f -name "*.out" -exec rm -f {} \;
	-find . -type f -name "*.order" -exec rm -f {} \;
	-find . -type f -name "*.mod.c" -exec rm -f {} \;
	-find . -type f -name "*.symvers" -exec rm -f {} \;
	rm .tmp_versions -frv
	rm -rf $(IMAGE_PATH_KO)/*.ko

install:
	@echo install the module to the root-file system

backup_objfile:
	@echo backup the temp files
	mkdir -p $(BACKUP_OBJFILE_FOLDER)
	-find . -type f -name "*.o" -exec mv -f {} $(BACKUP_OBJFILE_FOLDER) \;
	-find . -type f -name "*.o.uu" -exec mv -f {} $(BACKUP_OBJFILE_FOLDER) \;
	-find . -type f -name "*.cmd" -exec mv -f {} $(BACKUP_OBJFILE_FOLDER) \;
	-find . -type f -name "*.ko" -exec mv -f {} $(BACKUP_OBJFILE_FOLDER) \;
	-find . -type f -name "*.out" -exec mv -f {} $(BACKUP_OBJFILE_FOLDER) \;
	-find . -type f -name "*.order" -exec mv -f {} $(BACKUP_OBJFILE_FOLDER) \;
	-find . -type f -name "*.mod.c" -exec mv -f {} $(BACKUP_OBJFILE_FOLDER) \;
	-find . -type f -name "*.symvers" -exec mv -f {} $(BACKUP_OBJFILE_FOLDER) \;
	-find . -type f -name "*.c.swp" -exec mv -f {} $(BACKUP_OBJFILE_FOLDER) \;
	cp -fr .tmp_versions $(BACKUP_OBJFILE_FOLDER)
	rm .tmp_versions -frv
	mv $(DRV_DMAC_TEST_DIR)/*.o $(BACKUP_OBJFILE_FOLDER)
	mv $(DRV_HMAC_TEST_DIR)/*.o $(BACKUP_OBJFILE_FOLDER)
	mv $(DRV_HAL_TEST_DIR)/*.o $(BACKUP_OBJFILE_FOLDER)
	mv $(ALG_CHIP_TEST_PATH)/*.o $(BACKUP_OBJFILE_FOLDER)


create_objdump:
ifeq ($(OBJDUMP_ENABLE),y)
	@echo start to create_objdump file
	$(shell [ -d $(OBJDUMP_PATH) ] ||(mkdir -p $(OBJDUMP_PATH)))
	$(foreach OBJFILE, $($(PRODUCT_ID)-dump-objs), $(OBJDUMP) -S $(BACKUP_OBJFILE_DIR)/$(OBJFILE) >> $(BACKUP_OBJFILE_DIR)/$(OBJDUMP_FILE);)
	@mv -f $(BACKUP_OBJFILE_DIR)/*.objdump $(OBJDUMP_PATH)	
	
endif

create_objdump_txt:
	$(shell [ -d $(OBJDUMP_TXT_PATH) ] ||(mkdir -p $(OBJDUMP_TXT_PATH)))
	$(foreach OBJFILE, $(obj-m),$(OBJDUMP) -t $(BACKUP_OBJFILE_DIR)/$(obj-m) >> $(BACKUP_OBJFILE_DIR)/$(OBJDUMP_TXT_FILE);)
	@mv -f $(BACKUP_OBJFILE_DIR)/$(OBJDUMP_TXT_FILE) $(OBJDUMP_TXT_PATH)
create_original_ko:
	$(shell [ -d $(ORIGINAL_KO_PATH) ] ||(mkdir -p $(ORIGINAL_KO_PATH)))
	@mv -f $(BACKUP_OBJFILE_DIR)/*.ko $(ORIGINAL_KO_PATH)
	
create_readelf_header:
	$(shell [ -d $(OBJDUMP_TXT_PATH) ] ||(mkdir -p $(OBJDUMP_TXT_PATH)))
	@echo $(MODULE_NAME) > $(OBJDUMP_TXT_PATH)/$(MODULE_ELF_HEADER_FILE)
	@$(READELF) -S $(ORIGINAL_KO_PATH)/$(MODULE_NAME) >> $(OBJDUMP_TXT_PATH)/$(MODULE_ELF_HEADER_FILE)
	