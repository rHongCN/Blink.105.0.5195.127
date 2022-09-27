// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/display/mac/test/virtual_display_mac_util.h"

#include <CoreGraphics/CoreGraphics.h>
#import <Foundation/Foundation.h>

#include <map>

#include "base/auto_reset.h"
#include "base/check.h"
#include "base/check_op.h"
#include "base/mac/scoped_nsobject.h"
#include "build/build_config.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/size.h"

// These interfaces were generated from CoreGraphics binaries.
API_AVAILABLE(macos(10.14))
@interface CGVirtualDisplay : NSObject {
  unsigned int _vendorID;
  unsigned int _productID;
  unsigned int _serialNum;
  NSString* _name;
  struct CGSize _sizeInMillimeters;
  unsigned int _maxPixelsWide;
  unsigned int _maxPixelsHigh;
  struct CGPoint _redPrimary;
  struct CGPoint _greenPrimary;
  struct CGPoint _bluePrimary;
  struct CGPoint _whitePoint;
  id _queue;
  id _terminationHandler;
  unsigned int _displayID;
  unsigned int _hiDPI;
  NSArray* _modes;
}

@property(readonly, nonatomic)
    unsigned int vendorID;  // @synthesize vendorID=_vendorID;
@property(readonly, nonatomic)
    unsigned int productID;  // @synthesize productID=_productID;
@property(readonly, nonatomic)
    unsigned int serialNum;  // @synthesize serialNum=_serialNum;
@property(readonly, nonatomic) NSString* name;  // @synthesize name=_name;
@property(readonly, nonatomic) struct CGSize
    sizeInMillimeters;  // @synthesize sizeInMillimeters=_sizeInMillimeters;
@property(readonly, nonatomic)
    unsigned int maxPixelsWide;  // @synthesize maxPixelsWide=_maxPixelsWide;
@property(readonly, nonatomic)
    unsigned int maxPixelsHigh;  // @synthesize maxPixelsHigh=_maxPixelsHigh;
@property(readonly, nonatomic)
    struct CGPoint redPrimary;  // @synthesize redPrimary=_redPrimary;
@property(readonly, nonatomic)
    struct CGPoint greenPrimary;  // @synthesize greenPrimary=_greenPrimary;
@property(readonly, nonatomic)
    struct CGPoint bluePrimary;  // @synthesize bluePrimary=_bluePrimary;
@property(readonly, nonatomic)
    struct CGPoint whitePoint;            // @synthesize whitePoint=_whitePoint;
@property(readonly, nonatomic) id queue;  // @synthesize queue=_queue;
@property(readonly, nonatomic) id
    terminationHandler;  // @synthesize terminationHandler=_terminationHandler;
@property(readonly, nonatomic)
    unsigned int displayID;  // @synthesize displayID=_displayID;
@property(readonly, nonatomic) unsigned int hiDPI;  // @synthesize hiDPI=_hiDPI;
@property(readonly, nonatomic) NSArray* modes;      // @synthesize modes=_modes;
- (BOOL)applySettings:(id)arg1;
- (void)dealloc;
- (id)initWithDescriptor:(id)arg1;

@end

// These interfaces were generated from CoreGraphics binaries.
API_AVAILABLE(macos(10.14))
@interface CGVirtualDisplayDescriptor : NSObject {
  unsigned int _vendorID;
  unsigned int _productID;
  unsigned int _serialNum;
  NSString* _name;
  struct CGSize _sizeInMillimeters;
  unsigned int _maxPixelsWide;
  unsigned int _maxPixelsHigh;
  struct CGPoint _redPrimary;
  struct CGPoint _greenPrimary;
  struct CGPoint _bluePrimary;
  struct CGPoint _whitePoint;
  id _queue;
  id _terminationHandler;
}

@property(nonatomic) unsigned int vendorID;  // @synthesize vendorID=_vendorID;
@property(nonatomic)
    unsigned int productID;  // @synthesize productID=_productID;
@property(nonatomic)
    unsigned int serialNum;  // @synthesize serialNum=_serialNum;
@property(strong, nonatomic) NSString* name;  // @synthesize name=_name;
@property(nonatomic) struct CGSize
    sizeInMillimeters;  // @synthesize sizeInMillimeters=_sizeInMillimeters;
@property(nonatomic)
    unsigned int maxPixelsWide;  // @synthesize maxPixelsWide=_maxPixelsWide;
@property(nonatomic)
    unsigned int maxPixelsHigh;  // @synthesize maxPixelsHigh=_maxPixelsHigh;
@property(nonatomic)
    struct CGPoint redPrimary;  // @synthesize redPrimary=_redPrimary;
@property(nonatomic)
    struct CGPoint greenPrimary;  // @synthesize greenPrimary=_greenPrimary;
@property(nonatomic)
    struct CGPoint bluePrimary;  // @synthesize bluePrimary=_bluePrimary;
@property(nonatomic)
    struct CGPoint whitePoint;          // @synthesize whitePoint=_whitePoint;
@property(retain, nonatomic) id queue;  // @synthesize queue=_queue;
@property(copy, nonatomic) id
    terminationHandler;  // @synthesize terminationHandler=_terminationHandler;
- (void)dealloc;
- (id)init;
- (id)dispatchQueue;
- (void)setDispatchQueue:(id)arg1;

@end

// These interfaces were generated from CoreGraphics binaries.
API_AVAILABLE(macos(10.14))
@interface CGVirtualDisplayMode : NSObject {
  unsigned int _width;
  unsigned int _height;
  double _refreshRate;
}

@property(readonly, nonatomic) unsigned int width;  // @synthesize width=_width;
@property(readonly, nonatomic)
    unsigned int height;  // @synthesize height=_height;
@property(readonly, nonatomic)
    double refreshRate;  // @synthesize refreshRate=_refreshRate;
- (id)initWithWidth:(unsigned int)arg1
             height:(unsigned int)arg2
        refreshRate:(double)arg3;

@end

// These interfaces were generated from CoreGraphics binaries.
API_AVAILABLE(macos(10.14))
@interface CGVirtualDisplaySettings : NSObject {
  NSArray* _modes;
  unsigned int _hiDPI;
}

@property(strong, nonatomic) NSArray* modes;  // @synthesize modes=_modes;
@property(nonatomic) unsigned int hiDPI;      // @synthesize hiDPI=_hiDPI;
- (void)dealloc;
- (id)init;

@end

namespace {

static bool g_during_warm_up = false;

base::flat_set<int64_t> g_unexpected_display_ids;

// A global singleton that tracks the current set of mocked displays.
std::map<int64_t, base::scoped_nsobject<CGVirtualDisplay>> g_display_map
    API_AVAILABLE(macos(10.14));

// A helper function for creating virtual display and return CGVirtualDisplay
// object.
base::scoped_nsobject<CGVirtualDisplay> CreateVirtualDisplay(int width,
                                                             int height,
                                                             int ppi,
                                                             BOOL hiDPI,
                                                             NSString* name)
    API_AVAILABLE(macos(10.14)) {
  base::scoped_nsobject<CGVirtualDisplaySettings> settings(
      [[CGVirtualDisplaySettings alloc] init]);
  [settings setHiDPI:hiDPI];

  base::scoped_nsobject<CGVirtualDisplayDescriptor> descriptor(
      [[CGVirtualDisplayDescriptor alloc] init]);
  [descriptor
      setQueue:dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0)];
  [descriptor setName:name];

  // See System Preferences > Displays > Color > Open Profile > Apple display
  // native information
  [descriptor setWhitePoint:CGPointMake(0.3125, 0.3291)];
  [descriptor setBluePrimary:CGPointMake(0.1494, 0.0557)];
  [descriptor setGreenPrimary:CGPointMake(0.2559, 0.6983)];
  [descriptor setRedPrimary:CGPointMake(0.6797, 0.3203)];
  [descriptor setMaxPixelsHigh:height];
  [descriptor setMaxPixelsWide:width];
  [descriptor
      setSizeInMillimeters:CGSizeMake(25.4 * width / ppi, 25.4 * height / ppi)];
  [descriptor setSerialNum:0];
  [descriptor setProductID:0];
  [descriptor setVendorID:0];

  base::scoped_nsobject<CGVirtualDisplay> display(
      [[CGVirtualDisplay alloc] initWithDescriptor:descriptor]);

  if ([settings hiDPI]) {
    width /= 2;
    height /= 2;
  }
  base::scoped_nsobject<CGVirtualDisplayMode> mode([[CGVirtualDisplayMode alloc]
      initWithWidth:width
             height:height
        refreshRate:60]);
  [settings setModes:@[ mode ]];

  if (![display applySettings:settings])
    return base::scoped_nsobject<CGVirtualDisplay>();

  return display;
}

void DealWithUnexpectedDisplay(int64_t id) {
  if (g_unexpected_display_ids.count(id)) {
    g_unexpected_display_ids.erase(id);
  } else {
    g_unexpected_display_ids.insert(id);
  }
}

// This method detects whether the local machine is running headless.
// Typically returns true when the session is curtained or if there are no
// physical monitors attached.  In those two scenarios, the online display will
// be marked as virtual.
bool IsRunningHeadless() {
  // Most machines will have < 4 displays but a larger upper bound won't hurt.
  constexpr UInt32 kMaxDisplaysToQuery = 32;
  // 0x76697274 is a 4CC value for 'virt' which indicates the display is
  // virtual.
  constexpr CGDirectDisplayID kVirtualDisplayID = 0x76697274;

  CGDirectDisplayID online_displays[kMaxDisplaysToQuery];
  UInt32 online_display_count = 0;
  CGError return_code = CGGetOnlineDisplayList(
      kMaxDisplaysToQuery, online_displays, &online_display_count);
  if (return_code != kCGErrorSuccess) {
    LOG(ERROR) << __func__
               << " - CGGetOnlineDisplayList() failed: " << return_code << ".";
    // If this fails, assume machine is headless to err on the side of caution.
    return true;
  }

  bool is_running_headless = true;
  for (UInt32 i = 0; i < online_display_count; i++) {
    if (CGDisplayModelNumber(online_displays[i]) != kVirtualDisplayID) {
      // At least one monitor is attached so the machine is not headless.
      is_running_headless = false;
      break;
    }
  }

  // TODO(crbug.com/1126278): Please remove this log or replace it with
  // [D]CHECK() ASAP when the TEST is stable.
  LOG(INFO) << __func__ << " - Is running headless: " << is_running_headless
            << ". Online display count: " << online_display_count << ".";

  return is_running_headless;
}

}  // namespace

namespace display::test {

struct DisplayParams {
  DisplayParams(int width,
                int height,
                int ppi,
                bool hiDPI,
                std::string description)
      : width(width),
        height(height),
        ppi(ppi),
        hiDPI(hiDPI),
        description([NSString
            stringWithCString:description.c_str()
                     encoding:[NSString defaultCStringEncoding]]) {}

  bool IsValid() const {
    return width > 0 && height > 0 && ppi > 0 && [description length] > 0;
  }

  int width;
  int height;
  int ppi;
  BOOL hiDPI;
  base::scoped_nsobject<NSString> description;
};

VirtualDisplayMacUtil::VirtualDisplayMacUtil() {
  display::Screen::GetScreen()->AddObserver(this);
}

VirtualDisplayMacUtil::~VirtualDisplayMacUtil() {
  RemoveAllDisplays();
  display::Screen::GetScreen()->RemoveObserver(this);
}

void VirtualDisplayMacUtil::WarmUp() {
  constexpr int kMaxRetryCount = 4;

  // TODO(crbug.com/1126278): Please remove this log or replace it with
  // [D]CHECK() ASAP when the TEST is stable.
  LOG(INFO) << "VirtualDisplayMacUtil::" << __func__ << " - start.";

  base::AutoReset<bool> auto_reset(&g_during_warm_up, true);

  int retry_count = 0;
  do {
    for (int64_t display_id : {1, 2}) {
      AddDisplay(display_id, k1920x1080);
    }
    RemoveAllDisplays();

    retry_count++;
  } while (!g_unexpected_display_ids.empty() && retry_count <= kMaxRetryCount);

  // TODO(crbug.com/1126278): Please remove this log or replace it with
  // [D]CHECK() ASAP when the TEST is stable.
  LOG(INFO) << "VirtualDisplayMacUtil::" << __func__ << " - end.";
}

int64_t VirtualDisplayMacUtil::AddDisplay(int64_t display_id,
                                          const DisplayParams& display_params) {
  if (@available(macos 10.14, *)) {
    bool need_warm_up = NeedWarmUp();

    DCHECK(display_params.IsValid());

    NSString* display_name =
        [NSString stringWithFormat:@"Virtual Display #%lld", display_id];
    base::scoped_nsobject<CGVirtualDisplay> display = CreateVirtualDisplay(
        display_params.width, display_params.height, display_params.ppi,
        display_params.hiDPI, display_name);
    DCHECK(display.get());

    // TODO(crbug.com/1126278): Please remove this log or replace it with
    // [D]CHECK() ASAP when the TEST is stable.
    LOG(INFO) << "VirtualDisplayMacUtil::" << __func__
              << " - display id: " << display_id
              << ". CreateVirtualDisplay success.";

    int64_t id = [display displayID];
    DCHECK_NE(id, 0u);

    WaitForDisplay(id, /*added=*/true);

    // TODO(crbug.com/1126278): Please remove this log or replace it with
    // [D]CHECK() ASAP when the TEST is stable.
    LOG(INFO) << "VirtualDisplayMacUtil::" << __func__
              << " - display id: " << display_id << "(" << id
              << "). WaitForDisplay success.";

    DCHECK(!g_display_map[id]);
    g_display_map[id] = display;

    if (need_warm_up) {
      int64_t tmp_id = AddDisplay(0, display_params);
      RemoveDisplay(tmp_id);
    }

    return id;
  }
  return display::kInvalidDisplayId;
}

void VirtualDisplayMacUtil::RemoveDisplay(int64_t display_id) {
  if (@available(macos 10.14, *)) {
    auto it = g_display_map.find(display_id);
    DCHECK(it != g_display_map.end());

    g_display_map.erase(it);

    // TODO(crbug.com/1126278): Please remove this log or replace it with
    // [D]CHECK() ASAP when the TEST is stable.
    LOG(INFO) << "VirtualDisplayMacUtil::" << __func__
              << " - display id: " << display_id << ". Erase success.";

    WaitForDisplay(display_id, /*added=*/false);

    // TODO(crbug.com/1126278): Please remove this log or replace it with
    // [D]CHECK() ASAP when the TEST is stable.
    LOG(INFO) << "VirtualDisplayMacUtil::" << __func__
              << " - display id: " << display_id << ". WaitForDisplay success.";
  }
}

// static
bool VirtualDisplayMacUtil::IsAPIAvailable() {
  bool is_arch_cpu_arm_family = false;
#if defined(ARCH_CPU_ARM_FAMILY)
  is_arch_cpu_arm_family = true;
#endif  // defined(ARCH_CPU_ARM_FAMILY)
  if (is_arch_cpu_arm_family) {
    return false;
  }

  bool is_api_available = false;
  if (@available(macos 12.0, *)) {
    is_api_available = true;
  }
  if (!is_api_available) {
    return false;
  }

  // Only support non-headless bots now. Some odd behavior happened when
  // enable on headless bots. See
  // https://ci.chromium.org/ui/p/chromium/builders/try/mac-rel/1058024/test-results
  // for details.
  // TODO(crbug.com/1126278): Remove this restriction when support headless
  // environment.
  if (IsRunningHeadless()) {
    return false;
  }

  return true;
}

// Predefined display configurations from
// https://en.wikipedia.org/wiki/Graphics_display_resolution and
// https://www.theverge.com/tldr/2016/3/21/11278192/apple-iphone-ipad-screen-sizes-pixels-density-so-many-choices.
const DisplayParams VirtualDisplayMacUtil::k6016x3384 =
    DisplayParams(6016, 3384, 218, true, "Apple Pro Display XDR");
const DisplayParams VirtualDisplayMacUtil::k5120x2880 =
    DisplayParams(5120, 2880, 218, true, "27-inch iMac with Retina 5K display");
const DisplayParams VirtualDisplayMacUtil::k4096x2304 =
    DisplayParams(4096,
                  2304,
                  219,
                  true,
                  "21.5-inch iMac with Retina 4K display");
const DisplayParams VirtualDisplayMacUtil::k3840x2400 =
    DisplayParams(3840, 2400, 200, true, "WQUXGA");
const DisplayParams VirtualDisplayMacUtil::k3840x2160 =
    DisplayParams(3840, 2160, 200, true, "UHD");
const DisplayParams VirtualDisplayMacUtil::k3840x1600 =
    DisplayParams(3840, 1600, 200, true, "WQHD+, UW-QHD+");
const DisplayParams VirtualDisplayMacUtil::k3840x1080 =
    DisplayParams(3840, 1080, 200, true, "DFHD");
const DisplayParams VirtualDisplayMacUtil::k3072x1920 =
    DisplayParams(3072,
                  1920,
                  226,
                  true,
                  "16-inch MacBook Pro with Retina display");
const DisplayParams VirtualDisplayMacUtil::k2880x1800 =
    DisplayParams(2880,
                  1800,
                  220,
                  true,
                  "15.4-inch MacBook Pro with Retina display");
const DisplayParams VirtualDisplayMacUtil::k2560x1600 =
    DisplayParams(2560,
                  1600,
                  227,
                  true,
                  "WQXGA, 13.3-inch MacBook Pro with Retina display");
const DisplayParams VirtualDisplayMacUtil::k2560x1440 =
    DisplayParams(2560, 1440, 109, false, "27-inch Apple Thunderbolt display");
const DisplayParams VirtualDisplayMacUtil::k2304x1440 =
    DisplayParams(2304, 1440, 226, true, "12-inch MacBook with Retina display");
const DisplayParams VirtualDisplayMacUtil::k2048x1536 =
    DisplayParams(2048, 1536, 150, false, "QXGA");
const DisplayParams VirtualDisplayMacUtil::k2048x1152 =
    DisplayParams(2048, 1152, 150, false, "QWXGA");
const DisplayParams VirtualDisplayMacUtil::k1920x1200 =
    DisplayParams(1920, 1200, 150, false, "WUXGA");
const DisplayParams VirtualDisplayMacUtil::k1600x1200 =
    DisplayParams(1600, 1200, 125, false, "UXGA");
const DisplayParams VirtualDisplayMacUtil::k1920x1080 =
    DisplayParams(1920, 1080, 102, false, "HD, 21.5-inch iMac");
const DisplayParams VirtualDisplayMacUtil::k1680x1050 =
    DisplayParams(1680,
                  1050,
                  99,
                  false,
                  "WSXGA+, Apple Cinema Display (20-inch), 20-inch iMac");
const DisplayParams VirtualDisplayMacUtil::k1440x900 =
    DisplayParams(1440, 900, 127, false, "WXGA+, 13.3-inch MacBook Air");
const DisplayParams VirtualDisplayMacUtil::k1400x1050 =
    DisplayParams(1400, 1050, 125, false, "SXGA+");
const DisplayParams VirtualDisplayMacUtil::k1366x768 =
    DisplayParams(1366, 768, 135, false, "11.6-inch MacBook Air");
const DisplayParams VirtualDisplayMacUtil::k1280x1024 =
    DisplayParams(1280, 1024, 100, false, "SXGA");
const DisplayParams VirtualDisplayMacUtil::k1280x1800 =
    DisplayParams(1280, 800, 113, false, "13.3-inch MacBook Pro");

VirtualDisplayMacUtil::DisplaySleepBlocker::DisplaySleepBlocker() {
  IOReturn result = IOPMAssertionCreateWithName(
      kIOPMAssertionTypeNoDisplaySleep, kIOPMAssertionLevelOn,
      CFSTR("DisplaySleepBlocker"), &assertion_id_);
  DCHECK_EQ(result, kIOReturnSuccess);
}

VirtualDisplayMacUtil::DisplaySleepBlocker::~DisplaySleepBlocker() {
  IOReturn result = IOPMAssertionRelease(assertion_id_);
  DCHECK_EQ(result, kIOReturnSuccess);
}

void VirtualDisplayMacUtil::OnDisplayMetricsChanged(
    const display::Display& display,
    uint32_t changed_metrics) {
  LOG(INFO) << "VirtualDisplayMacUtil::" << __func__
            << " - display id: " << display.id()
            << ", changed_metrics: " << changed_metrics << ".";
}

void VirtualDisplayMacUtil::OnDisplayAdded(
    const display::Display& new_display) {
  // TODO(crbug.com/1126278): Please remove this log or replace it with
  // [D]CHECK() ASAP when the TEST is stable.
  LOG(INFO) << "VirtualDisplayMacUtil::" << __func__
            << " - display id: " << new_display.id() << ".";

  OnDisplayAddedOrRemoved(new_display.id());
}

void VirtualDisplayMacUtil::OnDisplayRemoved(
    const display::Display& old_display) {
  // TODO(crbug.com/1126278): Please remove this log or replace it with
  // [D]CHECK() ASAP when the TEST is stable.
  LOG(INFO) << "VirtualDisplayMacUtil::" << __func__
            << " - display id: " << old_display.id() << ".";

  OnDisplayAddedOrRemoved(old_display.id());
}

void VirtualDisplayMacUtil::OnDisplayAddedOrRemoved(int64_t id) {
  if (!waiting_for_ids_.count(id)) {
    DealWithUnexpectedDisplay(id);
    return;
  }

  waiting_for_ids_.erase(id);
  if (!waiting_for_ids_.empty()) {
    return;
  }

  run_loop_->Quit();
}

void VirtualDisplayMacUtil::RemoveAllDisplays() {
  if (@available(macos 10.14, *)) {
    int display_count = g_display_map.size();

    // TODO(crbug.com/1126278): Please remove this log or replace it with
    // [D]CHECK() ASAP when the TEST is stable.
    LOG(INFO) << "VirtualDisplayMacUtil::" << __func__
              << " - display count: " << display_count << ".";

    if (!display_count) {
      return;
    }

    auto iter = g_display_map.begin();
    while (iter != g_display_map.end()) {
      waiting_for_ids_.insert(iter->first);
      iter++;
    }

    g_display_map.clear();

    run_loop_ = std::make_unique<base::RunLoop>();
    run_loop_->Run();
  }
}

void VirtualDisplayMacUtil::WaitForDisplay(int64_t id, bool added) {
  display::Display d;
  if (display::Screen::GetScreen()->GetDisplayWithDisplayId(id, &d) == added) {
    return;
  }

  // TODO(crbug.com/1126278): Please remove this log or replace it with
  // [D]CHECK() ASAP when the TEST is stable.
  LOG(INFO) << "VirtualDisplayMacUtil::" << __func__ << " - display id: " << id
            << "(added: " << added << "). Start waiting.";

  waiting_for_ids_.insert(id);

  run_loop_ = std::make_unique<base::RunLoop>();
  run_loop_->Run();
}

bool VirtualDisplayMacUtil::NeedWarmUp() {
  if (g_during_warm_up) {
    return false;
  }

  if (@available(macos 10.14, *)) {
    return g_display_map.empty();
  }

  return false;
}

}  // namespace display::test
