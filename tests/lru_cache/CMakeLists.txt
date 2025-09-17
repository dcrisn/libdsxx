get_cwd_name(cwd_dirname)
SET(output_dir "${CMAKE_BINARY_DIR}/tests/${cwd_dirname}")

add_executable(lru_cache
	main.cpp
)

SET(deps tarp::dsxx)

CONFIGURE_TARGET(lru_cache ${output_dir} "${deps}")

