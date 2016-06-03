# $URL: http://subversion:8080/svn/gsc/trunk/drivers/gsc_common/linux/driver/gsc_makefile.inc $
# $Rev$
# $Date$

# Common Linux driver makefile.

# Most of this is ignored by the 2.6 module builder.
CC			= gcc
CC_FLAGS	+= -c -O6 -pipe -fomit-frame-pointer -Wall
CC_FLAGS	+= -D__KERNEL__ -DLINUX
CC_FLAGS	+= -I. -I${KERNELDIR}
DEP_FILE	= ${GSC_DEV_DIR}/makefile.dep
KERNELDIR	=
KERNELVER	= $(shell uname -r | cut -d . -f1-2 )
RELEASE_RM	= ${OBJ_FILES} .tmp* .*.o.cmd .*.ko.cmd *.mod.* Modules*



# kernel 2.2 ==================================================================
ifeq ("${KERNELVER}","2.2")
KERNELDIR	= /usr/src/linux
MODULE_NAME	= ${TARGET}.o

${MODULE_NAME}: ${GSC_SOURCES} ${OBJ_FILES}
	@echo ==== Linking: $@
	@ld -r -o $@ ${OBJ_FILES}
endif

# kernel 2.4 ==================================================================
ifeq ("${KERNELVER}","2.4")

K_DIR	:= $(shell ls -d /usr/src/linux/include 2>/dev/null | wc -l )
K_DIR	:= $(shell echo $(K_DIR) | sed -e 's/[ \t]*//g')

ifeq ("${K_DIR}","1")
KERNELDIR	= /usr/src/linux/include
else
KERNELDIR	= /usr/src/linux-2.4/include
endif

MODULE_NAME	= ${TARGET}.o

${MODULE_NAME}: ${GSC_SOURCES} ${OBJ_FILES}
	@echo ==== Linking: $@
	@ld -r -o $@ ${OBJ_FILES}
endif

# kernel 2.6 ==================================================================
ifeq ("${KERNELVER}","2.6")
${TARGET}-objs	= ${OBJ_FILES}
KERNELDIR		= /lib/modules/$(shell uname -r)/build
MODULE_NAME		= ${TARGET}.ko
obj-m			= ${TARGET}.o
PWD				= $(shell pwd)
RELEASE_RM		+= ${TARGET}.o Module.symvers

${MODULE_NAME}: ${GSC_SOURCES} *.c *.h
	@-chmod 666 ${DEP_FILE}
	@-echo -n > ${DEP_FILE}
	@make -C ${KERNELDIR} SUBDIRS=${PWD} modules
	@strip -d --strip-unneeded $@
	@rm -f *.mod.c
endif

# kernel OTHER ================================================================
ifeq ("${KERNELDIR}","")
KERNELDIR	= /UNKNOWN_AT_THIS_TIME
MODULE_NAME	= /UNKNOWN_AT_THIS_TIME

${MODULE_NAME}:
	@echo ERROR: KERNEL ${KERNELVER} IS NOT SUPPORTED BY THIS MAKEFILE.
	@_ERROR_ERROR_ERROR_ERROR_ERROR_ERROR_ERROR_ERROR_ERROR
endif



# COMMON ======================================================================

# This is ignored by the 2.6 module builder.
.c.o:
	@echo == Compiling: $<
	@-chmod +rw ${DEP_FILE}
	@# Get the dependency list for this module.
	@-${CC} -MM ${CC_FLAGS} $< >  .tmp1
	@# Remove trailing white space and backslash, if present.
	@-sed -e "s/[ ]*[\\\\]//g" < .tmp1 > .tmp2
	@# Put everything on seperate lines.
	@-tr [:space:] \\n < .tmp2 > .tmp3
	@# Remove all of the system include files.
	@-grep -v "^[ ]*/" < .tmp3 > .tmp4
	@# Remove all empty lines.
	@-grep [[:alnum:]] < .tmp4 > .tmp5
	@# Put everything on the same line.
	@-tr '\n' '\040' < .tmp5 > .tmp6
	@-echo -e '\012' >> .tmp6
	@# Add all the other dependencies to the end of this file.
	@-echo >> ${DEP_FILE}
	@-grep -v "^[ ]*$@" < ${DEP_FILE} >> .tmp6
	@# Remove blank lines from the list.
	@-grep "[[:alnum:]]" < .tmp6 > .tmp7
	@# Sort the list and put it in the dependency file.
	@-sort < .tmp7 > ${DEP_FILE}
	@# Cleanup.
	@rm -f .tmp*
	@# Compile the module.
	@${CC} ${CC_FLAGS} $< -o $@

ifeq ("${GSC_COMMON}","1")

./gsc_makefile.inc: ${GSC_DIR}/gsc_makefile.inc
	@echo == Getting: $@
	@rm -f $@
	@cp $< .
	@chmod 444 $@

./gsc_bar.c: ${GSC_DIR}/gsc_bar.c
	@echo == Getting: $@
	@rm -f $@
	@cp $< .
	@chmod 444 $@

./gsc_close.c: ${GSC_DIR}/gsc_close.c
	@echo == Getting: $@
	@rm -f $@
	@cp $< .
	@chmod 444 $@

./gsc_common.h: ${GSC_DIR}/gsc_common.h
	@echo == Getting: $@
	@rm -f $@
	@cp $< .
	@chmod 444 $@

./gsc_dma.c: ${GSC_DIR}/gsc_dma.c
	@echo == Getting: $@
	@rm -f $@
	@cp $< .
	@chmod 444 $@

./gsc_init.c: ${GSC_DIR}/gsc_init.c
	@echo == Getting: $@
	@rm -f $@
	@cp $< .
	@chmod 444 $@

./gsc_io.c: ${GSC_DIR}/gsc_io.c
	@echo == Getting: $@
	@rm -f $@
	@cp $< .
	@chmod 444 $@

./gsc_ioctl.c: ${GSC_DIR}/gsc_ioctl.c
	@echo == Getting: $@
	@rm -f $@
	@cp $< .
	@chmod 444 $@

./gsc_irq.c: ${GSC_DIR}/gsc_irq.c
	@echo == Getting: $@
	@rm -f $@
	@cp $< .
	@chmod 444 $@

./gsc_kernel_2_2.c: ${GSC_DIR}/gsc_kernel_2_2.c
	@echo == Getting: $@
	@rm -f $@
	@cp $< .
	@chmod 444 $@

./gsc_kernel_2_2.h: ${GSC_DIR}/gsc_kernel_2_2.h
	@echo == Getting: $@
	@rm -f $@
	@cp $< .
	@chmod 444 $@

./gsc_kernel_2_4.c: ${GSC_DIR}/gsc_kernel_2_4.c
	@echo == Getting: $@
	@rm -f $@
	@cp $< .
	@chmod 444 $@

./gsc_kernel_2_4.h: ${GSC_DIR}/gsc_kernel_2_4.h
	@echo == Getting: $@
	@rm -f $@
	@cp $< .
	@chmod 444 $@

./gsc_kernel_2_6.c: ${GSC_DIR}/gsc_kernel_2_6.c
	@echo == Getting: $@
	@rm -f $@
	@cp $< .
	@chmod 444 $@

./gsc_kernel_2_6.h: ${GSC_DIR}/gsc_kernel_2_6.h
	@echo == Getting: $@
	@rm -f $@
	@cp $< .
	@chmod 444 $@

./gsc_main.h: ${GSC_DIR}/gsc_main.h
	@echo == Getting: $@
	@rm -f $@
	@cp $< .
	@chmod 444 $@

./gsc_open.c: ${GSC_DIR}/gsc_open.c
	@echo == Getting: $@
	@rm -f $@
	@cp $< .
	@chmod 444 $@

./gsc_pci9056.h: ${GSC_DIR}/gsc_pci9056.h
	@echo == Getting: $@
	@rm -f $@
	@cp $< .
	@chmod 444 $@

./gsc_pci9060es.h: ${GSC_DIR}/gsc_pci9060es.h
	@echo == Getting: $@
	@rm -f $@
	@cp $< .
	@chmod 444 $@

./gsc_pci9080.h: ${GSC_DIR}/gsc_pci9080.h
	@echo == Getting: $@
	@rm -f $@
	@cp $< .
	@chmod 444 $@

./gsc_pci9656.h: ${GSC_DIR}/gsc_pci9656.h
	@echo == Getting: $@
	@rm -f $@
	@cp $< .
	@chmod 444 $@

./gsc_proc.c: ${GSC_DIR}/gsc_proc.c
	@echo == Getting: $@
	@rm -f $@
	@cp $< .
	@chmod 444 $@

./gsc_read.c: ${GSC_DIR}/gsc_read.c
	@echo == Getting: $@
	@rm -f $@
	@cp $< .
	@chmod 444 $@

./gsc_reg.c: ${GSC_DIR}/gsc_reg.c
	@echo == Getting: $@
	@rm -f $@
	@cp $< .
	@chmod 444 $@

./gsc_sem.c: ${GSC_DIR}/gsc_sem.c
	@echo == Getting: $@
	@rm -f $@
	@cp $< .
	@chmod 444 $@

./gsc_time.c: ${GSC_DIR}/gsc_time.c
	@echo == Getting: $@
	@rm -f $@
	@cp $< .
	@chmod 444 $@

./gsc_vpd.c: ${GSC_DIR}/gsc_vpd.c
	@echo == Getting: $@
	@rm -f $@
	@cp $< .
	@chmod 444 $@

./gsc_wait.c: ${GSC_DIR}/gsc_wait.c
	@echo == Getting: $@
	@rm -f $@
	@cp $< .
	@chmod 444 $@

./gsc_write.c: ${GSC_DIR}/gsc_write.c
	@echo == Getting: $@
	@rm -f $@
	@cp $< .
	@chmod 444 $@

endif



all: ${MODULE_NAME}
	@echo ==== All Done

release: ${MODULE_NAME}
	@rm -rf  ${RELEASE_RM}
	@echo ==== Release Done

clean:
	@echo ==== Cleaning ${MODULE_NAME} ...
	@rm -rf ${RELEASE_RM} *.o *.scc *.ko core Module.markers modules.order ${DEP_FILE}
	@echo > ${DEP_FILE}
	@chmod 666 ${DEP_FILE}

${DEP_FILE}:
	@echo ==== Creating: $@
	@echo > ${DEP_FILE}
	@chmod 666 ${DEP_FILE}

include ${DEP_FILE}

