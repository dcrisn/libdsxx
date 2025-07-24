include(CheckIncludeFileCXX)
find_package( Boost QUIET)
check_include_file_cxx("boost/intrusive/list.hpp" HAS_BOOST_INTRUSIVE_LIST)

get_cwd_name(cwd_dirname)
SET(output_dir "${CMAKE_BINARY_DIR}/tests/${cwd_dirname}")

add_executable(dllist
	dllist.cpp
)

add_executable(dllist_perf
	dllist_perf.cpp
)

if(HAS_BOOST_INTRUSIVE_LIST)
    target_compile_definitions(dllist_perf PRIVATE
        -DHAS_BOOST_INTRUSIVE
    )
endif()

SET(deps tarp::dsxx)

CONFIGURE_TARGET(dllist ${output_dir} "${deps}")
CONFIGURE_TARGET(dllist_perf ${output_dir} "${deps}")

