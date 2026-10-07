ch_add_test(chUtilitiesTest SOURCE chUtilitiesMain.cpp)

target_link_libraries(chUtilitiesTest PRIVATE chUtilities)

# catch.hpp lives next to the tests; SYSTEM keeps our warning level out of it.
target_include_directories(chUtilitiesTest SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
