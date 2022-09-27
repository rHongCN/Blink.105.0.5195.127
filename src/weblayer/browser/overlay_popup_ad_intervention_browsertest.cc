// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>

#include "base/test/scoped_feature_list.h"
#include "build/build_config.h"
#include "components/page_load_metrics/browser/observers/ad_metrics/ad_intervention_browser_test_utils.h"
#include "components/page_load_metrics/browser/page_load_metrics_test_waiter.h"
#include "components/subresource_filter/content/browser/content_subresource_filter_throttle_manager.h"
#include "components/subresource_filter/core/browser/subresource_filter_features.h"
#include "components/subresource_filter/core/common/common_features.h"
#include "components/subresource_filter/core/common/test_ruleset_utils.h"
#include "components/subresource_filter/core/mojom/subresource_filter.mojom.h"
#include "components/ukm/test_ukm_recorder.h"
#include "content/public/test/browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/test_navigation_observer.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/features.h"
#include "url/gurl.h"
#include "weblayer/test/subresource_filter_browser_test_harness.h"

namespace weblayer {

namespace {

const char kAdsInterventionRecordedHistogram[] =
    "SubresourceFilter.PageLoad.AdsInterventionTriggered";

}  // namespace

class OverlayPopupAdViolationBrowserTest : public SubresourceFilterBrowserTest {
 public:
  OverlayPopupAdViolationBrowserTest() = default;

  void SetUp() override {
    std::vector<base::Feature> enabled = {
        subresource_filter::kAdTagging,
        subresource_filter::kAdsInterventionsEnforced};
    std::vector<base::Feature> disabled = {
        blink::features::kFrequencyCappingForOverlayPopupDetection};

    feature_list_.InitWithFeatures(enabled, disabled);
    SubresourceFilterBrowserTest::SetUp();
  }

  void SetUpOnMainThread() override {
    SubresourceFilterBrowserTest::SetUpOnMainThread();
    SetRulesetWithRules(
        {subresource_filter::testing::CreateSuffixRule("ad_iframe_writer.js")});
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

IN_PROC_BROWSER_TEST_F(OverlayPopupAdViolationBrowserTest,
                       NoOverlayPopupAd_AdInterventionNotTriggered) {
  base::HistogramTester histogram_tester;

  GURL url = embedded_test_server()->GetURL(
      "a.com", "/ads_observer/large_scrollable_page_with_adiframe_writer.html");

  page_load_metrics::NavigateAndWaitForFirstMeaningfulPaint(web_contents(),
                                                            url);

  // Reload the page. Since we haven't seen any ad violations, expect that the
  // ad script is loaded and that the subresource filter UI doesn't show up.
  EXPECT_TRUE(content::NavigateToURL(web_contents(), url));

  EXPECT_TRUE(
      WasParsedScriptElementLoaded(web_contents()->GetPrimaryMainFrame()));
  histogram_tester.ExpectBucketCount(
      "SubresourceFilter.Actions2",
      subresource_filter::SubresourceFilterAction::kUIShown, 0);
  histogram_tester.ExpectBucketCount(
      kAdsInterventionRecordedHistogram,
      subresource_filter::mojom::AdsViolation::kOverlayPopupAd, 0);
}

// TODO(https://crbug.com/1287783): Fails on the linux and android.
#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_ANDROID)
#define MAYBE_OverlayPopupAd_AdInterventionTriggered \
  DISABLED_OverlayPopupAd_AdInterventionTriggered
#else
#define MAYBE_OverlayPopupAd_AdInterventionTriggered \
  OverlayPopupAd_AdInterventionTriggered
#endif
IN_PROC_BROWSER_TEST_F(OverlayPopupAdViolationBrowserTest,
                       MAYBE_OverlayPopupAd_AdInterventionTriggered) {
  base::HistogramTester histogram_tester;

  GURL url = embedded_test_server()->GetURL(
      "a.com", "/ads_observer/large_scrollable_page_with_adiframe_writer.html");

  page_load_metrics::NavigateAndWaitForFirstMeaningfulPaint(web_contents(),
                                                            url);

  page_load_metrics::TriggerAndDetectOverlayPopupAd(web_contents());

  // Reload the page. Since we are enforcing ad blocking on ads violations,
  // expect that the ad script is not loaded and that the subresource filter UI
  // shows up.
  EXPECT_TRUE(content::NavigateToURL(web_contents(), url));

  EXPECT_FALSE(
      WasParsedScriptElementLoaded(web_contents()->GetPrimaryMainFrame()));
  histogram_tester.ExpectBucketCount(
      "SubresourceFilter.Actions2",
      subresource_filter::SubresourceFilterAction::kUIShown, 1);
  histogram_tester.ExpectBucketCount(
      kAdsInterventionRecordedHistogram,
      subresource_filter::mojom::AdsViolation::kOverlayPopupAd, 1);
}

class OverlayPopupAdViolationBrowserTestWithoutEnforcement
    : public OverlayPopupAdViolationBrowserTest {
 public:
  OverlayPopupAdViolationBrowserTestWithoutEnforcement() = default;

  void SetUp() override {
    std::vector<base::Feature> enabled = {subresource_filter::kAdTagging};
    std::vector<base::Feature> disabled = {
        subresource_filter::kAdsInterventionsEnforced,
        blink::features::kFrequencyCappingForOverlayPopupDetection};

    feature_list_.InitWithFeatures(enabled, disabled);
    SubresourceFilterBrowserTest::SetUp();
  }

 private:
  base::test::ScopedFeatureList feature_list_;
};

// TODO(https://crbug.com/1344280): Test is flaky.
IN_PROC_BROWSER_TEST_F(OverlayPopupAdViolationBrowserTestWithoutEnforcement,
                       DISABLED_OverlayPopupAd_NoAdInterventionTriggered) {
  base::HistogramTester histogram_tester;

  GURL url = embedded_test_server()->GetURL(
      "a.com", "/ads_observer/large_scrollable_page_with_adiframe_writer.html");

  page_load_metrics::NavigateAndWaitForFirstMeaningfulPaint(web_contents(),
                                                            url);

  page_load_metrics::TriggerAndDetectOverlayPopupAd(web_contents());

  // Reload the page. Since we are not enforcing ad blocking on ads violations,
  // expect that the ad script is loaded and that the subresource filter UI
  // doesn't show up. Expect a histogram recording as the intervention is
  // running in dry run mode.
  EXPECT_TRUE(content::NavigateToURL(web_contents(), url));

  EXPECT_TRUE(
      WasParsedScriptElementLoaded(web_contents()->GetPrimaryMainFrame()));
  histogram_tester.ExpectBucketCount(
      "SubresourceFilter.Actions2",
      subresource_filter::SubresourceFilterAction::kUIShown, 0);
  histogram_tester.ExpectBucketCount(
      kAdsInterventionRecordedHistogram,
      subresource_filter::mojom::AdsViolation::kOverlayPopupAd, 1);
}

}  // namespace weblayer
