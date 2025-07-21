get_cwd_name(cwd_dirname)
SET(output_dir "${CMAKE_BINARY_DIR}/tests/${cwd_dirname}")

SET(deps
    libdsxx
)

add_executable( dllist
	dllist.cpp
)

CONFIGURE_TARGET(dllist ${output_dir} "${deps}")

