// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "weblayer/common/features.h"

namespace weblayer {
namespace features {

// Weblayer features in alphabetical order.

// Client side phishing detection support for weblayer
const base::Feature kWebLayerClientSidePhishingDetection{
    "WebLayerClientSidePhishingDetection", base::FEATURE_DISABLED_BY_DEFAULT};

// Safebrowsing support for weblayer.
const base::Feature kWebLayerSafeBrowsing{"WebLayerSafeBrowsing",
                                          base::FEATURE_ENABLED_BY_DEFAULT};

}  // namespace features
}  // namespace weblayer
