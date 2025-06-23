# File: main/CMakeLists.txt

idf_build_get_property(IDF_TARGET IDF_TARGET)

if(CONFIG_EXAMPLE_IPV4)
    set(client_src tcp_client_v4.c)
else()
    set(client_src tcp_client_v6.c)
endif()

idf_component_register(
    SRCS
        main.c
        sensor.c
        stepcounter.c
        tcp_client.c
        display.c
    INCLUDE_DIRS "."
    REQUIRES
        protocol_examples_common
        nvs_flash
        esp_timer
        esp_netif
        esp_event
        spiffs
        driver
        st7789
)

spiffs_create_partition_image(storage data FLASH_IN_PROJECT)
