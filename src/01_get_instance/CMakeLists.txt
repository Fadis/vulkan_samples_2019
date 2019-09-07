add_executable( get_instance get_instance.cpp )
target_link_libraries( get_instance vulkan_sample_common ${Vulkan_LIBRARIES} )
add_executable( get_instance_capi get_instance_capi.cpp )
target_link_libraries( get_instance_capi vulkan_sample_common ${Vulkan_LIBRARIES} )
