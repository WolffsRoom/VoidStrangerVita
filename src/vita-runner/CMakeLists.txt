cmake_minimum_required(VERSION 3.14)
if(NOT DEFINED ENV{VITASDK})
  message(FATAL_ERROR "VITASDK is not defined")
endif()
set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "Vita toolchain")
project(voidstranger_vita C CXX)
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)
set(VITA_MKSFOEX_FLAGS "-d ATTRIBUTE2=12")
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -Wall -Wextra")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfloat-abi=softfp")

file(GLOB BS_COMMON_SOURCES ../butterscotch/src/*.c)
file(GLOB BS_GL_SOURCES ../butterscotch/src/gl/*.c ../butterscotch/src/gl_common/*.c ../butterscotch/src/gl_legacy/*.c ../butterscotch/src/image/*.c)
add_executable(${PROJECT_NAME}
  source/playable_main.c source/vita_settings.c source/vita_borders.c
  source/vita_video.c source/vita_trophies.c ${BS_COMMON_SOURCES} ${BS_GL_SOURCES}
  ../butterscotch/src/audio/openal/al_audio_system.c
  ../butterscotch/src/audio/openal/wave.c ../butterscotch/src/android/stb_impl.c
  ../butterscotch/vendor/md5/md5.c ../butterscotch/vendor/sha1/sha1.c
  ../butterscotch/vendor/base64/base64.c
  ../butterscotch/vendor/bzip2/bz_internal_error.c ../butterscotch/vendor/bzip2/bzlib.c
  ../butterscotch/vendor/bzip2/crctable.c ../butterscotch/vendor/bzip2/decompress.c
  ../butterscotch/vendor/bzip2/huffman.c ../butterscotch/vendor/bzip2/randtable.c)
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)
target_include_directories(${PROJECT_NAME} PRIVATE
  ../butterscotch/src ../butterscotch/src/gl ../butterscotch/src/image
  ../butterscotch/src/gl_common ../butterscotch/src/gl_legacy
  ../butterscotch/src/audio/openal ../butterscotch/vendor/bzip2
  ../butterscotch/vendor/md5 ../butterscotch/vendor/sha1 ../butterscotch/vendor/base64
  ../butterscotch/vendor/stb/ds ../butterscotch/vendor/stb/image
  ../butterscotch/vendor/stb/image_write ../butterscotch/vendor/stb/vorbis)
target_compile_definitions(${PROJECT_NAME} PRIVATE
  ENABLE_WAD17 ENABLE_LEGACY_GL PLATFORM_VITA VOIDSTRANGER_VITA ENABLE_VM_GML_PROFILER
  BUTTERSCOTCH_COMMIT_HASH="voidstranger-vita" BUTTERSCOTCH_COMMIT_DATE="2026-08-24")
target_link_libraries(${PROJECT_NAME}
  ${CMAKE_SOURCE_DIR}/../../third_party/vitaGL-nosplash/libvitaGL.a
  vitashark SceShaccCgExt taihen_stub SceShaccCg_stub SceAppMgr_stub
  SceCommonDialog_stub SceCtrl_stub SceDisplay_stub SceGxm_stub
  SceKernelDmacMgr_stub ScePower_stub SceRtc_stub SceSysmem_stub SceTouch_stub
  kubridge_stub mathneon openal pthread SceAudio_stub SceAudioIn_stub
  SceAppUtil_stub SceNpTrophy_stub SceAvPlayer_stub z m)
vita_create_self(eboot.bin ${PROJECT_NAME} UNSAFE)
vita_create_vpk(VoidStranger.vpk VSTR00001 eboot.bin
  VERSION 01.00 NAME "Void Stranger"
  FILE
    ${CMAKE_SOURCE_DIR}/../../assets/icon0.png sce_sys/icon0.png
    ${CMAKE_SOURCE_DIR}/../../assets/pic0.png sce_sys/pic0.png
    ${CMAKE_SOURCE_DIR}/../../assets/startup.png sce_sys/livearea/contents/startup.png
    ${CMAKE_SOURCE_DIR}/../../assets/bg0.png sce_sys/livearea/contents/bg0.png
    ${CMAKE_SOURCE_DIR}/../../assets/template.xml sce_sys/livearea/contents/template.xml
    ${CMAKE_SOURCE_DIR}/../../assets/load_assets/frame0.png assets/loading/frame0.png
    ${CMAKE_SOURCE_DIR}/../../assets/load_assets/frame1.png assets/loading/frame1.png)
