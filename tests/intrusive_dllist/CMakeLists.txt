get_cwd_name(cwd_dirname)
SET(output_dir "${CMAKE_BINARY_DIR}/tests/${cwd_dirname}")

add_executable(dllist
	dllist.cpp
)

add_executable(dllist_perf
	dllist_perf.cpp
)

SET(deps tarp::dsxx)

CONFIGURE_TARGET(dllist ${output_dir} "${deps}")
CONFIGURE_TARGET(dllist_perf ${output_dir} "${deps}")

