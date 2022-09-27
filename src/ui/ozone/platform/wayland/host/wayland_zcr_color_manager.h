// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_WAYLAND_HOST_WAYLAND_ZCR_COLOR_MANAGER_H_
#define UI_OZONE_PLATFORM_WAYLAND_HOST_WAYLAND_ZCR_COLOR_MANAGER_H_

#include "base/memory/raw_ptr.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "ui/ozone/platform/wayland/common/wayland_object.h"
#include "ui/ozone/platform/wayland/common/wayland_util.h"

struct zcr_color_manager_v1;

namespace ui {

class WaylandConnection;

// Wrapper around |zcr_color_manager_v1| Wayland factory
class WaylandZcrColorManager
    : public wl::GlobalObjectRegistrar<WaylandZcrColorManager> {
 public:
  static constexpr char kInterfaceName[] = "zcr_color_manager_v1";

  static void Instantiate(WaylandConnection* connection,
                          wl_registry* registry,
                          uint32_t name,
                          const std::string& interface,
                          uint32_t version);

  WaylandZcrColorManager(zcr_color_manager_v1* zcr_color_manager_,
                         WaylandConnection* connection);

  WaylandZcrColorManager(const WaylandZcrColorManager&) = delete;
  WaylandZcrColorManager& operator=(const WaylandZcrColorManager&) = delete;

  ~WaylandZcrColorManager();

  wl::Object<zcr_color_management_output_v1> CreateColorManagementOutput(
      wl_output* output);

 private:
  // Holds pointer to the zcr_color_manager_v1 Wayland factory.
  const wl::Object<zcr_color_manager_v1> zcr_color_manager_;

  // Non-owned.
  const raw_ptr<WaylandConnection> connection_;
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_WAYLAND_HOST_WAYLAND_ZCR_COLOR_MANAGER_H_
