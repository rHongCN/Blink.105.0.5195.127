// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gtk/gtk_ui_platform_stub.h"

#include "base/callback.h"

namespace gtk {

GtkUiPlatformStub::GtkUiPlatformStub() = default;

GtkUiPlatformStub::~GtkUiPlatformStub() = default;

void GtkUiPlatformStub::OnInitialized(GtkWidget* widget) {}

GdkKeymap* GtkUiPlatformStub::GetGdkKeymap() {
  return nullptr;
}

GdkModifierType GtkUiPlatformStub::GetGdkKeyEventState(
    const ui::KeyEvent& key_event) {
  return static_cast<GdkModifierType>(0);
}

int GtkUiPlatformStub::GetGdkKeyEventGroup(const ui::KeyEvent& key_event) {
  return 0;
}

GdkWindow* GtkUiPlatformStub::GetGdkWindow(gfx::AcceleratedWidget window_id) {
  return nullptr;
}

bool GtkUiPlatformStub::SetGtkWidgetTransientFor(
    GtkWidget* widget,
    gfx::AcceleratedWidget parent) {
  return false;
}

void GtkUiPlatformStub::ClearTransientFor(gfx::AcceleratedWidget parent) {}

void GtkUiPlatformStub::ShowGtkWindow(GtkWindow* window) {
  gtk_window_present(window);
}

}  // namespace gtk
