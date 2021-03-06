#include <iostream>
#include <vector>
#include <filesystem>
#include <fstream>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <vw/config.h>
#include <vw/instance.h>
#include <vw/list_device.h>
#include <vw/is_capable.h>
#include <vw/context.h>
#include <vw/exceptions.h>
#include <vw/shader.h>

int main( int argc, const char *argv[] ) {
  const auto configs = vw::parse_configs( argc, argv );
  auto instance = vw::create_instance(
    configs,
    {},
    {}
  );
  if( configs.list ) {
    vw::list_devices( *instance, configs );
    return 0;
  }
  std::vector< const char* > dext{
    VK_KHR_IMAGE_FORMAT_LIST_EXTENSION_NAME,
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_SWAPCHAIN_MUTABLE_FORMAT_EXTENSION_NAME
  };
  std::vector< const char* > dlayers{};
  if( !vw::is_capable(
    *instance,
    configs,
    dext,
    dlayers
  ) ) {
    std::cout << "指定された条件に合うデバイスは無かった" << std::endl;
    return 0;
  }
  vw::context_t context;
  create_surface( context, *instance, configs, dext, dlayers );
  create_device( context, dext, dlayers );

  const auto filename = std::filesystem::path( configs.shader ) / std::filesystem::path( "world.vert.spv" );
  std::fstream file( filename.c_str(), std::ios::in|std::ios::binary );
  if( !file.good() ) throw vw::unable_to_load_shader();
  const std::vector< char > bin( ( std::istreambuf_iterator< char >( file ) ), std::istreambuf_iterator<char>() );
  VkShaderModuleCreateInfo shader_module_create_info;
  shader_module_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  shader_module_create_info.pNext = nullptr;
  shader_module_create_info.flags = 0;
  shader_module_create_info.codeSize = bin.size();
  shader_module_create_info.pCode = reinterpret_cast< const uint32_t* >( bin.data() );
  VkShaderModule shader_module;
  if( vkCreateShaderModule( *context.device, &shader_module_create_info, nullptr, &shader_module ) != VK_SUCCESS )
    return 1;
  vkDestroyShaderModule( *context.device, shader_module, nullptr );
}

