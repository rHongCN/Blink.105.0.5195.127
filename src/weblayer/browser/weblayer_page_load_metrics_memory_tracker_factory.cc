// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "weblayer/browser/weblayer_page_load_metrics_memory_tracker_factory.h"

#include "base/memory/singleton.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/keyed_service/content/browser_context_keyed_service_factory.h"
#include "components/page_load_metrics/browser/page_load_metrics_memory_tracker.h"

namespace weblayer {

page_load_metrics::PageLoadMetricsMemoryTracker*
WeblayerPageLoadMetricsMemoryTrackerFactory::GetForBrowserContext(
    content::BrowserContext* context) {
  return static_cast<page_load_metrics::PageLoadMetricsMemoryTracker*>(
      GetInstance()->GetServiceForBrowserContext(context, true));
}

WeblayerPageLoadMetricsMemoryTrackerFactory*
WeblayerPageLoadMetricsMemoryTrackerFactory::GetInstance() {
  return base::Singleton<WeblayerPageLoadMetricsMemoryTrackerFactory>::get();
}

WeblayerPageLoadMetricsMemoryTrackerFactory::
    WeblayerPageLoadMetricsMemoryTrackerFactory()
    : BrowserContextKeyedServiceFactory(
          "PageLoadMetricsMemoryTracker",
          BrowserContextDependencyManager::GetInstance()) {}

bool WeblayerPageLoadMetricsMemoryTrackerFactory::
    ServiceIsCreatedWithBrowserContext() const {
  return base::FeatureList::IsEnabled(features::kV8PerFrameMemoryMonitoring);
}

KeyedService*
WeblayerPageLoadMetricsMemoryTrackerFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  return new page_load_metrics::PageLoadMetricsMemoryTracker();
}

content::BrowserContext*
WeblayerPageLoadMetricsMemoryTrackerFactory::GetBrowserContextToUse(
    content::BrowserContext* context) const {
  return context;
}

}  // namespace weblayer
