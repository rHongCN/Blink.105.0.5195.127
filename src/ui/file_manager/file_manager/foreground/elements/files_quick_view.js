// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'chrome://resources/cr_elements/cr_button/cr_button.m.js';
import 'chrome://resources/polymer/v3_0/iron-icon/iron-icon.js';
import './files_icon_button.js';
import './files_metadata_box.js';
import './files_safe_media.js';
import './files_tooltip.js';
import './icons.js';

import {assert} from 'chrome://resources/js/assert.m.js';
import {html, Polymer} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {toSandboxedURL} from '../../common/js/url_constants.js';

export const FilesQuickView = Polymer({
  _template: html`{__html_template__}`,

  is: 'files-quick-view',

  properties: {
    /**
     * True if the Quick View is used in legacy mode.
     * @type {boolean}
     */
    isLegacy: Boolean,

    // File media type, e.g. image, video.
    type: String,
    subtype: String,
    filePath: String,

    // True if there is a file task that can open the file type.
    hasTask: Boolean,

    /**
     * True if the entry shown in Quick View can be deleted.
     * @type {boolean}
     */
    canDelete: Boolean,

    // Preview content to be sent rendered in a sandboxed environment.
    sourceContent: {
      type: Object,
      observer: 'refreshUntrustedIframe_',
    },

    videoPoster: String,
    audioArtwork: String,

    // Autoplay property for audio, video.
    autoplay: Boolean,

    // True if this file is not image, audio, video nor HTML but is supported
    // by Chrome - content that is directly preview-able in Chrome by setting
    // the <webview> src attribute. Examples: pdf, text.
    browsable: Boolean,

    // The metadata-box-active-changed event is fired on attribute change.
    metadataBoxActive: {
      value: true,
      type: Boolean,
      notify: true,
    },

    // Text shown when there is no playback/preview available.
    noPlaybackText: String,
    noPreviewText: String,

    /**
     * True if the Files app window is a dialog, e.g. save-as or open-with.
     * @type {boolean}
     */
    isModal: Boolean,
  },

  listeners: {
    'files-safe-media-tap-inside': 'tapInside',
    'files-safe-media-tap-outside': 'close',
    'files-safe-media-load-error': 'loaderror',
  },

  /**
   * Generate URL for browsable files (Text/PDF).
   */
  generateBrowsableUrl_: function(sourceContent, subtype) {
    let contentUrl = '';
    switch (sourceContent.dataType) {
      case 'url':
        contentUrl = sourceContent.data;
        break;
      case 'blob':
        contentUrl = URL.createObjectURL(sourceContent.data);
        break;
      default:
        return contentUrl;
    }

    if (subtype === 'PDF') {
      // contentUrls are only modified for PDFs, see getQuickViewParameters_ in
      // quick_view_controller.js.
      contentUrl += '#view=FitH';
    }

    return contentUrl;
  },

  /**
   * Apply custom CSS to autogenerated webview content.
   *
   * @param {!Event} e
   */
  applyTextCss: function(e) {
    // Don't override the Chrome PDF viewer's CSS: crbug.com/1001034.
    if (this.subtype === 'PDF') {
      return;
    }

    const webview = /** @type {WebView} */ (e.target);
    webview.insertCSS({
      'file': 'untrusted_resources/files_text_content.css',
    });
  },

  /**
   * Send browsable preview content (i.e. content that can be displayed by the
   * browser directly as PDF/text/html) to the chrome-untrusted:// <iframe>.
   */
  refreshUntrustedIframe_: function() {
    if (this.isLegacy || !this.browsable) {
      return;
    }

    const iframe = this.shadowRoot.querySelector('#untrusted');
    if (!iframe) {
      return;
    }

    const data = {
      browsable: this.browsable,
      subtype: this.subtype,
      sourceContent: this.sourceContent,
    };
    iframe.contentWindow.postMessage(data, toSandboxedURL().origin);
  },

  // Clears fields.
  clear: function() {
    this.setProperties({
      type: '',
      subtype: '',
      filePath: '',
      hasTask: false,
      canDelete: false,
      sourceContent: {
        data: null,
        dataType: '',
      },
      videoPoster: '',
      audioArtwork: '',
      autoplay: false,
      browsable: false,
    });

    const video = this.$.contentPanel.querySelector('#videoSafeMedia');
    if (video) {
      video.src = '';
      video.fire('src-changed');
    }

    this.removeAttribute('load-error');
  },

  // Handle load error from the files-safe-media container.
  loaderror: function() {
    this.setAttribute('load-error', '');
    this.sourceContent = {
      data: null,
      dataType: '',
    };
  },

  /** @return {boolean} */
  isOpened: function() {
    return this.$.dialog.open;
  },

  // Opens the dialog.
  open: function() {
    if (!this.isOpened()) {
      this.$.dialog.showModal();
      // Make dialog focusable and set focus to a dialog. This is how we can
      // prevent default behaviour of a dialog which by default sets focus to
      // the first input inside itself. When a dialog gains focus we remove
      // focusability to prevent selecting dialog when moving with a keyboard.
      this.$.dialog.setAttribute('tabindex', '0');
      this.$.dialog.focus();
      this.$.dialog.setAttribute('tabindex', '-1');
    }
  },

  // Closes the dialog.
  close: function() {
    if (this.isOpened()) {
      this.$.dialog.close();
    }
  },

  tapInside: function(e) {
    if (this.type === 'image') {
      const dialog = this.shadowRoot.querySelector('#dialog');
      dialog.focus();
    }
  },

  /**
   * @return {!FilesMetadataBoxElement}
   */
  getFilesMetadataBox: function() {
    return /** @type {!FilesMetadataBoxElement} */ (this.$['metadata-box']);
  },

  /**
   * Client should assign the function to open the file.
   *
   * @param {!Event} event
   */
  onOpenInNewButtonTap: function(event) {},

  /**
   * @param {boolean} hasTask
   * @param {boolean} isModal
   * @return {boolean}
   *
   * @private
   */
  shouldShowOpenButton_: function(hasTask, isModal) {
    return hasTask && !isModal;
  },

  /**
   * Client should assign the function to delete the file.
   *
   * @param {!Event} event
   */
  onDeleteButtonTap: function(event) {},

  /**
   * @param {boolean} canDelete
   * @param {boolean} isModal
   * @return {boolean}
   *
   * @private
   */
  shouldShowDeleteButton_: function(canDelete, isModal) {
    return canDelete && !isModal;
  },

  /**
   * See the changes on crbug.com/641587, but crbug.com/779044#c11 later undid
   * that work. So the focus remains on the metadata button when clicked after
   * the crbug.com/779044 "ghost focus" fix.
   *
   * crbug.com/641587 mentions a different UI behavior, that was wanted to fix
   * that bug. TODO(files-ng): UX to resolve the correct behavior needed here.
   *
   * @param {!Event} event tap event.
   *
   * @private
   */
  onMetadataButtonTap_: function(event) {
    if (this.hasAttribute('files-ng')) {
      this.metadataBoxActive = !this.metadataBoxActive;
    }
  },

  /**
   * Close Quick View unless the clicked target or its ancestor contains
   * 'no-close-on-click' class.
   *
   * @param {!Event} event tap event.
   *
   * @private
   */
  onContentPanelTap_: function(event) {
    let target = event.detail.sourceEvent.target;
    while (target) {
      if (target.classList.contains('no-close-on-click')) {
        return;
      }
      target = target.parentElement;
    }
    this.close();
  },

  /**
   * @param {Object} sourceContent
   * @return {boolean}
   *
   * @private
   */
  hasContent_: function(sourceContent) {
    return sourceContent.dataType !== '';
  },

  /**
   * @param {string} type
   * @param {string} subtype
   * @return {boolean}
   *
   * @private
   */
  isHtml_: function(type, subtype) {
    return type === 'document' && subtype === 'HTML';
  },

  /**
   * @param {string} type
   * @return {boolean}
   *
   * @private
   */
  isImage_: function(type) {
    return type === 'image';
  },

  /**
   * @param {string} type
   * @return {boolean}
   *
   * @private
   */
  isVideo_: function(type) {
    return type === 'video';
  },

  /**
   * @param {string} type
   * @return {boolean}
   *
   * @private
   */
  isAudio_: function(type) {
    return type === 'audio';
  },

  /**
   * @param {Object} sourceContent
   * @param {string} type
   * @return {Object}
   *
   * @private
   */
  audioContent_: function(sourceContent, type) {
    if (this.isAudio_(type)) {
      return sourceContent;
    }
    return {
      data: null,
      dataType: '',
    };
  },

  /**
   * @param {string} type
   * @return {boolean}
   *
   * @private
   */
  isUnsupported_: function(type, subtype, browsable) {
    return !this.isImage_(type) && !this.isVideo_(type) &&
        !this.isAudio_(type) && !this.isHtml_(type, subtype) && !browsable;
  },

  /** @private */
  onDialogClose_: function(e) {
    assert(e.target === this.$.dialog);

    this.clear();

    // Catch and re-fire the 'close' event such that it bubbles across Shadow
    // DOM v1.
    this.fire('close');
  },
});

//# sourceURL=//ui/file_manager/file_manager/foreground/elements/files_quick_view.js
