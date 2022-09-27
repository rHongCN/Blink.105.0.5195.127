// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview Polymer element for displaying and modifying cellular sim info.
 */

(function() {

const TOGGLE_DEBOUNCE_MS = 500;

/**
 * State of the element. <network-siminfo> shows 1 of 2 modes:
 *   SIM_LOCKED: Shows an alert that the SIM is locked and provides an "Unlock"
 *       button which allows the user to unlock the SIM.
 *   SIM_UNLOCKED: Provides an option to lock the SIM if desired. If SIM-lock is
 *       on, this UI also allows the user to update the PIN used.
 * @enum {number}
 */
const State = {
  SIM_LOCKED: 0,
  SIM_UNLOCKED: 1,
};

Polymer({
  is: 'network-siminfo',

  behaviors: [I18nBehavior],

  properties: {
    /** @type {?OncMojo.DeviceStateProperties} */
    deviceState: {
      type: Object,
      value: null,
      observer: 'deviceStateChanged_',
    },

    /** @type {?OncMojo.NetworkStateProperties} */
    networkState: {
      type: Object,
      value: null,
    },

    /** @type {!chromeos.networkConfig.mojom.GlobalPolicy|undefined} */
    globalPolicy: Object,

    disabled: {
      type: Boolean,
      value: false,
    },

    /** Used to reference the State enum in HTML. */
    State: {
      type: Object,
      value: State,
    },

    /**
     * Reflects deviceState.simLockStatus.lockEnabled for the
     * toggle button.
     * @private
     */
    lockEnabled_: {
      type: Boolean,
      value: false,
    },

    /** @private {boolean} */
    isDialogOpen_: {
      type: Boolean,
      value: false,
      observer: 'onDialogOpenChanged_',
    },

    /**
     * If set to true, shows the Change PIN dialog if the device is unlocked.
     * @private {boolean}
     */
    showChangePin_: {
      type: Boolean,
      value: false,
    },

    /**
     * Indicates that the current network is on the active sim slot.
     * @private {boolean}
     */
    isActiveSim_: {
      type: Boolean,
      value: false,
      computed: 'computeIsActiveSim_(networkState, deviceState)',
    },

    /** @private {!State} */
    state_: {
      type: Number,
      value: State.SIM_UNLOCKED,
      computed: 'computeState_(networkState, deviceState, deviceState.*,' +
          'isActiveSim_)',
    },

    /** @private {boolean} */
    isSimLockPolicyEnabled_: {
      type: Boolean,
      value() {
        return loadTimeData.valueExists('isSimLockPolicyEnabled') &&
            loadTimeData.getBoolean('isSimLockPolicyEnabled');
      },
    },

    /** @private {boolean} */
    isSimPinLockRestricted_: {
      type: Boolean,
      value: false,
      computed: 'computeIsSimPinLockRestricted_(isSimLockPolicyEnabled_,' +
          'globalPolicy, globalPolicy.*, lockEnabled_)',
    },
  },

  /** @private {boolean|undefined} */
  setLockEnabled_: undefined,

  /*
   * Returns the sim lock CrToggleElement.
   * @return {?CrToggleElement}
   */
  getSimLockToggle() {
    return /** @type {?CrToggleElement} */ (this.$$('#simLockButton'));
  },

  /**
   * @return {?CrButtonElement}
   */
  getUnlockButton() {
    return /** @type {?CrButtonElement} */ (this.$$('#unlockPinButton'));
  },

  /** @private */
  onDialogOpenChanged_() {
    if (this.isDialogOpen_) {
      return;
    }

    this.delayUpdateLockEnabled_();
    this.updateFocus_();
  },

  /**
   * Sets default focus when dialog is closed.
   * @private
   */
  updateFocus_() {
    const state = this.computeState_();
    switch (state) {
      case State.SIM_LOCKED:
        if (this.$$('#unlockPinButton')) {
          this.$$('#unlockPinButton').focus();
        }
        break;
      case State.SIM_UNLOCKED:
        if (this.$$('#simLockButton')) {
          this.$$('#simLockButton').focus();
        }
        break;
    }
  },

  /** @private */
  deviceStateChanged_() {
    if (!this.deviceState) {
      return;
    }
    const simLockStatus = this.deviceState.simLockStatus;
    if (!simLockStatus) {
      return;
    }

    const lockEnabled = this.isActiveSim_ && simLockStatus.lockEnabled;
    if (lockEnabled !== this.lockEnabled_) {
      this.setLockEnabled_ = lockEnabled;
      this.updateLockEnabled_();
    } else {
      this.setLockEnabled_ = undefined;
    }
  },

  /**
   * Wrapper method to prevent changing |lockEnabled_| while a dialog is open
   * to avoid confusion while a SIM operation is in progress. This must be
   * called after closing any dialog (and not opening another) to set the
   * correct state.
   * @private
   */
  updateLockEnabled_() {
    if (this.setLockEnabled_ === undefined || this.isDialogOpen_) {
      return;
    }
    this.lockEnabled_ = this.setLockEnabled_;
    this.setLockEnabled_ = undefined;
  },

  /** @private */
  delayUpdateLockEnabled_() {
    setTimeout(() => {
      this.updateLockEnabled_();
    }, TOGGLE_DEBOUNCE_MS);
  },

  /**
   * Opens the pin dialog when the sim lock enabled state changes.
   * @param {!Event} event
   * @private
   */
  onSimLockEnabledChange_(event) {
    if (!this.deviceState) {
      return;
    }
    // Do not change the toggle state after toggle is clicked. The toggle
    // should only be updated when the device state changes or dialog has been
    // closed. Changing the UI toggle before the device state changes or dialog
    // is closed can be confusing to the user, as it indicates the action was
    // successful.
    this.lockEnabled_ = !this.lockEnabled_;
    this.showSimLockDialog_(/*showChangePin=*/ false);
  },

  /**
   * Opens the Change PIN dialog.
   * @param {!Event} event
   * @private
   */
  onChangePinTap_(event) {
    event.stopPropagation();
    if (!this.deviceState) {
      return;
    }
    this.showSimLockDialog_(true);
  },

  /**
   * Opens the Unlock PIN / PUK dialog.
   * @param {!Event} event
   * @private
   */
  onUnlockPinTap_(event) {
    event.stopPropagation();
    this.showSimLockDialog_(true);
  },

  /**
   * @param {boolean} showChangePin
   * @private
   */
  showSimLockDialog_(showChangePin) {
    this.showChangePin_ = showChangePin;
    this.isDialogOpen_ = true;
  },

  /**
   * @return {boolean}
   * @private
   */
  computeIsActiveSim_() {
    return isActiveSim(this.networkState, this.deviceState);
  },

  /**
   * @return {boolean}
   * @private
   */
  showChangePinButton_() {
    if (this.isSimPinLockRestricted_) {
      return false;
    }

    if (!this.deviceState || !this.deviceState.simLockStatus) {
      return false;
    }

    return this.deviceState.simLockStatus.lockEnabled && this.isActiveSim_;
  },

  /**
   * @return {boolean}
   * @private
   */
  isSimLockButtonDisabled_() {
    // If SIM PIN locking is restricted by admin, and the SIM does not have SIM
    // PIN lock enabled, users should not be able to enable PIN locking.
    if (this.isSimPinLockRestricted_ && !this.lockEnabled_) {
      return true;
    }

    return this.disabled || !this.isActiveSim_;
  },

  /**
   * @return {!State}
   * @private
   */
  computeState_() {
    const simLockStatus = this.deviceState && this.deviceState.simLockStatus;

    // If a lock is set and the network in question is the active SIM, show the
    // "locked SIM" UI. Note that we can only detect the locked state of the
    // active SIM, so it is possible that we fall through to the SIM_UNLOCKED
    // case below even for a locked SIM if that SIM is not the active one.
    if (this.isActiveSim_ && simLockStatus && !!simLockStatus.lockType) {
      return State.SIM_LOCKED;
    }

    // Note that if this is not the active SIM, we cannot read to lock state, so
    // we default to showing the "unlocked" UI unless we know otherwise.
    return State.SIM_UNLOCKED;
  },

  /**
   * @return {boolean}
   * @private
   */
  shouldShowPolicyIndicator_() {
    return this.isSimPinLockRestricted_ && this.isActiveSim_;
  },

  /**
   * @return {boolean}
   * @private
   */
  computeIsSimPinLockRestricted_() {
    return this.isSimLockPolicyEnabled_ && !!this.globalPolicy &&
        !this.globalPolicy.allowCellularSimLock;
  },

  /**
   * @param {!State} state1
   * @param {!State} state2
   * @return {boolean} Whether state1 is the same as state2.
   */
  eq_(state1, state2) {
    return state1 === state2;
  },
});
})();
