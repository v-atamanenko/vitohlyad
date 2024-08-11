cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
	if(DEFINED ENV{VITASDK})
		set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
	else()
		message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
	endif()
endif()

project(vitohlyad)

include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "Vitohlyad")
set(VITA_TITLEID "VUKR00001")
set(VITA_VPKNAME "vitohlyad")
set(VITA_VERSION "00.90")
set(VITA_MKSFOEX_FLAGS "-d ATTRIBUTE2=12")

# Optional: used for "send" and "dump" targets
set(PSVITAIP "192.168.1.110" CACHE STRING "PSVita IP (for FTP access)")
set(PARSECOREPATH "$ENV{PARSECOREPATH}" CACHE STRING "vita-parse-core main.py")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O3")

add_custom_command(
		OUTPUT data.h
		COMMAND ${CMAKE_SOURCE_DIR}/extras/scripts/generate_data_h.sh ${CMAKE_SOURCE_DIR}/data/ ${CMAKE_SOURCE_DIR}/src/data.h
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/
)

add_custom_command(
		OUTPUT pic0.png.h
		COMMAND ${CMAKE_SOURCE_DIR}/extras/scripts/binary_to_header.sh ${CMAKE_SOURCE_DIR}/extras/livearea/pic0.png ${CMAKE_SOURCE_DIR}/src/pic0.png.h
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/
)

add_executable(${PROJECT_NAME}
		${PROJECT_SOURCE_DIR}/src/main.c
		${PROJECT_SOURCE_DIR}/src/utils.c
		${PROJECT_SOURCE_DIR}/src/sha1.c
		${PROJECT_SOURCE_DIR}/src/psp2_msgbox.c
		pic0.png.h
		data.h)

add_subdirectory(cdlg-fix/)
add_dependencies(${CMAKE_PROJECT_NAME} cdlg-fix.skprx-self)

include_directories(
		${PROJECT_SOURCE_DIR}/src
)

find_package(SDL2 REQUIRED)
target_link_libraries(${PROJECT_NAME}
		vitaGL
		vitashark
		mathneon
		SceKernelDmacMgr_stub
		SceShaccCgExt
		SceShaccCg_stub
		taihen_stub
		SDL2_image
		SDL2::SDL2-static
		webp
		jpeg
		png
		pthread
		ScePower_stub
		SceDisplay_stub
		SceCtrl_stub
		SceAudio_stub
		SceSysmodule_stub
		SceShellSvc_stub
		SceGxm_stub
		SceIme_stub
		SceCommonDialog_stub
		SceAppMgr_stub
		SceTouch_stub
		SceHid_stub
		SceMotion_stub
		SceVshBridge_stub
		z
		m
		c
		)

vita_create_self(eboot.bin ${CMAKE_PROJECT_NAME} UNSAFE NOASLR)
vita_create_vpk(${VITA_VPKNAME}.vpk ${VITA_TITLEID} eboot.bin
		VERSION ${VITA_VERSION}
		NAME ${VITA_APP_NAME}
		FILE ${CMAKE_SOURCE_DIR}/data data
		${CMAKE_BINARY_DIR}/cdlg-fix/cdlg-fix.skprx cdlg-fix.skprx
		${CMAKE_SOURCE_DIR}/extras/nosleep.skprx nosleep.skprx
		${CMAKE_SOURCE_DIR}/extras/livearea/icon0.png sce_sys/icon0.png
		${CMAKE_SOURCE_DIR}/extras/livearea/pic0.png sce_sys/pic0.png
		${CMAKE_SOURCE_DIR}/extras/livearea/startup.png sce_sys/livearea/contents/startup.png
		${CMAKE_SOURCE_DIR}/extras/livearea/bg0.png sce_sys/livearea/contents/bg0.png
		${CMAKE_SOURCE_DIR}/extras/livearea/template.xml sce_sys/livearea/contents/template.xml
)

add_custom_target(send
                  COMMAND echo destroy | nc ${PSVITAIP} 1338
                  COMMAND echo destroy | nc ${PSVITAIP} 1338
                  COMMAND curl -T eboot.bin ftp://${PSVITAIP}:1337/ux0:/app/${VITA_TITLEID}/
                  COMMAND echo launch ${VITA_TITLEID} | nc ${PSVITAIP} 1338
                  DEPENDS ${VITA_VPKNAME}.vpk-vpk
                  )

add_custom_target(dump
                  COMMAND ${CMAKE_SOURCE_DIR}/extras/scripts/get_dump.sh ${PSVITAIP}
                  COMMAND python3 ${PARSECOREPATH} coredump ${CMAKE_PROJECT_NAME}
                  )
