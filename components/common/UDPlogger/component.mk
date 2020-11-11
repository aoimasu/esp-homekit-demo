# Component makefile for UDPlogger

INC_DIRS += $(udplogger_ROOT)

udplogger_INC_DIR = $(udplogger_ROOT)
udplogger_SRC_DIR = $(udplogger_ROOT)

$(eval $(call component_compile_rules,udplogger))
