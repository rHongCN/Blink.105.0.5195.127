// Copyright 2022 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ui/base/cocoa/nsmenu_additions.h"

#include "base/mac/scoped_nsobject.h"
#include "base/test/gtest_util.h"
#include "testing/gtest/include/gtest/gtest.h"

@interface NSMenuAdditionsUnitTestMenuItem : NSMenuItem
@end

@implementation NSMenuAdditionsUnitTestMenuItem {
  BOOL includeFunctionModifierInFlags_;
}

- (void)setKeyEquivalentModifierMask:(NSEventModifierFlags)mask {
  // The AppKit ignores NSEventModifierFlagFunction when it's included
  // in the mask. Note that it was included so we can fake it later.
  includeFunctionModifierInFlags_ = (mask & NSEventModifierFlagFunction) > 0;

  // Remove the flag to avoid a warning from the AppKit.
  mask &= ~NSEventModifierFlagFunction;

  [super setKeyEquivalentModifierMask:mask];
}

- (NSEventModifierFlags)keyEquivalentModifierMask {
  NSEventModifierFlags flags = [super keyEquivalentModifierMask];
  if (includeFunctionModifierInFlags_) {
    flags |= NSEventModifierFlagFunction;
  }
  return flags;
}

@end

namespace {

NSMenu* Menu(NSString* title) {
  NSMenu* menu = [[[NSMenu alloc] initWithTitle:title] autorelease];
  [menu setAutoenablesItems:NO];
  return menu;
}

NSMenuItem* MenuItem(NSString* title,
                     NSString* key_equivalent = @"",
                     NSEventModifierFlags modifier_mask = 0) {
  NSMenuAdditionsUnitTestMenuItem* item =
      [[[NSMenuAdditionsUnitTestMenuItem alloc] initWithTitle:title
                                                       action:NULL
                                                keyEquivalent:key_equivalent]
          autorelease];
  [item setKeyEquivalentModifierMask:modifier_mask];
  [item setEnabled:YES];
  return item;
}

NSEvent* KeyEvent(const NSEventModifierFlags modifierFlags,
                  NSString* chars,
                  NSString* charsNoMods = nil,
                  const NSUInteger keyCode = 0) {
  if (charsNoMods == nil)
    charsNoMods = chars;

  return [NSEvent keyEventWithType:NSEventTypeKeyDown
                          location:NSZeroPoint
                     modifierFlags:modifierFlags
                         timestamp:0.0
                      windowNumber:0
                           context:nil
                        characters:chars
       charactersIgnoringModifiers:charsNoMods
                         isARepeat:NO
                           keyCode:keyCode];
}

}  // namespace

TEST(NSMenuAdditionsTest, TestMenuItemForKeyEquivalentEvent) {
  NSMenu* main_menu = Menu(@"Main Menu");

  [main_menu addItem:MenuItem(@"App")];
  NSString* file_title = @"File";
  [main_menu addItem:MenuItem(file_title)];
  [main_menu addItem:MenuItem(@"Edit")];
  [main_menu addItem:MenuItem(@"Window")];
  [main_menu addItem:MenuItem(@"Help")];

  NSMenu* file_menu = Menu(file_title);
  NSMenuItem* file_menu_item = [main_menu itemWithTitle:[file_menu title]];
  [file_menu_item setSubmenu:file_menu];

  [file_menu addItem:MenuItem(@"New")];
  NSString* open_title = @"Open";
  [file_menu addItem:MenuItem(open_title, @"o", NSEventModifierFlagCommand)];
  NSString* open_recent_title = @"Open Recent";
  [file_menu addItem:MenuItem(open_recent_title)];
  NSString* close_all_title = @"Close All";
  [file_menu
      addItem:MenuItem(close_all_title, @"W", NSEventModifierFlagCommand)];

  NSMenu* open_recent_menu = Menu(open_recent_title);
  NSMenuItem* open_recent_menu_item =
      [file_menu itemWithTitle:[open_recent_menu title]];
  [open_recent_menu_item setSubmenu:open_recent_menu];

  [open_recent_menu addItem:MenuItem(@"Mock up")];
  [open_recent_menu addItem:MenuItem(@"Preview-1")];
  [open_recent_menu addItem:MenuItem(@"Scrtachpad")];
  [open_recent_menu addItem:[NSMenuItem separatorItem]];
  NSString* clear_menu_title = @"Clear Menu";
  [open_recent_menu
      addItem:MenuItem(clear_menu_title, @"c",
                       NSEventModifierFlagCommand | NSEventModifierFlagControl |
                           NSEventModifierFlagOption |
                           NSEventModifierFlagFunction)];

  NSEvent* event = KeyEvent(NSEventModifierFlagCommand, @"o");
  EXPECT_EQ([file_menu itemWithTitle:open_title],
            [main_menu cr_menuItemForKeyEquivalentEvent:event]);

  event = KeyEvent(NSEventModifierFlagCommand, @"W");
  EXPECT_EQ([file_menu itemWithTitle:close_all_title],
            [main_menu cr_menuItemForKeyEquivalentEvent:event]);

  event = KeyEvent(NSEventModifierFlagCommand | NSEventModifierFlagControl |
                       NSEventModifierFlagOption | NSEventModifierFlagFunction,
                   @"c");
  EXPECT_EQ([open_recent_menu itemWithTitle:clear_menu_title],
            [main_menu cr_menuItemForKeyEquivalentEvent:event]);

  event = KeyEvent(NSEventModifierFlagCommand, @"g");
  EXPECT_EQ(nil, [main_menu cr_menuItemForKeyEquivalentEvent:event]);
}

// Tests that a set pre-search block is executed during calls to
// -[NSMenu cr_menuItemForKeyEquivalentEvent:].
TEST(NSMenuAdditionsTest, TestPreSearchBlock) {
  __block bool block_was_called = false;

  [NSMenu cr_setMenuItemForKeyEquivalentEventPreSearchBlock:^{
    block_was_called = true;
  }];

  NSMenu* main_menu = Menu(@"Main Menu");
  NSEvent* event = KeyEvent(NSEventModifierFlagCommand, @"c");
  [main_menu cr_menuItemForKeyEquivalentEvent:event];

  EXPECT_TRUE(block_was_called);

  // Setting the block again should cause a crash (the API only supports a
  // single pre-search block).
  EXPECT_CHECK_DEATH(
      [NSMenu cr_setMenuItemForKeyEquivalentEventPreSearchBlock:^{
        block_was_called = true;
      }]);
}
