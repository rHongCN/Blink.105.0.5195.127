// Copyright (c) 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gl/gl_display.h"

#include <string>
#include <vector>

#include "base/command_line.h"
#include "base/containers/contains.h"
#include "base/debug/crash_logging.h"
#include "base/logging.h"
#include "base/metrics/histogram_macros.h"
#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/system/sys_info.h"
#include "build/build_config.h"
#include "ui/gl/angle_platform_impl.h"
#include "ui/gl/egl_util.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_context_egl.h"
#include "ui/gl/gl_display_egl_util.h"
#include "ui/gl/gl_implementation.h"
#include "ui/gl/gl_surface.h"

#if defined(USE_GLX)
#include "ui/gl/glx_util.h"
#endif  // defined(USE_GLX)

#if defined(USE_OZONE)
#include "ui/ozone/buildflags.h"
#endif  // defined(USE_OZONE)

#if BUILDFLAG(IS_ANDROID)
#include <android/native_window_jni.h>
#include "base/android/build_info.h"
#endif

// From ANGLE's egl/eglext.h.

#ifndef EGL_ANGLE_platform_angle
#define EGL_ANGLE_platform_angle 1
#define EGL_PLATFORM_ANGLE_ANGLE 0x3202
#define EGL_PLATFORM_ANGLE_TYPE_ANGLE 0x3203
#define EGL_PLATFORM_ANGLE_MAX_VERSION_MAJOR_ANGLE 0x3204
#define EGL_PLATFORM_ANGLE_MAX_VERSION_MINOR_ANGLE 0x3205
#define EGL_PLATFORM_ANGLE_TYPE_DEFAULT_ANGLE 0x3206
#define EGL_PLATFORM_ANGLE_DEBUG_LAYERS_ENABLED_ANGLE 0x3451
#define EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE 0x3209
#define EGL_PLATFORM_ANGLE_DEVICE_TYPE_EGL_ANGLE 0x348E
#define EGL_PLATFORM_ANGLE_DEVICE_TYPE_HARDWARE_ANGLE 0x320A
#define EGL_PLATFORM_ANGLE_DEVICE_TYPE_NULL_ANGLE 0x345E
#define EGL_PLATFORM_ANGLE_DEVICE_TYPE_SWIFTSHADER_ANGLE 0x3487
#define EGL_PLATFORM_ANGLE_NATIVE_PLATFORM_TYPE_ANGLE 0x348F
#endif /* EGL_ANGLE_platform_angle */

#ifndef EGL_ANGLE_platform_angle_d3d
#define EGL_ANGLE_platform_angle_d3d 1
#define EGL_PLATFORM_ANGLE_TYPE_D3D9_ANGLE 0x3207
#define EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE 0x3208
#define EGL_PLATFORM_ANGLE_DEVICE_TYPE_D3D_WARP_ANGLE 0x320B
#define EGL_PLATFORM_ANGLE_DEVICE_TYPE_D3D_REFERENCE_ANGLE 0x320C
#endif /* EGL_ANGLE_platform_angle_d3d */

#ifndef EGL_ANGLE_platform_angle_d3d_luid
#define EGL_ANGLE_platform_angle_d3d_luid 1
#define EGL_PLATFORM_ANGLE_D3D_LUID_HIGH_ANGLE 0x34A0
#define EGL_PLATFORM_ANGLE_D3D_LUID_LOW_ANGLE 0x34A1
#endif /* EGL_ANGLE_platform_angle_d3d_luid */

#ifndef EGL_ANGLE_platform_angle_d3d11on12
#define EGL_ANGLE_platform_angle_d3d11on12 1
#define EGL_PLATFORM_ANGLE_D3D11ON12_ANGLE 0x3488
#endif /* EGL_ANGLE_platform_angle_d3d11on12 */

#ifndef EGL_ANGLE_platform_angle_opengl
#define EGL_ANGLE_platform_angle_opengl 1
#define EGL_PLATFORM_ANGLE_TYPE_OPENGL_ANGLE 0x320D
#define EGL_PLATFORM_ANGLE_TYPE_OPENGLES_ANGLE 0x320E
#endif /* EGL_ANGLE_platform_angle_opengl */

#ifndef EGL_ANGLE_platform_angle_null
#define EGL_ANGLE_platform_angle_null 1
#define EGL_PLATFORM_ANGLE_TYPE_NULL_ANGLE 0x33AE
#endif /* EGL_ANGLE_platform_angle_null */

#ifndef EGL_ANGLE_platform_angle_vulkan
#define EGL_ANGLE_platform_angle_vulkan 1
#define EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE 0x3450
#define EGL_PLATFORM_VULKAN_DISPLAY_MODE_HEADLESS_ANGLE 0x34A5
#endif /* EGL_ANGLE_platform_angle_vulkan */

#ifndef EGL_ANGLE_platform_angle_metal
#define EGL_ANGLE_platform_angle_metal 1
#define EGL_PLATFORM_ANGLE_TYPE_METAL_ANGLE 0x3489
#endif /* EGL_ANGLE_platform_angle_metal */

#ifndef EGL_ANGLE_x11_visual
#define EGL_ANGLE_x11_visual 1
#define EGL_X11_VISUAL_ID_ANGLE 0x33A3
#endif /* EGL_ANGLE_x11_visual */

#ifndef EGL_ANGLE_direct_composition
#define EGL_ANGLE_direct_composition 1
#define EGL_DIRECT_COMPOSITION_ANGLE 0x33A5
#endif /* EGL_ANGLE_direct_composition */

#ifndef EGL_ANGLE_display_robust_resource_initialization
#define EGL_ANGLE_display_robust_resource_initialization 1
#define EGL_DISPLAY_ROBUST_RESOURCE_INITIALIZATION_ANGLE 0x3453
#endif /* EGL_ANGLE_display_robust_resource_initialization */

#ifndef EGL_ANGLE_display_power_preference
#define EGL_ANGLE_display_power_preference 1
#define EGL_POWER_PREFERENCE_ANGLE 0x3482
#define EGL_LOW_POWER_ANGLE 0x0001
#define EGL_HIGH_POWER_ANGLE 0x0002
#endif /* EGL_ANGLE_power_preference */

#ifndef EGL_ANGLE_platform_angle_device_id
#define EGL_ANGLE_platform_angle_device_id
#define EGL_PLATFORM_ANGLE_DEVICE_ID_HIGH_ANGLE 0x34D6
#define EGL_PLATFORM_ANGLE_DEVICE_ID_LOW_ANGLE 0x34D7
#endif /* EGL_ANGLE_platform_angle_device_id */

// From ANGLE's egl/eglext.h.
#ifndef EGL_ANGLE_feature_control
#define EGL_ANGLE_feature_control 1
#define EGL_FEATURE_NAME_ANGLE 0x3460
#define EGL_FEATURE_CATEGORY_ANGLE 0x3461
#define EGL_FEATURE_DESCRIPTION_ANGLE 0x3462
#define EGL_FEATURE_BUG_ANGLE 0x3463
#define EGL_FEATURE_STATUS_ANGLE 0x3464
#define EGL_FEATURE_COUNT_ANGLE 0x3465
#define EGL_FEATURE_OVERRIDES_ENABLED_ANGLE 0x3466
#define EGL_FEATURE_OVERRIDES_DISABLED_ANGLE 0x3467
#define EGL_FEATURE_ALL_DISABLED_ANGLE 0x3469
#endif /* EGL_ANGLE_feature_control */

using ui::GetLastEGLErrorString;

namespace gl {

namespace {

std::vector<const char*> GetAttribArrayFromStringVector(
    const std::vector<std::string>& strings) {
  std::vector<const char*> attribs;
  for (const std::string& item : strings) {
    attribs.push_back(item.c_str());
  }
  attribs.push_back(nullptr);
  return attribs;
}

std::vector<std::string> GetStringVectorFromCommandLine(
    const base::CommandLine* command_line,
    const char switch_name[]) {
  std::string command_string = command_line->GetSwitchValueASCII(switch_name);
  return base::SplitString(command_string, ", ;", base::TRIM_WHITESPACE,
                           base::SPLIT_WANT_NONEMPTY);
}

EGLDisplay GetPlatformANGLEDisplay(
    EGLNativeDisplayType display,
    EGLenum platform_type,
    const std::vector<std::string>& enabled_features,
    const std::vector<std::string>& disabled_features,
    const std::vector<EGLAttrib>& extra_display_attribs) {
  std::vector<EGLAttrib> display_attribs(extra_display_attribs);

  display_attribs.push_back(EGL_PLATFORM_ANGLE_TYPE_ANGLE);
  display_attribs.push_back(static_cast<EGLAttrib>(platform_type));

  if (platform_type == EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE) {
    base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
    if (command_line->HasSwitch(switches::kUseAdapterLuid)) {
      // If the LUID is specified, the format is <high part>,<low part>. Split
      // and add them to the EGL_ANGLE_platform_angle_d3d_luid ext attributes.
      std::string luid =
          command_line->GetSwitchValueASCII(switches::kUseAdapterLuid);
      size_t comma = luid.find(',');
      if (comma != std::string::npos) {
        int32_t high;
        uint32_t low;
        if (!base::StringToInt(luid.substr(0, comma), &high) ||
            !base::StringToUint(luid.substr(comma + 1), &low))
          return EGL_NO_DISPLAY;

        display_attribs.push_back(EGL_PLATFORM_ANGLE_D3D_LUID_HIGH_ANGLE);
        display_attribs.push_back(high);

        display_attribs.push_back(EGL_PLATFORM_ANGLE_D3D_LUID_LOW_ANGLE);
        display_attribs.push_back(low);
      }
    }
  }

  GLDisplayEglUtil::GetInstance()->GetPlatformExtraDisplayAttribs(
      platform_type, &display_attribs);

  std::vector<const char*> enabled_features_attribs =
      GetAttribArrayFromStringVector(enabled_features);
  std::vector<const char*> disabled_features_attribs =
      GetAttribArrayFromStringVector(disabled_features);
  if (g_driver_egl.client_ext.b_EGL_ANGLE_feature_control) {
    if (!enabled_features_attribs.empty()) {
      display_attribs.push_back(EGL_FEATURE_OVERRIDES_ENABLED_ANGLE);
      display_attribs.push_back(
          reinterpret_cast<EGLAttrib>(enabled_features_attribs.data()));
    }
    if (!disabled_features_attribs.empty()) {
      display_attribs.push_back(EGL_FEATURE_OVERRIDES_DISABLED_ANGLE);
      display_attribs.push_back(
          reinterpret_cast<EGLAttrib>(disabled_features_attribs.data()));
    }
  }
  // TODO(dbehr) Add an attrib to Angle to pass EGL platform.

  if (g_driver_egl.client_ext.b_EGL_ANGLE_display_power_preference) {
    GpuPreference pref =
        GLSurface::AdjustGpuPreference(GpuPreference::kDefault);
    switch (pref) {
      case GpuPreference::kDefault:
        // Don't request any GPU, let ANGLE and the native driver decide.
        break;
      case GpuPreference::kLowPower:
        display_attribs.push_back(EGL_POWER_PREFERENCE_ANGLE);
        display_attribs.push_back(EGL_LOW_POWER_ANGLE);
        break;
      case GpuPreference::kHighPerformance:
        display_attribs.push_back(EGL_POWER_PREFERENCE_ANGLE);
        display_attribs.push_back(EGL_HIGH_POWER_ANGLE);
        break;
      default:
        NOTREACHED();
    }
  }

  display_attribs.push_back(EGL_NONE);

  // This is an EGL 1.5 function that we know ANGLE supports. It's used to pass
  // EGLAttribs (pointers) instead of EGLints into the display
  return eglGetPlatformDisplay(EGL_PLATFORM_ANGLE_ANGLE,
                               reinterpret_cast<void*>(display),
                               &display_attribs[0]);
}

EGLDisplay GetDisplayFromType(
    DisplayType display_type,
    EGLDisplayPlatform native_display,
    const std::vector<std::string>& enabled_angle_features,
    const std::vector<std::string>& disabled_angle_features,
    bool disable_all_angle_features,
    uint64_t system_device_id) {
  std::vector<EGLAttrib> extra_display_attribs;
  if (disable_all_angle_features) {
    extra_display_attribs.push_back(EGL_FEATURE_ALL_DISABLED_ANGLE);
    extra_display_attribs.push_back(EGL_TRUE);
  }
  if (system_device_id != 0 &&
      g_driver_egl.client_ext.b_EGL_ANGLE_platform_angle_device_id) {
    uint32_t low_part = system_device_id & 0xffffffff;
    extra_display_attribs.push_back(EGL_PLATFORM_ANGLE_DEVICE_ID_LOW_ANGLE);
    extra_display_attribs.push_back(low_part);

    uint32_t high_part = (system_device_id >> 32) & 0xffffffff;
    extra_display_attribs.push_back(EGL_PLATFORM_ANGLE_DEVICE_ID_HIGH_ANGLE);
    extra_display_attribs.push_back(high_part);
  }
  EGLNativeDisplayType display = native_display.GetDisplay();
  switch (display_type) {
    case DEFAULT:
    case SWIFT_SHADER: {
      if (native_display.GetPlatform() != 0) {
        return eglGetPlatformDisplay(native_display.GetPlatform(),
                                     reinterpret_cast<void*>(display), nullptr);
      }
      return eglGetDisplay(display);
    }
    case ANGLE_D3D9:
      return GetPlatformANGLEDisplay(
          display, EGL_PLATFORM_ANGLE_TYPE_D3D9_ANGLE, enabled_angle_features,
          disabled_angle_features, extra_display_attribs);
    case ANGLE_D3D11:
      return GetPlatformANGLEDisplay(
          display, EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE, enabled_angle_features,
          disabled_angle_features, extra_display_attribs);
    case ANGLE_D3D11_NULL:
      extra_display_attribs.push_back(EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE);
      extra_display_attribs.push_back(
          EGL_PLATFORM_ANGLE_DEVICE_TYPE_NULL_ANGLE);
      return GetPlatformANGLEDisplay(
          display, EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE, enabled_angle_features,
          disabled_angle_features, extra_display_attribs);
    case ANGLE_OPENGL:
      return GetPlatformANGLEDisplay(
          display, EGL_PLATFORM_ANGLE_TYPE_OPENGL_ANGLE, enabled_angle_features,
          disabled_angle_features, extra_display_attribs);
    case ANGLE_OPENGL_EGL:
      extra_display_attribs.push_back(EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE);
      extra_display_attribs.push_back(EGL_PLATFORM_ANGLE_DEVICE_TYPE_EGL_ANGLE);
      return GetPlatformANGLEDisplay(
          display, EGL_PLATFORM_ANGLE_TYPE_OPENGL_ANGLE, enabled_angle_features,
          disabled_angle_features, extra_display_attribs);
    case ANGLE_OPENGL_NULL:
      extra_display_attribs.push_back(EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE);
      extra_display_attribs.push_back(
          EGL_PLATFORM_ANGLE_DEVICE_TYPE_NULL_ANGLE);
      return GetPlatformANGLEDisplay(
          display, EGL_PLATFORM_ANGLE_TYPE_OPENGL_ANGLE, enabled_angle_features,
          disabled_angle_features, extra_display_attribs);
    case ANGLE_OPENGLES:
      return GetPlatformANGLEDisplay(
          display, EGL_PLATFORM_ANGLE_TYPE_OPENGLES_ANGLE,
          enabled_angle_features, disabled_angle_features,
          extra_display_attribs);
    case ANGLE_OPENGLES_EGL:
      extra_display_attribs.push_back(EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE);
      extra_display_attribs.push_back(EGL_PLATFORM_ANGLE_DEVICE_TYPE_EGL_ANGLE);
      return GetPlatformANGLEDisplay(
          display, EGL_PLATFORM_ANGLE_TYPE_OPENGLES_ANGLE,
          enabled_angle_features, disabled_angle_features,
          extra_display_attribs);
    case ANGLE_OPENGLES_NULL:
      extra_display_attribs.push_back(EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE);
      extra_display_attribs.push_back(
          EGL_PLATFORM_ANGLE_DEVICE_TYPE_NULL_ANGLE);
      return GetPlatformANGLEDisplay(
          display, EGL_PLATFORM_ANGLE_TYPE_OPENGLES_ANGLE,
          enabled_angle_features, disabled_angle_features,
          extra_display_attribs);
    case ANGLE_NULL:
      return GetPlatformANGLEDisplay(
          display, EGL_PLATFORM_ANGLE_TYPE_NULL_ANGLE, enabled_angle_features,
          disabled_angle_features, extra_display_attribs);
    case ANGLE_VULKAN:
      return GetPlatformANGLEDisplay(
          display, EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE, enabled_angle_features,
          disabled_angle_features, extra_display_attribs);
    case ANGLE_VULKAN_NULL:
      extra_display_attribs.push_back(EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE);
      extra_display_attribs.push_back(
          EGL_PLATFORM_ANGLE_DEVICE_TYPE_NULL_ANGLE);
      return GetPlatformANGLEDisplay(
          display, EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE, enabled_angle_features,
          disabled_angle_features, extra_display_attribs);
    case ANGLE_D3D11on12:
      extra_display_attribs.push_back(EGL_PLATFORM_ANGLE_D3D11ON12_ANGLE);
      extra_display_attribs.push_back(EGL_TRUE);
      return GetPlatformANGLEDisplay(
          display, EGL_PLATFORM_ANGLE_TYPE_D3D11_ANGLE, enabled_angle_features,
          disabled_angle_features, extra_display_attribs);
    case ANGLE_SWIFTSHADER:
      extra_display_attribs.push_back(EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE);
      extra_display_attribs.push_back(
          EGL_PLATFORM_ANGLE_DEVICE_TYPE_SWIFTSHADER_ANGLE);
#if defined(USE_OZONE)
#if BUILDFLAG(IS_CHROMEOS) && BUILDFLAG(OZONE_PLATFORM_X11)
      extra_display_attribs.push_back(
          EGL_PLATFORM_ANGLE_NATIVE_PLATFORM_TYPE_ANGLE);
      extra_display_attribs.push_back(
          EGL_PLATFORM_VULKAN_DISPLAY_MODE_HEADLESS_ANGLE);
#endif  // BUILDFLAG(OZONE_PLATFORM_X11)
#endif  // defined(USE_OZONE)
      return GetPlatformANGLEDisplay(
          display, EGL_PLATFORM_ANGLE_TYPE_VULKAN_ANGLE, enabled_angle_features,
          disabled_angle_features, extra_display_attribs);
    case ANGLE_METAL:
      return GetPlatformANGLEDisplay(
          display, EGL_PLATFORM_ANGLE_TYPE_METAL_ANGLE, enabled_angle_features,
          disabled_angle_features, extra_display_attribs);
    case ANGLE_METAL_NULL:
      extra_display_attribs.push_back(EGL_PLATFORM_ANGLE_DEVICE_TYPE_ANGLE);
      extra_display_attribs.push_back(
          EGL_PLATFORM_ANGLE_DEVICE_TYPE_NULL_ANGLE);
      return GetPlatformANGLEDisplay(
          display, EGL_PLATFORM_ANGLE_TYPE_METAL_ANGLE, enabled_angle_features,
          disabled_angle_features, extra_display_attribs);
    default:
      NOTREACHED();
      return EGL_NO_DISPLAY;
  }
}

ANGLEImplementation GetANGLEImplementationFromDisplayType(
    DisplayType display_type) {
  switch (display_type) {
    case ANGLE_D3D9:
      return ANGLEImplementation::kD3D9;
    case ANGLE_D3D11:
    case ANGLE_D3D11_NULL:
    case ANGLE_D3D11on12:
      return ANGLEImplementation::kD3D11;
    case ANGLE_OPENGL:
    case ANGLE_OPENGL_NULL:
      return ANGLEImplementation::kOpenGL;
    case ANGLE_OPENGLES:
    case ANGLE_OPENGLES_NULL:
      return ANGLEImplementation::kOpenGLES;
    case ANGLE_NULL:
      return ANGLEImplementation::kNull;
    case ANGLE_VULKAN:
    case ANGLE_VULKAN_NULL:
      return ANGLEImplementation::kVulkan;
    case ANGLE_SWIFTSHADER:
      return ANGLEImplementation::kSwiftShader;
    case ANGLE_METAL:
    case ANGLE_METAL_NULL:
      return ANGLEImplementation::kMetal;
    default:
      return ANGLEImplementation::kNone;
  }
}

const char* DisplayTypeString(DisplayType display_type) {
  switch (display_type) {
    case DEFAULT:
      return "Default";
    case SWIFT_SHADER:
      return "SwiftShader";
    case ANGLE_D3D9:
      return "D3D9";
    case ANGLE_D3D11:
      return "D3D11";
    case ANGLE_D3D11_NULL:
      return "D3D11Null";
    case ANGLE_OPENGL:
      return "OpenGL";
    case ANGLE_OPENGL_NULL:
      return "OpenGLNull";
    case ANGLE_OPENGLES:
      return "OpenGLES";
    case ANGLE_OPENGLES_NULL:
      return "OpenGLESNull";
    case ANGLE_NULL:
      return "Null";
    case ANGLE_VULKAN:
      return "Vulkan";
    case ANGLE_VULKAN_NULL:
      return "VulkanNull";
    case ANGLE_D3D11on12:
      return "D3D11on12";
    case ANGLE_SWIFTSHADER:
      return "SwANGLE";
    case ANGLE_OPENGL_EGL:
      return "OpenGLEGL";
    case ANGLE_OPENGLES_EGL:
      return "OpenGLESEGL";
    case ANGLE_METAL:
      return "Metal";
    case ANGLE_METAL_NULL:
      return "MetalNull";
    default:
      NOTREACHED();
      return "Err";
  }
}

void AddInitDisplay(std::vector<DisplayType>* init_displays,
                    DisplayType display_type) {
  // Make sure to not add the same display type twice.
  if (!base::Contains(*init_displays, display_type))
    init_displays->push_back(display_type);
}

const char* GetDebugMessageTypeString(EGLint source) {
  switch (source) {
    case EGL_DEBUG_MSG_CRITICAL_KHR:
      return "Critical";
    case EGL_DEBUG_MSG_ERROR_KHR:
      return "Error";
    case EGL_DEBUG_MSG_WARN_KHR:
      return "Warning";
    case EGL_DEBUG_MSG_INFO_KHR:
      return "Info";
    default:
      return "UNKNOWN";
  }
}

void EGLAPIENTRY LogEGLDebugMessage(EGLenum error,
                                    const char* command,
                                    EGLint message_type,
                                    EGLLabelKHR thread_label,
                                    EGLLabelKHR object_label,
                                    const char* message) {
  std::string formatted_message = std::string("EGL Driver message (") +
                                  GetDebugMessageTypeString(message_type) +
                                  ") " + command + ": " + message;

  // Assume that all labels that have been set are strings
  if (thread_label) {
    formatted_message += " thread: ";
    formatted_message += static_cast<const char*>(thread_label);
  }
  if (object_label) {
    formatted_message += " object: ";
    formatted_message += static_cast<const char*>(object_label);
  }

  if (message_type == EGL_DEBUG_MSG_CRITICAL_KHR ||
      message_type == EGL_DEBUG_MSG_ERROR_KHR) {
    LOG(ERROR) << formatted_message;
  } else {
    DVLOG(1) << formatted_message;
  }
}

void GetEGLInitDisplays(bool supports_angle_d3d,
                        bool supports_angle_opengl,
                        bool supports_angle_null,
                        bool supports_angle_vulkan,
                        bool supports_angle_swiftshader,
                        bool supports_angle_egl,
                        bool supports_angle_metal,
                        const base::CommandLine* command_line,
                        std::vector<DisplayType>* init_displays) {
  // If we're already requesting software GL, make sure we don't fallback to the
  // GPU
  bool forceSoftwareGL = IsSoftwareGLImplementation(GetGLImplementationParts());

  std::string requested_renderer =
      forceSoftwareGL ? kANGLEImplementationSwiftShaderName
                      : command_line->GetSwitchValueASCII(switches::kUseANGLE);

  bool use_angle_default =
      !forceSoftwareGL &&
      (!command_line->HasSwitch(switches::kUseANGLE) ||
       requested_renderer == kANGLEImplementationDefaultName);

  if (supports_angle_null &&
      requested_renderer == kANGLEImplementationNullName) {
    AddInitDisplay(init_displays, ANGLE_NULL);
    return;
  }

  // If no display has been explicitly requested and the DefaultANGLEOpenGL
  // experiment is enabled, try creating OpenGL displays first.
  // TODO(oetuaho@nvidia.com): Only enable this path on specific GPUs with a
  // blocklist entry. http://crbug.com/693090
  if (supports_angle_opengl && use_angle_default &&
      base::FeatureList::IsEnabled(features::kDefaultANGLEOpenGL)) {
    AddInitDisplay(init_displays, ANGLE_OPENGL);
    AddInitDisplay(init_displays, ANGLE_OPENGLES);
  }

  if (supports_angle_metal && use_angle_default &&
      base::FeatureList::IsEnabled(features::kDefaultANGLEMetal)) {
    AddInitDisplay(init_displays, ANGLE_METAL);
  }

  if (supports_angle_vulkan && use_angle_default &&
      features::IsDefaultANGLEVulkan()) {
    AddInitDisplay(init_displays, ANGLE_VULKAN);
  }

  if (supports_angle_d3d) {
    if (use_angle_default) {
      // Default mode for ANGLE - try D3D11, else try D3D9
      if (!command_line->HasSwitch(switches::kDisableD3D11)) {
        AddInitDisplay(init_displays, ANGLE_D3D11);
      }
      AddInitDisplay(init_displays, ANGLE_D3D9);
    } else {
      if (requested_renderer == kANGLEImplementationD3D11Name) {
        AddInitDisplay(init_displays, ANGLE_D3D11);
      } else if (requested_renderer == kANGLEImplementationD3D9Name) {
        AddInitDisplay(init_displays, ANGLE_D3D9);
      } else if (requested_renderer == kANGLEImplementationD3D11NULLName) {
        AddInitDisplay(init_displays, ANGLE_D3D11_NULL);
      } else if (requested_renderer == kANGLEImplementationD3D11on12Name) {
        AddInitDisplay(init_displays, ANGLE_D3D11on12);
      }
    }
  }

  if (supports_angle_opengl) {
    if (use_angle_default && !supports_angle_d3d) {
#if BUILDFLAG(IS_ANDROID)
      // Don't request desktopGL on android
      AddInitDisplay(init_displays, ANGLE_OPENGLES);
#else
      AddInitDisplay(init_displays, ANGLE_OPENGL);
      AddInitDisplay(init_displays, ANGLE_OPENGLES);
#endif
    } else {
      if (requested_renderer == kANGLEImplementationOpenGLName) {
        AddInitDisplay(init_displays, ANGLE_OPENGL);
      } else if (requested_renderer == kANGLEImplementationOpenGLESName) {
        AddInitDisplay(init_displays, ANGLE_OPENGLES);
      } else if (requested_renderer == kANGLEImplementationOpenGLNULLName) {
        AddInitDisplay(init_displays, ANGLE_OPENGL_NULL);
      } else if (requested_renderer == kANGLEImplementationOpenGLESNULLName) {
        AddInitDisplay(init_displays, ANGLE_OPENGLES_NULL);
      } else if (requested_renderer == kANGLEImplementationOpenGLEGLName &&
                 supports_angle_egl) {
        AddInitDisplay(init_displays, ANGLE_OPENGL_EGL);
      } else if (requested_renderer == kANGLEImplementationOpenGLESEGLName &&
                 supports_angle_egl) {
        AddInitDisplay(init_displays, ANGLE_OPENGLES_EGL);
      }
    }
  }

  if (supports_angle_vulkan) {
    if (use_angle_default) {
      if (!supports_angle_d3d && !supports_angle_opengl) {
        AddInitDisplay(init_displays, ANGLE_VULKAN);
      }
    } else if (requested_renderer == kANGLEImplementationVulkanName) {
      AddInitDisplay(init_displays, ANGLE_VULKAN);
    } else if (requested_renderer == kANGLEImplementationVulkanNULLName) {
      AddInitDisplay(init_displays, ANGLE_VULKAN_NULL);
    }
  }

  if (supports_angle_swiftshader) {
    if (requested_renderer == kANGLEImplementationSwiftShaderName ||
        requested_renderer == kANGLEImplementationSwiftShaderForWebGLName) {
      AddInitDisplay(init_displays, ANGLE_SWIFTSHADER);
    }
  }

  if (supports_angle_metal) {
    if (use_angle_default) {
      if (!supports_angle_opengl) {
        AddInitDisplay(init_displays, ANGLE_METAL);
      }
    } else if (requested_renderer == kANGLEImplementationMetalName) {
      AddInitDisplay(init_displays, ANGLE_METAL);
    } else if (requested_renderer == kANGLEImplementationMetalNULLName) {
      AddInitDisplay(init_displays, ANGLE_METAL_NULL);
    }
  }

  // If no displays are available due to missing angle extensions or invalid
  // flags, request the default display.
  if (init_displays->empty()) {
    init_displays->push_back(DEFAULT);
  }
}

}  // namespace

void GetEGLInitDisplaysForTesting(bool supports_angle_d3d,
                                  bool supports_angle_opengl,
                                  bool supports_angle_null,
                                  bool supports_angle_vulkan,
                                  bool supports_angle_swiftshader,
                                  bool supports_angle_egl,
                                  bool supports_angle_metal,
                                  const base::CommandLine* command_line,
                                  std::vector<DisplayType>* init_displays) {
  GetEGLInitDisplays(supports_angle_d3d, supports_angle_opengl,
                     supports_angle_null, supports_angle_vulkan,
                     supports_angle_swiftshader, supports_angle_egl,
                     supports_angle_metal, command_line, init_displays);
}

GLDisplay::GLDisplay(uint64_t system_device_id)
    : system_device_id_(system_device_id) {}

GLDisplay::~GLDisplay() = default;

#if defined(USE_EGL)
GLDisplayEGL::EGLGpuSwitchingObserver::EGLGpuSwitchingObserver(
    EGLDisplay display)
    : display_(display) {
  DCHECK(display != EGL_NO_DISPLAY);
}

void GLDisplayEGL::EGLGpuSwitchingObserver::OnGpuSwitched(
    GpuPreference active_gpu_heuristic) {
  eglHandleGPUSwitchANGLE(display_);
}

GLDisplayEGL::GLDisplayEGL(uint64_t system_device_id)
    : GLDisplay(system_device_id) {
  ext = std::make_unique<DisplayExtensionsEGL>();
  display_ = EGL_NO_DISPLAY;
}

GLDisplayEGL::~GLDisplayEGL() = default;

EGLDisplay GLDisplayEGL::GetDisplay() {
  return display_;
}

void GLDisplayEGL::Shutdown() {
  if (display_ == EGL_NO_DISPLAY)
    return;

  if (gpu_switching_observer_.get()) {
    ui::GpuSwitchingManager::GetInstance()->RemoveObserver(
        gpu_switching_observer_.get());
    gpu_switching_observer_.reset();
  }

  angle::ResetPlatform(display_);
  DCHECK(g_driver_egl.fn.eglTerminateFn);
  eglTerminate(display_);

  display_ = EGL_NO_DISPLAY;
  egl_surfaceless_context_supported_ = false;
  egl_context_priority_supported_ = false;
  egl_android_native_fence_sync_supported_ = false;
}

void GLDisplayEGL::SetDisplay(EGLDisplay display) {
  display_ = display;
}

EGLDisplayPlatform GLDisplayEGL::GetNativeDisplay() const {
  return native_display_;
}

DisplayType GLDisplayEGL::GetDisplayType() const {
  return display_type_;
}

// static
GLDisplayEGL* GLDisplayEGL::GetDisplayForCurrentContext() {
  GLContext* context = GLContext::GetCurrent();
  return context ? context->GetGLDisplayEGL() : nullptr;
}

bool GLDisplayEGL::IsEGLSurfacelessContextSupported() {
  return egl_surfaceless_context_supported_;
}

bool GLDisplayEGL::IsEGLContextPrioritySupported() {
  return egl_context_priority_supported_;
}

bool GLDisplayEGL::IsAndroidNativeFenceSyncSupported() {
  return egl_android_native_fence_sync_supported_;
}

bool GLDisplayEGL::IsANGLEExternalContextAndSurfaceSupported() {
  return this->ext->b_EGL_ANGLE_external_context_and_surface;
}

bool GLDisplayEGL::Initialize(EGLDisplayPlatform native_display) {
  if (display_ != EGL_NO_DISPLAY)
    return true;

  if (!InitializeDisplay(native_display))
    return false;
  InitializeCommon();

  if (ext->b_EGL_ANGLE_power_preference) {
    gpu_switching_observer_ =
        std::make_unique<EGLGpuSwitchingObserver>(display_);
    ui::GpuSwitchingManager::GetInstance()->AddObserver(
        gpu_switching_observer_.get());
  }
  return true;
}

void GLDisplayEGL::InitializeForTesting() {
  display_ = eglGetCurrentDisplay();
  ext->InitializeExtensionSettings(display_);
  InitializeCommon();
}

bool GLDisplayEGL::InitializeExtensionSettings() {
  if (display_ == EGL_NO_DISPLAY)
    return false;
  ext->UpdateConditionalExtensionSettings(display_);
  return true;
}

// InitializeDisplay is necessary because the static binding code
// needs a full Display init before it can query the Display extensions.
bool GLDisplayEGL::InitializeDisplay(EGLDisplayPlatform native_display) {
  if (display_ != EGL_NO_DISPLAY)
    return true;

  native_display_ = native_display;

  bool supports_egl_debug = g_driver_egl.client_ext.b_EGL_KHR_debug;
  if (supports_egl_debug) {
    EGLAttrib controls[] = {
        EGL_DEBUG_MSG_CRITICAL_KHR,
        EGL_TRUE,
        EGL_DEBUG_MSG_ERROR_KHR,
        EGL_TRUE,
        EGL_DEBUG_MSG_WARN_KHR,
        EGL_TRUE,
        EGL_DEBUG_MSG_INFO_KHR,
        EGL_TRUE,
        EGL_NONE,
        EGL_NONE,
    };

    eglDebugMessageControlKHR(&LogEGLDebugMessage, controls);
  }

  bool supports_angle_d3d = false;
  bool supports_angle_opengl = false;
  bool supports_angle_null = false;
  bool supports_angle_vulkan = false;
  bool supports_angle_swiftshader = false;
  bool supports_angle_egl = false;
  bool supports_angle_metal = false;
  // Check for availability of ANGLE extensions.
  if (g_driver_egl.client_ext.b_EGL_ANGLE_platform_angle) {
    supports_angle_d3d = g_driver_egl.client_ext.b_EGL_ANGLE_platform_angle_d3d;
    supports_angle_opengl =
        g_driver_egl.client_ext.b_EGL_ANGLE_platform_angle_opengl;
    supports_angle_null =
        g_driver_egl.client_ext.b_EGL_ANGLE_platform_angle_null;
    supports_angle_vulkan =
        g_driver_egl.client_ext.b_EGL_ANGLE_platform_angle_vulkan;
    supports_angle_swiftshader =
        g_driver_egl.client_ext
            .b_EGL_ANGLE_platform_angle_device_type_swiftshader;
    supports_angle_egl = g_driver_egl.client_ext
                             .b_EGL_ANGLE_platform_angle_device_type_egl_angle;
    supports_angle_metal =
        g_driver_egl.client_ext.b_EGL_ANGLE_platform_angle_metal;
  }

  bool supports_angle = supports_angle_d3d || supports_angle_opengl ||
                        supports_angle_null || supports_angle_vulkan ||
                        supports_angle_swiftshader || supports_angle_metal;

  std::vector<DisplayType> init_displays;
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  GetEGLInitDisplays(supports_angle_d3d, supports_angle_opengl,
                     supports_angle_null, supports_angle_vulkan,
                     supports_angle_swiftshader, supports_angle_egl,
                     supports_angle_metal, command_line, &init_displays);

  std::vector<std::string> enabled_angle_features =
      GetStringVectorFromCommandLine(command_line,
                                     switches::kEnableANGLEFeatures);
  std::vector<std::string> disabled_angle_features =
      GetStringVectorFromCommandLine(command_line,
                                     switches::kDisableANGLEFeatures);

  bool disable_all_angle_features =
      command_line->HasSwitch(switches::kDisableGpuDriverBugWorkarounds);

  for (size_t disp_index = 0; disp_index < init_displays.size(); ++disp_index) {
    DisplayType display_type = init_displays[disp_index];
    EGLDisplay display = GetDisplayFromType(
        display_type, native_display, enabled_angle_features,
        disabled_angle_features, disable_all_angle_features, system_device_id_);
    if (display == EGL_NO_DISPLAY) {
      LOG(ERROR) << "EGL display query failed with error "
                 << GetLastEGLErrorString();
    }

    // Init ANGLE platform now that we have the global display.
    if (supports_angle) {
      if (!angle::InitializePlatform(display)) {
        LOG(ERROR) << "ANGLE Platform initialization failed.";
      }

      SetANGLEImplementation(
          GetANGLEImplementationFromDisplayType(display_type));
    }

    // The platform may need to unset its platform specific display env in case
    // of vulkan if the platform doesn't support Vulkan surface.
    absl::optional<base::ScopedEnvironmentVariableOverride> unset_display;
    if (display_type == ANGLE_VULKAN) {
      unset_display = GLDisplayEglUtil::GetInstance()
                          ->MaybeGetScopedDisplayUnsetForVulkan();
    }

    if (!eglInitialize(display, nullptr, nullptr)) {
      bool is_last = disp_index == init_displays.size() - 1;

      LOG(ERROR) << "eglInitialize " << DisplayTypeString(display_type)
                 << " failed with error " << GetLastEGLErrorString()
                 << (is_last ? "" : ", trying next display type");
      continue;
    }

    std::ostringstream display_type_string;
    auto gl_implementation = GetGLImplementationParts();
    display_type_string << GetGLImplementationGLName(gl_implementation);
    if (gl_implementation.gl == kGLImplementationEGLANGLE) {
      display_type_string << ":" << DisplayTypeString(display_type);
    }

    static auto* egl_display_type_key = base::debug::AllocateCrashKeyString(
        "egl-display-type", base::debug::CrashKeySize::Size32);
    base::debug::SetCrashKeyString(egl_display_type_key,
                                   display_type_string.str());

    UMA_HISTOGRAM_ENUMERATION("GPU.EGLDisplayType", display_type,
                              DISPLAY_TYPE_MAX);
    display_ = display;
    display_type_ = display_type;
    ext->InitializeExtensionSettings(display);
    return true;
  }

  return false;
}

void GLDisplayEGL::InitializeCommon() {
  // According to https://source.android.com/compatibility/android-cdd.html the
  // EGL_IMG_context_priority extension is mandatory for Virtual Reality High
  // Performance support, but due to a bug in Android Nougat the extension
  // isn't being reported even when it's present. As a fallback, check if other
  // related extensions that were added for VR support are present, and assume
  // that this implies context priority is also supported. See also:
  // https://github.com/googlevr/gvr-android-sdk/issues/330
  egl_context_priority_supported_ =
      ext->b_EGL_IMG_context_priority ||
      (ext->b_EGL_ANDROID_front_buffer_auto_refresh &&
       ext->b_EGL_ANDROID_create_native_client_buffer);

  // Check if SurfacelessEGL is supported.
  egl_surfaceless_context_supported_ = ext->b_EGL_KHR_surfaceless_context;

  // TODO(oetuaho@nvidia.com): Surfaceless is disabled on Android as a temporary
  // workaround, since code written for Android WebView takes different paths
  // based on whether GL surface objects have underlying EGL surface handles,
  // conflicting with the use of surfaceless. ANGLE can still expose surfacelss
  // because it is emulated with pbuffers if native support is not present. See
  // https://crbug.com/382349.

#if BUILDFLAG(IS_ANDROID)
  // Use the WebGL compatibility extension for detecting ANGLE. ANGLE always
  // exposes it.
  bool is_angle = ext->b_EGL_ANGLE_create_context_webgl_compatibility;
  if (!is_angle) {
    egl_surfaceless_context_supported_ = false;
  }
#endif

  if (egl_surfaceless_context_supported_) {
    // EGL_KHR_surfaceless_context is supported but ensure
    // GL_OES_surfaceless_context is also supported. We need a current context
    // to query for supported GL extensions.
    scoped_refptr<GLSurface> surface =
        new SurfacelessEGL(this, gfx::Size(1, 1));
    scoped_refptr<GLContext> context = InitializeGLContext(
        new GLContextEGL(nullptr), surface.get(), GLContextAttribs());
    if (!context || !context->MakeCurrent(surface.get()))
      egl_surfaceless_context_supported_ = false;

    // Ensure context supports GL_OES_surfaceless_context.
    if (egl_surfaceless_context_supported_) {
      egl_surfaceless_context_supported_ =
          context->HasExtension("GL_OES_surfaceless_context");
      context->ReleaseCurrent(surface.get());
    }
  }

  // The native fence sync extension is a bit complicated. It's reported as
  // present for ChromeOS, but Android currently doesn't report this extension
  // even when it's present, and older devices and Android emulator may export
  // a useless wrapper function. See crbug.com/775707 for details. In short, if
  // the symbol is present and we're on Android N or newer and we are not on
  // Android emulator, assume that it's usable even if the extension wasn't
  // reported. TODO(https://crbug.com/1086781): Once this is fixed at the
  // Android level, update the heuristic to trust the reported extension from
  // that version onward.
  egl_android_native_fence_sync_supported_ =
      ext->b_EGL_ANDROID_native_fence_sync;
#if BUILDFLAG(IS_ANDROID)
  if (!egl_android_native_fence_sync_supported_ &&
      base::android::BuildInfo::GetInstance()->sdk_int() >=
          base::android::SDK_VERSION_NOUGAT &&
      g_driver_egl.fn.eglDupNativeFenceFDANDROIDFn &&
      base::SysInfo::GetAndroidHardwareEGL() != "swiftshader" &&
      base::SysInfo::GetAndroidHardwareEGL() != "emulation") {
    egl_android_native_fence_sync_supported_ = true;
  }
#endif
}
#endif  // defined(USE_EGL)

#if defined(USE_GLX)
GLDisplayX11::GLDisplayX11(uint64_t system_device_id)
    : GLDisplay(system_device_id) {}

GLDisplayX11::~GLDisplayX11() = default;

void* GLDisplayX11::GetDisplay() {
  return x11::Connection::Get()->GetXlibDisplay();
}

void GLDisplayX11::Shutdown() {}
#endif  // defined(USE_GLX)

}  // namespace gl
