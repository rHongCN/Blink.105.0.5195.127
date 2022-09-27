// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/compositor/presentation_time_recorder.h"

#include "base/run_loop.h"
#include "base/test/bind.h"
#include "base/test/metrics/histogram_tester.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/compositor/compositor.h"
#include "ui/compositor/layer.h"
#include "ui/compositor/test/test_compositor_host.h"
#include "ui/compositor/test/test_context_factories.h"
#include "ui/compositor/test/test_utils.h"
#include "ui/gfx/presentation_feedback.h"

namespace ui {

class PresentationTimeRecorderTest : public testing::Test {
 protected:
  void SetUp() override {
    context_factories_ = std::make_unique<TestContextFactories>(false);
    const gfx::Rect bounds(100, 100);
    host_.reset(TestCompositorHost::Create(
        bounds, context_factories_->GetContextFactory()));
    host_->Show();
    host_->GetCompositor()->SetRootLayer(&root_);
  }

  void TearDown() override {
    host_.reset();
    context_factories_.reset();
  }

  base::test::TaskEnvironment task_environment_{
      base::test::TaskEnvironment::MainThreadType::UI};

  Layer root_;
  std::unique_ptr<TestContextFactories> context_factories_;
  std::unique_ptr<TestCompositorHost> host_;
};

constexpr char kName[] = "Histogram";
constexpr char kMaxLatencyName[] = "MaxLatency.Histogram";

TEST_F(PresentationTimeRecorderTest, Histogram) {
  base::HistogramTester histogram_tester;

  auto* compositor = host_->GetCompositor();
  auto test_recorder = CreatePresentationTimeHistogramRecorder(
      compositor, kName, kMaxLatencyName);
  // Flush pending draw callbask by waiting for presentation until it times out.
  // We assume if the new frame wasn't generated for 100ms (6 frames worth
  // time) there is no pending draw request.
  while (ui::WaitForNextFrameToBePresented(compositor, base::Milliseconds(100)))
    ;

  compositor->ScheduleFullRedraw();
  histogram_tester.ExpectTotalCount(kName, 0);
  test_recorder->RequestNext();
  histogram_tester.ExpectTotalCount(kName, 0);
  test_recorder->RequestNext();
  histogram_tester.ExpectTotalCount(kName, 0);
  histogram_tester.ExpectTotalCount(kMaxLatencyName, 0);

  EXPECT_TRUE(ui::WaitForNextFrameToBePresented(compositor));
  histogram_tester.ExpectTotalCount(kName, 1);
  histogram_tester.ExpectTotalCount(kMaxLatencyName, 0);

  compositor->ScheduleFullRedraw();
  EXPECT_TRUE(ui::WaitForNextFrameToBePresented(compositor));
  histogram_tester.ExpectTotalCount(kName, 1);
  histogram_tester.ExpectTotalCount(kMaxLatencyName, 0);

  test_recorder->RequestNext();
  compositor->ScheduleFullRedraw();
  EXPECT_TRUE(ui::WaitForNextFrameToBePresented(compositor));
  histogram_tester.ExpectTotalCount(kName, 2);
  histogram_tester.ExpectTotalCount(kMaxLatencyName, 0);

  // Drawing without RequestNext should not affect histogram.
  compositor->ScheduleFullRedraw();
  EXPECT_TRUE(ui::WaitForNextFrameToBePresented(compositor));
  histogram_tester.ExpectTotalCount(kName, 2);
  histogram_tester.ExpectTotalCount(kMaxLatencyName, 0);

  // Make sure the max latency is recorded upon deletion.
  test_recorder.reset();
  histogram_tester.ExpectTotalCount(kName, 2);
  histogram_tester.ExpectTotalCount(kMaxLatencyName, 1);
}

TEST_F(PresentationTimeRecorderTest, NoSuccessNoHistogram) {
  base::HistogramTester histogram_tester;
  auto* compositor = host_->GetCompositor();
  auto test_recorder = CreatePresentationTimeHistogramRecorder(
      compositor, kName, kMaxLatencyName);
  PresentationTimeRecorder::TestApi test_api(test_recorder.get());
  base::TimeDelta interval_not_used = base::Milliseconds(0);
  gfx::PresentationFeedback failure(
      base::TimeTicks() + base::Milliseconds(2000), interval_not_used,
      gfx::PresentationFeedback::kFailure);
  base::TimeTicks start = base::TimeTicks() + base::Milliseconds(1000);
  test_recorder->RequestNext();
  test_api.OnPresented(0, start, failure);

  test_recorder.reset();
  histogram_tester.ExpectTotalCount(kName, 0);
  histogram_tester.ExpectTotalCount(kMaxLatencyName, 0);
}

TEST_F(PresentationTimeRecorderTest, DelayedHistogram) {
  base::HistogramTester histogram_tester;
  auto* compositor = host_->GetCompositor();
  auto test_recorder = CreatePresentationTimeHistogramRecorder(
      compositor, kName, kMaxLatencyName);
  test_recorder->RequestNext();

  // Delete the recorder while waiting for the presentation callback.
  test_recorder.reset();
  histogram_tester.ExpectTotalCount(kName, 0);
  histogram_tester.ExpectTotalCount(kMaxLatencyName, 0);

  // Draw next frame and make sure the histgoram is recorded.
  compositor->ScheduleFullRedraw();
  EXPECT_TRUE(ui::WaitForNextFrameToBePresented(compositor));
  histogram_tester.ExpectTotalCount(kName, 1);
  histogram_tester.ExpectTotalCount(kMaxLatencyName, 1);
}

TEST_F(PresentationTimeRecorderTest, Failure) {
  auto* compositor = host_->GetCompositor();
  auto test_recorder = CreatePresentationTimeHistogramRecorder(
      compositor, kName, kMaxLatencyName);
  PresentationTimeRecorder::TestApi test_api(test_recorder.get());
  test_recorder->RequestNext();
  test_api.OnCompositingDidCommit(compositor);
  base::TimeDelta interval_not_used = base::Milliseconds(0);
  base::TimeTicks start = base::TimeTicks() + base::Milliseconds(1000);
  gfx::PresentationFeedback success(
      base::TimeTicks() + base::Milliseconds(1100), interval_not_used,
      /*flags=*/0);
  test_api.OnPresented(0, start, success);
  EXPECT_EQ(100, test_api.GetMaxLatencyMs());
  EXPECT_EQ(1, test_api.GetSuccessCount());
  gfx::PresentationFeedback failure(
      base::TimeTicks() + base::Milliseconds(2000), interval_not_used,
      gfx::PresentationFeedback::kFailure);
  test_api.OnPresented(0, start, failure);
  // Failure should not be included in max latency.
  EXPECT_EQ(100, test_api.GetMaxLatencyMs());
  EXPECT_EQ(50, test_api.GetFailureRatio());
}

}  // namespace ui
