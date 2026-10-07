ch_add_test(chCoreTest SOURCE chCoreTestMain.cpp)

# chCore brings the CH_*_ENABLED defines its headers depend on.
target_link_libraries(chCoreTest PRIVATE chCore)

# catch.hpp is shared with chUtilitiesTest; SYSTEM keeps our warning level out of it.
target_include_directories(chCoreTest SYSTEM PRIVATE ${CMAKE_SOURCE_DIR}/chUtilitiesTest)
