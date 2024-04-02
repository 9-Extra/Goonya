add_library(FreeImage SHARED IMPORTED GLOBAL)

set_target_properties(FreeImage PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/lib/FreeImage.dll
    IMPORTED_IMPLIB ${CMAKE_CURRENT_SOURCE_DIR}/lib/FreeImage.lib
)

target_include_directories(FreeImage INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)