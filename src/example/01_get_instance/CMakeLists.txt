add_executable( get_instance get_instance.cpp )
target_link_libraries( get_instance vw ${Vulkan_LIBRARIES} )
add_executable( get_instance_capi get_instance_capi.cpp )
target_link_libraries( get_instance_capi vw ${Vulkan_LIBRARIES} )
