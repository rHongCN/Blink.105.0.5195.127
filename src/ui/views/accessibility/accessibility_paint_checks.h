// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_VIEWS_ACCESSIBILITY_ACCESSIBILITY_PAINT_CHECKS_H_
#define UI_VIEWS_ACCESSIBILITY_ACCESSIBILITY_PAINT_CHECKS_H_

#include "ui/base/class_property.h"
#include "ui/views/views_export.h"

namespace views {

class View;

// This runs DCHECKs related to the view's state when painting. Generally, when
// a View is ready to be displayed to the user it should also be accessible.
void RunAccessibilityPaintChecks(View* view);

// Skip accessibility paint checks on a specific View.
// TODO(pbos): Remove this key. Do not add new uses to it, instead make sure
// that the offending View is fixed.
VIEWS_EXPORT extern const ui::ClassProperty<bool>* const
    kSkipAccessibilityPaintChecks;

}  // namespace views

#endif  // UI_VIEWS_ACCESSIBILITY_ACCESSIBILITY_PAINT_CHECKS_H_
