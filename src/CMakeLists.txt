set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_package(OpenMP)

add_executable(nbody main.cpp bh_sim_utils.c)

target_link_libraries(
    nbody
    PRIVATE
    SDL3::SDL3
    imgui
    OpenMP::OpenMP_CXX
)

add_custom_command(TARGET nbody POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:SDL3::SDL3>
        $<TARGET_FILE_DIR:nbody>
)