deps_config := \
	/Users/sky/esp/esp-idf/components/app_trace/Kconfig \
	/Users/sky/esp/esp-idf/components/aws_iot/Kconfig \
	/Users/sky/esp/esp-idf/components/bt/Kconfig \
	/Users/sky/esp/esp-idf/components/esp32/Kconfig \
	/Users/sky/esp/esp-idf/components/ethernet/Kconfig \
	/Users/sky/esp/esp-idf/components/fatfs/Kconfig \
	/Users/sky/esp/esp-idf/components/freertos/Kconfig \
	/Users/sky/esp/esp-idf/components/heap/Kconfig \
	/Users/sky/esp/esp-idf/components/log/Kconfig \
	/Users/sky/esp/esp-idf/components/lwip/Kconfig \
	/Users/sky/esp/esp-idf/components/mbedtls/Kconfig \
	/Users/sky/esp/esp-idf/components/openssl/Kconfig \
	/Users/sky/esp/esp-idf/components/pthread/Kconfig \
	/Users/sky/esp/esp-idf/components/spi_flash/Kconfig \
	/Users/sky/esp/esp-idf/components/tcpip_adapter/Kconfig \
	/Users/sky/esp/esp-idf/components/wear_levelling/Kconfig \
	/Users/sky/esp/esp-idf/components/bootloader/Kconfig.projbuild \
	/Users/sky/esp/esp-idf/components/esptool_py/Kconfig.projbuild \
	/Users/sky/esp/esp_demo/ESP32_Voice_record/main/Kconfig.projbuild \
	/Users/sky/esp/esp-idf/components/partition_table/Kconfig.projbuild \
	/Users/sky/esp/esp-idf/Kconfig

include/config/auto.conf: \
	$(deps_config)


$(deps_config): ;
