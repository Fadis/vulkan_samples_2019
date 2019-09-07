add_library( vulkan_sample_common SHARED config.cpp vma.cpp draw_state.cpp mesh.cpp )
target_link_libraries( vulkan_sample_common ${Boost_PROGRAM_OPTIONS_LIBRARIES}
	${Boost_FILESYSTEM_LIBRARIES} ${Boost_SYSTEM_LIBRARIES}
	${GLFW_LIBRARIES} ${Vulkan_LIBRARIES}
	${ASSIMP_LIBRARIES} stdc++fs )

