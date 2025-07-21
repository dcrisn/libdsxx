add_custom_target(
    tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running tests ..."
)

# configure the executable test target that has name tgname.
MACRO(CONFIGURE_TARGET tgname output_dir lib_deps)
    # naming the target with a test. prefix to take advantage of tab
    # autocompletion when looking for test targets to run.
    SET(test_runtg test.${tgname})

    target_include_directories(${tgname} PRIVATE
            ${PROJECT_SOURCE_DIR}/src/
    )

    target_link_libraries(${tgname}
        doctest::doctest
        ${lib_deps}
    )

    set_target_properties(${tgname} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${output_dir}"
    )

    # add a target that runs the test binary when invoked.
    # The dependency on the test binary ensures the binary
    # is always built first.
    add_custom_target(${test_runtg}
        DEPENDS ${tgname}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running ${output_dir}/${tgname} ..."
        COMMAND ${tgname}
    )

    add_dependencies(tests ${test_runtg})
ENDMACRO()

MACRO(get_cwd_name var)
    get_filename_component(${var} ${CMAKE_CURRENT_SOURCE_DIR} NAME)
ENDMACRO()


# the source root directory i.e. ${PROJECT_SOURCE_DIR}/src/
SET(S ${PROJECT_SOURCE_DIR}/src)

add_subdirectory(intrusive)
add_subdirectory(intrusive_dllist)

