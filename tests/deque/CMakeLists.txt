get_cwd_name(cwd_dirname)
SET(output_dir "${CMAKE_BINARY_DIR}/tests/${cwd_dirname}")

add_executable(deque
	main.cpp
)

SET(deps tarp::dsxx)

CONFIGURE_TARGET(deque ${output_dir} "${deps}")

add_executable(benchmark benchmark.cpp)
CONFIGURE_TARGET(benchmark ${output_dir} "${deps}")
