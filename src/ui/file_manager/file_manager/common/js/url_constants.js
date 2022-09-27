// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @const {string} */
export const LEGACY_FILES_EXTENSION_ID = 'hhaomjibdihmijegdhdafkllkbggdgoj';

/**
 * App ID generated by the SWA framework.
 * @const {string}
 */
export const SWA_APP_ID = 'fkiggjmkendpmbegkagpmagjepfkpmeb';

/** @const {string} */
export const SWA_FILES_APP_HOST = 'file-manager';

/**
 * The URL of the legacy version of File Manager.
 * @const {!URL}
 */
export const LEGACY_FILES_APP_URL =
    new URL(`chrome-extension://${LEGACY_FILES_EXTENSION_ID}`);

/**
 * The URL of the System Web App version of File Manager.
 * @const {!URL}
 */
export const SWA_FILES_APP_URL = new URL(`chrome://${SWA_FILES_APP_HOST}`);

/**
 * The path to the File Manager icon.
 * @const {string}
 */
export const FILES_APP_ICON_PATH = 'common/images/icon96.png';

/**
 * @param {string=} path relative to the Files app root.
 * @return {!URL} The absolute URL for a path within the Files app.
 */
export function toFilesAppURL(path = '') {
  return new URL(path, window.isSWA ? SWA_FILES_APP_URL : LEGACY_FILES_APP_URL);
}

/**
 * @param {string=} path relative to the sandboxed page origin.
 * @return {!URL} The absolute URL.
 */
export function toSandboxedURL(path = '') {
  const SANDBOXED_URL = window.isSWA ?
      new URL(`chrome-untrusted://${SWA_FILES_APP_HOST}`) :
      LEGACY_FILES_APP_URL;
  return new URL(path, SANDBOXED_URL);
}

/**
 * @return {!URL} The URL of the file that holds Files App icon.
 */
export function getFilesAppIconURL() {
  return toFilesAppURL(FILES_APP_ICON_PATH);
}
