# Compiles HLSL shaders with DXC into ${CH_OUTPUT_DIR}/Shaders, one folder per format:
# SPIRV for Vulkan on every platform and DXIL for Direct3D 12 on Windows.
#
# Each .hlsl file is one shader; its entry points are found by name (VSMain, PSMain,
# CSMain) and each one becomes <name>.<vs|ps|cs>.<spv|dxil>. A line "CH_VARIANT <suffix>
# <DEFINE>" in the file (in a comment) also builds every entry point with <DEFINE> defined
# into <name>_<suffix>.<stage>.<format>, so a shader can have versions without copies of
# its source. A change to any .hlsli in the same folder recompiles every shader of that
# folder.

find_program(CH_DXC_EXECUTABLE dxc
  HINTS "$ENV{VULKAN_SDK}/Bin" "$ENV{VULKAN_SDK}/bin"
  DOC "DirectX Shader Compiler, shipped with the Vulkan SDK")
if(NOT CH_DXC_EXECUTABLE)
  message(FATAL_ERROR "dxc not found. Install the Vulkan SDK and set VULKAN_SDK.")
endif()

set(CH_SHADER_MODEL "6_6")
set(CH_SHADER_OUTPUT_DIR "${CH_OUTPUT_DIR}/Shaders")

# ch_add_shaders(<target> <source folder>)
# Creates <target>, which builds every shader of <source folder>.
function(ch_add_shaders TARGET_NAME SOURCE_DIR)
  file(GLOB HLSL_FILES CONFIGURE_DEPENDS "${SOURCE_DIR}/*.hlsl")
  file(GLOB HLSL_INCLUDES CONFIGURE_DEPENDS "${SOURCE_DIR}/*.hlsli")

  set(FORMATS SPIRV)
  if(WIN32)
    list(APPEND FORMATS DXIL)
  endif()

  set(OUTPUTS "")
  foreach(HLSL_FILE ${HLSL_FILES})
    get_filename_component(SHADER_NAME ${HLSL_FILE} NAME_WE)
    file(READ ${HLSL_FILE} HLSL_TEXT)

    # "base" is the shader as written; each CH_VARIANT line adds one built with its define.
    set(VARIANTS base)
    string(REGEX MATCHALL "CH_VARIANT[ \t]+[A-Za-z0-9_]+[ \t]+[A-Za-z0-9_]+"
           VARIANT_LINES "${HLSL_TEXT}")
    foreach(VARIANT_LINE ${VARIANT_LINES})
      string(REGEX REPLACE "CH_VARIANT[ \t]+([A-Za-z0-9_]+)[ \t]+([A-Za-z0-9_]+)" "\\1:\\2"
             VARIANT "${VARIANT_LINE}")
      list(APPEND VARIANTS ${VARIANT})
    endforeach()

    foreach(VARIANT ${VARIANTS})
      if(VARIANT STREQUAL "base")
        set(OUTPUT_NAME ${SHADER_NAME})
        set(DEFINE_FLAGS "")
      else()
        string(REPLACE ":" ";" VARIANT_PARTS ${VARIANT})
        list(GET VARIANT_PARTS 0 VARIANT_SUFFIX)
        list(GET VARIANT_PARTS 1 VARIANT_DEFINE)
        set(OUTPUT_NAME ${SHADER_NAME}_${VARIANT_SUFFIX})
        set(DEFINE_FLAGS -D ${VARIANT_DEFINE})
      endif()

      foreach(STAGE vs ps cs)
        string(TOUPPER ${STAGE} STAGE_UPPER)
        set(ENTRY_POINT "${STAGE_UPPER}Main")
        if(NOT HLSL_TEXT MATCHES "(^|[^A-Za-z0-9_])${ENTRY_POINT}[ \t]*\\(")
          continue()
        endif()

        foreach(FORMAT ${FORMATS})
          if(FORMAT STREQUAL "SPIRV")
            set(EXTENSION spv)
            # Storage images get no format from their HLSL type, so one RWTexture type can
            # write any format (needs the WithoutFormat device features).
            set(FORMAT_FLAGS -spirv -fspv-target-env=vulkan1.3 -fspv-use-unknown-image-format)
          else()
            set(EXTENSION dxil)
            # DXIL has no use for the [[vk::...]] attributes, so their warnings are noise.
            set(FORMAT_FLAGS -Wno-ignored-attributes)
          endif()

          # CH_OUTPUT_DIR holds $<CONFIG> with Visual Studio; OUTPUT accepts it since
          # CMake 3.20.
          set(OUTPUT_FILE
              "${CH_SHADER_OUTPUT_DIR}/${FORMAT}/${OUTPUT_NAME}.${STAGE}.${EXTENSION}")
          add_custom_command(
            OUTPUT ${OUTPUT_FILE}
            COMMAND ${CMAKE_COMMAND} -E make_directory "${CH_SHADER_OUTPUT_DIR}/${FORMAT}"
            COMMAND ${CH_DXC_EXECUTABLE}
                    -T ${STAGE}_${CH_SHADER_MODEL} -E ${ENTRY_POINT} -HV 2021
                    ${FORMAT_FLAGS} ${DEFINE_FLAGS}
                    "$<IF:$<CONFIG:Debug>,-Od;-Zi;-Qembed_debug,-O3>"
                    -I "${SOURCE_DIR}"
                    -Fo ${OUTPUT_FILE} ${HLSL_FILE}
            DEPENDS ${HLSL_FILE} ${HLSL_INCLUDES}
            COMMENT "Compiling ${OUTPUT_NAME}.${STAGE} (${FORMAT})"
            COMMAND_EXPAND_LISTS
            VERBATIM)
          list(APPEND OUTPUTS ${OUTPUT_FILE})
        endforeach()
      endforeach()
    endforeach()
  endforeach()

  # Listed only so they show up in the IDE; Visual Studio would build them with FXC otherwise.
  set_source_files_properties(${HLSL_FILES} ${HLSL_INCLUDES} PROPERTIES VS_TOOL_OVERRIDE None)
  add_custom_target(${TARGET_NAME} ALL
    DEPENDS ${OUTPUTS}
    SOURCES ${HLSL_FILES} ${HLSL_INCLUDES})
endfunction()
