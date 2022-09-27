// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview <cr-auto-img> is a specialized <img> that facilitates embedding
 * images into WebUIs via its auto-src attribute. <cr-auto-img> automatically
 * determines if the image is local (e.g. data: or chrome://) or external (e.g.
 * https://), and embeds the image directly or via the chrome://image data
 * source accordingly. Usage:
 *
 *   1. In C++ register |SanitizedImageSource| for your WebUI.
 *
 *   2. In HTML instantiate
 *
 *      <img is="cr-auto-img" auto-src="https://foo.com/bar.png">
 *
 *      If your image URL points to Google Photos storage, meaning it needs an
 *      auth token to be downloaded, you can use the is-google-photos attribute
 *      as follows:
 *
 *      <img is="cr-auto-img" auto-src="https://foo.com/bar.png"
 *          is-google-photos>
 *
 *      If you want the image to reset to an empty state when auto-src changes
 *      and the new image is still loading, set the clear-src attribute:
 *
 *      <img is="cr-auto-img" auto-src="[[calculateSrc()]]" clear-src>
 *
 * NOTE: Since <cr-auto-img> may use the chrome://image data source some images
 * may be transcoded to PNG.
 */

const AUTO_SRC: string = 'auto-src';

const CLEAR_SRC: string = 'clear-src';

const IS_GOOGLE_PHOTOS: string = 'is-google-photos';

export class CrAutoImgElement extends HTMLImageElement {
  static get observedAttributes() {
    return [AUTO_SRC, IS_GOOGLE_PHOTOS];
  }

  attributeChangedCallback(
      name: string, oldValue: string|null, newValue: string|null) {
    if (name !== AUTO_SRC && name !== IS_GOOGLE_PHOTOS) {
      return;
    }

    // Changes to |IS_GOOGLE_PHOTOS| are only interesting when the attribute is
    // being added or removed.
    if (name === IS_GOOGLE_PHOTOS &&
        ((oldValue === null) === (newValue === null))) {
      return;
    }

    if (this.hasAttribute(CLEAR_SRC)) {
      // Remove the src attribute so that the old image is not shown while the
      // new one is loading.
      this.removeAttribute('src');
    }

    let url = null;
    try {
      url = new URL(this.getAttribute(AUTO_SRC) || '');
    } catch (_) {
    }

    if (!url || url.protocol === 'chrome-untrusted:') {
      // Loading chrome-untrusted:// directly kills the renderer process.
      // Loading chrome-untrusted:// via the chrome://image data source
      // results in a broken image.
      this.removeAttribute('src');
    } else if (url.protocol === 'data:' || url.protocol === 'chrome:') {
      this.src = url.href;
    } else if (this.hasAttribute(IS_GOOGLE_PHOTOS)) {
      this.src = `chrome://image?url=${
          encodeURIComponent(url.href)}&isGooglePhotos=true`;
    } else {
      this.src = 'chrome://image?' + url.href;
    }
  }

  set autoSrc(src: string) {
    this.setAttribute(AUTO_SRC, src);
  }

  get autoSrc(): string {
    return this.getAttribute(AUTO_SRC)!;
  }

  set clearSrc(_: string) {
    this.setAttribute(CLEAR_SRC, '');
  }

  get clearSrc(): string {
    return this.getAttribute(CLEAR_SRC)!;
  }

  set isGooglePhotos(enabled: boolean) {
    if (enabled) {
      this.setAttribute(IS_GOOGLE_PHOTOS, '');
    } else {
      this.removeAttribute(IS_GOOGLE_PHOTOS);
    }
  }

  get isGooglePhotos(): boolean {
    return this.hasAttribute(IS_GOOGLE_PHOTOS);
  }
}

customElements.define('cr-auto-img', CrAutoImgElement, {extends: 'img'});
