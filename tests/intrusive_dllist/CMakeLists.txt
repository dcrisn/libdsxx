get_cwd_name(cwd_dirname)
SET(output_dir "${CMAKE_BINARY_DIR}/tests/${cwd_dirname}")

add_executable(dllist
	dllist.cpp
)

SET(deps tarp::dsxx)

CONFIGURE_TARGET(dllist ${output_dir} "${deps}")

