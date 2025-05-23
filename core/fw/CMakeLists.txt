project(fw)

##
# A bit hacky, but there's ONE file in softrend that needs to be in the DDI.
# Do the needful.
##
set(SOFTREND_DDI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../drivers/softrend/ddi)

target_include_directories(brender-inc INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/brender>
        )

target_include_directories(brender-inc-ddi INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ddi>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${SOFTREND_DDI_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/brender/ddi>
        )

add_custom_command(
        OUTPUT dev_objs.cgh
        # This will work with MSVC and GCC
        COMMAND "${CMAKE_C_COMPILER}" -D__CLASSGEN__ -E "${CMAKE_CURRENT_SOURCE_DIR}/dev_objs.hpp" > dev_objs.tmp
        COMMAND "${PERL_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/../../make/classgen.pl" < dev_objs.tmp > dev_objs.cgh
        MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/dev_objs.hpp"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../../make/classgen.pl"
        BYPRODUCTS dev_objs.tmp
)

set(GENERATED_FILES
        ${CMAKE_CURRENT_BINARY_DIR}/dev_objs.cgh
        )

set(FW_DEV_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/dev_objs.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../make/classgen.pl
        )

source_group("Device" FILES ${FW_DEV_FILES} ${GENERATED_FILES})
set_source_files_properties(${FW_DEV_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)

add_custom_target(fw-generated DEPENDS ${GENERATED_FILES})

set(DDI_FILES
        ddi/brassert.h
        ddi/brddi.h
        ddi/brddi_p.h
        ddi/brexcept.h
        ddi/brhton.h
        ddi/brimage.h
        ddi/datafile.h
        ddi/dataf_p.h
        ddi/formats.h
        ddi/lexer.h
        ddi/register.h
        ddi/shortcut.h
        ddi/tokenmap.h

        ${CMAKE_CURRENT_BINARY_DIR}/dev_objs.cgh

        ${SOFTREND_DDI_DIR}/priminfo.h
        )

set(INC_FILES
        include/brlists.h
        include/brstdlib.h
        )

set(FW_FILES
        assocarr.c
        brbegin.c
        brbhook.c
        brlists.c
        brqsort.c
        bswap.c
        datafile.c
        defdiag.c
        deffile.c
        defmem.c
        devlist.c
        devquery.c
        devsetup.c
        diag.c
        error.c
        file.c
        fwsetup.c
        genfile.c
        image.c
        lexer.c
        loader.c
        mem.c
        object.c
        objectc.c
        pattern.c
        pool.c
        printf.c
        register.c
        resource.c
        resreg.c
        scratch.c
        scrstr.c
        sys_conf.c
        token.c
        tokenval.c

        fw.h
        fw_ip.h
        )

add_library(fw OBJECT ${FW_FILES} ${FW_DEV_FILES})

target_sources(fw INTERFACE
        FILE_SET include
        TYPE HEADERS
        BASE_DIRS include
        FILES ${INC_FILES}
        )

target_sources(fw INTERFACE
        FILE_SET ddi
        TYPE HEADERS
        BASE_DIRS ddi ${CMAKE_CURRENT_BINARY_DIR} ${SOFTREND_DDI_DIR}
        FILES ${DDI_FILES}
        )

target_include_directories(fw PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(fw PRIVATE __BR_V1DB__=0)

target_link_libraries(fw PRIVATE brender-inc-ddi)

add_dependencies(brender-inc INTERFACE fw-generated)
