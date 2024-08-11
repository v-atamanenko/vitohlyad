cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
	if(DEFINED ENV{VITASDK})
		set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
	else()
		message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
	endif()
endif()

project(cdlg-fix)
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3 -std=gnu99")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -nostdlib")

link_directories(
	${CMAKE_CURRENT_BINARY_DIR}
)

add_executable(${PROJECT_NAME}
	${PROJECT_SOURCE_DIR}/cdlg-fix.c
)

target_link_libraries(${PROJECT_NAME}
	k
	taihenForKernel_stub
	SceSysmemForDriver_stub
	SceThreadmgrForDriver_stub
	SceIofilemgrForDriver_stub
	SceDebugForDriver_stub
)

vita_create_self(${PROJECT_NAME}.skprx ${PROJECT_NAME}
  UNSAFE
  CONFIG ${PROJECT_SOURCE_DIR}/exports.yml
)
