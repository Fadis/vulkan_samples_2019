add_executable( create_surface create_surface.cpp )
target_link_libraries( create_surface vw ${Vulkan_LIBRARIES}
	${GLFW_LIBRARIES} )
add_executable( create_surface_capi create_surface_capi.cpp )
target_link_libraries( create_surface_capi vw
	${Vulkan_LIBRARIES} ${GLFW_LIBRARIES} )
