// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * UI element which shows a loading spinner.
 */

import 'chrome://resources/polymer/v3_0/paper-spinner/paper-spinner-lite.js';

import './bluetooth_base_page.js';
import {html, PolymerElement} from '//resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {ButtonBarState, ButtonState} from './bluetooth_types.js';

/** @polymer */
export class SettingsBluetoothSpinnerPageElement extends PolymerElement {
  static get is() {
    return 'bluetooth-spinner-page';
  }

  static get template() {
    return html`{__html_template__}`;
  }

  static get properties() {
    return {
      /** @private {!ButtonBarState} */
      buttonBarState_: {
        type: Object,
        value: {
          cancel: ButtonState.ENABLED,
          pair: ButtonState.DISABLED,
        },
      },
    };
  }
}

customElements.define(
    SettingsBluetoothSpinnerPageElement.is,
    SettingsBluetoothSpinnerPageElement);
