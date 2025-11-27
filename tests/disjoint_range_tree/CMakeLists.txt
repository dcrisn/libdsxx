get_cwd_name(cwd_dirname)
SET(output_dir "${CMAKE_BINARY_DIR}/tests/${cwd_dirname}")

add_executable(disjoint_range_tree
	main.cpp
)

SET(deps tarp::dsxx)

CONFIGURE_TARGET(disjoint_range_tree ${output_dir} "${deps}")

