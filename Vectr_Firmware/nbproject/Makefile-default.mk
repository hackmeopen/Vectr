#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Include project Makefile
ifeq "${IGNORE_LOCAL}" "TRUE"
# do not include local makefile. User is passing all local related variables already
else
include Makefile
# Include makefile containing local settings
ifeq "$(wildcard nbproject/Makefile-local-default.mk)" "nbproject/Makefile-local-default.mk"
include nbproject/Makefile-local-default.mk
endif
endif

# Environment
MKDIR=gnumkdir -p
RM=rm -f 
MV=mv 
CP=cp 

# Macros
CND_CONF=default
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
IMAGE_TYPE=debug
OUTPUT_SUFFIX=elf
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/Vectr_Firmware.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
else
IMAGE_TYPE=production
OUTPUT_SUFFIX=hex
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/Vectr_Firmware.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
endif

# Object Directory
OBJECTDIR=build/${CND_CONF}/${IMAGE_TYPE}

# Distribution Directory
DISTDIR=dist/${CND_CONF}/${IMAGE_TYPE}

# Source Files Quoted if spaced
SOURCEFILES_QUOTED_IF_SPACED=C:/microchip/harmony/v0_80b/apps/Vectr/Vectr_Firmware/system_interrupt.c app.c bsp_init.c config_performance.c dac.c heap_2.c i2c.c leds.c main.c master_control.c mem.c menu.c system_config.c system_init.c system_interrupt.S system_tasks.c quantization_tables.c ../../../framework/system/int/src/sys_int_pic32.c ../../../third_party/rtos/FreeRTOS/Source/list.c ../../../third_party/rtos/FreeRTOS/Source/queue.c ../../../third_party/rtos/FreeRTOS/Source/tasks.c ../../../third_party/rtos/FreeRTOS/Source/timers.c ../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX/port.c port_asm.S

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/_ext/1523875924/system_interrupt.o ${OBJECTDIR}/app.o ${OBJECTDIR}/bsp_init.o ${OBJECTDIR}/config_performance.o ${OBJECTDIR}/dac.o ${OBJECTDIR}/heap_2.o ${OBJECTDIR}/i2c.o ${OBJECTDIR}/leds.o ${OBJECTDIR}/main.o ${OBJECTDIR}/master_control.o ${OBJECTDIR}/mem.o ${OBJECTDIR}/menu.o ${OBJECTDIR}/system_config.o ${OBJECTDIR}/system_init.o ${OBJECTDIR}/system_interrupt.o ${OBJECTDIR}/system_tasks.o ${OBJECTDIR}/quantization_tables.o ${OBJECTDIR}/_ext/1882072036/sys_int_pic32.o ${OBJECTDIR}/_ext/1351035265/list.o ${OBJECTDIR}/_ext/1351035265/queue.o ${OBJECTDIR}/_ext/1351035265/tasks.o ${OBJECTDIR}/_ext/1351035265/timers.o ${OBJECTDIR}/_ext/668018795/port.o ${OBJECTDIR}/port_asm.o
POSSIBLE_DEPFILES=${OBJECTDIR}/_ext/1523875924/system_interrupt.o.d ${OBJECTDIR}/app.o.d ${OBJECTDIR}/bsp_init.o.d ${OBJECTDIR}/config_performance.o.d ${OBJECTDIR}/dac.o.d ${OBJECTDIR}/heap_2.o.d ${OBJECTDIR}/i2c.o.d ${OBJECTDIR}/leds.o.d ${OBJECTDIR}/main.o.d ${OBJECTDIR}/master_control.o.d ${OBJECTDIR}/mem.o.d ${OBJECTDIR}/menu.o.d ${OBJECTDIR}/system_config.o.d ${OBJECTDIR}/system_init.o.d ${OBJECTDIR}/system_interrupt.o.d ${OBJECTDIR}/system_tasks.o.d ${OBJECTDIR}/quantization_tables.o.d ${OBJECTDIR}/_ext/1882072036/sys_int_pic32.o.d ${OBJECTDIR}/_ext/1351035265/list.o.d ${OBJECTDIR}/_ext/1351035265/queue.o.d ${OBJECTDIR}/_ext/1351035265/tasks.o.d ${OBJECTDIR}/_ext/1351035265/timers.o.d ${OBJECTDIR}/_ext/668018795/port.o.d ${OBJECTDIR}/port_asm.o.d

# Object Files
OBJECTFILES=${OBJECTDIR}/_ext/1523875924/system_interrupt.o ${OBJECTDIR}/app.o ${OBJECTDIR}/bsp_init.o ${OBJECTDIR}/config_performance.o ${OBJECTDIR}/dac.o ${OBJECTDIR}/heap_2.o ${OBJECTDIR}/i2c.o ${OBJECTDIR}/leds.o ${OBJECTDIR}/main.o ${OBJECTDIR}/master_control.o ${OBJECTDIR}/mem.o ${OBJECTDIR}/menu.o ${OBJECTDIR}/system_config.o ${OBJECTDIR}/system_init.o ${OBJECTDIR}/system_interrupt.o ${OBJECTDIR}/system_tasks.o ${OBJECTDIR}/quantization_tables.o ${OBJECTDIR}/_ext/1882072036/sys_int_pic32.o ${OBJECTDIR}/_ext/1351035265/list.o ${OBJECTDIR}/_ext/1351035265/queue.o ${OBJECTDIR}/_ext/1351035265/tasks.o ${OBJECTDIR}/_ext/1351035265/timers.o ${OBJECTDIR}/_ext/668018795/port.o ${OBJECTDIR}/port_asm.o

# Source Files
SOURCEFILES=C:/microchip/harmony/v0_80b/apps/Vectr/Vectr_Firmware/system_interrupt.c app.c bsp_init.c config_performance.c dac.c heap_2.c i2c.c leds.c main.c master_control.c mem.c menu.c system_config.c system_init.c system_interrupt.S system_tasks.c quantization_tables.c ../../../framework/system/int/src/sys_int_pic32.c ../../../third_party/rtos/FreeRTOS/Source/list.c ../../../third_party/rtos/FreeRTOS/Source/queue.c ../../../third_party/rtos/FreeRTOS/Source/tasks.c ../../../third_party/rtos/FreeRTOS/Source/timers.c ../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX/port.c port_asm.S


CFLAGS=
ASFLAGS=
LDLIBSOPTIONS=

############# Tool locations ##########################################
# If you copy a project from one host to another, the path where the  #
# compiler is installed may be different.                             #
# If you open this project with MPLAB X in the new host, this         #
# makefile will be regenerated and the paths will be corrected.       #
#######################################################################
# fixDeps replaces a bunch of sed/cat/printf statements that slow down the build
FIXDEPS=fixDeps

.build-conf:  ${BUILD_SUBPROJECTS}
	${MAKE} ${MAKE_OPTIONS} -f nbproject/Makefile-default.mk dist/${CND_CONF}/${IMAGE_TYPE}/Vectr_Firmware.${IMAGE_TYPE}.${OUTPUT_SUFFIX}

MP_PROCESSOR_OPTION=32MX450F256H
MP_LINKER_FILE_OPTION=
# ------------------------------------------------------------------------------------
# Rules for buildStep: assemble
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assembleWithPreprocess
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/system_interrupt.o: system_interrupt.S  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/system_interrupt.o.d 
	@${RM} ${OBJECTDIR}/system_interrupt.o 
	@${RM} ${OBJECTDIR}/system_interrupt.o.ok ${OBJECTDIR}/system_interrupt.o.err 
	@${FIXDEPS} "${OBJECTDIR}/system_interrupt.o.d" "${OBJECTDIR}/system_interrupt.o.asm.d" -t $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC} $(MP_EXTRA_AS_PRE)  -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1 -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/system_interrupt.o.d"  -o ${OBJECTDIR}/system_interrupt.o system_interrupt.S  -Wa,--defsym=__MPLAB_BUILD=1$(MP_EXTRA_AS_POST),-MD="${OBJECTDIR}/system_interrupt.o.asm.d",--defsym=__ICD2RAM=1,--defsym=__MPLAB_DEBUG=1,--gdwarf-2,--defsym=__DEBUG=1,--defsym=__MPLAB_DEBUGGER_ICD3=1
	
${OBJECTDIR}/port_asm.o: port_asm.S  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/port_asm.o.d 
	@${RM} ${OBJECTDIR}/port_asm.o 
	@${RM} ${OBJECTDIR}/port_asm.o.ok ${OBJECTDIR}/port_asm.o.err 
	@${FIXDEPS} "${OBJECTDIR}/port_asm.o.d" "${OBJECTDIR}/port_asm.o.asm.d" -t $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC} $(MP_EXTRA_AS_PRE)  -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1 -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/port_asm.o.d"  -o ${OBJECTDIR}/port_asm.o port_asm.S  -Wa,--defsym=__MPLAB_BUILD=1$(MP_EXTRA_AS_POST),-MD="${OBJECTDIR}/port_asm.o.asm.d",--defsym=__ICD2RAM=1,--defsym=__MPLAB_DEBUG=1,--gdwarf-2,--defsym=__DEBUG=1,--defsym=__MPLAB_DEBUGGER_ICD3=1
	
else
${OBJECTDIR}/system_interrupt.o: system_interrupt.S  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/system_interrupt.o.d 
	@${RM} ${OBJECTDIR}/system_interrupt.o 
	@${RM} ${OBJECTDIR}/system_interrupt.o.ok ${OBJECTDIR}/system_interrupt.o.err 
	@${FIXDEPS} "${OBJECTDIR}/system_interrupt.o.d" "${OBJECTDIR}/system_interrupt.o.asm.d" -t $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC} $(MP_EXTRA_AS_PRE)  -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/system_interrupt.o.d"  -o ${OBJECTDIR}/system_interrupt.o system_interrupt.S  -Wa,--defsym=__MPLAB_BUILD=1$(MP_EXTRA_AS_POST),-MD="${OBJECTDIR}/system_interrupt.o.asm.d",--gdwarf-2
	
${OBJECTDIR}/port_asm.o: port_asm.S  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/port_asm.o.d 
	@${RM} ${OBJECTDIR}/port_asm.o 
	@${RM} ${OBJECTDIR}/port_asm.o.ok ${OBJECTDIR}/port_asm.o.err 
	@${FIXDEPS} "${OBJECTDIR}/port_asm.o.d" "${OBJECTDIR}/port_asm.o.asm.d" -t $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC} $(MP_EXTRA_AS_PRE)  -c -mprocessor=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/port_asm.o.d"  -o ${OBJECTDIR}/port_asm.o port_asm.S  -Wa,--defsym=__MPLAB_BUILD=1$(MP_EXTRA_AS_POST),-MD="${OBJECTDIR}/port_asm.o.asm.d",--gdwarf-2
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: compile
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/_ext/1523875924/system_interrupt.o: C:/microchip/harmony/v0_80b/apps/Vectr/Vectr_Firmware/system_interrupt.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/1523875924 
	@${RM} ${OBJECTDIR}/_ext/1523875924/system_interrupt.o.d 
	@${RM} ${OBJECTDIR}/_ext/1523875924/system_interrupt.o 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1523875924/system_interrupt.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1 -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/_ext/1523875924/system_interrupt.o.d" -o ${OBJECTDIR}/_ext/1523875924/system_interrupt.o C:/microchip/harmony/v0_80b/apps/Vectr/Vectr_Firmware/system_interrupt.c   
	
${OBJECTDIR}/app.o: app.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/app.o.d 
	@${RM} ${OBJECTDIR}/app.o 
	@${FIXDEPS} "${OBJECTDIR}/app.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1 -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/app.o.d" -o ${OBJECTDIR}/app.o app.c   
	
${OBJECTDIR}/bsp_init.o: bsp_init.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/bsp_init.o.d 
	@${RM} ${OBJECTDIR}/bsp_init.o 
	@${FIXDEPS} "${OBJECTDIR}/bsp_init.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1 -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/bsp_init.o.d" -o ${OBJECTDIR}/bsp_init.o bsp_init.c   
	
${OBJECTDIR}/config_performance.o: config_performance.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/config_performance.o.d 
	@${RM} ${OBJECTDIR}/config_performance.o 
	@${FIXDEPS} "${OBJECTDIR}/config_performance.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1 -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/config_performance.o.d" -o ${OBJECTDIR}/config_performance.o config_performance.c   
	
${OBJECTDIR}/dac.o: dac.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/dac.o.d 
	@${RM} ${OBJECTDIR}/dac.o 
	@${FIXDEPS} "${OBJECTDIR}/dac.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1 -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/dac.o.d" -o ${OBJECTDIR}/dac.o dac.c   
	
${OBJECTDIR}/heap_2.o: heap_2.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/heap_2.o.d 
	@${RM} ${OBJECTDIR}/heap_2.o 
	@${FIXDEPS} "${OBJECTDIR}/heap_2.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1 -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/heap_2.o.d" -o ${OBJECTDIR}/heap_2.o heap_2.c   
	
${OBJECTDIR}/i2c.o: i2c.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/i2c.o.d 
	@${RM} ${OBJECTDIR}/i2c.o 
	@${FIXDEPS} "${OBJECTDIR}/i2c.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1 -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/i2c.o.d" -o ${OBJECTDIR}/i2c.o i2c.c   
	
${OBJECTDIR}/leds.o: leds.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/leds.o.d 
	@${RM} ${OBJECTDIR}/leds.o 
	@${FIXDEPS} "${OBJECTDIR}/leds.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1 -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/leds.o.d" -o ${OBJECTDIR}/leds.o leds.c   
	
${OBJECTDIR}/main.o: main.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/main.o.d 
	@${RM} ${OBJECTDIR}/main.o 
	@${FIXDEPS} "${OBJECTDIR}/main.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1 -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/main.o.d" -o ${OBJECTDIR}/main.o main.c   
	
${OBJECTDIR}/master_control.o: master_control.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/master_control.o.d 
	@${RM} ${OBJECTDIR}/master_control.o 
	@${FIXDEPS} "${OBJECTDIR}/master_control.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1 -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/master_control.o.d" -o ${OBJECTDIR}/master_control.o master_control.c   
	
${OBJECTDIR}/mem.o: mem.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/mem.o.d 
	@${RM} ${OBJECTDIR}/mem.o 
	@${FIXDEPS} "${OBJECTDIR}/mem.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1 -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/mem.o.d" -o ${OBJECTDIR}/mem.o mem.c   
	
${OBJECTDIR}/menu.o: menu.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/menu.o.d 
	@${RM} ${OBJECTDIR}/menu.o 
	@${FIXDEPS} "${OBJECTDIR}/menu.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1 -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/menu.o.d" -o ${OBJECTDIR}/menu.o menu.c   
	
${OBJECTDIR}/system_config.o: system_config.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/system_config.o.d 
	@${RM} ${OBJECTDIR}/system_config.o 
	@${FIXDEPS} "${OBJECTDIR}/system_config.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1 -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/system_config.o.d" -o ${OBJECTDIR}/system_config.o system_config.c   
	
${OBJECTDIR}/system_init.o: system_init.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/system_init.o.d 
	@${RM} ${OBJECTDIR}/system_init.o 
	@${FIXDEPS} "${OBJECTDIR}/system_init.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1 -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/system_init.o.d" -o ${OBJECTDIR}/system_init.o system_init.c   
	
${OBJECTDIR}/system_tasks.o: system_tasks.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/system_tasks.o.d 
	@${RM} ${OBJECTDIR}/system_tasks.o 
	@${FIXDEPS} "${OBJECTDIR}/system_tasks.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1 -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/system_tasks.o.d" -o ${OBJECTDIR}/system_tasks.o system_tasks.c   
	
${OBJECTDIR}/quantization_tables.o: quantization_tables.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/quantization_tables.o.d 
	@${RM} ${OBJECTDIR}/quantization_tables.o 
	@${FIXDEPS} "${OBJECTDIR}/quantization_tables.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1 -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/quantization_tables.o.d" -o ${OBJECTDIR}/quantization_tables.o quantization_tables.c   
	
${OBJECTDIR}/_ext/1882072036/sys_int_pic32.o: ../../../framework/system/int/src/sys_int_pic32.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/1882072036 
	@${RM} ${OBJECTDIR}/_ext/1882072036/sys_int_pic32.o.d 
	@${RM} ${OBJECTDIR}/_ext/1882072036/sys_int_pic32.o 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1882072036/sys_int_pic32.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1 -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/_ext/1882072036/sys_int_pic32.o.d" -o ${OBJECTDIR}/_ext/1882072036/sys_int_pic32.o ../../../framework/system/int/src/sys_int_pic32.c   
	
${OBJECTDIR}/_ext/1351035265/list.o: ../../../third_party/rtos/FreeRTOS/Source/list.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/1351035265 
	@${RM} ${OBJECTDIR}/_ext/1351035265/list.o.d 
	@${RM} ${OBJECTDIR}/_ext/1351035265/list.o 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1351035265/list.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1 -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/_ext/1351035265/list.o.d" -o ${OBJECTDIR}/_ext/1351035265/list.o ../../../third_party/rtos/FreeRTOS/Source/list.c   
	
${OBJECTDIR}/_ext/1351035265/queue.o: ../../../third_party/rtos/FreeRTOS/Source/queue.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/1351035265 
	@${RM} ${OBJECTDIR}/_ext/1351035265/queue.o.d 
	@${RM} ${OBJECTDIR}/_ext/1351035265/queue.o 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1351035265/queue.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1 -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/_ext/1351035265/queue.o.d" -o ${OBJECTDIR}/_ext/1351035265/queue.o ../../../third_party/rtos/FreeRTOS/Source/queue.c   
	
${OBJECTDIR}/_ext/1351035265/tasks.o: ../../../third_party/rtos/FreeRTOS/Source/tasks.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/1351035265 
	@${RM} ${OBJECTDIR}/_ext/1351035265/tasks.o.d 
	@${RM} ${OBJECTDIR}/_ext/1351035265/tasks.o 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1351035265/tasks.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1 -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/_ext/1351035265/tasks.o.d" -o ${OBJECTDIR}/_ext/1351035265/tasks.o ../../../third_party/rtos/FreeRTOS/Source/tasks.c   
	
${OBJECTDIR}/_ext/1351035265/timers.o: ../../../third_party/rtos/FreeRTOS/Source/timers.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/1351035265 
	@${RM} ${OBJECTDIR}/_ext/1351035265/timers.o.d 
	@${RM} ${OBJECTDIR}/_ext/1351035265/timers.o 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1351035265/timers.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1 -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/_ext/1351035265/timers.o.d" -o ${OBJECTDIR}/_ext/1351035265/timers.o ../../../third_party/rtos/FreeRTOS/Source/timers.c   
	
${OBJECTDIR}/_ext/668018795/port.o: ../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX/port.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/668018795 
	@${RM} ${OBJECTDIR}/_ext/668018795/port.o.d 
	@${RM} ${OBJECTDIR}/_ext/668018795/port.o 
	@${FIXDEPS} "${OBJECTDIR}/_ext/668018795/port.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE) -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1 -fframe-base-loclist  -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/_ext/668018795/port.o.d" -o ${OBJECTDIR}/_ext/668018795/port.o ../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX/port.c   
	
else
${OBJECTDIR}/_ext/1523875924/system_interrupt.o: C:/microchip/harmony/v0_80b/apps/Vectr/Vectr_Firmware/system_interrupt.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/1523875924 
	@${RM} ${OBJECTDIR}/_ext/1523875924/system_interrupt.o.d 
	@${RM} ${OBJECTDIR}/_ext/1523875924/system_interrupt.o 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1523875924/system_interrupt.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/_ext/1523875924/system_interrupt.o.d" -o ${OBJECTDIR}/_ext/1523875924/system_interrupt.o C:/microchip/harmony/v0_80b/apps/Vectr/Vectr_Firmware/system_interrupt.c   
	
${OBJECTDIR}/app.o: app.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/app.o.d 
	@${RM} ${OBJECTDIR}/app.o 
	@${FIXDEPS} "${OBJECTDIR}/app.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/app.o.d" -o ${OBJECTDIR}/app.o app.c   
	
${OBJECTDIR}/bsp_init.o: bsp_init.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/bsp_init.o.d 
	@${RM} ${OBJECTDIR}/bsp_init.o 
	@${FIXDEPS} "${OBJECTDIR}/bsp_init.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/bsp_init.o.d" -o ${OBJECTDIR}/bsp_init.o bsp_init.c   
	
${OBJECTDIR}/config_performance.o: config_performance.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/config_performance.o.d 
	@${RM} ${OBJECTDIR}/config_performance.o 
	@${FIXDEPS} "${OBJECTDIR}/config_performance.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/config_performance.o.d" -o ${OBJECTDIR}/config_performance.o config_performance.c   
	
${OBJECTDIR}/dac.o: dac.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/dac.o.d 
	@${RM} ${OBJECTDIR}/dac.o 
	@${FIXDEPS} "${OBJECTDIR}/dac.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/dac.o.d" -o ${OBJECTDIR}/dac.o dac.c   
	
${OBJECTDIR}/heap_2.o: heap_2.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/heap_2.o.d 
	@${RM} ${OBJECTDIR}/heap_2.o 
	@${FIXDEPS} "${OBJECTDIR}/heap_2.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/heap_2.o.d" -o ${OBJECTDIR}/heap_2.o heap_2.c   
	
${OBJECTDIR}/i2c.o: i2c.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/i2c.o.d 
	@${RM} ${OBJECTDIR}/i2c.o 
	@${FIXDEPS} "${OBJECTDIR}/i2c.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/i2c.o.d" -o ${OBJECTDIR}/i2c.o i2c.c   
	
${OBJECTDIR}/leds.o: leds.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/leds.o.d 
	@${RM} ${OBJECTDIR}/leds.o 
	@${FIXDEPS} "${OBJECTDIR}/leds.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/leds.o.d" -o ${OBJECTDIR}/leds.o leds.c   
	
${OBJECTDIR}/main.o: main.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/main.o.d 
	@${RM} ${OBJECTDIR}/main.o 
	@${FIXDEPS} "${OBJECTDIR}/main.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/main.o.d" -o ${OBJECTDIR}/main.o main.c   
	
${OBJECTDIR}/master_control.o: master_control.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/master_control.o.d 
	@${RM} ${OBJECTDIR}/master_control.o 
	@${FIXDEPS} "${OBJECTDIR}/master_control.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/master_control.o.d" -o ${OBJECTDIR}/master_control.o master_control.c   
	
${OBJECTDIR}/mem.o: mem.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/mem.o.d 
	@${RM} ${OBJECTDIR}/mem.o 
	@${FIXDEPS} "${OBJECTDIR}/mem.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/mem.o.d" -o ${OBJECTDIR}/mem.o mem.c   
	
${OBJECTDIR}/menu.o: menu.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/menu.o.d 
	@${RM} ${OBJECTDIR}/menu.o 
	@${FIXDEPS} "${OBJECTDIR}/menu.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/menu.o.d" -o ${OBJECTDIR}/menu.o menu.c   
	
${OBJECTDIR}/system_config.o: system_config.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/system_config.o.d 
	@${RM} ${OBJECTDIR}/system_config.o 
	@${FIXDEPS} "${OBJECTDIR}/system_config.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/system_config.o.d" -o ${OBJECTDIR}/system_config.o system_config.c   
	
${OBJECTDIR}/system_init.o: system_init.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/system_init.o.d 
	@${RM} ${OBJECTDIR}/system_init.o 
	@${FIXDEPS} "${OBJECTDIR}/system_init.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/system_init.o.d" -o ${OBJECTDIR}/system_init.o system_init.c   
	
${OBJECTDIR}/system_tasks.o: system_tasks.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/system_tasks.o.d 
	@${RM} ${OBJECTDIR}/system_tasks.o 
	@${FIXDEPS} "${OBJECTDIR}/system_tasks.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/system_tasks.o.d" -o ${OBJECTDIR}/system_tasks.o system_tasks.c   
	
${OBJECTDIR}/quantization_tables.o: quantization_tables.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR} 
	@${RM} ${OBJECTDIR}/quantization_tables.o.d 
	@${RM} ${OBJECTDIR}/quantization_tables.o 
	@${FIXDEPS} "${OBJECTDIR}/quantization_tables.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/quantization_tables.o.d" -o ${OBJECTDIR}/quantization_tables.o quantization_tables.c   
	
${OBJECTDIR}/_ext/1882072036/sys_int_pic32.o: ../../../framework/system/int/src/sys_int_pic32.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/1882072036 
	@${RM} ${OBJECTDIR}/_ext/1882072036/sys_int_pic32.o.d 
	@${RM} ${OBJECTDIR}/_ext/1882072036/sys_int_pic32.o 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1882072036/sys_int_pic32.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/_ext/1882072036/sys_int_pic32.o.d" -o ${OBJECTDIR}/_ext/1882072036/sys_int_pic32.o ../../../framework/system/int/src/sys_int_pic32.c   
	
${OBJECTDIR}/_ext/1351035265/list.o: ../../../third_party/rtos/FreeRTOS/Source/list.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/1351035265 
	@${RM} ${OBJECTDIR}/_ext/1351035265/list.o.d 
	@${RM} ${OBJECTDIR}/_ext/1351035265/list.o 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1351035265/list.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/_ext/1351035265/list.o.d" -o ${OBJECTDIR}/_ext/1351035265/list.o ../../../third_party/rtos/FreeRTOS/Source/list.c   
	
${OBJECTDIR}/_ext/1351035265/queue.o: ../../../third_party/rtos/FreeRTOS/Source/queue.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/1351035265 
	@${RM} ${OBJECTDIR}/_ext/1351035265/queue.o.d 
	@${RM} ${OBJECTDIR}/_ext/1351035265/queue.o 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1351035265/queue.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/_ext/1351035265/queue.o.d" -o ${OBJECTDIR}/_ext/1351035265/queue.o ../../../third_party/rtos/FreeRTOS/Source/queue.c   
	
${OBJECTDIR}/_ext/1351035265/tasks.o: ../../../third_party/rtos/FreeRTOS/Source/tasks.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/1351035265 
	@${RM} ${OBJECTDIR}/_ext/1351035265/tasks.o.d 
	@${RM} ${OBJECTDIR}/_ext/1351035265/tasks.o 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1351035265/tasks.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/_ext/1351035265/tasks.o.d" -o ${OBJECTDIR}/_ext/1351035265/tasks.o ../../../third_party/rtos/FreeRTOS/Source/tasks.c   
	
${OBJECTDIR}/_ext/1351035265/timers.o: ../../../third_party/rtos/FreeRTOS/Source/timers.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/1351035265 
	@${RM} ${OBJECTDIR}/_ext/1351035265/timers.o.d 
	@${RM} ${OBJECTDIR}/_ext/1351035265/timers.o 
	@${FIXDEPS} "${OBJECTDIR}/_ext/1351035265/timers.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/_ext/1351035265/timers.o.d" -o ${OBJECTDIR}/_ext/1351035265/timers.o ../../../third_party/rtos/FreeRTOS/Source/timers.c   
	
${OBJECTDIR}/_ext/668018795/port.o: ../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX/port.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} ${OBJECTDIR}/_ext/668018795 
	@${RM} ${OBJECTDIR}/_ext/668018795/port.o.d 
	@${RM} ${OBJECTDIR}/_ext/668018795/port.o 
	@${FIXDEPS} "${OBJECTDIR}/_ext/668018795/port.o.d" $(SILENT) -rsi ${MP_CC_DIR}../  -c ${MP_CC}  $(MP_EXTRA_CC_PRE)  -g -x c -c -mprocessor=$(MP_PROCESSOR_OPTION) -I"." -I"../../../framework/peripheral" -I"../../../framework/driver" -I"../../../bsp" -I"../../../framework" -I"../../../framework/system" -I"../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX" -I"../../../third_party/rtos/FreeRTOS/Source/include" -MMD -MF "${OBJECTDIR}/_ext/668018795/port.o.d" -o ${OBJECTDIR}/_ext/668018795/port.o ../../../third_party/rtos/FreeRTOS/Source/portable/MPLAB/PIC32MX/port.c   
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: compileCPP
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: link
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
dist/${CND_CONF}/${IMAGE_TYPE}/Vectr_Firmware.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk  ../../../bin/framework/peripheral/PIC32MX450F256H_peripherals.a ..\\..\\..\\bin\\framework\\peripheral\\PIC32MX450F256H_peripherals.a  
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -mdebugger -D__MPLAB_DEBUGGER_ICD3=1 -mprocessor=$(MP_PROCESSOR_OPTION)  -o dist/${CND_CONF}/${IMAGE_TYPE}/Vectr_Firmware.${IMAGE_TYPE}.${OUTPUT_SUFFIX} ${OBJECTFILES_QUOTED_IF_SPACED}    ..\..\..\bin\framework\peripheral\PIC32MX450F256H_peripherals.a ..\..\..\bin\framework\peripheral\PIC32MX450F256H_peripherals.a       -mreserve=data@0x0:0x1FC -mreserve=boot@0x1FC02000:0x1FC02FEF -mreserve=boot@0x1FC02000:0x1FC0275F  -Wl,--defsym=__MPLAB_BUILD=1$(MP_EXTRA_LD_POST)$(MP_LINKER_FILE_OPTION),--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,--defsym=__MPLAB_DEBUGGER_ICD3=1,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map"
	
else
dist/${CND_CONF}/${IMAGE_TYPE}/Vectr_Firmware.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk  ../../../bin/framework/peripheral/PIC32MX450F256H_peripherals.a ..\\..\\..\\bin\\framework\\peripheral\\PIC32MX450F256H_peripherals.a 
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -mprocessor=$(MP_PROCESSOR_OPTION)  -o dist/${CND_CONF}/${IMAGE_TYPE}/Vectr_Firmware.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} ${OBJECTFILES_QUOTED_IF_SPACED}    ..\..\..\bin\framework\peripheral\PIC32MX450F256H_peripherals.a ..\..\..\bin\framework\peripheral\PIC32MX450F256H_peripherals.a      -Wl,--defsym=__MPLAB_BUILD=1$(MP_EXTRA_LD_POST)$(MP_LINKER_FILE_OPTION),-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map"
	${MP_CC_DIR}\\xc32-bin2hex dist/${CND_CONF}/${IMAGE_TYPE}/Vectr_Firmware.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} 
endif


# Subprojects
.build-subprojects:


# Subprojects
.clean-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r build/default
	${RM} -r dist/default

# Enable dependency checking
.dep.inc: .depcheck-impl

DEPFILES=$(shell mplabwildcard ${POSSIBLE_DEPFILES})
ifneq (${DEPFILES},)
include ${DEPFILES}
endif
