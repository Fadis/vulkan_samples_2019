#include <iostream>
#include <vulkan/vulkan.h>
#include <vw/config.h>
#include <vw/instance.h>

int main( int argc, const char *argv[] ) {
  const auto configs = vw::parse_configs( argc, argv );
  auto instance = vw::create_instance(
    configs,
    {},
    {}
  );
  uint32_t device_count = 0u;
  if( vkEnumeratePhysicalDevices( *instance, &device_count, nullptr ) != VK_SUCCESS )
    return 1;
  std::vector< VkPhysicalDevice > devices( device_count );
  if( vkEnumeratePhysicalDevices( *instance, &device_count, devices.data() ) != VK_SUCCESS )
    return 1;
  unsigned int index = 0;
  for( const auto &device: devices ) {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties( device, &props );
    std::cout << index++ << " : " << props.deviceName << "(";
    if( props.deviceType == VK_PHYSICAL_DEVICE_TYPE_OTHER )
      std::cout << "その他のデバイス";
    else if( props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU )
      std::cout << "統合GPU";
    else if( props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU )
      std::cout << "ディスクリートGPU";
    else if( props.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU )
      std::cout << "仮想GPU";
    else if( props.deviceType == VK_PHYSICAL_DEVICE_TYPE_CPU )
      std::cout << "CPU";
    else
      std::cout << "不明なデバイス" << std::endl;
    std::cout << ")" << std::endl;
    std::cout << "  APIバージョン" << std::endl;
    std::cout << "    " << VK_VERSION_MAJOR( props.apiVersion ) << "." << VK_VERSION_MINOR( props.apiVersion ) << "." << VK_VERSION_PATCH( props.apiVersion ) << std::endl;
    std::cout << "  ドライババージョン" << std::endl;
    std::cout << "    " << VK_VERSION_MAJOR( props.driverVersion ) << "." << VK_VERSION_MINOR( props.apiVersion ) << "." << VK_VERSION_PATCH( props.apiVersion ) << std::endl;
    std::cout << "  ベンダーID" << std::endl;
    std::cout << "    " << props.vendorID << std::endl;
    std::cout << "  デバイスID" << std::endl;
    std::cout << "    " << props.deviceID << std::endl;
    uint32_t ext_count = 0u;
    if( vkEnumerateDeviceExtensionProperties( device, nullptr, &ext_count, nullptr ) != VK_SUCCESS )
      return 1;
    std::vector< VkExtensionProperties > avail_dext( ext_count );
    if( vkEnumerateDeviceExtensionProperties( device, nullptr, &ext_count, avail_dext.data() ) != VK_SUCCESS )
      return 1;
    std::cout << "  利用可能な拡張" << std::endl;
    for( const auto &ext: avail_dext )
      std::cout << "    " << ext.extensionName << " 規格バージョン" << ext.specVersion << std::endl;
    if( avail_dext.empty() )
      std::cout << "    (なし)" << std::endl;
    uint32_t layer_count = 0u;
    if( vkEnumerateDeviceLayerProperties( device, &layer_count, nullptr ) != VK_SUCCESS )
      return 1;
    std::vector< VkLayerProperties > avail_dlayers( layer_count );
    if( vkEnumerateDeviceLayerProperties( device, &layer_count, avail_dlayers.data() ) != VK_SUCCESS )
      return 1;
    std::cout << "  利用可能なレイヤー" << std::endl;
    for( const auto &layer: avail_dlayers ) {
      std::cout << "    " << layer.layerName << " 規格バージョン" << layer.specVersion << " 実装バージョン" << layer.implementationVersion << std::endl;
      std::cout << "      " << layer.description << std::endl;
    }
    if( avail_dlayers.empty() )
      std::cout << "    (なし)" << std::endl;
    if( configs.direct ) {
      uint32_t display_count = 0u;
      if( vkGetPhysicalDeviceDisplayPropertiesKHR( device, &display_count, nullptr ) != VK_SUCCESS )
        return 1;
      std::vector< VkDisplayPropertiesKHR > displays( display_count );
      if( vkGetPhysicalDeviceDisplayPropertiesKHR( device, &display_count, displays.data() ) != VK_SUCCESS )
        return 1;
      std::cout << "  利用可能なディスプレイ" << std::endl;
      for( const auto &d: displays ) {
        std::cout << "    " << d.displayName << std::endl;
        std::cout << "      サイズ " << d.physicalDimensions.width << "x" << d.physicalDimensions.height << std::endl;
        std::cout << "      解像度 " << d.physicalResolution.width << "x" << d.physicalResolution.height << std::endl;
        if( d.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR )
          std::cout << "      そのままの向きで表示できる" << std::endl;
        if( d.supportedTransforms & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR )
          std::cout << "      90度回転できる" << std::endl;
        if( d.supportedTransforms & VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR )
          std::cout << "      180度回転できる" << std::endl;
        if( d.supportedTransforms & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR )
          std::cout << "      270度回転できる" << std::endl;
        if( d.supportedTransforms & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR )
          std::cout << "      左右反転できる" << std::endl;
        if( d.supportedTransforms & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR )
          std::cout << "      左右反転かつ90度回転できる" << std::endl;
        if( d.supportedTransforms & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR )
          std::cout << "      左右反転かつ180度回転できる" << std::endl;
        if( d.supportedTransforms & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR )
          std::cout << "      左右反転かつ270度回転できる" << std::endl;
        if( d.supportedTransforms & VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR )
          std::cout << "      システムが指定する向きで表示できる" << std::endl;
        if( d.planeReorderPossible )
          std::cout << "      planeの優先度を変更できる" << std::endl;
        if( d.persistentContent )
          std::cout << "      自力でrefreshできる" << std::endl;
        uint32_t mode_count = 0u;
        if( vkGetDisplayModePropertiesKHR( device, d.display, &mode_count, nullptr ) != VK_SUCCESS )
          return 1;
        std::vector< VkDisplayModePropertiesKHR > modes( mode_count );
        if( vkGetDisplayModePropertiesKHR( device, d.display, &mode_count, modes.data() ) != VK_SUCCESS )
          return 1;
        std::cout << "      表示モード" << std::endl;
        for( const auto &m: modes ) {
          std::cout << "        " << m.parameters.visibleRegion.width << "x" << m.parameters.visibleRegion.height << "@" << double( m.parameters.refreshRate )/1000.0 << "Hz" << std::endl; 
        }
        auto highest_rate = std::max_element( modes.begin(), modes.end(), []( const auto &l, const auto &r ) { return l.parameters.refreshRate < r.parameters.refreshRate; } );
        auto &mode = *highest_rate;
        uint32_t plane_index = 0u;
        uint32_t plane_count = 0u;
        if( vkGetPhysicalDeviceDisplayPlanePropertiesKHR( device, &plane_count, nullptr ) != VK_SUCCESS )
          return 1;
        std::vector< VkDisplayPlanePropertiesKHR > planes( plane_count );
        if( vkGetPhysicalDeviceDisplayPlanePropertiesKHR( device, &plane_count, planes.data() ) != VK_SUCCESS )
          return 1;
        for( uint32_t pi = 0u; pi != planes.size(); ++pi ) {
          uint32_t plane_count = 0u;
          if( vkGetDisplayPlaneSupportedDisplaysKHR( device, pi, &plane_count, nullptr ) != VK_SUCCESS )
            return 1;
          std::vector< VkDisplayKHR > planes( plane_count );
          if( vkGetDisplayPlaneSupportedDisplaysKHR( device, pi, &plane_count, planes.data() ) != VK_SUCCESS )
            return 1;
          if( std::find( planes.begin(), planes.end(), d.display ) != planes.end() )
            plane_index = pi;
        }
        VkSurfaceKHR surface;
        {
          VkDisplaySurfaceCreateInfoKHR create_info;
          create_info.sType = VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR;
          create_info.pNext = nullptr;
          create_info.flags = 0;
          create_info.displayMode = mode.displayMode;
          create_info.planeIndex = plane_index;
          create_info.planeStackIndex = 0;
          create_info.transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
          create_info.alphaMode = VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR;
          create_info.imageExtent = mode.parameters.visibleRegion;
          if( vkCreateDisplayPlaneSurfaceKHR( *instance, &create_info, nullptr, &surface ) != VK_SUCCESS )
            return 1;
        }
        uint32_t format_count = 0u;
        if( vkGetPhysicalDeviceSurfaceFormatsKHR( device, surface, &format_count, nullptr ) != VK_SUCCESS )
          return 1;
        std::vector< VkSurfaceFormatKHR > formats( format_count );
        if( vkGetPhysicalDeviceSurfaceFormatsKHR( device, surface, &format_count, formats.data() ) != VK_SUCCESS )
          return 1;
        std::cout << "      ピクセルフォーマット" << std::endl;
        std::cout << "        (略)" << std::endl;
      }
      uint32_t plane_count = 0u;
      if( vkGetPhysicalDeviceDisplayPlanePropertiesKHR( device, &plane_count, nullptr ) != VK_SUCCESS )
        return 1;
      std::vector< VkDisplayPlanePropertiesKHR > planes( plane_count );
      if( vkGetPhysicalDeviceDisplayPlanePropertiesKHR( device, &plane_count, planes.data() ) != VK_SUCCESS )
        return 1;
      std::cout << "  利用可能なPlane" << std::endl;
      for( const auto &p: planes ) {
        std::cout << "    Plane" << std::endl;
        if( !p.currentDisplay ) 
          std::cout << "      ディスプレイに割り当てられていない" << std::endl;
        std::cout << "      深度 " << p.currentStackIndex << std::endl;
      }
    }
  }
}

