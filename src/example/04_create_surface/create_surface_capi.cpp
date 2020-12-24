#include <cstring>
#include <iostream>
#include <vector>
#include <algorithm>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
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
  uint32_t device_count = 0u;
  if( vkEnumeratePhysicalDevices( *instance, &device_count, nullptr ) != VK_SUCCESS ) 
    return 1;
  std::vector< VkPhysicalDevice > devices( device_count );
  if( vkEnumeratePhysicalDevices( *instance, &device_count, devices.data() ) != VK_SUCCESS )
    return 1;
  if( devices.size() <= configs.device_index )
    throw vw::unable_to_create_surface{};
  context.set_physical_device( devices[ configs.device_index ] );
  uint32_t ext_count = 0u;
  if( vkEnumerateDeviceExtensionProperties( context.physical_device, nullptr, &ext_count, nullptr ) != VK_SUCCESS )
    return 1;
  std::vector< VkExtensionProperties > avail_dext( ext_count );
  if( vkEnumerateDeviceExtensionProperties( context.physical_device, nullptr, &ext_count, avail_dext.data() ) != VK_SUCCESS )
    return 1;
  for( const char *w: dext ) {
    if( std::find_if( avail_dext.begin(), avail_dext.end(), [&]( const auto &v ) { return !strcmp( v.extensionName, w ); } ) == avail_dext.end() )
      throw vw::unable_to_create_surface{};
  }
  uint32_t layer_count = 0u;
  if( vkEnumerateDeviceLayerProperties( context.physical_device, &layer_count, nullptr ) != VK_SUCCESS )
    return true;
  std::vector< VkLayerProperties > avail_dlayers( layer_count );
  if( vkEnumerateDeviceLayerProperties( context.physical_device, &layer_count, avail_dlayers.data() ) != VK_SUCCESS )
    return true;
  for( const char *w: dlayers ) {
    if( std::find_if( avail_dlayers.begin(), avail_dlayers.end(), [&]( const auto &v ) { return !strcmp( v.layerName, w ); } ) == avail_dlayers.end() )
      throw vw::unable_to_create_surface{};
  }
  uint32_t width = configs.width;
  uint32_t height = configs.height;
  VkSurfaceKHR surface;
  if( configs.direct ) {
    VkDisplayPropertiesKHR display;
    uint32_t display_count = 0u;
    if( vkGetPhysicalDeviceDisplayPropertiesKHR( context.physical_device, &display_count, nullptr ) != VK_SUCCESS )
      return 1;
    std::vector< VkDisplayPropertiesKHR > displays( display_count );
    if( vkGetPhysicalDeviceDisplayPropertiesKHR( context.physical_device, &display_count, displays.data() ) != VK_SUCCESS )
      return 1;
    if( displays.size() <= configs.display_index )
      throw vw::unable_to_create_surface{};
    display = displays[ configs.display_index ];
    uint32_t mode_count = 0u;
    if( vkGetDisplayModePropertiesKHR( context.physical_device, display.display, &mode_count, nullptr ) != VK_SUCCESS )
      return 1;
    std::vector< VkDisplayModePropertiesKHR > modes( mode_count );
    if( vkGetDisplayModePropertiesKHR( context.physical_device, display.display, &mode_count, modes.data() ) != VK_SUCCESS )
      return 1;
    if( width == 0 && height == 0 ) {
      width = display.physicalResolution.width;
      height = display.physicalResolution.height;
    }
    std::vector< VkDisplayModePropertiesKHR > available_modes;
    std::copy_if( modes.begin(), modes.end(), std::back_inserter( available_modes ), [&]( const auto &m ) { return m.parameters.visibleRegion.width == width && m.parameters.visibleRegion.height == height; } );
    if( available_modes.empty() )
      throw vw::unable_to_create_surface{};
    auto highest_rate = std::max_element( available_modes.begin(), available_modes.end(), []( const auto &l, const auto &r ) { return l.parameters.refreshRate < r.parameters.refreshRate; } );
    auto &mode = *highest_rate;
    uint32_t plane_count = 0u;
    if( vkGetPhysicalDeviceDisplayPlanePropertiesKHR( context.physical_device, &plane_count, nullptr ) != VK_SUCCESS )
      return 1;
    std::vector< VkDisplayPlanePropertiesKHR > planes( plane_count );
    if( vkGetPhysicalDeviceDisplayPlanePropertiesKHR( context.physical_device, &plane_count, planes.data() ) != VK_SUCCESS )
      return 1;
    uint32_t plane_index = 0u;
    for( uint32_t pi = 0u; pi != planes.size(); ++pi ) {
      uint32_t supported_display_count = 0u;
      if( vkGetDisplayPlaneSupportedDisplaysKHR( context.physical_device, pi, &supported_display_count, nullptr ) != VK_SUCCESS )
        return 1;
      std::vector< VkDisplayKHR > supported_displays( supported_display_count );
      if( vkGetDisplayPlaneSupportedDisplaysKHR( context.physical_device, pi, &supported_display_count, supported_displays.data() ) != VK_SUCCESS )
        return 1;
      if( std::find( supported_displays.begin(), supported_displays.end(), display.display ) != supported_displays.end() )
        plane_index = pi;
    }
    VkDisplaySurfaceCreateInfoKHR surface_create_info;
    surface_create_info.sType = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR;
    surface_create_info.pNext = nullptr;
    surface_create_info.flags = 0;
    surface_create_info.displayMode = mode.displayMode;
    surface_create_info.planeIndex = plane_index;
    surface_create_info.planeStackIndex = 0;
    surface_create_info.transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    surface_create_info.globalAlpha = 0;
    surface_create_info.alphaMode = VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR;
    surface_create_info.imageExtent = mode.parameters.visibleRegion;
    if( vkCreateDisplayPlaneSurfaceKHR( *instance, &surface_create_info, nullptr, &surface ) != VK_SUCCESS )
      return 1;
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties( context.physical_device, &props );
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
    if( glfwCreateWindowSurface( *instance, raw_window, nullptr, &surface ) != VK_SUCCESS )
      return 1;
  }
  vkDestroySurfaceKHR( *instance, surface, nullptr );
}

