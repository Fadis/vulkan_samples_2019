add_executable( list_devices list_devices.cpp )
target_link_libraries( list_devices vulkan_sample_common ${Vulkan_LIBRARIES} )
add_executable( list_devices_capi list_devices_capi.cpp )
target_link_libraries( list_devices_capi vulkan_sample_common ${Vulkan_LIBRARIES} )
