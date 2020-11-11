PROGRAM = sonoff_d1

EXTRA_COMPONENTS = \
	extras/http-parser \
	extras/dhcpserver \
	$(abspath ../../components/esp8266-open-rtos/wifi_config) \
	$(abspath ../../components/esp8266-open-rtos/cJSON) \
	$(abspath ../../components/common/wolfssl) \
	$(abspath ../../components/common/homekit) \
	$(abspath ../../components/common/UDPlogger)

FLASH_SIZE ?= 8
FLASH_MODE ?= dout
FLASH_SPEED ?= 40
HOMEKIT_SPI_FLASH_BASE_ADDR ?= 0x7A000

EXTRA_CFLAGS += -I../.. -DHOMEKIT_SHORT_APPLE_UUIDS
EXTRA_CFLAGS += -DUDPLOG_PRINTF_TO_UDP

include $(SDK_PATH)/common.mk

monitor:
	$(FILTEROUTPUT) --port /dev/tty.usbserial-AK05CHOE --baud 115200 --elf $(PROGRAM_OUT)
