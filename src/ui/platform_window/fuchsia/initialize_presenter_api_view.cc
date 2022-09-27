// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
#include "ui/platform_window/fuchsia/initialize_presenter_api_view.h"

#include <fuchsia/ui/policy/cpp/fidl.h>
#include <lib/sys/cpp/component_context.h>
#include <lib/ui/scenic/cpp/view_ref_pair.h>
#include <lib/ui/scenic/cpp/view_token_pair.h>

#include <utility>

#include "base/fuchsia/fuchsia_logging.h"
#include "base/fuchsia/process_context.h"
#include "base/no_destructor.h"

namespace ui {
namespace fuchsia {
namespace {

ScenicPresentViewCallback& GetScenicViewPresenterInternal() {
  static base::NoDestructor<ScenicPresentViewCallback> view_presenter;
  return *view_presenter;
}

FlatlandPresentViewCallback& GetFlatlandViewPresenterInternal() {
  static base::NoDestructor<FlatlandPresentViewCallback> view_presenter;
  return *view_presenter;
}

}  // namespace

void InitializeViewTokenAndPresentView(
    ui::PlatformWindowInitProperties* window_properties_out) {
  DCHECK(window_properties_out);

  // Generate ViewToken and ViewHolderToken for the new view.
  auto view_tokens = scenic::ViewTokenPair::New();
  window_properties_out->view_token = std::move(view_tokens.view_token);

  // Create a ViewRefPair so the view can be registered to the SemanticsManager.
  window_properties_out->view_ref_pair = scenic::ViewRefPair::New();

  // Request Presenter to show the view full-screen.
  auto presenter = base::ComponentContextForProcess()
                       ->svc()
                       ->Connect<::fuchsia::ui::policy::Presenter>();

  presenter->PresentOrReplaceView(std::move(view_tokens.view_holder_token),
                                  nullptr);
}

void SetScenicViewPresenter(ScenicPresentViewCallback view_presenter) {
  GetScenicViewPresenterInternal() = std::move(view_presenter);
}

const ScenicPresentViewCallback& GetScenicViewPresenter() {
  return GetScenicViewPresenterInternal();
}

void SetFlatlandViewPresenter(FlatlandPresentViewCallback view_presenter) {
  GetFlatlandViewPresenterInternal() = std::move(view_presenter);
}

const FlatlandPresentViewCallback& GetFlatlandViewPresenter() {
  return GetFlatlandViewPresenterInternal();
}

void IgnorePresentCallsForTest() {
  SetScenicViewPresenter(
      base::BindRepeating([](::fuchsia::ui::views::ViewHolderToken view_holder,
                             ::fuchsia::ui::views::ViewRef view_ref)
                              -> ::fuchsia::element::ViewControllerPtr {
        DCHECK(view_holder.value);
        DCHECK(view_ref.reference);
        DVLOG(1) << "Present call ignored for test.";
        return nullptr;
      }));
  SetFlatlandViewPresenter(base::BindRepeating(
      [](::fuchsia::ui::views::ViewportCreationToken viewport_creation_token)
          -> ::fuchsia::element::ViewControllerPtr {
        DCHECK(viewport_creation_token.value);
        DVLOG(1) << "Present call ignored for test.";
        return nullptr;
      }));
}

}  // namespace fuchsia
}  // namespace ui
