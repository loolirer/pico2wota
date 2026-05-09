add_library(pico2wota_reboot INTERFACE)

target_include_directories(pico2wota_reboot INTERFACE
	${CMAKE_CURRENT_LIST_DIR}/include
)

target_sources(pico2wota_reboot INTERFACE
	${CMAKE_CURRENT_LIST_DIR}/reboot.c
)

target_link_libraries(pico2wota_reboot INTERFACE
	cmsis_core
	hardware_structs
)
