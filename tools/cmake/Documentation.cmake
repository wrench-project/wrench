find_package(Doxygen)

if (DOXYGEN_FOUND)

    foreach (SECTION USER DEVELOPER INTERNAL)
        string(TOLOWER ${SECTION} SECTION_LOWER)
        set(DOXYGEN_OUT ${CMAKE_HOME_DIRECTORY}/docs/logs/Doxyfile_${SECTION})

        if (${SECTION} STREQUAL "INTERNAL")
            set(WRENCH_SECTIONS "INTERNAL DEVELOPER USER")
        elseif (${SECTION} STREQUAL "DEVELOPER")
            set(WRENCH_SECTIONS "DEVELOPER USER")
        elseif (${SECTION} STREQUAL "USER")
            set(WRENCH_SECTIONS "USER")
        else ()
            set(WRENCH_SECTIONS "NODICE")
        endif ()

#        set(WRENCH_SECTIONS "DEVELOPER USER")

        set(WRENCH_SECTIONS_OUTPUT ${SECTION_LOWER})
        configure_file(${CMAKE_HOME_DIRECTORY}/tools/doxygen/Doxyfile.in ${DOXYGEN_OUT} @ONLY)

        add_custom_target(doc-${SECTION_LOWER}
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
                COMMENT "Generating WRENCH ${SECTION} documentation" VERBATIM)
        add_custom_command(TARGET doc-${SECTION_LOWER}
                COMMAND mkdir -p ${CMAKE_HOME_DIRECTORY}/docs/${WRENCH_RELEASE_VERSION}/${SECTION_LOWER}
                COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT})

        LIST(APPEND WRENCH_SECTIONS_LIST doc-${SECTION_LOWER})
    endforeach ()

    get_directory_property(extra_clean_files ADDITIONAL_MAKE_CLEAN_FILES)
    set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES "${extra_clean_files};${CMAKE_HOME_DIRECTORY}/docs")

    add_custom_target(doc DEPENDS wrench ${WRENCH_SECTIONS_LIST})

    # generate docs for github pages
    add_custom_target(doc-gh DEPENDS doc)
    foreach (DEP_NAME user developer internal)
        add_custom_command(TARGET doc-gh
                COMMAND mkdir -p ${CMAKE_HOME_DIRECTORY}/docs/gh-pages/${WRENCH_RELEASE_VERSION}/${DEP_NAME}
                COMMAND cp -R ${CMAKE_HOME_DIRECTORY}/docs/${WRENCH_RELEASE_VERSION}/${DEP_NAME}/html/* ${CMAKE_HOME_DIRECTORY}/docs/gh-pages/${WRENCH_RELEASE_VERSION}/${DEP_NAME})
    endforeach ()

else (DOXYGEN_FOUND)
    message("Doxygen need to be installed to generate WRENCH documentation")
endif (DOXYGEN_FOUND)
