include_guard(GLOBAL)

# Exact native identity input. Artifact hashes are recorded separately after install.
function(pixie_sdk_prepare)
    if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR
       NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "The initial SDK identity contract supports Windows x64 MSVC only")
    endif()
    if(DEFINED PIXIE_AVX2 AND NOT PIXIE_AVX2)
        message(FATAL_ERROR "The initial SDK tuple requires AVX2/FMA/F16C; PIXIE_AVX2=OFF is not qualified")
    endif()
    set(sdk_root "${PixieEngine_SOURCE_DIR}")
    set(sdk_output "${CMAKE_CURRENT_BINARY_DIR}/sdk")
    set(sdk_include "${sdk_output}/include")
    if(PIXIE_SDK_PROFILE STREQUAL "local-development")
        set(sdk_distribution local-private)
        set(sdk_capabilities 7)
        set(sdk_modules_json true)
        set(sdk_automation_json true)
    elseif(PIXIE_SDK_PROFILE STREQUAL "authoring")
        set(sdk_distribution public)
        set(sdk_capabilities 5)
        set(sdk_modules_json true)
        set(sdk_automation_json false)
    elseif(PIXIE_SDK_PROFILE STREQUAL "shipping")
        set(sdk_distribution public)
        set(sdk_capabilities 4)
        set(sdk_modules_json false)
        set(sdk_automation_json false)
    else()
        message(FATAL_ERROR "Unknown SDK identity profile: ${PIXIE_SDK_PROFILE}")
    endif()
    set(sdk_windows_version "${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}")
    if(NOT sdk_windows_version)
        set(sdk_windows_version "$ENV{WindowsSDKVersion}")
        string(REGEX REPLACE "[/\\]+$" "" sdk_windows_version "${sdk_windows_version}")
    endif()
    if(NOT sdk_windows_version MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+$")
        message(FATAL_ERROR "The selected Windows SDK version must be available from the actual VS/developer toolchain")
    endif()

    # Inventory only inputs that affect the native producer or installed support.
    # Tests, schedulers, ordinary docs, ignored builds and full dirty provenance are
    # intentionally separate; absolute paths never enter an installed identity.
    set(sdk_patterns
        engine/*.cpp engine/*.hpp engine/CMakeLists.txt engine/*/CMakeLists.txt
        runtime/src/*.cpp runtime/include/*.hpp runtime/player/*.cpp runtime/CMakeLists.txt
        capi/src/*.cpp capi/src/*.hpp capi/include/*.h capi/CMakeLists.txt
        cmake/*.cmake cmake/sdk/*.in scripts/sdk/*.py templates/*
        tools/export/*.cpp tools/export/*.hpp tools/export/CMakeLists.txt
        external/pixielib/cpp/include/* external/pixielib/cpp/math/include/*
        external/pixielib/cpp/math/src/* external/glm/glm/* external/entt/src/*
        external/glew/include/* external/glew/src/glew.c external/glfw/src/* external/glfw/include/*
        external/glfw/CMake/* external/stb/*.h external/nlohmann_json/single_include/*
        external/box2d/src/* external/box2d/include/*)
    if(PIXIE_DEV_CHANNEL)
        list(APPEND sdk_patterns devchannel/src/* devchannel/include/* devchannel/CMakeLists.txt)
    endif()
    set(sdk_inputs CMakeLists.txt version.json external/CMakeLists.txt
        external/pixielib/cpp/CMakeLists.txt external/glfw/CMakeLists.txt external/box2d/CMakeLists.txt
        capi/bindings/dotnet/PixieCapi.cs tools/NativeApiVersion.cs tools/NativeApiVersion.props)
    foreach(pattern IN LISTS sdk_patterns)
        file(GLOB_RECURSE matches CONFIGURE_DEPENDS LIST_DIRECTORIES FALSE RELATIVE "${sdk_root}" "${sdk_root}/${pattern}")
        list(APPEND sdk_inputs ${matches})
    endforeach()
    list(REMOVE_DUPLICATES sdk_inputs)
    list(REMOVE_ITEM sdk_inputs cmake/PixieRootSolution.cmake cmake/PixieSnapshots.cmake)
    list(SORT sdk_inputs)
    set(sdk_inventory "")
    set(sdk_dependencies "")
    foreach(input IN LISTS sdk_inputs)
        file(SHA256 "${sdk_root}/${input}" digest)
        # Glob verification detects membership only. Existing input content edits
        # must regenerate identity before an ordinary incremental compilation too.
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${sdk_root}/${input}")
        string(APPEND sdk_inventory "${input}\t${digest}\n")
        if(input MATCHES "^external/")
            string(APPEND sdk_dependencies "${input}\t${digest}\n")
        endif()
    endforeach()
    string(SHA256 sdk_dependency_hash "${sdk_dependencies}")
    string(SHA256 sdk_source_hash "${sdk_inventory}")
    string(SHA256 sdk_id "pixie-sdk/1\n${sdk_inventory}\n${CMAKE_CXX_COMPILER_ID}\n${CMAKE_CXX_COMPILER_VERSION}\n${sdk_windows_version}\n${CMAKE_CXX_FLAGS}\n${CMAKE_CXX_FLAGS_DEBUG}\n${CMAKE_CXX_FLAGS_RELEASE}\n")
    file(MAKE_DIRECTORY "${sdk_output}")
    file(WRITE "${sdk_output}/producer-inputs.tsv" "${sdk_inventory}")
    file(WRITE "${sdk_output}/dependency-inputs.tsv" "${sdk_dependencies}")
    file(READ "${sdk_root}/capi/include/pixie_capi.h" sdk_capi_header)
    string(REGEX MATCH "#define PIXIE_CAPI_VERSION ([0-9]+)" unused "${sdk_capi_header}")
    set(sdk_capi_version "${CMAKE_MATCH_1}")
    if(NOT sdk_capi_version MATCHES "^[0-9]+$")
        message(FATAL_ERROR "Cannot derive the native C API version from its owning header")
    endif()

    set(sdk_probe [=[
#include <cstdio>
#include <string>
#ifdef _CPPUNWIND
#define SDK_EXCEPTIONS 1
#else
#define SDK_EXCEPTIONS 0
#endif
#ifdef _CPPRTTI
#define SDK_RTTI 1
#else
#define SDK_RTTI 0
#endif
int main() {
    std::printf("{\"msvcFullVersion\":%d,\"stlVersion\":%d,\"stlUpdate\":%d,\"iteratorDebugLevel\":%d,\"exceptions\":%d,\"rtti\":%d}",
        _MSC_FULL_VER, _MSVC_STL_VERSION, _MSVC_STL_UPDATE, _ITERATOR_DEBUG_LEVEL, SDK_EXCEPTIONS, SDK_RTTI);
}
]=])
    foreach(sdk_configuration Debug Release)
        if(PIXIE_SDK_PROFILE STREQUAL "shipping" AND sdk_configuration STREQUAL "Debug")
            file(MAKE_DIRECTORY "${sdk_include}/Debug")
            file(WRITE "${sdk_include}/Debug/pixie_sdk_build.h" "#error Shipping Debug is not an admitted SDK tuple; select Release.\n")
            continue()
        endif()
        set(CMAKE_TRY_COMPILE_CONFIGURATION "${sdk_configuration}")
        try_run(sdk_run sdk_compiled SOURCE_FROM_VAR sdk_tuple.cpp sdk_probe NO_CACHE
            CXX_STANDARD 20 CXX_STANDARD_REQUIRED TRUE CXX_EXTENSIONS FALSE
            RUN_OUTPUT_VARIABLE sdk_probe_output COMPILE_OUTPUT_VARIABLE sdk_probe_log)
        if(NOT sdk_compiled OR NOT sdk_run EQUAL 0)
            message(FATAL_ERROR "Cannot observe actual ${sdk_configuration} SDK compiler/STL tuple: ${sdk_probe_log}\n${sdk_probe_output}")
        endif()
        foreach(field msvcFullVersion stlVersion stlUpdate iteratorDebugLevel exceptions rtti)
            string(JSON sdk_${field} GET "${sdk_probe_output}" "${field}")
        endforeach()
        if(sdk_exceptions)
            set(sdk_exceptions_json true)
        else()
            set(sdk_exceptions_json false)
        endif()
        if(sdk_rtti)
            set(sdk_rtti_json true)
        else()
            set(sdk_rtti_json false)
        endif()
        if(sdk_configuration STREQUAL "Debug")
            set(sdk_crt MDd)
            set(sdk_debug 1)
        else()
            set(sdk_crt MD)
            set(sdk_debug 0)
        endif()
        string(SHA256 sdk_build_id "${sdk_id}\n${sdk_distribution}\n${PIXIE_SDK_PROFILE}\n${sdk_configuration}\n${sdk_probe_output}\n${sdk_crt}\nwindows\nx64\nc++20\navx2,fma,f16c\n${sdk_windows_version}\n")
        set(sdk_runtime_json "{\"schemaVersion\":1,\"sdkId\":\"${sdk_id}\",\"sdkBuildId\":\"${sdk_build_id}\",\"productVersion\":\"${PixieEngine_VERSION}\",\"cApiVersion\":${sdk_capi_version},\"moduleIdentityContract\":1,\"platform\":\"windows\",\"architecture\":\"x64\",\"compilerId\":\"MSVC\",\"compilerVersion\":\"${CMAKE_CXX_COMPILER_VERSION}\",\"msvcFullVersion\":${sdk_msvcFullVersion},\"stlVersion\":${sdk_stlVersion},\"stlUpdate\":${sdk_stlUpdate},\"iteratorDebugLevel\":${sdk_iteratorDebugLevel},\"cppStandard\":20,\"configuration\":\"${sdk_configuration}\",\"crt\":\"${sdk_crt}\",\"cpuFeatures\":[\"avx2\",\"fma\",\"f16c\"],\"profile\":\"${PIXIE_SDK_PROFILE}\",\"distributionClass\":\"${sdk_distribution}\",\"exceptions\":${sdk_exceptions_json},\"rtti\":${sdk_rtti_json},\"windowsSdkVersion\":\"${sdk_windows_version}\",\"dependencyInventorySha256\":\"${sdk_dependency_hash}\",\"capabilities\":${sdk_capabilities}}")
        string(JSON sdk_runtime_json SET "${sdk_runtime_json}" capabilities
            "{\"moduleLoading\":${sdk_modules_json},\"nativeAutomation\":${sdk_automation_json},\"candidateAuthoring\":true}")
        string(JSON sdk_runtime_json SET "${sdk_runtime_json}" capabilityBits "${sdk_capabilities}")
        string(REPLACE "\\" "\\\\" sdk_runtime_json_escaped "${sdk_runtime_json}")
        string(REPLACE "\"" "\\\"" sdk_runtime_json_escaped "${sdk_runtime_json_escaped}")
        string(REPLACE "\n" "\\n" sdk_runtime_json_escaped "${sdk_runtime_json_escaped}")
        file(MAKE_DIRECTORY "${sdk_include}/${sdk_configuration}")
        configure_file("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/PixieSdkBuild.h.in"
            "${sdk_include}/${sdk_configuration}/pixie_sdk_build.h" @ONLY)
        file(WRITE "${sdk_output}/identity-${sdk_configuration}.json" "${sdk_runtime_json}\n")
    endforeach()
    # Unsupported configurations have an explicit compiler refusal instead of remapping.
    foreach(configuration IN LISTS CMAKE_CONFIGURATION_TYPES)
        if(NOT configuration MATCHES "^(Debug|Release)$")
            file(MAKE_DIRECTORY "${sdk_include}/${configuration}")
            file(WRITE "${sdk_include}/${configuration}/pixie_sdk_build.h" "#error This SDK supports only its explicitly admitted Debug/Release tuples.\n")
        endif()
    endforeach()
    set(PIXIE_SDK_GENERATED_INCLUDE_DIR "${sdk_include}/$<CONFIG>" PARENT_SCOPE)
    set(PIXIE_SDK_ID "${sdk_id}" PARENT_SCOPE)
    set(PIXIE_SDK_SOURCE_INVENTORY_SHA256 "${sdk_source_hash}" PARENT_SCOPE)
endfunction()

function(pixie_sdk_attach target)
    target_include_directories(${target} PUBLIC "$<BUILD_INTERFACE:${PIXIE_SDK_GENERATED_INCLUDE_DIR}>")
    target_include_directories(pixie_capi_objects PRIVATE "${PIXIE_SDK_GENERATED_INCLUDE_DIR}")
    target_compile_definitions(pixie_capi_objects PRIVATE PIXIE_SDK_PRODUCER)
    # Existing JSON is an implementation dependency, never a new public SDK dependency.
    target_link_libraries(pixie_capi_objects PRIVATE nlohmann_json::nlohmann_json)
endfunction()

# Registration is opt-in so ordinary source/game builds acquire no packaging tool
# dependency. B0 owns the root call; installation uses dedicated immutable staging units.
function(pixie_sdk_install)
    option(PIXIE_SDK_INSTALL "Register native SDK install-unit rules" OFF)
    if(NOT PIXIE_SDK_INSTALL)
        return()
    endif()
    if(NOT PROJECT_IS_TOP_LEVEL OR NOT TARGET pixie-player)
        message(FATAL_ERROR "SDK production requires a top-level Engine build with its generic pixie-player")
    endif()
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    # The compiled public math archive must not require a producer compiler PDB.
    set_property(TARGET pixie_math PROPERTY MSVC_DEBUG_INFORMATION_FORMAT Embedded)
    include(CMakePackageConfigHelpers)
    set(sdk_root "${PixieEngine_SOURCE_DIR}")
    set(sdk_output "${CMAKE_CURRENT_BINARY_DIR}/sdk")
    set(sdk_id "${PIXIE_SDK_ID}")
    set(tuple "profiles/${PIXIE_SDK_PROFILE}/$<CONFIG>")
    file(MAKE_DIRECTORY "${sdk_output}/package")
    configure_file("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/PixieEngineConfig.cmake.in"
        "${sdk_output}/package/PixieEngineConfig.cmake" @ONLY)
    write_basic_package_version_file("${sdk_output}/package/PixieEngineConfigVersion.cmake"
        VERSION "${PixieEngine_VERSION}" COMPATIBILITY ExactVersion)
    install(FILES "${sdk_output}/package/PixieEngineConfig.cmake"
        "${sdk_output}/package/PixieEngineConfigVersion.cmake" DESTINATION lib/cmake/PixieEngine COMPONENT PixieSDK)
    install(FILES "$<TARGET_FILE:pixie_shared>" "$<TARGET_FILE:glfw>" "$<TARGET_FILE:pixie-player>"
        DESTINATION "${tuple}/bin" COMPONENT PixieSDK)
    install(FILES "$<TARGET_LINKER_FILE:pixie_shared>" DESTINATION "${tuple}/lib" COMPONENT PixieSDK)
    install(FILES "$<TARGET_FILE:pixie_math>" DESTINATION "${tuple}/lib" COMPONENT PixieSDK)
    install(FILES "${PIXIE_SDK_GENERATED_INCLUDE_DIR}/pixie_sdk_build.h" DESTINATION "${tuple}/include" COMPONENT PixieSDK)
    foreach(boundary core platform render app)
        if(PIXIE_SDK_PROFILE STREQUAL "local-development")
            install(DIRECTORY "${sdk_root}/engine/${boundary}/include/" DESTINATION include COMPONENT PixieSDK
                FILES_MATCHING PATTERN "*.hpp")
        else()
            install(DIRECTORY "${sdk_root}/engine/${boundary}/include/" DESTINATION include COMPONENT PixieSDK
                FILES_MATCHING PATTERN "*.hpp" PATTERN "LocalCommandChannel.hpp" EXCLUDE)
        endif()
    endforeach()
    install(DIRECTORY "${sdk_root}/runtime/include/" DESTINATION include COMPONENT PixieSDK FILES_MATCHING PATTERN "*.hpp")
    install(FILES "${sdk_root}/capi/include/pixie_capi.h" DESTINATION include COMPONENT PixieSDK)
    install(DIRECTORY "${sdk_root}/external/pixielib/cpp/include/" "${sdk_root}/external/pixielib/cpp/math/include/"
        "${sdk_root}/external/entt/src/" DESTINATION third-party/include COMPONENT PixieSDK
        FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h" PATTERN "*.inl")
    install(DIRECTORY "${sdk_root}/external/glm/glm" DESTINATION third-party/include COMPONENT PixieSDK
        FILES_MATCHING PATTERN "*.hpp" PATTERN "*.h" PATTERN "*.inl")
    install(DIRECTORY "${sdk_root}/external/glew/include/GL" DESTINATION third-party/include COMPONENT PixieSDK
        FILES_MATCHING PATTERN "*.h")
    if(PIXIE_SDK_PROFILE STREQUAL "local-development")
        install(DIRECTORY "${sdk_root}/devchannel/include/" DESTINATION include COMPONENT PixieSDK FILES_MATCHING PATTERN "*.hpp")
        install(DIRECTORY "${sdk_root}/external/nlohmann_json/single_include/nlohmann" DESTINATION third-party/include COMPONENT PixieSDK
            FILES_MATCHING PATTERN "*.hpp")
    endif()
    install(FILES "${sdk_root}/capi/src/GameModuleEntry.cpp" DESTINATION support/module COMPONENT PixieSDK)
    install(FILES "${sdk_root}/runtime/player/PlayerMain.cpp" "${sdk_root}/capi/src/SceneGame.cpp"
        DESTINATION support/player COMPONENT PixieSDK)
    install(FILES "${sdk_root}/capi/bindings/dotnet/PixieCapi.cs" "${sdk_root}/tools/NativeApiVersion.cs"
        "${sdk_root}/tools/NativeApiVersion.props" DESTINATION bindings/dotnet COMPONENT PixieSDK)
    foreach(helper PixieTargetDefaults PixieProject PixieGame PixieAddCode PixieComponentRegistry PixieInitComponents PixieExportProduct)
        install(FILES "${sdk_root}/cmake/${helper}.cmake" DESTINATION cmake COMPONENT PixieSDK)
    endforeach()
    install(DIRECTORY "${sdk_root}/templates/project-code/" DESTINATION templates/project-code COMPONENT PixieSDK
        FILES_MATCHING PATTERN "*.in" PATTERN "versions.json")
    install(FILES "${sdk_root}/scripts/sdk/verify_sdk.py" "${sdk_root}/scripts/sdk/assemble_sdk.py" DESTINATION tools/sdk COMPONENT PixieSDK)
    install(FILES "${sdk_output}/producer-inputs.tsv" "${sdk_output}/dependency-inputs.tsv" DESTINATION provenance COMPONENT PixieSDK)
    install(FILES "${sdk_root}/LICENSE" DESTINATION notices/PixieEngine COMPONENT PixieSDK)
    foreach(dependency pixielib glm entt glew glfw stb box2d nlohmann_json)
        if(dependency STREQUAL "glm")
            set(license copying.txt)
        elseif(dependency STREQUAL "glew")
            set(license LICENSE.txt)
        elseif(dependency STREQUAL "glfw")
            set(license LICENSE.md)
        elseif(dependency STREQUAL "nlohmann_json")
            set(license LICENSE.MIT)
        else()
            set(license LICENSE)
        endif()
        install(FILES "${sdk_root}/external/${dependency}/${license}" DESTINATION "notices/${dependency}" COMPONENT PixieSDK)
    endforeach()
    # Windows system API/UCRT remains an OS prerequisite. Release has actual app-local
    # compiler runtimes; Debug explicitly declares the matching developer CRT instead
    # of silently copying Debug_NonRedist into a public SDK payload.
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
    set(CMAKE_INSTALL_DEBUG_LIBRARIES FALSE)
    set(CMAKE_INSTALL_UCRT_LIBRARIES FALSE)
    include(InstallRequiredSystemLibraries)
    if(NOT CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
        message(FATAL_ERROR "The selected compiler provides no qualified Release runtime inputs")
    endif()
    install(FILES ${CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS} DESTINATION "${tuple}/bin" CONFIGURATIONS Release COMPONENT PixieSDK)
    set(crt_provenance "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}\nRelease app-local input hashes:\n")
    foreach(runtime IN LISTS CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
        get_filename_component(name "${runtime}" NAME)
        file(SHA256 "${runtime}" hash)
        string(APPEND crt_provenance "${name}\t${hash}\n")
    endforeach()
    string(APPEND crt_provenance "Debug requires its matching installed developer CRT.\nWindows system APIs/UCRT and an OpenGL 4.5 core driver are system prerequisites.\n")
    file(WRITE "${sdk_output}/package/compiler-runtime.txt" "${crt_provenance}")
    install(FILES "${sdk_output}/package/compiler-runtime.txt" DESTINATION notices/Microsoft-runtime COMPONENT PixieSDK)
    configure_file("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/PixieSdkManifest.cmake.in"
        "${sdk_output}/package/PixieSdkManifest.cmake" @ONLY)
    install(SCRIPT "${sdk_output}/package/PixieSdkManifest.cmake" COMPONENT PixieSDK)
endfunction()
