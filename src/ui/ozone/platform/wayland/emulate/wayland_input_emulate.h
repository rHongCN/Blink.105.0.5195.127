// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_WAYLAND_EMULATE_WAYLAND_INPUT_EMULATE_H_
#define UI_OZONE_PLATFORM_WAYLAND_EMULATE_WAYLAND_INPUT_EMULATE_H_

#include <memory>

#include <wayland-util.h>

#include "base/component_export.h"
#include "base/containers/circular_deque.h"
#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "ui/events/keycodes/dom/dom_code.h"
#include "ui/events/types/event_type.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/ozone/platform/wayland/host/proxy/wayland_proxy.h"

struct wl_buffer;
struct wl_registry;
struct weston_test;
struct wl_callback;

namespace wl {

// Emulates Keyboard, Pointer, and Touch events that ui_interactive_tests test
// suite sends. Mustn't be used in production code.
class WaylandInputEmulate : public wl::WaylandProxy::Delegate {
 public:
  // Notifies the observer about events sent by Wayland compositor.
  class Observer : public base::CheckedObserver {
   public:
    virtual void OnPointerMotionGlobal(const gfx::Point& screen_position) = 0;

    // Notifies Wayland compositor has sent |button| event that corresponds to
    // event codes in Linux's input-event-codes.h.
    virtual void OnPointerButtonGlobal(int32_t button, bool pressed) = 0;

    // Notifies Wayland compositor has sent |key| event that corresponds to
    // event codes in Linux's input-event-codes.h.
    virtual void OnKeyboardKey(int32_t key, bool pressed) = 0;

    // Notifies that the Wayland compositor has sent a touch event to
    // |screen_position|.
    virtual void OnTouchReceived(const gfx::Point& screen_position) = 0;

   protected:
    ~Observer() override = default;
  };

  WaylandInputEmulate();
  ~WaylandInputEmulate() override;

  void AddObserver(Observer* obs);
  void RemoveObserver(Observer* obs);
  void EmulatePointerMotion(gfx::AcceleratedWidget widget,
                            const gfx::Point& mouse_surface_loc,
                            const gfx::Point& mouse_screen_loc_in_px);
  void EmulatePointerButton(gfx::AcceleratedWidget widget,
                            ui::EventType event_type,
                            uint32_t changed_button);
  void EmulateKeyboardKey(gfx::AcceleratedWidget widget,
                          ui::EventType event_type,
                          ui::DomCode dom_code);
  void EmulateTouch(gfx::AcceleratedWidget widget,
                    ui::EventType event_type,
                    int id,
                    const gfx::Point& touch_screen_loc);

 private:
  // Pending emulated events. Can be ET_MOUSE_MOVED,
  // ET_MOUSE_PRESSED/ET_MOUSE_RELEASED, ET_KEY_PRESSED/ET_KEY_RELEASED, or
  // ET_TOUCH_PRESSED/ET_TOUCH_MOVED/ET_TOUCH_RELEASED.
  struct PendingEvent {
    PendingEvent(ui::EventType event_type,
                 gfx::AcceleratedWidget target_widget);
    ~PendingEvent();

    ui::EventType type;
    gfx::AcceleratedWidget widget;

    // Set for type == ET_MOUSE_MOVED. Locations are
    // in surface local, and pixel screen coordinates respectively.
    gfx::Point pointer_surface_location;
    gfx::Point pointer_screen_location_in_px;

    // Set for type == ET_TOUCH_*. Location is in dip screen coordinates.
    gfx::Point touch_screen_location;

    // Set for type == ET_MOUSE_PRESSED || type == ET_MOUSE_RELEASED.
    uint32_t mouse_button = 0;

    // Set for type == ET_KEY_PRESSED || type == ET_KEY_RELEASED.
    ui::DomCode key_dom_code = ui::DomCode::NONE;

    // Set for type == ET_TOUCH_*.
    int touch_id = 0;
  };

  // A container that tracks created WaylandWindows and keeps some fundamental
  // bits to make emulation work flawlessly.
  struct TestWindow {
    TestWindow(gfx::AcceleratedWidget target_widget,
               WaylandInputEmulate* emulate);
    ~TestWindow();

    // Widget that this TestWindow represents. This corresponds to a
    // WaylandWindow created on the Ozone/Wayland side.
    gfx::AcceleratedWidget widget;

    // Non-owned pointer to input emulation.
    raw_ptr<WaylandInputEmulate> emulate = nullptr;

    // Control flag that says if the buffer has been attached and a consequent
    // frame callback has been received. This is required to be able to know
    // that the surface has consumed the attached buffer and Wayland properly
    // set the size of the surface. Otherwise, the surface in question may not
    // receive any events. Set during WaylandInputEmulate::FrameCallbackHandler
    // call.
    bool buffer_attached_and_configured = false;

    // Pending events to be sent for this TestWindow. These are sent after
    // buffer_attached_and_configured is set to true.
    base::circular_deque<std::unique_ptr<PendingEvent>> pending_events;

    // Frame callback that invokes WaylandInputEmulate::FrameCallbackHandler.
    raw_ptr<struct wl_callback> frame_callback = nullptr;

    // The attached buffer.
    raw_ptr<wl_buffer> buffer = nullptr;
  };

  // WaylandProxy::Delegate.
  void OnWindowAdded(gfx::AcceleratedWidget widget) override;
  void OnWindowRemoved(gfx::AcceleratedWidget widget) override;
  void OnWindowConfigured(gfx::AcceleratedWidget widget,
                          bool is_configured) override;

  // weston_test_listener.
  static void HandlePointerPosition(void* data,
                                    struct weston_test* weston_test,
                                    wl_fixed_t x,
                                    wl_fixed_t y);
  static void HandlePointerButton(void* data,
                                  struct weston_test* weston_test,
                                  int32_t button,
                                  uint32_t state);
  static void HandleKeyboardKey(void* data,
                                struct weston_test* weston_test,
                                uint32_t key,
                                uint32_t state);
  static void HandleTouchReceived(void* data,
                                  struct weston_test* weston_test,
                                  wl_fixed_t x,
                                  wl_fixed_t y);

  // wl_registry_listener.
  static void Global(void* data,
                     wl_registry* registry,
                     uint32_t name,
                     const char* interface,
                     uint32_t version);

  // wl_callback_listener.
  static void FrameCallbackHandler(void* data,
                                   struct wl_callback* callback,
                                   uint32_t time);

  // Stores existing windows.
  base::flat_map<gfx::AcceleratedWidget,
                 std::unique_ptr<WaylandInputEmulate::TestWindow>>
      windows_;

  base::ObserverList<WaylandInputEmulate::Observer> observers_;

  // Owned raw pointers. wl::Object is not used because the component this
  // class belongs to cannot depend on the "wayland" target in the
  // //ui/ozone/platform/wayland/BUILD.gn
  raw_ptr<struct wl_registry> registry_ = nullptr;
  raw_ptr<struct weston_test> weston_test_ = nullptr;
};

}  // namespace wl

#endif  // UI_OZONE_PLATFORM_WAYLAND_EMULATE_WAYLAND_INPUT_EMULATE_H_
