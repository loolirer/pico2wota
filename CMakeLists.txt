# Derived from the Pico SDK, which carries the following
# LICENSE.txt:
# Copyright 2020 (c) 2020 Raspberry Pi (Trading) Ltd.
#
# Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
# following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
#    disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
#    disclaimer in the documentation and/or other materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(pico2wota C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(pico2wota
	main.c
	tcp_comm.c
	dhcpserver/dhcpserver.c
)

function(target_cl_options option)
    target_compile_options(pico2wota PRIVATE ${option})
    target_link_options(pico2wota PRIVATE ${option})
endfunction()

target_cl_options("-Wall")
target_cl_options("-Os")
target_cl_options("-ffunction-sections")
target_cl_options("-fdata-sections")
target_link_options(pico2wota PRIVATE "LINKER:--gc-sections")

pico_add_extra_outputs(pico2wota)

target_include_directories(pico2wota PRIVATE
	${CMAKE_CURRENT_LIST_DIR} # Needed so that lwip can find lwipopts.h
	${CMAKE_CURRENT_LIST_DIR}/dhcpserver)

pico_enable_stdio_usb(pico2wota 1)

add_subdirectory(pico2wota_reboot)

target_link_libraries(pico2wota
	cmsis_core
	hardware_dma
	hardware_flash
	hardware_resets
	hardware_structs
	pico_cyw43_arch_lwip_poll
	pico_stdlib
	pico_sync
	pico_util
	pico2wota_reboot
)

# Retrieves build variables from the environment if present
function(pico2wota_retrieve_variable name hidden)
	if (DEFINED ENV{${name}} AND (NOT ${name}))
		set(${name} $ENV{${name}} PARENT_SCOPE)
		if (hidden)
			set(log_value "hidden")
		else()
			set(log_value "'$ENV{${name}}'")
		endif()
		message("Using ${name} from environment (${log_value})")
	endif()
endfunction()

pico2wota_retrieve_variable(PICO2WOTA_WIFI_SSID false)
pico2wota_retrieve_variable(PICO2WOTA_WIFI_PASS true)
pico2wota_retrieve_variable(PICO2WOTA_WIFI_AP false)

if ((NOT PICO2WOTA_WIFI_SSID) OR (NOT PICO2WOTA_WIFI_PASS))
        message(FATAL_ERROR
		"WiFi SSID/Pass not set, please set PICO2WOTA_WIFI_SSID/PICO2WOTA_WIFI_PASS."
	)
endif ()

# TODO: This causes a full rebuild if they change, configure_file might
# be better.
target_compile_definitions(pico2wota PUBLIC PICO2WOTA_WIFI_SSID=${PICO2WOTA_WIFI_SSID})
target_compile_definitions(pico2wota PUBLIC PICO2WOTA_WIFI_PASS=${PICO2WOTA_WIFI_PASS})

# Use the WiFi AP mode upon request
if (PICO2WOTA_WIFI_AP)
	target_compile_definitions(pico2wota PUBLIC PICO2WOTA_WIFI_AP=1)
	message("Building in WiFi AP mode.")
endif()

# Provide a helper to build a standalone target
function(pico2wota_build_standalone NAME)
	get_target_property(PICO2WOTA_SRC_DIR pico2wota SOURCE_DIR)
	pico_set_linker_script(${NAME} ${PICO2WOTA_SRC_DIR}/standalone.ld)
	pico_add_bin_output(${NAME})
endfunction()

# Provide a helper to build a combined target
# The build process is roughly:
# 1. Build the bootloader, using a special linker script which leaves
#    two sections to be filled in with the header (.app_hdr) and
#    app binary (.app_bin)
# 2. Build the app binary, using a special linker script to set the load
#    address properly and skip boot2.
# 3. Calculate the checksum of the app binary
# 4. Update the header and binary sections in the ELF from 1.
function(pico2wota_build_combined NAME)
	set(APP_BIN ${CMAKE_CURRENT_BINARY_DIR}/${NAME}.bin)
	set(APP_HDR_BIN ${CMAKE_CURRENT_BINARY_DIR}/${NAME}_hdr.bin)
	set(COMBINED pico2wota_${NAME})
	get_target_property(PICO2WOTA_SRC_DIR pico2wota SOURCE_DIR)
	get_target_property(PICO2WOTA_BIN_DIR pico2wota BINARY_DIR)

	get_directory_property(initial_clean ADDITIONAL_MAKE_CLEAN_FILES)
	list(APPEND clean_files ${initial_clean})
	list(APPEND clean_files ${APP_BIN})
	list(APPEND clean_files ${APP_HDR_BIN})

	# The app must be built with the correct linker script (and a .bin)
	pico2wota_build_standalone(${NAME})

	# Build the bootloader with the sections to fill in
	pico_set_linker_script(pico2wota ${PICO2WOTA_SRC_DIR}/bootloader_shell.ld)

	add_custom_target(${NAME}_hdr DEPENDS ${NAME} pico2wota)
	add_custom_command(TARGET ${NAME}_hdr POST_BUILD
		COMMAND ${PICO2WOTA_SRC_DIR}/gen_imghdr.py --map ${PICO2WOTA_BIN_DIR}/pico2wota.elf.map --section .app_bin ${APP_BIN} ${APP_HDR_BIN}
	)

	add_custom_target(${COMBINED} ALL)
	add_dependencies(${COMBINED} pico2wota ${NAME}_hdr)
	add_custom_command(TARGET ${COMBINED} POST_BUILD
		COMMAND ${CMAKE_OBJCOPY}
			--update-section .app_hdr=${APP_HDR_BIN}
			--update-section .app_bin=${APP_BIN} ${PICO2WOTA_BIN_DIR}/pico2wota.elf ${COMBINED}.elf
	)
	list(APPEND clean_files ${COMBINED}.bin)
	list(APPEND clean_files ${COMBINED}.elf)

	add_custom_command(TARGET ${COMBINED} POST_BUILD
		COMMAND ${CMAKE_OBJCOPY} -Obinary ${COMBINED}.elf ${COMBINED}.bin
	)

	add_custom_command(
        TARGET ${COMBINED} POST_BUILD
        COMMAND picotool uf2 convert ${COMBINED}.elf ${COMBINED}.uf2 --family rp2350-arm-s
    )

	set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${clean_files}")
endfunction()
