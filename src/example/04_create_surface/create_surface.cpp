#include <iostream>
#include <vector>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include <vw/config.h>
#include <vw/instance.h>
#include <vw/list_device.h>
#include <vw/is_capable.h>
#include <vw/context.h>
#include <vw/exceptions.h>

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
  const auto devices = instance->enumeratePhysicalDevices();
  if( devices.size() <= configs.device_index )
    throw vw::unable_to_create_surface{};
  context.set_physical_device( devices[ configs.device_index ] );
  const auto avail_dext = context.physical_device.enumerateDeviceExtensionProperties();
  for( const char *w: dext ) {
    if( std::find_if( avail_dext.begin(), avail_dext.end(), [&]( const auto &v ) { return !strcmp( v.extensionName, w ); } ) == avail_dext.end() )
      throw vw::unable_to_create_surface{};
  }
  const auto avail_dlayers = context.physical_device.enumerateDeviceLayerProperties();
  for( const char *w: dlayers ) {
    if( std::find_if( avail_dlayers.begin(), avail_dlayers.end(), [&]( const auto &v ) { return !strcmp( v.layerName, w ); } ) == avail_dlayers.end() )
      throw vw::unable_to_create_surface{};
  }
  uint32_t width = configs.width;
  uint32_t height = configs.height;
  if( configs.direct ) {
    vk::DisplayPropertiesKHR display;
    const auto displays = context.physical_device.getDisplayPropertiesKHR();
    if( displays.size() <= configs.display_index )
      throw vw::unable_to_create_surface{};
    display = displays[ configs.display_index ];
    auto modes = context.physical_device.getDisplayModePropertiesKHR( display.display );
    if( width == 0 && height == 0 ) {
      width = display.physicalResolution.width;
      height = display.physicalResolution.height;
    }
    std::vector< vk::DisplayModePropertiesKHR > available_modes;
    std::copy_if( modes.begin(), modes.end(), std::back_inserter( available_modes ), [&]( const auto &m ) { return m.parameters.visibleRegion.width == width && m.parameters.visibleRegion.height == height; } );
    if( available_modes.empty() )
      throw vw::unable_to_create_surface{};
    auto highest_rate = std::max_element( available_modes.begin(), available_modes.end(), []( const auto &l, const auto &r ) { return l.parameters.refreshRate < r.parameters.refreshRate; } );
    auto &mode = *highest_rate;
    auto planes = context.physical_device.getDisplayPlanePropertiesKHR();
    uint32_t plane_index = 0u;
    for( uint32_t pi = 0u; pi != planes.size(); ++pi ) {
      auto supported_displays = context.physical_device.getDisplayPlaneSupportedDisplaysKHR( pi );
      if( std::find( supported_displays.begin(), supported_displays.end(), display.display ) != supported_displays.end() )
        plane_index = pi;
    }
    context.set_surface( instance->createDisplayPlaneSurfaceKHRUnique(
      vk::DisplaySurfaceCreateInfoKHR()
        .setDisplayMode( mode.displayMode )
        .setPlaneIndex( plane_index )
        .setPlaneStackIndex( 0 )
        .setImageExtent( mode.parameters.visibleRegion )
    ) );
    const auto props = context.physical_device.getProperties();
    std::cout << "デバイス " << props.deviceName << " に接続されているディスプレイ " << display.displayName << " に " << mode.parameters.visibleRegion.width << "x" << mode.parameters.visibleRegion.height << "@" << double( mode.parameters.refreshRate )/1000.0 << "Hz のサーフェスが作成されました" << std::endl;
  }
  else {
    glfwSetErrorCallback( []( int, const char* description ) { std::cout << description << std::endl; } );
    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
    if( configs.width == 0 && configs.height == 0 ) {
      width = 640;
      height = 480;
    }
    else {
      width = configs.width;
      height = configs.height;
    }
    auto raw_window = glfwCreateWindow( width, height, configs.prog_name.c_str(), configs.fullscreen ? glfwGetPrimaryMonitor() : nullptr, nullptr );
    if( !raw_window ) {
      throw vw::unable_to_create_surface{};
    }
    int width_;
    int height_;
    glfwGetWindowSize( raw_window, &width_, &height_ );
    width = width_;
    height = height_;
    glfwSetWindowUserPointer( raw_window, reinterpret_cast< void* >( context.input_state.get() ) );
    glfwSetKeyCallback( raw_window, vw::on_key_event );
    context.set_window(
      vw::window_info_t()
        .emplace_window( raw_window, vw::glfw_window_deleter() )
    );
    VkSurfaceKHR raw_surface;
    VkResult err = glfwCreateWindowSurface( *instance, raw_window, nullptr, &raw_surface );
    if( err ) {
      vk::throwResultException( vk::Result( err ), "サーフェスを作成できない" );
      throw vw::unable_to_create_surface{};
    }
    vk::ObjectDestroy< vk::Instance, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE > deleter( *instance, nullptr, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE () );
    context.set_surface( vk::UniqueHandle< vk::SurfaceKHR, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE >( raw_surface, deleter ) );
  }
}

