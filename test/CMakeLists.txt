find_package (Boost COMPONENTS unit_test_framework REQUIRED)
include_directories(
    ${PROJECT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
    )
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=address -Wall -Wextra -Werror -g -std=c++14 -Ofast")
set (CMAKE_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} -fno-omit-frame-pointer -fsanitize=address -g -std=c++14 -Ofast")


#file(GLOB TEST_SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/*.cpp)
AUX_SOURCE_DIRECTORY(. TEST_SRCS)
#Run through each source
foreach(testSrc ${TEST_SRCS})
	message("test=${testSrc}")
        #Extract the filename without an extension (NAME_WE)
        get_filename_component(testName ${testSrc} NAME_WE)

        #Add compile target
        add_executable(${testName} ${testSrc})

        #link to Boost libraries AND your targets and dependencies
        target_link_libraries(${testName} ${Boost_LIBRARIES})

        #I like to move testing binaries into a testBin directory
        set_target_properties(${testName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_CURRENT_SOURCE_DIR})
        #Finally add it to test execution - 
        #Notice the WORKING_DIRECTORY and COMMAND
        add_test(NAME ${testName} 
                 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} 
                 COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/${testName} )
endforeach(testSrc)