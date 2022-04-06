find_package(Doxygen QUIET)

if (DOXYGEN_FOUND)

    # WRENCH APIs documentation
    foreach (SECTION USER DEVELOPER INTERNAL)
        string(TOLOWER ${SECTION} SECTION_LOWER)
        set(DOXYGEN_OUT ${CMAKE_HOME_DIRECTORY}/docs/logs/Doxyfile_${SECTION})
        set(WRENCH_DOC_INPUT "${CMAKE_HOME_DIRECTORY}/src ${CMAKE_HOME_DIRECTORY}/include")

        if (${SECTION} STREQUAL "INTERNAL")
            set(WRENCH_SECTIONS "INTERNAL DEVELOPER USER")
        elseif (${SECTION} STREQUAL "DEVELOPER")
            set(WRENCH_SECTIONS "DEVELOPER USER")
        elseif (${SECTION} STREQUAL "USER")
            set(WRENCH_SECTIONS "USER")
        else ()
            set(WRENCH_SECTIONS "NODICE")
        endif ()

        set(WRENCH_SECTIONS_OUTPUT ${SECTION_LOWER})
        configure_file(${CMAKE_HOME_DIRECTORY}/conf/doxygen/Doxyfile.in ${DOXYGEN_OUT} @ONLY)

        add_custom_target(doc-${SECTION_LOWER}
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                COMMENT "Generating WRENCH ${SECTION} documentation" VERBATIM)
        add_custom_command(TARGET doc-${SECTION_LOWER}
                COMMAND mkdir -p ${CMAKE_HOME_DIRECTORY}/docs/${WRENCH_RELEASE_VERSION}/${SECTION_LOWER}
                COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT})

        LIST(APPEND WRENCH_SECTIONS_LIST doc-${SECTION_LOWER})
    endforeach ()

    # WRENCH documentation pages
    set(DOXYGEN_OUT ${CMAKE_HOME_DIRECTORY}/docs/logs/Doxyfile_pages)
    set(WRENCH_DOC_INPUT "${CMAKE_HOME_DIRECTORY}/doc ${CMAKE_HOME_DIRECTORY}/src ${CMAKE_HOME_DIRECTORY}/include")
    set(WRENCH_SECTIONS "INTERNAL DEVELOPER USER")
    set(WRENCH_SECTIONS_OUTPUT "pages")

    configure_file(${CMAKE_HOME_DIRECTORY}/conf/doxygen/Doxyfile.in ${DOXYGEN_OUT} @ONLY)

    # add_custom_target(doc-pages
    #         WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    #         COMMENT "Generating WRENCH documentation pages" VERBATIM)

    # add_custom_command(TARGET doc-pages
    #         COMMAND mkdir -p ${CMAKE_HOME_DIRECTORY}/docs/${WRENCH_RELEASE_VERSION}/pages
    #         COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT})
    # LIST(APPEND WRENCH_SECTIONS_LIST doc-pages)

    # add_custom_target(doc-deploy DEPENDS wrench ${WRENCH_SECTIONS_LIST})
    # foreach (DEP_NAME user developer internal)
    #     add_custom_command(TARGET doc-deploy
    #             COMMAND mkdir -p ${CMAKE_HOME_DIRECTORY}/docs/wrench-doc/${WRENCH_RELEASE_VERSION}/${DEP_NAME}
    #             COMMAND cp -R ${CMAKE_HOME_DIRECTORY}/docs/${WRENCH_RELEASE_VERSION}/${DEP_NAME}/html/* ${CMAKE_HOME_DIRECTORY}/docs/wrench-doc/${WRENCH_RELEASE_VERSION}/${DEP_NAME})
    # endforeach ()
    # add_custom_command(TARGET doc-deploy
    #         COMMAND cp -R ${CMAKE_HOME_DIRECTORY}/docs/${WRENCH_RELEASE_VERSION}/pages/html/* ${CMAKE_HOME_DIRECTORY}/docs/wrench-doc/${WRENCH_RELEASE_VERSION})
    # add_custom_command(TARGET doc-deploy
    #         COMMAND cp -R ${CMAKE_HOME_DIRECTORY}/docs/wrench-doc/${WRENCH_RELEASE_VERSION} ${CMAKE_HOME_DIRECTORY}/docs/wrench-doc/latest)

    get_directory_property(extra_clean_files ADDITIONAL_MAKE_CLEAN_FILES)
    set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${extra_clean_files};${CMAKE_HOME_DIRECTORY}/docs")

    # add_custom_target(doc DEPENDS doc-deploy)
    add_custom_target(doc DEPENDS wrench ${WRENCH_SECTIONS_LIST})

    add_custom_command(TARGET doc COMMAND python3 
                        ${CMAKE_HOME_DIRECTORY}/doc/scripts/generate_rst.py 
                        ${CMAKE_HOME_DIRECTORY}/docs/${WRENCH_RELEASE_VERSION}
                        ${CMAKE_HOME_DIRECTORY}/doc/source
                        ${WRENCH_RELEASE_VERSION})
    add_custom_command(TARGET doc COMMAND sphinx-build 
                        ${CMAKE_HOME_DIRECTORY}/doc/source 
                        ${CMAKE_HOME_DIRECTORY}/docs/build/${WRENCH_RELEASE_VERSION})

else (DOXYGEN_FOUND)
    message("-- Doxygen: No (warning: Doxygen is needed in case you want to generate WRENCH documentation)")
endif (DOXYGEN_FOUND)
