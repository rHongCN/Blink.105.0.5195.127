// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {assertArrayEquals, assertEquals, assertGT, assertLT, assertTrue} from 'chrome://webui-test/chai_assert.js';

import {FileOperationProgressEvent} from '../../common/js/file_operation_common.js';
import {installMockChrome} from '../../common/js/mock_chrome.js';
import {ProgressItemState} from '../../common/js/progress_center_common.js';
import {util} from '../../common/js/util.js';

import {FileOperationHandler} from './file_operation_handler.js';
import {fileOperationUtil} from './file_operation_util.js';
import {MockFileOperationManager} from './mock_file_operation_manager.js';
import {MockProgressCenter} from './mock_progress_center.js';

/** @type {!MockFileOperationManager} */
let fileOperationManager;

/** @type {!MockProgressCenter} */
let progressCenter;

/** @type {!FileOperationHandler} */
let fileOperationHandler;

/**
 * Mock chrome APIs.
 * @type {Object}
 */
const mockChrome = {};

mockChrome.fileManagerPrivate = {
  onIOTaskProgressStatus: {
    addListener: function(callback) {},
  },
};

/**
 * Mock JS Date.
 *
 * The stop() method should be called to restore original Date.
 */
class MockDate {
  constructor() {
    this.originalNow = Date.now;
    Date.tick_ = 0;
    Date.now = this.now;
  }

  /**
   * Increases current timestamp.
   *
   * @param {number} msec Milliseconds to add to the current timestamp.
   */
  tick(msec) {
    Date.tick_ += msec;
  }

  /**
   * @returns {number} Current timestamp of the mock object.
   */
  now() {
    return Date.tick_;
  }

  /**
   * Restore original Date.now method.
   */
  stop() {
    Date.now = this.originalNow;
  }
}

// Set up the test components.
export function setUp() {
  // Mock LoadTimeData strings.
  window.loadTimeData.resetForTesting({
    COPY_FILE_NAME: 'Copying $1...',
    COPY_TARGET_EXISTS_ERROR: '$1 is already exists.',
    COPY_FILESYSTEM_ERROR: 'Copy filesystem error: $1',
    FILE_ERROR_GENERIC: 'File error generic.',
    COPY_UNEXPECTED_ERROR: 'Copy unexpected error: $1',
  });

  // Install mock chrome APIs.
  installMockChrome(mockChrome);

  // Create mock items needed for FileOperationHandler.
  fileOperationManager = new MockFileOperationManager();
  progressCenter = new MockProgressCenter();

  // Create FileOperationHandler. Note: the file operation handler is
  // required, but not used directly, by the unittests.
  fileOperationHandler =
      new FileOperationHandler(fileOperationManager, progressCenter);
}

/**
 * Tests copy success.
 */
export function testCopySuccess() {
  // Dispatch copy event.
  fileOperationManager.dispatchEvent(
      /** @type {!Event} */ (Object.assign(new Event('copy-progress'), {
        taskId: 'TASK_ID',
        reason: FileOperationProgressEvent.EventType.BEGIN,
        status: {
          operationType: 'COPY',
          numRemainingItems: 1,
          processingEntryName: 'sample.txt',
          totalBytes: 200,
          processedBytes: 0,
        },
      })));

  // Check the updated item.
  let item = progressCenter.items['TASK_ID'];
  assertEquals(ProgressItemState.PROGRESSING, item.state);
  assertEquals('TASK_ID', item.id);
  assertEquals('Copying sample.txt...', item.message);
  assertEquals('copy', item.type);
  assertEquals(true, item.single);
  assertEquals(0, item.progressRateInPercent);

  // Dispatch success event.
  fileOperationManager.dispatchEvent(
      /** @type {!Event} */ (Object.assign(new Event('copy-progress'), {
        taskId: 'TASK_ID',
        reason: FileOperationProgressEvent.EventType.SUCCESS,
        status: {
          operationType: 'COPY',
        },
      })));

  // Check the item completed.
  item = progressCenter.items['TASK_ID'];
  assertEquals(ProgressItemState.COMPLETED, item.state);
  assertEquals('TASK_ID', item.id);
  assertEquals('', item.message);
  assertEquals('copy', item.type);
  assertEquals(true, item.single);
  assertEquals(100, item.progressRateInPercent);
}

/**
 * Tests copy cancel.
 */
export function testCopyCancel() {
  // Dispatch copy event.
  fileOperationManager.dispatchEvent(
      /** @type {!Event} */ (Object.assign(new Event('copy-progress'), {
        taskId: 'TASK_ID',
        reason: FileOperationProgressEvent.EventType.BEGIN,
        status: {
          operationType: 'COPY',
          numRemainingItems: 1,
          processingEntryName: 'sample.txt',
          totalBytes: 200,
          processedBytes: 0,
        },
      })));

  // Check the updated item.
  let item = progressCenter.items['TASK_ID'];
  assertEquals(ProgressItemState.PROGRESSING, item.state);
  assertEquals('Copying sample.txt...', item.message);
  assertEquals('copy', item.type);
  assertEquals(true, item.single);
  assertEquals(0, item.progressRateInPercent);

  // Setup cancel event.
  fileOperationManager.cancelEvent =
      /** @type {!Event} */ (Object.assign(new Event('copy-progress'), {
        taskId: 'TASK_ID',
        reason: FileOperationProgressEvent.EventType.CANCELED,
        status: {
          operationType: 'COPY',
        },
      }));

  // Dispatch cancel event.
  assertTrue(item.cancelable);
  item.cancelCallback();

  // Check the item cancelled.
  item = progressCenter.items['TASK_ID'];
  assertEquals(ProgressItemState.CANCELED, item.state);
  assertEquals('', item.message);
  assertEquals('copy', item.type);
  assertEquals(true, item.single);
  assertEquals(0, item.progressRateInPercent);
}

/**
 * Tests target already exists error.
 */
export function testCopyTargetExistsError() {
  // Dispatch error event.
  fileOperationManager.dispatchEvent(
      /** @type {!Event} */ (Object.assign(new Event('copy-progress'), {
        taskId: 'TASK_ID',
        reason: FileOperationProgressEvent.EventType.ERROR,
        status: {
          operationType: 'COPY',
        },
        error: {
          code: util.FileOperationErrorType.TARGET_EXISTS,
          data: {
            name: 'sample.txt',
          },
        },
      })));

  // Check the item errored.
  const item = progressCenter.items['TASK_ID'];
  assertEquals(ProgressItemState.ERROR, item.state);
  assertEquals('sample.txt is already exists.', item.message);
  assertEquals('copy', item.type);
  assertEquals(true, item.single);
  assertEquals(0, item.progressRateInPercent);
}

/**
 * Tests file system error.
 */
export function testCopyFileSystemError() {
  // Dispatch error event.
  fileOperationManager.dispatchEvent(
      /** @type {!Event} */ (Object.assign(new Event('copy-progress'), {
        taskId: 'TASK_ID',
        reason: FileOperationProgressEvent.EventType.ERROR,
        status: {
          operationType: 'COPY',
        },
        error: {
          code: util.FileOperationErrorType.FILESYSTEM_ERROR,
          data: {
            name: 'sample.txt',
          },
        },
      })));

  // Check the item errored.
  const item = progressCenter.items['TASK_ID'];
  assertEquals(ProgressItemState.ERROR, item.state);
  assertEquals('Copy filesystem error: File error generic.', item.message);
  assertEquals('copy', item.type);
  assertEquals(true, item.single);
  assertEquals(0, item.progressRateInPercent);
}

/**
 * Tests unexpected error.
 */
export function testCopyUnexpectedError() {
  // Dispatch error event.
  fileOperationManager.dispatchEvent(
      /** @type {!Event} */ (Object.assign(new Event('copy-progress'), {
        taskId: 'TASK_ID',
        reason: FileOperationProgressEvent.EventType.ERROR,
        status: {
          operationType: 'COPY',
        },
        error: {
          code: 'Unexpected',
          data: {
            name: 'sample.txt',
          },
        },
      })));

  // Check the item errored.
  const item = progressCenter.items['TASK_ID'];
  assertEquals(ProgressItemState.ERROR, item.state);
  assertEquals('Copy unexpected error: Unexpected', item.message);
  assertEquals('copy', item.type);
  assertEquals(true, item.single);
  assertEquals(0, item.progressRateInPercent);
}

/**
 * Tests Speedometer's speed calculations.
 */
export function testSpeedometerMovingAverage() {
  const speedometer = new fileOperationUtil.Speedometer();
  const mockDate = new MockDate();

  speedometer.setTotalBytes(2000);

  // Time elapsed before the first sample shouldn't have any impact.
  mockDate.tick(10000);

  assertEquals(0, speedometer.getSampleCount());
  assertTrue(isNaN(speedometer.getRemainingTime()));

  // 1st sample, t=0s.
  mockDate.tick(1000);
  speedometer.update(100);

  assertEquals(1, speedometer.getSampleCount());
  assertTrue(isNaN(speedometer.getRemainingTime()));

  // Sample received less than a second after the previous one should be
  // ignored.
  mockDate.tick(999);
  speedometer.update(300);

  assertEquals(1, speedometer.getSampleCount());
  assertTrue(isNaN(speedometer.getRemainingTime()));

  // From 2 samples, the average and the current speed can be computed.
  // 2nd sample, t=1s.
  mockDate.tick(1);
  speedometer.update(300);

  assertEquals(2, speedometer.getSampleCount());
  assertEquals(9, Math.round(speedometer.getRemainingTime()));

  // 3rd sample, t=2s.
  mockDate.tick(1000);
  speedometer.update(300);

  assertEquals(3, speedometer.getSampleCount());
  assertEquals(17, Math.round(speedometer.getRemainingTime()));

  // 4th sample, t=4s.
  mockDate.tick(2000);
  speedometer.update(300);

  assertEquals(4, speedometer.getSampleCount());
  assertEquals(42, Math.round(speedometer.getRemainingTime()));

  // 5th sample, t=5s.
  mockDate.tick(1000);
  speedometer.update(600);

  assertEquals(5, speedometer.getSampleCount());
  assertEquals(20, Math.round(speedometer.getRemainingTime()));

  // Elapsed time should be taken in account by getRemainingTime().
  mockDate.tick(12000);
  assertEquals(8, Math.round(speedometer.getRemainingTime()));

  // getRemainingTime() can return a negative value.
  mockDate.tick(12000);
  assertEquals(-4, Math.round(speedometer.getRemainingTime()));

  mockDate.stop();
}

/**
 * Tests Speedometer's sample queue.
 */
export function testSpeedometerBufferRing() {
  const maxSamples = 20;
  const speedometer = new fileOperationUtil.Speedometer(maxSamples);
  const mockDate = new MockDate();

  speedometer.setTotalBytes(20000);

  // Slow speed of 100 bytes per second.
  for (let i = 0; i < maxSamples; i++) {
    assertEquals(i, speedometer.getSampleCount());
    mockDate.tick(1000);
    speedometer.update(i * 100);
  }

  assertEquals(maxSamples, speedometer.getSampleCount());
  assertEquals(181, Math.round(speedometer.getRemainingTime()));

  // Faster speed of 300 bytes per second.
  for (let i = 0; i < maxSamples; i++) {
    // Check buffer not expanded more than the specified length.
    assertEquals(maxSamples, speedometer.getSampleCount());
    mockDate.tick(1000);
    speedometer.update(2100 + i * 300);

    // Current speed should be seen as accelerating, thus the remaining time
    // decreasing.
    assertGT(181, Math.round(speedometer.getRemainingTime()));
  }

  assertEquals(maxSamples, speedometer.getSampleCount());
  assertEquals(41, Math.round(speedometer.getRemainingTime()));

  // Stalling.
  for (let i = 0; i < maxSamples; i++) {
    // Check buffer not expanded more than the specified length.
    assertEquals(maxSamples, speedometer.getSampleCount());
    mockDate.tick(1000);
    speedometer.update(7965);
  }

  assertEquals(maxSamples, speedometer.getSampleCount());

  // getRemainingTime() can return an infinite value.
  assertEquals(Infinity, Math.round(speedometer.getRemainingTime()));

  mockDate.stop();
}
