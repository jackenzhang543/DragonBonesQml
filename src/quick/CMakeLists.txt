file(GLOB_RECURSE DRAGONBONES_SOURCES CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/3rd/dragonBones/*.cpp
)

qt_add_library(DragonBonesPlugin SHARED)

qt_add_qml_module(DragonBonesPlugin
    URI DragonBones
    VERSION 1.0
    OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/qml/DragonBones
    SOURCES
        DragonBonesItem.h
        DragonBonesItem.cpp
        QtDragonBonesBackend.h
        QtDragonBonesBackend.cpp
        RenderData.h
        legenddragonbones_global.h
        ${DRAGONBONES_SOURCES}
    RESOURCE_PREFIX /qt/qml
)

target_compile_definitions(DragonBonesPlugin
    PRIVATE LEGENDDRAGONBONES_LIBRARY
)

target_include_directories(DragonBonesPlugin
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/3rd
)

target_link_libraries(DragonBonesPlugin
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Quick
        Qt6::Qml
)

if(MINGW)
    target_compile_options(DragonBonesPlugin PRIVATE -Wno-deprecated-declarations)
endif()

install(TARGETS DragonBonesPlugin
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
