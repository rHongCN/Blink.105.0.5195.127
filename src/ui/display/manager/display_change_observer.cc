// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/display/manager/display_change_observer.h"

#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "base/check_op.h"
#include "base/command_line.h"
#include "build/chromeos_buildflags.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/user_activity/user_activity_detector.h"
#include "ui/display/display.h"
#include "ui/display/display_features.h"
#include "ui/display/display_layout.h"
#include "ui/display/display_switches.h"
#include "ui/display/manager/display_layout_store.h"
#include "ui/display/manager/display_manager.h"
#include "ui/display/manager/display_manager_utilities.h"
#include "ui/display/manager/touch_device_manager.h"
#include "ui/display/types/display_constants.h"
#include "ui/display/types/display_mode.h"
#include "ui/display/types/display_snapshot.h"
#include "ui/display/util/display_util.h"
#include "ui/display/util/edid_parser.h"
#include "ui/events/devices/device_data_manager.h"
#include "ui/events/devices/touchscreen_device.h"
#include "ui/strings/grit/ui_strings.h"

namespace display {
namespace {

// TODO(crbug/1262970): Delete when we can read radius from command line.
const float kRoundedDisplayRadius = 16.0;

// The DPI threshold to determine the device scale factor.
// DPI higher than |dpi| will use |device_scale_factor|.
struct DeviceScaleFactorDPIThreshold {
  float dpi;
  float device_scale_factor;
};

// Update the list of zoom levels whenever a new device scale factor is added
// here. See zoom level list in /ui/display/manager/display_manager_util.cc
const DeviceScaleFactorDPIThreshold kThresholdTableForInternal[] = {
    {310.f, kDsf_2_666}, {270.0f, 2.4f},  {230.0f, 2.0f}, {220.0f, kDsf_1_777},
    {180.0f, 1.6f},      {150.0f, 1.25f}, {0.0f, 1.0f},
};

// Returns a list of display modes for the given |output| that doesn't exclude
// any mode. The returned list is sorted by size, then by refresh rate, then by
// is_interlaced.
ManagedDisplayInfo::ManagedDisplayModeList GetModeListWithAllRefreshRates(
    const DisplaySnapshot& output) {
  ManagedDisplayInfo::ManagedDisplayModeList display_mode_list;
  for (const auto& mode_info : output.modes()) {
    display_mode_list.emplace_back(mode_info->size(), mode_info->refresh_rate(),
                                   mode_info->is_interlaced(),
                                   output.native_mode() == mode_info.get(),
                                   1.0);
  }

  std::sort(
      display_mode_list.begin(), display_mode_list.end(),
      [](const ManagedDisplayMode& lhs, const ManagedDisplayMode& rhs) {
        return std::forward_as_tuple(lhs.size().width(), lhs.size().height(),
                                     lhs.refresh_rate(), lhs.is_interlaced()) <
               std::forward_as_tuple(rhs.size().width(), rhs.size().height(),
                                     rhs.refresh_rate(), rhs.is_interlaced());
      });

  return display_mode_list;
}

}  // namespace

// static
ManagedDisplayInfo::ManagedDisplayModeList
DisplayChangeObserver::GetInternalManagedDisplayModeList(
    const ManagedDisplayInfo& display_info,
    const DisplaySnapshot& output) {
  const DisplayMode* ui_native_mode = output.native_mode();
  ManagedDisplayMode native_mode(ui_native_mode->size(),
                                 ui_native_mode->refresh_rate(),
                                 ui_native_mode->is_interlaced(), true,
                                 display_info.device_scale_factor());
  return CreateInternalManagedDisplayModeList(native_mode);
}

// static
ManagedDisplayInfo::ManagedDisplayModeList
DisplayChangeObserver::GetExternalManagedDisplayModeList(
    const DisplaySnapshot& output) {
  if (display::features::IsListAllDisplayModesEnabled())
    return GetModeListWithAllRefreshRates(output);

  struct SizeComparator {
    constexpr bool operator()(const gfx::Size& lhs,
                              const gfx::Size& rhs) const {
      return std::forward_as_tuple(lhs.width(), lhs.height()) <
             std::forward_as_tuple(rhs.width(), rhs.height());
    }
  };

  using DisplayModeMap =
      std::map<gfx::Size, ManagedDisplayMode, SizeComparator>;
  DisplayModeMap display_mode_map;

  ManagedDisplayMode native_mode;
  for (const auto& mode_info : output.modes()) {
    const gfx::Size size = mode_info->size();

    ManagedDisplayMode display_mode(
        mode_info->size(), mode_info->refresh_rate(),
        mode_info->is_interlaced(), output.native_mode() == mode_info.get(),
        1.0);
    if (display_mode.native())
      native_mode = display_mode;

    // Add the display mode if it isn't already present and override interlaced
    // display modes with non-interlaced ones. We prioritize having non
    // interlaced mode over refresh rate. A mode having lower refresh rate
    // but is not interlaced will be picked over a mode having high refresh
    // rate but is interlaced.
    auto display_mode_it = display_mode_map.find(size);
    if (display_mode_it == display_mode_map.end()) {
      display_mode_map.emplace(size, display_mode);
    } else if (display_mode_it->second.is_interlaced() &&
               !display_mode.is_interlaced()) {
      display_mode_it->second = std::move(display_mode);
    } else if (!display_mode.is_interlaced() &&
               display_mode_it->second.refresh_rate() <
                   display_mode.refresh_rate()) {
      display_mode_it->second = std::move(display_mode);
    }
  }

  if (output.native_mode()) {
    const gfx::Size size = native_mode.size();

    auto it = display_mode_map.find(size);
    DCHECK(it != display_mode_map.end())
        << "Native mode must be part of the mode list.";

    // If the native mode was replaced (e.g. by a mode with similar size but
    // higher refresh rate), we overwrite that mode with the native mode. The
    // native mode will always be chosen as the best mode for this size (see
    // DisplayConfigurator::FindDisplayModeMatchingSize()).
    if (!it->second.native())
      it->second = native_mode;
  }

  ManagedDisplayInfo::ManagedDisplayModeList display_mode_list;
  for (const auto& display_mode_pair : display_mode_map)
    display_mode_list.push_back(std::move(display_mode_pair.second));

  return display_mode_list;
}

DisplayChangeObserver::DisplayChangeObserver(DisplayManager* display_manager)
    : display_manager_(display_manager) {
  ui::DeviceDataManager::GetInstance()->AddObserver(this);
}

DisplayChangeObserver::~DisplayChangeObserver() {
  ui::DeviceDataManager::GetInstance()->RemoveObserver(this);
}

MultipleDisplayState DisplayChangeObserver::GetStateForDisplayIds(
    const DisplayConfigurator::DisplayStateList& display_states) {
  UpdateInternalDisplay(display_states);
  if (display_states.size() == 1)
    return MULTIPLE_DISPLAY_STATE_SINGLE;
  DisplayIdList list =
      GenerateDisplayIdList(display_states.begin(), display_states.end(),
                            [](const DisplaySnapshot* display_state) {
                              return display_state->display_id();
                            });
  return display_manager_->ShouldSetMirrorModeOn(
             list, /*should_check_hardware_mirrorring=*/true)
             ? MULTIPLE_DISPLAY_STATE_MULTI_MIRROR
             : MULTIPLE_DISPLAY_STATE_MULTI_EXTENDED;
}

bool DisplayChangeObserver::GetSelectedModeForDisplayId(
    int64_t display_id,
    ManagedDisplayMode* out_mode) const {
  return display_manager_->GetSelectedModeForDisplayId(display_id, out_mode);
}

void DisplayChangeObserver::OnDisplayModeChanged(
    const DisplayConfigurator::DisplayStateList& display_states) {
  UpdateInternalDisplay(display_states);

  std::vector<ManagedDisplayInfo> displays;
  for (const DisplaySnapshot* state : display_states) {
    const DisplayMode* mode_info = state->current_mode();
    if (!mode_info)
      continue;

    displays.emplace_back(CreateManagedDisplayInfo(state, mode_info));
  }

  display_manager_->touch_device_manager()->AssociateTouchscreens(
      &displays, ui::DeviceDataManager::GetInstance()->GetTouchscreenDevices());
  display_manager_->OnNativeDisplaysChanged(displays);

  // For the purposes of user activity detection, ignore synthetic mouse events
  // that are triggered by screen resizes: http://crbug.com/360634
  ui::UserActivityDetector* user_activity_detector =
      ui::UserActivityDetector::Get();
  if (user_activity_detector)
    user_activity_detector->OnDisplayPowerChanging();
}

void DisplayChangeObserver::OnDisplayModeChangeFailed(
    const DisplayConfigurator::DisplayStateList& displays,
    MultipleDisplayState failed_new_state) {
  // If display configuration failed during startup, simply update the display
  // manager with detected displays. If no display is detected, it will
  // create a pseudo display.
  if (display_manager_->GetNumDisplays() == 0)
    OnDisplayModeChanged(displays);
}

void DisplayChangeObserver::OnInputDeviceConfigurationChanged(
    uint8_t input_device_types) {
  if (input_device_types & ui::InputDeviceEventObserver::kTouchscreen) {
    // If there are no cached display snapshots, either there are no attached
    // displays or the cached snapshots have been invalidated. For the first
    // case there aren't any touchscreens to associate. For the second case,
    // the displays and touch input-devices will get associated when display
    // configuration finishes.
    const auto& cached_displays =
        display_manager_->configurator()->cached_displays();
    if (!cached_displays.empty())
      OnDisplayModeChanged(cached_displays);
  }
}

void DisplayChangeObserver::UpdateInternalDisplay(
    const DisplayConfigurator::DisplayStateList& display_states) {
  bool force_first_display_internal = ForceFirstDisplayInternal();

  for (auto* state : display_states) {
    if (state->type() == DISPLAY_CONNECTION_TYPE_INTERNAL ||
        (force_first_display_internal &&
         (!HasInternalDisplay() || IsInternalDisplayId(state->display_id())))) {
      if (HasInternalDisplay())
        DCHECK_EQ(Display::InternalDisplayId(), state->display_id());
      SetInternalDisplayIds({state->display_id()});

      if (state->native_mode() &&
          (!display_manager_->IsDisplayIdValid(state->display_id()) ||
           !state->current_mode())) {
        // Register the internal display info if
        // 1) If it's not already registered. It'll be treated as
        // new display in |UpdateDisplaysWith()|.
        // 2) If it's not connected, because the display info will not
        // be updated in |UpdateDisplaysWith()|, which will skips the
        // disconnected displays.
        ManagedDisplayInfo new_info =
            CreateManagedDisplayInfo(state, state->native_mode());
        display_manager_->UpdateInternalDisplay(new_info);
      }
      return;
    }
  }
}

ManagedDisplayInfo DisplayChangeObserver::CreateManagedDisplayInfo(
    const DisplaySnapshot* snapshot,
    const DisplayMode* mode_info) {
  std::string name = (snapshot->type() == DISPLAY_CONNECTION_TYPE_INTERNAL)
                         ? l10n_util::GetStringUTF8(IDS_DISPLAY_NAME_INTERNAL)
                         : snapshot->display_name();

  if (name.empty())
    name = l10n_util::GetStringUTF8(IDS_DISPLAY_NAME_UNKNOWN);

  const bool has_overscan = snapshot->has_overscan();
  const int64_t id = snapshot->display_id();

  ManagedDisplayInfo new_info = ManagedDisplayInfo(id, name, has_overscan);

  if (snapshot->product_code() != DisplaySnapshot::kInvalidProductCode) {
    uint16_t manufacturer_id = 0;
    uint16_t product_id = 0;
    EdidParser::SplitProductCodeInManufacturerIdAndProductId(
        snapshot->product_code(), &manufacturer_id, &product_id);
    new_info.set_manufacturer_id(
        EdidParser::ManufacturerIdToString(manufacturer_id));
    new_info.set_product_id(EdidParser::ProductIdToString(product_id));
  }
  new_info.set_year_of_manufacture(snapshot->year_of_manufacture());

  new_info.set_panel_orientation(snapshot->panel_orientation());
  new_info.set_sys_path(snapshot->sys_path());
  new_info.set_from_native_platform(true);

  float device_scale_factor = 1.0f;
  // Sets dpi only if the screen size is valid.
  const float dpi = IsDisplaySizeValid(snapshot->physical_size())
                        ? kInchInMm * mode_info->size().width() /
                              snapshot->physical_size().width()
                        : 0;
  if (snapshot->type() == DISPLAY_CONNECTION_TYPE_INTERNAL) {
    new_info.set_native(true);
    device_scale_factor = FindDeviceScaleFactor(dpi, mode_info->size());
  } else {
    ManagedDisplayMode mode;
    if (display_manager_->GetSelectedModeForDisplayId(snapshot->display_id(),
                                                      &mode)) {
      device_scale_factor = mode.device_scale_factor();
      new_info.set_native(mode.native());
    }
  }
  new_info.set_device_scale_factor(device_scale_factor);

  const gfx::Rect display_bounds(snapshot->origin(), mode_info->size());
  new_info.SetBounds(display_bounds);
  new_info.set_is_aspect_preserving_scaling(
      snapshot->is_aspect_preserving_scaling());
  if (dpi)
    new_info.set_device_dpi(dpi);

#if !BUILDFLAG(IS_CHROMEOS_ASH)
  // TODO(crbug.com/1012846): This should configure the HDR color spaces.
  gfx::DisplayColorSpaces display_color_spaces(
      snapshot->color_space(), DisplaySnapshot::PrimaryFormat());
  new_info.set_display_color_spaces(display_color_spaces);
  new_info.set_bits_per_channel(snapshot->bits_per_channel());
#else
  // TODO(crbug.com/1012846): Remove kEnableUseHDRTransferFunction usage when
  // HDR is fully supported on ChromeOS.
  const bool allow_high_bit_depth =
      base::FeatureList::IsEnabled(features::kUseHDRTransferFunction);
  new_info.set_display_color_spaces(
      CreateDisplayColorSpaces(snapshot->color_space(), allow_high_bit_depth,
                               snapshot->hdr_static_metadata()));
  constexpr int32_t kNormalBitDepth = 8;
  new_info.set_bits_per_channel(
      allow_high_bit_depth ? snapshot->bits_per_channel() : kNormalBitDepth);
#endif

  new_info.set_refresh_rate(mode_info->refresh_rate());
  new_info.set_is_interlaced(mode_info->is_interlaced());

  ManagedDisplayInfo::ManagedDisplayModeList display_modes =
      (snapshot->type() == DISPLAY_CONNECTION_TYPE_INTERNAL)
          ? GetInternalManagedDisplayModeList(new_info, *snapshot)
          : GetExternalManagedDisplayModeList(*snapshot);
  new_info.SetManagedDisplayModes(display_modes);

  new_info.set_maximum_cursor_size(snapshot->maximum_cursor_size());
  // Temporary adding rounded corners to the internal display info.
  if (display::features::IsRoundedDisplayEnabled() &&
      snapshot->type() == DISPLAY_CONNECTION_TYPE_INTERNAL) {
    new_info.set_rounded_corners_radii(
        gfx::RoundedCornersF(kRoundedDisplayRadius));
  }
  return new_info;
}

// static
float DisplayChangeObserver::FindDeviceScaleFactor(
    float dpi,
    const gfx::Size& size_in_pixels) {
  // Nocturne has special scale factor 3000/1332=2.252.. for the panel 3kx2k.
  constexpr gfx::Size k225DisplaySizeHackNocturne(3000, 2000);
  // Keep the Chell's scale factor 2.252 until we make decision.
  constexpr gfx::Size k2DisplaySizeHackChell(3200, 1800);
  constexpr gfx::Size k18DisplaySizeHackCoachZ(2160, 1440);

  if (size_in_pixels == k225DisplaySizeHackNocturne) {
    return kDsf_2_252;
  } else if (size_in_pixels == k2DisplaySizeHackChell) {
    return 2.f;
  } else if (size_in_pixels == k18DisplaySizeHackCoachZ) {
    return kDsf_1_8;
  } else {
    for (size_t i = 0; i < std::size(kThresholdTableForInternal); ++i) {
      if (dpi >= kThresholdTableForInternal[i].dpi)
        return kThresholdTableForInternal[i].device_scale_factor;
    }
  }
  return 1.0f;
}

}  // namespace display
