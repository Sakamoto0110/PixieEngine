# Build-owned product inputs. The native exporter expands the runtime closure and asset references
# into a concrete, hashed file manifest; no export recursively copies a source or binary directory.
function(pixie_export_product target)
    cmake_parse_arguments(PARSE_ARGV 1 PRODUCT "" "" "ASSETS")
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS_SKIP TRUE)
    include(InstallRequiredSystemLibraries)
    set(runtime "")
    foreach(file IN LISTS CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS)
        if(runtime)
            string(APPEND runtime ",")
        endif()
        string(APPEND runtime "\"${file}\"")
    endforeach()
    set(assets "")
    foreach(file IN LISTS PRODUCT_ASSETS)
        if(IS_ABSOLUTE "${file}" OR file MATCHES "(^|/)\\.\\.(/|$)" OR file MATCHES "[\\\";]")
            message(FATAL_ERROR "Export ASSETS must be portable project-relative paths")
        endif()
        if(assets)
            string(APPEND assets ",")
        endif()
        string(APPEND assets "\"${file}\"")
    endforeach()
    file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/pixie-export/$<CONFIG>/${target}.json" CONTENT
"{\n\"format\":\"pixie-product\",\"version\":1,\"configuration\":\"$<CONFIG>\",
\"development\":$<IF:$<BOOL:${PIXIE_DEVELOPMENT}>,true,false>,
\"dev_channel\":$<IF:$<BOOL:${PIXIE_DEV_CHANNEL}>,true,false>,
\"player\":\"$<TARGET_FILE:${target}>\",
\"libraries\":[\"$<TARGET_FILE:pixie_shared>\",\"$<TARGET_FILE:glfw>\"],
\"runtime\":[${runtime}],\"assets\":[${assets}]}\n")
endfunction()
