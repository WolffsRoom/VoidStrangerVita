cmake_minimum_required(VERSION 3.14)
if(NOT DEFINED ENV{VITASDK})
  message(FATAL_ERROR "VITASDK is not defined")
endif()
set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "Vita toolchain")
project(voidstranger_vita C CXX)
include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)
set(VITA_MKSFOEX_FLAGS "-d ATTRIBUTE2=12")
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfloat-abi=softfp")

file(GLOB BS_COMMON_SOURCES ../butterscotch/src/*.c)
file(GLOB BS_GL_SOURCES ../butterscotch/src/gl/*.c ../butterscotch/src/gl_common/*.c ../butterscotch/src/gl_legacy/*.c ../butterscotch/src/image/*.c)
add_executable(${PROJECT_NAME}
  source/playable_main.c source/vita_settings.c source/vita_borders.c
  source/missing_data_scene.c
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
  ENABLE_WAD17 ENABLE_LEGACY_GL PLATFORM_VITA VOIDSTRANGER_VITA
  BUTTERSCOTCH_COMMIT_HASH="voidstranger-vita" BUTTERSCOTCH_COMMIT_DATE="2026-08-24")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_definitions(${PROJECT_NAME} PRIVATE ENABLE_VM_GML_PROFILER)
endif()
target_link_libraries(${PROJECT_NAME}
  ${CMAKE_SOURCE_DIR}/../../third_party/vitaGL-nosplash/libvitaGL.a
  vitashark SceShaccCgExt taihen_stub SceShaccCg_stub SceAppMgr_stub
  SceCommonDialog_stub SceCtrl_stub SceDisplay_stub SceGxm_stub
  SceKernelDmacMgr_stub ScePower_stub SceRtc_stub SceSysmem_stub SceTouch_stub
  kubridge_stub mathneon openal pthread SceAudio_stub SceAudioIn_stub
  SceAppUtil_stub SceNpTrophy_stub SceAvPlayer_stub crypto z m)

# Package the in-game PS Vita manual under sce_sys/manual.
# CONFIGURE_DEPENDS keeps future numbered pages in assets/manual/manual
# included automatically without editing this CMakeLists again.
file(GLOB VITA_MANUAL_FILES CONFIGURE_DEPENDS
  "${CMAKE_SOURCE_DIR}/../../assets/manual/manual/*")
set(VITA_MANUAL_VPK_FILES)
foreach(MANUAL_FILE IN LISTS VITA_MANUAL_FILES)
  if(NOT IS_DIRECTORY "${MANUAL_FILE}")
    get_filename_component(MANUAL_NAME "${MANUAL_FILE}" NAME)
    list(APPEND VITA_MANUAL_VPK_FILES
      "${MANUAL_FILE}" "sce_sys/manual/${MANUAL_NAME}")
  endif()
endforeach()

vita_create_self(eboot.bin ${PROJECT_NAME} UNSAFE)
vita_create_vpk(VoidStranger.vpk VSTR00001 eboot.bin
  VERSION 02.00 NAME "Void Stranger"
  FILE
    ${VITA_MANUAL_VPK_FILES}
    ${CMAKE_SOURCE_DIR}/../../assets/icon0.png sce_sys/icon0.png
    ${CMAKE_SOURCE_DIR}/../../assets/pic0.png sce_sys/pic0.png
    ${CMAKE_SOURCE_DIR}/../../assets/startup.png sce_sys/livearea/contents/startup.png
    ${CMAKE_SOURCE_DIR}/../../assets/bg0.png sce_sys/livearea/contents/bg0.png
    ${CMAKE_SOURCE_DIR}/../../assets/template.xml sce_sys/livearea/contents/template.xml
    ${CMAKE_SOURCE_DIR}/../../Trophy/void_stranger/TROPHY.TRP sce_sys/trophy/VSTR00001_00/TROPHY.TRP
    ${CMAKE_SOURCE_DIR}/../../assets/load_assets/loading_sprites.png assets/loading/loading_sprites.png
    ${CMAKE_SOURCE_DIR}/../../assets/load_assets/loading_digits.png assets/loading/loading_digits.png
    ${CMAKE_SOURCE_DIR}/../../assets/load_assets/loading_en.png assets/loading/loading_en.png
    ${CMAKE_SOURCE_DIR}/../../assets/load_assets/complete_en.png assets/loading/complete_en.png
    ${CMAKE_SOURCE_DIR}/../../assets/load_assets/loading_ptbr.png assets/loading/loading_ptbr.png
    ${CMAKE_SOURCE_DIR}/../../assets/load_assets/complete_ptbr.png assets/loading/complete_ptbr.png
    ${CMAKE_SOURCE_DIR}/../../assets/load_assets/loading_es.png assets/loading/loading_es.png
    ${CMAKE_SOURCE_DIR}/../../assets/load_assets/complete_es.png assets/loading/complete_es.png
    ${CMAKE_SOURCE_DIR}/../../assets/load_assets/loading_it.png assets/loading/loading_it.png
    ${CMAKE_SOURCE_DIR}/../../assets/load_assets/complete_it.png assets/loading/complete_it.png
    ${CMAKE_SOURCE_DIR}/../../assets/load_assets/loading_tr.png assets/loading/loading_tr.png
    ${CMAKE_SOURCE_DIR}/../../assets/load_assets/complete_tr.png assets/loading/complete_tr.png
    ${CMAKE_SOURCE_DIR}/../../assets/info_load/background/background.png assets/info_load/background/background.png
    ${CMAKE_SOURCE_DIR}/../../assets/info_load/background/effect.png assets/info_load/background/effect.png
    ${CMAKE_SOURCE_DIR}/../../assets/info_load/character/spr_sera_000.png assets/info_load/character/spr_sera_000.png
    ${CMAKE_SOURCE_DIR}/../../assets/info_load/character/spr_sera_001.png assets/info_load/character/spr_sera_001.png
    ${CMAKE_SOURCE_DIR}/../../assets/info_load/character/spr_sera_002.png assets/info_load/character/spr_sera_002.png
    ${CMAKE_SOURCE_DIR}/../../assets/info_load/character/spr_sera_003.png assets/info_load/character/spr_sera_003.png
    ${CMAKE_SOURCE_DIR}/../../assets/info_load/portrait/spr_sera_port_neutral_000.png assets/info_load/portrait/spr_sera_port_neutral_000.png
    ${CMAKE_SOURCE_DIR}/../../assets/info_load/portrait/spr_sera_port_happy.png assets/info_load/portrait/spr_sera_port_happy.png
    ${CMAKE_SOURCE_DIR}/../../assets/info_load/portrait/spr_sera_port_sigh.png assets/info_load/portrait/spr_sera_port_sigh.png
    ${CMAKE_SOURCE_DIR}/../../assets/info_load/ui/textbox.png assets/info_load/ui/textbox.png
    ${CMAKE_SOURCE_DIR}/../../assets/info_load/sound/snd_lev.wav assets/info_load/sound/snd_lev.wav
    ${CMAKE_SOURCE_DIR}/../../assets/info_load/sound/snd_lev_vita.wav assets/info_load/sound/snd_lev_vita.wav
    ${CMAKE_SOURCE_DIR}/../../assets/info_load/generated/dialog_hello.png assets/info_load/generated/dialog_hello.png
    ${CMAKE_SOURCE_DIR}/../../assets/info_load/generated/dialog_need.png assets/info_load/generated/dialog_need.png
    ${CMAKE_SOURCE_DIR}/../../assets/info_load/generated/dialog_yes_place.png assets/info_load/generated/dialog_yes_place.png
    ${CMAKE_SOURCE_DIR}/../../assets/info_load/generated/dialog_yes_check.png assets/info_load/generated/dialog_yes_check.png
    ${CMAKE_SOURCE_DIR}/../../assets/info_load/generated/dialog_missing.png assets/info_load/generated/dialog_missing.png
    ${CMAKE_SOURCE_DIR}/../../assets/info_load/generated/dialog_prepare.png assets/info_load/generated/dialog_prepare.png
    ${CMAKE_SOURCE_DIR}/../../assets/info_load/generated/dialog_help1.png assets/info_load/generated/dialog_help1.png
    ${CMAKE_SOURCE_DIR}/../../assets/info_load/generated/dialog_help2.png assets/info_load/generated/dialog_help2.png
    ${CMAKE_SOURCE_DIR}/../../assets/info_load/generated/dialog_final_exit.png assets/info_load/generated/dialog_final_exit.png
    ${CMAKE_SOURCE_DIR}/../../assets/info_load/generated/organize_question_yes.png assets/info_load/generated/organize_question_yes.png
    ${CMAKE_SOURCE_DIR}/../../assets/info_load/generated/organize_question_no.png assets/info_load/generated/organize_question_no.png
    ${CMAKE_SOURCE_DIR}/../../assets/info_load/generated/dialog_organize_decline.png assets/info_load/generated/dialog_organize_decline.png
    ${CMAKE_SOURCE_DIR}/../../assets/info_load/generated/dialog_organize_manual.png assets/info_load/generated/dialog_organize_manual.png
    ${CMAKE_SOURCE_DIR}/../../assets/info_load/generated/question_yes.png assets/info_load/generated/question_yes.png
    ${CMAKE_SOURCE_DIR}/../../assets/info_load/generated/question_no.png assets/info_load/generated/question_no.png
    ${CMAKE_SOURCE_DIR}/../../assets/info_load/generated/github_qr.png assets/info_load/generated/github_qr.png
    ${CMAKE_SOURCE_DIR}/../../assets/info_load/generated/github_url.png assets/info_load/generated/github_url.png
    ${CMAKE_SOURCE_DIR}/../../assets/info_load/generated/alkhemikal_dialog_atlas.png assets/info_load/generated/alkhemikal_dialog_atlas.png
    ${CMAKE_SOURCE_DIR}/../../assets/info_load/generated/textbox_crop.png assets/info_load/generated/textbox_crop.png)
