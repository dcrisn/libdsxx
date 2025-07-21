get_cwd_name(cwd_dirname)
SET(output_dir "${CMAKE_BINARY_DIR}/tests/${cwd_dirname}")

SET(deps
)

add_executable( intrusive
	intrusive.cpp
)

CONFIGURE_TARGET(intrusive ${output_dir} "${deps}")

