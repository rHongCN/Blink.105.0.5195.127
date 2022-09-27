// Copyright 2020 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview 'certificate-provisioning-entry' is an element that displays
 * one certificate provisioning processes.
 */
import '../../cr_elements/cr_action_menu/cr_action_menu.js';
import '../../cr_elements/cr_icon_button/cr_icon_button.m.js';
import '../../cr_elements/cr_lazy_render/cr_lazy_render.m.js';
import 'chrome://resources/polymer/v3_0/iron-flex-layout/iron-flex-layout-classes.js';
import './certificate_shared.css.js';

import {PolymerElement} from 'chrome://resources/polymer/v3_0/polymer/polymer_bundled.min.js';

import {CrActionMenuElement} from '../../cr_elements/cr_action_menu/cr_action_menu.js';
import {CrLazyRenderElement} from '../../cr_elements/cr_lazy_render/cr_lazy_render.m.js';
import {I18nMixin} from '../../js/i18n_mixin.js';

import {CertificateProvisioningViewDetailsActionEvent} from './certificate_manager_types.js';
import {CertificateProvisioningProcess} from './certificate_provisioning_browser_proxy.js';
import {getTemplate} from './certificate_provisioning_entry.html.js';

export interface CertificateProvisioningEntryElement {
  $: {
    dots: HTMLElement,
    menu: CrLazyRenderElement<CrActionMenuElement>,
  };
}

const CertificateProvisioningEntryElementBase = I18nMixin(PolymerElement);

export class CertificateProvisioningEntryElement extends
    CertificateProvisioningEntryElementBase {
  static get is() {
    return 'certificate-provisioning-entry';
  }

  static get template() {
    return getTemplate();
  }

  static get properties() {
    return {
      model: Object,
    };
  }

  model: CertificateProvisioningProcess;

  private closePopupMenu_() {
    this.shadowRoot!.querySelector('cr-action-menu')!.close();
  }

  private onDotsClick_() {
    this.$.menu.get().showAt(this.$.dots);
  }

  private onDetailsClick_() {
    this.closePopupMenu_();
    this.dispatchEvent(
        new CustomEvent(CertificateProvisioningViewDetailsActionEvent, {
          bubbles: true,
          composed: true,
          detail: {
            model: this.model,
            anchor: this.$.dots,
          },
        }));
  }
}

declare global {
  interface HTMLElementTagNameMap {
    'certificate-provisioning-entry': CertificateProvisioningEntryElement;
  }
}

customElements.define(
    CertificateProvisioningEntryElement.is,
    CertificateProvisioningEntryElement);
