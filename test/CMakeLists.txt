find_package( Boost REQUIRED unit_test_framework date_time )
include_directories(${Boost_INCLUDE_DIRS})
link_directories( ${Boost_LIBRARY_DIRS} )

include_directories(${sevent_SOURCE_DIR})

function(add_boost_test testname)
	add_executable(${testname} ${ARGV1})
	target_link_libraries(${testname} ${Boost_LIBRARIES})
    add_test(${testname} ${testname})
endfunction(add_boost_test)

#add_boost_test(TestBoostSerialize TestBoostSerialize.cc)
add_boost_test(TestEvent TestEvent.cc)

add_boost_test(TestExample TestExample.cc)
target_link_libraries(TestExample sevent)

add_subdirectory(socket)
add_subdirectory(serialize)
