add_executable( mesh mesh.cpp )
target_link_libraries( mesh vulkan_sample_common ${Vulkan_LIBRARIES} )
