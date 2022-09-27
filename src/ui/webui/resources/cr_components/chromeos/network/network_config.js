// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * 'network-config' provides configuration of authentication properties for new
 * and existing networks.
 */

/**
 * Note: closure does not always recognize this if inside function() {}.
 * @enum {string}
 */
const VPNConfigType = {
  IKEV2: 'IKEv2',
  L2TP_IPSEC: 'L2TP_IPsec',
  OPEN_VPN: 'OpenVPN',
  WIREGUARD: 'WireGuard',
};

/**
 * Authentication types for IPsec-based VPNs.
 * @enum {string}
 */
const IpsecAuthType = {
  PSK: 'PSK',
  CERT: 'Cert',
  EAP: 'EAP',
};

/**
 * Method to configure WireGuard local key pair.
 * @enum {string}
 */
const WireGuardKeyConfigType = {
  USE_CURRENT: 'UseCurrent',
  GENERATE_NEW: 'GenerateNew',
  USER_INPUT: 'UserInput',
};

// Note: This pattern does not work for elements that are stamped on initial
// load because chromeos.networkConfig is not defined yet. <network-config>
// however is always embedded in a <cr-dialog> so it is not stamped immediately.
// TODO(stevenjb): Figure out a better way to do this.
const mojom = chromeos.networkConfig.mojom;

/** @type {string}  */ const DEFAULT_HASH = 'default';
/** @type {string}  */ const DO_NOT_CHECK_HASH = 'do-not-check';
/** @type {string}  */ const NO_CERTS_HASH = 'no-certs';
/** @type {string}  */ const NO_USER_CERT_HASH = 'no-user-cert';

/** @type {string}  */ const PLACEHOLDER_CREDENTIAL = '(credential)';

Polymer({
  is: 'network-config',

  behaviors: [
    NetworkListenerBehavior,
    I18nBehavior,
  ],

  properties: {
    /** @type {!chromeos.networkConfig.mojom.GlobalPolicy|undefined} */
    globalPolicy_: Object,

    /**
     * The GUID when an existing network is being configured. This will be
     * empty when configuring a new network.
     */
    guid: String,

    /** The type of network being configured as a string. */
    type: String,

    /**
     * The type of network being configured as an enum.
     * @private{chromeos.networkConfig.mojom.NetworkType|undefined}
     */
    mojoType_: Number,

    /** True if the user configuring the network can toggle the shared state. */
    shareAllowEnable: Boolean,

    /** The default shared state. */
    shareDefault: Boolean,

    enableConnect: {
      type: Boolean,
      notify: true,
      value: false,
    },

    enableSave: {
      type: Boolean,
      notify: true,
      value: false,
    },

    /**
     * Whether pressing the "Enter" key within the password field should start a
     * connection attempt. If this field is false, pressing "Enter" saves the
     * current configuration but does not connect.
     */
    connectOnEnter: {
      type: Boolean,
      value: false,
    },

    /** Set to any error from the last configuration result. */
    error: {
      type: String,
      notify: true,
    },

    /** @private {?chromeos.networkConfig.mojom.ManagedProperties} */
    managedProperties_: {
      type: Object,
      value: null,
    },

    /**
     * Managed EAP properties used for determination of managed EAP fields.
     * @private {?chromeos.networkConfig.mojom.ManagedEAPProperties}
     */
    managedEapProperties_: {
      type: Object,
      value: null,
    },

    /** Set once managedProperties_ have been sent; prevents multiple saves. */
    propertiesSent_: Boolean,

    /**
     * The configuration properties for the network.
     * @private {!chromeos.networkConfig.mojom.ConfigProperties|undefined}
     */
    configProperties_: Object,

    /**
     * Reference to the EAP properties for the current type or null if all EAP
     * properties should be hidden (e.g. WiFi networks with non EAP Security).
     * Note: even though this references an entry in configProperties_, we
     * need to send a separate notification when it changes for data binding
     * (e.g. by using 'set').
     * @private {?chromeos.networkConfig.mojom.EAPConfigProperties}
     */
    eapProperties_: {
      type: Object,
      value: null,
    },

    /**
     * The cached result of installed server CA certificates.
     * @private {
     *     undefined|Array<!chromeos.networkConfig.mojom.NetworkCertificate>}
     */
    cachedServerCaCerts_: Array,

    /**
     * The cached result of installed user certificates.
     * @private {
     *     undefined|Array<!chromeos.networkConfig.mojom.NetworkCertificate>}
     */
    cachedUserCerts_: Array,

    /**
     * Used to populate the 'Server CA certificate' dropdown.
     * @private {!Array<!chromeos.networkConfig.mojom.NetworkCertificate>}
     */
    serverCaCerts_: {
      type: Array,
      value() {
        return [];
      },
    },

    /** @private {string|undefined} */
    selectedServerCaHash_: String,

    /**
     * Used to populate the 'User certificate' dropdown.
     * @private {!Array<!chromeos.networkConfig.mojom.NetworkCertificate>}
     */
    userCerts_: {
      type: Array,
      value() {
        return [];
      },
    },

    /** @private {string|undefined} */
    selectedUserCertHash_: String,

    /**
     * Whether all required properties have been set.
     * @private
     */
    isConfigured_: {
      type: Boolean,
      value: false,
    },

    /**
     * Whether this network should be shared with other users of the device.
     * @private
     */
    shareNetwork_: {
      type: Boolean,
      value: true,
    },

    /**
     * Whether the device should automatically connect to the network.
     * @private
     */
    autoConnect_: Boolean,

    /**
     * Whether or not to show the hidden network warning.
     * @private
     */
    hiddenNetworkWarning_: Boolean,

    /**
     * Security value, used for Ethernet and Wifi and to detect when Security
     * changes. NOTE: the <select> element might set this to a string, see
     * crbug.com/1046149.
     * @private {!chromeos.networkConfig.mojom.SecurityType|string|undefined}
     */
    securityType_: Number,

    /**
     * 'SaveCredentials' value used for VPN (OpenVPN, IPsec, and L2TP).
     * @private
     */
    vpnSaveCredentials_: {
      type: Boolean,
      value: false,
    },

    /**
     * VPN Type from vpnTypeItems_.
     * @private {VPNConfigType|undefined}
     */
    vpnType_: String,

    /**
     * Ipsec auth type from ipsecAuthTypeItems_.
     * @private {IpsecAuthType}
     */
    ipsecAuthType_: {
      type: String,
      value: IpsecAuthType.PSK,
    },

    /** @private {WireGuardKeyConfigType|undefined} */
    wireguardKeyType_: String,

    /** @private {string|undefined} */
    ipAddressInput_: String,

    /** @private {string|undefined} */
    nameServersInput_: String,

    /**
     * Dictionary of boolean values determining which EAP properties to show,
     * or null to hide all EAP settings.
     * @private {?{
     *   Outer: (boolean|undefined),
     *   Inner: (boolean|undefined),
     *   ServerCA: (boolean|undefined),
     *   EapServerCertMatch: (boolean|undefined),
     *   UserCert: (boolean|undefined),
     *   Identity: (boolean|undefined),
     *   Password: (boolean|undefined),
     *   AnonymousIdentity: (boolean|undefined),
     * }}
     */
    showEap_: {
      type: Object,
      value: null,
    },

    /**
     * Dictionary of boolean values determining which VPN properties to show,
     * or null to hide all VPN settings.
     * @private {?{
     *   IPsec: (boolean|undefined),
     *   IPsecPSK: (boolean|undefined),
     *   IPsecEAP: (boolean|undefined),
     *   IKEv2: (boolean|undefined),
     *   OpenVPN: (boolean|undefined),
     *   WireGuard: (boolean|undefined),
     *   ServerCA: (boolean|undefined),
     *   UserCert: (boolean|undefined),
     * }}
     */
    showVpn_: {
      type: Object,
      value: null,
    },

    /**
     * Whether the WireGuard private key input box should be shown.
     * @private
     */
    isWireGuardUserPrivateKeyInputActive_: {
      type: Boolean,
      computed: 'updateWireGuardKeyType_(wireguardKeyType_)',
    },

    /**
     * Array of values for the EAP Method (Outer) dropdown.
     * @private {!Array<string>}
     */
    eapOuterItems_: {
      type: Array,
      readOnly: true,
      value: ['LEAP', 'PEAP', 'EAP-TLS', 'EAP-TTLS'],
    },

    /**
     * Array of values for the EAP EAP Phase 2 authentication (Inner) dropdown
     * when the Outer type is PEAP.
     * @private {!Array<string>}
     * @const
     */
    eapInnerItemsPeap_: {
      type: Array,
      readOnly: true,
      value: () => {
        const values = ['Automatic', 'MD5', 'MSCHAPv2'];
        if (loadTimeData.getBoolean('eapGtcWifiAuthentication')) {
          values.push('GTC');
        }
        return values;
      },
    },

    /**
     * Array of values for the EAP EAP Phase 2 authentication (Inner) dropdown
     * when the Outer type is EAP-TTLS.
     * @private {!Array<string>}
     * @const
     */
    eapInnerItemsTtls_: {
      type: Array,
      readOnly: true,
      value: ['Automatic', 'MD5', 'MSCHAP', 'MSCHAPv2', 'PAP', 'CHAP', 'GTC'],
    },

    /**
     * Array of values for the VPN Type dropdown.
     * Note: closure does not recognize Array<VPNConfigType> here.
     * @private {!Array<string>}
     */
    vpnTypeItems_: {
      type: Array,
      value: [
        VPNConfigType.L2TP_IPSEC,
        VPNConfigType.OPEN_VPN,
      ],
    },

    /**
     * Array of values for the Authentication Type dropdown for IPsec-based
     * VPNs.
     * @private {!Array<string>}
     */
    ipsecAuthTypeItems_: {
      type: Array,
      value: [],
    },

    /**
     * Array of values for the WireGuard key configuration method dropdown.
     * The actual value is set in initWireGuardKeyConfigType_() since the
     * value differs for new network and existing networks.
     * @private {!Array<string>}
     */
    wireguardKeyTypeItems_: {
      type: Array,
      value: [],
    },

    /**
     * Whether the current network configuration allows only device-wide
     * certificates (e.g. shared EAP TLS networks).
     * @private
     */
    deviceCertsOnly_: {
      type: Boolean,
      value: false,
    },

    /** @private */
    configRequiresPassphrase_: {
      type: Boolean,
      computed: 'computeConfigRequiresPassphrase_(mojoType_, securityType_)',
    },

    /** @private */
    serializedDomainSuffixMatch_: {
      type: String,
      value: '',
    },

    /** @private */
    serializedSubjectAltNameMatch_: {
      type: String,
      value: '',
    },
  },

  observers: [
    'setEnableConnect_(isConfigured_, propertiesSent_)',
    'setEnableSave_(isConfigured_, managedProperties_)',
    'setShareNetwork_(mojoType_, managedProperties_, securityType_,' +
        'shareDefault, shareAllowEnable)',
    'updateHiddenNetworkWarning_(autoConnect_)',
    'updateConfigProperties_(mojoType_, managedProperties_)',
    'updateSecurity_(configProperties_, securityType_)',
    'updateCertItems_(cachedServerCaCerts_, cachedUserCerts_, vpnType_, ' +
        'securityType_, eapProperties_.outer)',
    'updateEapOuter_(eapProperties_.outer)',
    'updateEapCerts_(eapProperties_.*, serverCaCerts_, userCerts_)',
    'updateShowEap_(configProperties_.*, eapProperties_.*, securityType_)',
    'updateVpnType_(configProperties_, vpnType_, ipsecAuthType_)',
    'updateVpnIPsecAuthTypeItems_(vpnType_)',
    'updateVpnIPsecCerts_(vpnType_, ipsecAuthType_,' +
        'configProperties_.typeConfig.vpn.ipSec.*, serverCaCerts_, userCerts_)',
    'updateOpenVPNCerts_(vpnType_,' +
        'configProperties_.typeConfig.vpn.openVpn.*,' +
        'serverCaCerts_, userCerts_)',
    // Multiple updateIsConfigured observers for different configurations.
    'updateIsConfigured_(configProperties_.*, securityType_)',
    'updateIsConfigured_(configProperties_, eapProperties_.*)',
    'updateIsConfigured_(configProperties_.typeConfig.wifi.*)',
    'updateIsConfigured_(configProperties_.typeConfig.vpn.*, vpnType_,' +
        'ipsecAuthType_)',
    'updateIsConfigured_(selectedUserCertHash_)',
  ],

  listeners: {'enter': 'onEnterEvent_'},

  /** @const */
  MIN_PASSPHRASE_LENGTH: 5,

  /** @private {?mojom.CrosNetworkConfigRemote} */
  networkConfig_: null,

  /** @override */
  created() {
    this.networkConfig_ = network_config.MojoInterfaceProviderImpl.getInstance()
                              .getMojoServiceRemote();
  },

  /** @override */
  attached() {
    this.networkConfig_.getGlobalPolicy().then(response => {
      this.globalPolicy_ = response.result;
    });
  },

  init() {
    this.mojoType_ = undefined;
    this.vpnType_ = undefined;
    this.managedProperties_ = null;
    this.configProperties_ = undefined;
    this.propertiesSent_ = false;
    this.cachedServerCaCerts_ = undefined;
    this.cachedUserCerts_ = undefined;
    this.selectedServerCaHash_ = undefined;
    this.selectedUserCertHash_ = undefined;

    this.networkConfig_.getSupportedVpnTypes().then(response => {
      this.updateVpnTypeItems_(response.vpnTypes);
    });
    this.initWireGuardKeyConfigType_();

    if (this.guid) {
      this.networkConfig_.getManagedProperties(this.guid).then(response => {
        this.getManagedPropertiesCallback_(response.result);
      });
    } else {
      const mojoType = OncMojo.getNetworkTypeFromString(this.type);
      const managedProperties =
          OncMojo.getDefaultManagedProperties(mojoType, this.guid, this.name);
      // Allow securityType_ to be set externally (e.g. in tests).
      if (mojoType === mojom.NetworkType.kWiFi &&
          this.securityType_ !== undefined) {
        managedProperties.typeProperties.wifi.security =
            /**@type{!mojom.SecurityType}*/ (this.securityType_);
      }
      this.managedProperties_ = managedProperties;
      this.mojoType_ = mojoType;
      setTimeout(() => {
        this.focusFirstInput_();
      });
    }

    if (this.mojoType_ === mojom.NetworkType.kVPN ||
        (this.globalPolicy_ &&
         this.globalPolicy_.allowOnlyPolicyWifiNetworksToConnect)) {
      this.autoConnect_ = false;
    } else {
      this.autoConnect_ = true;
    }
    this.hiddenNetworkWarning_ = this.showHiddenNetworkWarning_();

    this.updateIsConfigured_();
  },

  save() {
    this.saveAndConnect_(false /* connect */);
  },

  connect() {
    this.saveAndConnect_(true /* connect */);
  },


  /** @private */
  focusPassphrase_() {
    const passphraseInput = this.$$('#wifi-passphrase');
    if (passphraseInput) {
      passphraseInput.focus();
    }
  },

  /** @private */
  getHiddenNetworkMigrationEnabled() {
    return loadTimeData.getBoolean('enableHiddenNetworkMigration');
  },

  /**
   * @param {boolean} connect If true, connect after save.
   * @private
   */
  saveAndConnect_(connect) {
    if (!this.managedProperties_ || this.propertiesSent_) {
      return;
    }
    this.propertiesSent_ = true;
    this.error = '';
    if (this.eapProperties_) {
      const dsm = OncMojo.deserializeDomainSuffixMatch(
          this.serializedDomainSuffixMatch_);
      if (!dsm) {
        this.setError_('invalidDomainSuffixMatchEntry');
        this.propertiesSent_ = false;
        return;
      }
      this.eapProperties_.domainSuffixMatch = dsm;

      const sanm = OncMojo.deserializeSubjectAltNameMatch(
          this.serializedSubjectAltNameMatch_);
      if (!sanm) {
        this.setError_('invalidSubjectAlternativeNameMatchEntry');
        this.propertiesSent_ = false;
        return;
      }
      this.eapProperties_.subjectAltNameMatch = sanm;
    }
    const propertiesToSet = this.getPropertiesToSet_();
    if (this.managedProperties_.source === mojom.OncSource.kNone) {
      if (this.getHiddenNetworkMigrationEnabled()) {
        // Note: Set hidden SSID mode of new WiFi networks to disabled to avoid
        // unintentionally marking networks as hidden if not in range or
        // misspelled, etc.
        if (this.mojoType_ === mojom.NetworkType.kWiFi) {
          propertiesToSet.typeConfig.wifi.hiddenSsid =
              chromeos.networkConfig.mojom.HiddenSsidMode.kDisabled;
        }
      }
      if (!this.autoConnect_) {
        // Note: Do not set autoConnect to true, the connection manager will do
        // that on a successful connection (unless set to false here).
        propertiesToSet.autoConnect = {value: false};
      }
      this.networkConfig_.configureNetwork(propertiesToSet, this.shareNetwork_)
          .then(response => {
            this.createNetworkCallback_(
                response.guid, response.errorMessage, connect);
          });
    } else {
      this.networkConfig_.setProperties(this.guid, propertiesToSet)
          .then(response => {
            this.setPropertiesCallback_(
                response.success, response.errorMessage, connect);
          });
    }
    this.fire('properties-set');
  },

  /** @private */
  focusFirstInput_() {
    Polymer.dom.flush();
    const e = this.$$(
        'network-config-input:not([readonly]),' +
        'network-password-input:not([disabled]),' +
        'network-config-select:not([disabled])');
    if (e) {
      e.focus();
    }
  },

  /**
   * @param {!Event} event
   * @private
   */
  onEnterEvent_(event) {
    if (event.composedPath()[0].localName === 'network-config-input' ||
        event.composedPath()[0].localName === 'network-password-input') {
      this.onEnterPressedInInput_();
      event.stopPropagation();
    }
  },

  /** @private */
  onEnterPressedInInput_() {
    if (!this.isConfigured_) {
      return;
    }

    if (this.connectOnEnter) {
      this.connect();
    } else {
      this.save();
    }
  },

  /** @private */
  close_() {
    this.guid = '';
    this.type = '';
    this.securityType_ = undefined;
    this.fire('close');
  },

  /**
   * @return {boolean}
   * @private
   */
  hasGuid_() {
    return !!this.guid;
  },

  /**
   * @return {boolean}
   * @private
   */
  isIkev2Supported_() {
    return this.vpnTypeItems_.includes(VPNConfigType.IKEV2);
  },

  /**
   * @return {boolean}
   * @private
   */
  isWireGuardSupported_() {
    return this.vpnTypeItems_.includes(VPNConfigType.WIREGUARD);
  },

  /**
   * @param {!Array<string>|undefined} responseTypes
   * @private
   */
  updateVpnTypeItems_(responseTypes) {
    this.vpnTypeItems_ = [
      VPNConfigType.L2TP_IPSEC,
      VPNConfigType.OPEN_VPN,
    ];
    if (responseTypes.includes('ikev2')) {
      this.vpnTypeItems_.unshift(VPNConfigType.IKEV2);
    }
    if (responseTypes.includes('wireguard')) {
      this.vpnTypeItems_.push(VPNConfigType.WIREGUARD);
    }
  },

  /** @private */
  initWireGuardKeyConfigType_() {
    let items = [
      WireGuardKeyConfigType.GENERATE_NEW,
      WireGuardKeyConfigType.USER_INPUT,
    ];
    if (this.hasGuid_()) {
      items = [...items, WireGuardKeyConfigType.USE_CURRENT];
      this.wireguardKeyType_ = WireGuardKeyConfigType.USE_CURRENT;
    } else {
      this.wireguardKeyType_ = WireGuardKeyConfigType.GENERATE_NEW;
    }
    this.wireguardKeyTypeItems_ = items;
  },

  /** NetworkListenerBehavior override */
  onNetworkCertificatesChanged() {
    this.networkConfig_.getNetworkCertificates().then(response => {
      this.set('cachedServerCaCerts_', response.serverCas.slice());
      this.set('cachedUserCerts_', response.userCerts.slice());
    });
  },

  /**
   * @param {chromeos.networkConfig.mojom.CertificateType} type
   * @param {string} desc
   * @param {string} hash
   * @return {!chromeos.networkConfig.mojom.NetworkCertificate}
   * @private
   */
  getDefaultCert_(type, desc, hash) {
    return {
      type: type,
      hash: hash,
      issuedBy: desc,
      issuedTo: '',
      pemOrId: '',
      availableForNetworkAuth: false,
      hardwareBacked: false,
      // Default cert entries should always be shown, even in the login UI,
      // so treat thiem as device-wide.
      deviceWide: true,
    };
  },

  /**
   * @param {?mojom.ManagedBoolean|undefined} property
   * @return {boolean}
   * @private
   */
  getActiveBoolean_(property) {
    if (!property) {
      return false;
    }
    return property.activeValue;
  },

  /**
   * @param {?mojom.ManagedInt32|undefined} property
   * @return {number}
   * @private
   */
  getActiveInt32_(property) {
    if (!property) {
      return 0;
    }
    return property.activeValue;
  },

  /**
   * @param {?mojom.ManagedStringList|undefined} property
   * @return {!Array<string>|undefined}
   * @private
   */
  getActiveStringList_(property) {
    if (!property) {
      return undefined;
    }
    return property.activeValue;
  },

  /**
   * @param {?mojom.ManagedProperties} managedProperties
   * @private
   */
  getManagedPropertiesCallback_(managedProperties) {
    if (!managedProperties) {
      // The network no longer exists; close the page.
      console.warn('Network no longer exists: ' + this.guid);
      this.close_();
      return;
    }

    this.managedProperties_ = managedProperties;
    this.managedEapProperties_ = this.getManagedEap_(managedProperties);
    this.mojoType_ = managedProperties.type;

    if (this.mojoType_ === mojom.NetworkType.kVPN) {
      let saveCredentials = false;
      const vpn = managedProperties.typeProperties.vpn;
      if (vpn.type === mojom.VpnType.kOpenVPN) {
        saveCredentials = this.getActiveBoolean_(vpn.openVpn.saveCredentials);
      } else if (vpn.type === mojom.VpnType.kIKEv2) {
        saveCredentials = this.getActiveBoolean_(vpn.ipSec.saveCredentials);
      } else if (vpn.type === mojom.VpnType.kL2TPIPsec) {
        saveCredentials = this.getActiveBoolean_(vpn.ipSec.saveCredentials) ||
            this.getActiveBoolean_(vpn.l2tp.saveCredentials);
      } else if (vpn.type === mojom.VpnType.kWireGuard) {
        saveCredentials = true;
      }
      this.vpnSaveCredentials_ = saveCredentials;
    }

    this.setError_(managedProperties.errorState);
    this.updateCertError_();
    this.focusFirstInput_();
  },

  /**
   * @return {!Array<mojom.SecurityType>}
   * @private
   */
  getSecurityItems_() {
    if (this.mojoType_ === mojom.NetworkType.kWiFi) {
      return [
        mojom.SecurityType.kNone,
        mojom.SecurityType.kWepPsk,
        mojom.SecurityType.kWpaPsk,
        mojom.SecurityType.kWpaEap,
      ];
    }
    return [
      mojom.SecurityType.kNone,
      mojom.SecurityType.kWpaEap,
    ];
  },

  /** @private */
  setShareNetwork_() {
    if (this.mojoType_ === undefined || !this.managedProperties_ ||
        !this.securityType_ === undefined) {
      return;
    }
    const source = this.managedProperties_.source;
    if (source !== mojom.OncSource.kNone) {
      // Configured networks can not change whether they are shared.
      this.shareNetwork_ = source === mojom.OncSource.kDevice ||
          source === mojom.OncSource.kDevicePolicy;
      return;
    }
    if (!this.shareIsVisible_()) {
      this.shareNetwork_ = false;
      return;
    }
    if (this.shareAllowEnable) {
      // By default, Wi-Fi networks which require passwords are not shared,
      // but "insecure" networks with no passwords are shared. In either case,
      // the user can change the sharing setting by updating the toggle in the
      // UI.

      if (this.mojoType_ === mojom.NetworkType.kWiFi) {
        this.shareNetwork_ = this.securityType_ === mojom.SecurityType.kNone;
        return;
      }
    }
    this.shareNetwork_ = this.shareDefault;
  },

  /** @private */
  onShareChanged_(event) {
    this.updateSelectedCerts_();
  },

  /**
   * @param {!mojom.ManagedEAPProperties} eap
   * @return {!mojom.EAPConfigProperties}
   * @private
   */
  getEAPConfigProperties_(eap) {
    return {
      anonymousIdentity: OncMojo.getActiveString(eap.anonymousIdentity),
      clientCertType: OncMojo.getActiveString(eap.clientCertType),
      clientCertPkcs11Id: OncMojo.getActiveString(eap.clientCertPkcs11Id),
      domainSuffixMatch: this.getActiveStringList_(eap.domainSuffixMatch) || [],
      identity: OncMojo.getActiveString(eap.identity),
      inner: OncMojo.getActiveString(eap.inner),
      outer: OncMojo.getActiveString(eap.outer) || 'LEAP',
      password: OncMojo.getActiveString(eap.password),
      saveCredentials: this.getActiveBoolean_(eap.saveCredentials),
      serverCaPems: this.getActiveStringList_(eap.serverCaPems),
      subjectAltNameMatch:
          /** @type {!Array<!chromeos.networkConfig.mojom.SubjectAltName>} */
          (OncMojo.getActiveValue(eap.subjectAltNameMatch) || []),
      subjectMatch: OncMojo.getActiveString(eap.subjectMatch),
      useSystemCas: this.getActiveBoolean_(eap.useSystemCas),
    };
  },

  /**
   * @param {!mojom.ManagedIPSecProperties} ipSec
   * @return {!mojom.IPSecConfigProperties}
   * @private
   */
  getIPSecConfigProperties_(ipSec) {
    return {
      authenticationType:
          OncMojo.getActiveString(ipSec.authenticationType) || 'PSK',
      clientCertPkcs11Id: OncMojo.getActiveString(ipSec.clientCertPkcs11Id),
      clientCertType: OncMojo.getActiveString(ipSec.clientCertType),
      eap: ipSec.eap ? this.getEAPConfigProperties_(ipSec.eap) : null,
      group: OncMojo.getActiveString(ipSec.group),
      ikeVersion: this.getActiveInt32_(ipSec.ikeVersion),
      localIdentity: OncMojo.getActiveString(ipSec.localIdentity),
      psk: OncMojo.getActiveString(ipSec.psk),
      remoteIdentity: OncMojo.getActiveString(ipSec.remoteIdentity),
      saveCredentials: this.getActiveBoolean_(ipSec.saveCredentials),
      serverCaPems: this.getActiveStringList_(ipSec.serverCaPems),
      serverCaRefs: this.getActiveStringList_(ipSec.serverCaRefs),
    };
  },

  /**
   * @param {!mojom.ManagedL2TPProperties} l2tp
   * @return {!mojom.L2TPConfigProperties}
   * @private
   */
  getL2TPConfigProperties_(l2tp) {
    return {
      lcpEchoDisabled: this.getActiveBoolean_(l2tp.lcpEchoDisabled),
      password: OncMojo.getActiveString(l2tp.password),
      saveCredentials: this.getActiveBoolean_(l2tp.saveCredentials),
      username: OncMojo.getActiveString(l2tp.username),
    };
  },

  /**
   * @param {!mojom.ManagedOpenVPNProperties} openVpn
   * @return {!mojom.OpenVPNConfigProperties}
   * @private
   */
  getOpenVPNConfigProperties_(openVpn) {
    return {
      clientCertPkcs11Id: OncMojo.getActiveString(openVpn.clientCertPkcs11Id),
      clientCertType: OncMojo.getActiveString(openVpn.clientCertType),
      extraHosts: this.getActiveStringList_(openVpn.extraHosts),
      otp: '',
      password: OncMojo.getActiveString(openVpn.password),
      saveCredentials: this.getActiveBoolean_(openVpn.saveCredentials),
      serverCaPems: this.getActiveStringList_(openVpn.serverCaPems),
      serverCaRefs: this.getActiveStringList_(openVpn.serverCaRefs),
      userAuthenticationType:
          OncMojo.getActiveString(openVpn.userAuthenticationType),
      username: OncMojo.getActiveString(openVpn.username),
    };
  },

  /**
   * @param {!mojom.ManagedWireGuardProperties} wireguard
   * @return {!mojom.WireGuardConfigProperties}
   * @private
   */
  getWireGuardConfigProperties_(wireguard) {
    const config = {
      privateKey: OncMojo.getActiveString(wireguard.privateKey),
      peers: [],
    };
    if (wireguard.peers && wireguard.peers.activeValue) {
      for (const peer of wireguard.peers.activeValue) {
        const peerCopied = Object.assign({}, peer);
        if (this.hasGuid_()) {
          // Shill does not return exact value for crendential fields, showing
          // a placeholder here.
          peerCopied.presharedKey = PLACEHOLDER_CREDENTIAL;
        }
        config.peers.push(peerCopied);
      }
    }
    return config;
  },

  /**
   * Updates the config properties when |this.managedProperties| changes.
   * This gets called once when navigating to the page when default properties
   * are set, and again for existing networks when the properties are received.
   * @private
   */
  updateConfigProperties_() {
    if (this.mojoType_ === undefined || !this.managedProperties_) {
      return;
    }
    this.showEap_ = null;
    this.showVpn_ = null;
    this.vpnType_ = undefined;

    const managedProperties = this.managedProperties_;
    const configProperties =
        OncMojo.getDefaultConfigProperties(managedProperties.type);
    configProperties.name = OncMojo.getActiveString(managedProperties.name);

    let autoConnect;
    let security = mojom.SecurityType.kNone;
    switch (managedProperties.type) {
      case mojom.NetworkType.kWiFi:
        const wifi = managedProperties.typeProperties.wifi;
        const configWifi = configProperties.typeConfig.wifi;
        autoConnect = this.getActiveBoolean_(wifi.autoConnect);
        configWifi.passphrase = OncMojo.getActiveString(wifi.passphrase);
        configWifi.ssid = OncMojo.getActiveString(wifi.ssid);
        if (wifi.eap) {
          configWifi.eap = this.getEAPConfigProperties_(wifi.eap);
        }
        // updateSecurity_ will ensure that EAP properties are set correctly.
        security = wifi.security;
        configWifi.security = security;
        break;
      case mojom.NetworkType.kEthernet:
        const eap = managedProperties.typeProperties.ethernet.eap ?
            this.getEAPConfigProperties_(
                managedProperties.typeProperties.ethernet.eap) :
            undefined;
        security = eap ? mojom.SecurityType.kWpaEap : mojom.SecurityType.kNone;
        const auth = security === mojom.SecurityType.kWpaEap ? '8021X' : 'None';
        configProperties.typeConfig.ethernet.authentication = auth;
        configProperties.typeConfig.ethernet.eap = eap;
        break;
      case mojom.NetworkType.kVPN:
        const vpn = managedProperties.typeProperties.vpn;
        const vpnType = vpn.type;
        const configVpn = configProperties.typeConfig.vpn;
        configVpn.host = OncMojo.getActiveString(vpn.host);
        configVpn.type = {value: vpnType};
        if (vpnType === mojom.VpnType.kIKEv2) {
          if (!this.isIkev2Supported_()) {
            break;
          }
          assert(vpn.ipSec);
          configVpn.ipSec = this.getIPSecConfigProperties_(vpn.ipSec);
        } else if (vpnType === mojom.VpnType.kL2TPIPsec) {
          assert(vpn.ipSec);
          configVpn.ipSec = this.getIPSecConfigProperties_(vpn.ipSec);
          assert(vpn.l2tp);
          configVpn.l2tp = this.getL2TPConfigProperties_(vpn.l2tp);
        } else if (vpnType === mojom.VpnType.kOpenVPN) {
          assert(vpn.openVpn);
          configVpn.openVpn = this.getOpenVPNConfigProperties_(vpn.openVpn);
        } else if (vpnType === mojom.VpnType.kWireGuard) {
          if (!this.isWireGuardSupported_()) {
            break;
          }
          assert(vpn.wireguard);
          assert(managedProperties.staticIpConfig);
          configVpn.wireguard =
              this.getWireGuardConfigProperties_(vpn.wireguard);
          const staticIpConfig = managedProperties.staticIpConfig;
          this.ipAddressInput_ = staticIpConfig.ipAddress.activeValue;
          if (staticIpConfig.nameServers) {
            this.nameServersInput_ =
                staticIpConfig.nameServers.activeValue.join(',');
          }
        } else {
          assertNotReached();
        }
        security = mojom.SecurityType.kNone;
        break;
    }
    if (autoConnect !== undefined) {
      configProperties.autoConnect = {value: autoConnect};
    }
    // Request certificates the first time |configProperties_| is set.
    const requestCertificates = this.configProperties_ === undefined;
    this.configProperties_ = configProperties;
    this.securityType_ = security;
    this.set('eapProperties_', this.getEap_(this.configProperties_));
    if (!this.eapProperties_) {
      this.showEap_ = null;
    } else {
      this.serializedDomainSuffixMatch_ = OncMojo.serializeDomainSuffixMatch(
          this.eapProperties_.domainSuffixMatch);
      this.serializedSubjectAltNameMatch_ =
          OncMojo.serializeSubjectAltNameMatch(
              this.eapProperties_.subjectAltNameMatch);
    }
    if (managedProperties.type === mojom.NetworkType.kVPN) {
      this.vpnType_ = this.getVpnTypeFromProperties_(this.configProperties_);
      this.ipsecAuthType_ =
          this.getIpsecAuthTypeFromProperties_(this.configProperties_);
    }
    if (requestCertificates) {
      this.onNetworkCertificatesChanged();
    }
  },

  /**
   * Ensures that the appropriate properties are set or deleted when
   * |securityType_| changes.
   * @private
   */
  updateSecurity_() {
    if (this.securityType_ === undefined || !this.configProperties_) {
      return;
    }
    const type = this.mojoType_;
    // Force |securityType_| to an enum value when the <select> element sets it
    // to a string. See crbug.com/1046149 for details.
    if (typeof (this.securityType_) === 'string') {
      this.securityType_ =
          /** @type{!chromeos.networkConfig.mojom.SecurityType}*/ (
              Number.parseInt(/** @type{string}*/ (this.securityType_), 10));
    }
    const security = this.securityType_;
    if (type === mojom.NetworkType.kWiFi) {
      this.configProperties_.typeConfig.wifi.security = security;
    } else if (type === mojom.NetworkType.kEthernet) {
      const auth = security === mojom.SecurityType.kWpaEap ? '8021X' : 'None';
      this.configProperties_.typeConfig.ethernet.authentication = auth;
    }
    let eap;
    if (security === mojom.SecurityType.kWpaEap) {
      eap = this.getEap_(this.configProperties_, true);
      eap.outer = eap.outer || 'LEAP';
    }
    this.setEap_(eap);
  },

  /**
   * Ensures that the appropriate EAP properties are created (or deleted when
   * the eap.outer property changes.
   * @private
   */
  updateEapOuter_() {
    const eap = this.eapProperties_;
    if (!eap || !eap.outer) {
      return;
    }
    const innerItems = this.getEapInnerItems_(eap.outer);
    if (innerItems.length > 0) {
      if (!eap.inner || innerItems.indexOf(eap.inner) < 0) {
        this.set('eapProperties_.inner', innerItems[0]);
      }
    } else {
      this.set('eapProperties_.inner', undefined);
    }

    if (eap.outer !== 'EAP-TLS') {
      this.set('eapProperties_.clientCertType', 'None');
      this.set('eapProperties_.clientCertPkcs11Id', '');
      this.selectedUserCertHash_ = NO_USER_CERT_HASH;
    }
  },

  /** @private */
  updateEapCerts_() {
    // EAP is used for all configurable types except VPN.
    if (this.mojoType_ === mojom.NetworkType.kVPN) {
      return;
    }
    const eap = this.eapProperties_;
    const pem = eap && eap.serverCaPems ? eap.serverCaPems[0] : '';
    const certId =
        eap && eap.clientCertType === 'PKCS11Id' ? eap.clientCertPkcs11Id : '';
    this.setSelectedCerts_(pem, certId);
  },

  /** @private */
  updateShowEap_() {
    if (!this.eapProperties_ ||
        this.securityType_ === mojom.SecurityType.kNone) {
      this.showEap_ = null;
      this.updateCertError_();
      return;
    }
    const outer = this.eapProperties_.outer;
    switch (this.mojoType_) {
      case mojom.NetworkType.kWiFi:
      case mojom.NetworkType.kEthernet:
        this.showEap_ = {
          Outer: true,
          Inner: outer === 'PEAP' || outer === 'EAP-TTLS',
          ServerCA: outer !== 'LEAP',
          EapServerCertMatch:
              outer === 'EAP-TLS' || outer === 'EAP-TTLS' || outer === 'PEAP',
          UserCert: outer === 'EAP-TLS',
          Identity: true,
          Password: outer !== 'EAP-TLS',
          AnonymousIdentity: outer === 'PEAP' || outer === 'EAP-TTLS',
        };
        break;
    }
    this.updateCertError_();
  },

  /**
   * @param {!mojom.ConfigProperties} properties
   * @param {boolean=} opt_create
   * @return {?mojom.EAPConfigProperties}
   * @private
   */
  getEap_(properties, opt_create) {
    let eap;
    if (properties.typeConfig.wifi) {
      eap = properties.typeConfig.wifi.eap;
    } else if (properties.typeConfig.ethernet) {
      eap = properties.typeConfig.ethernet.eap;
    } else if (properties.typeConfig.vpn && properties.typeConfig.vpn.ipSec) {
      eap = properties.typeConfig.vpn.ipSec.eap;
    }
    if (opt_create) {
      return eap || {
        saveCredentials: false,
        useSystemCas: false,
        domainSuffixMatch: [],
        subjectAltNameMatch: [],
      };
    }
    return eap || null;
  },

  /**
   * @param {!mojom.EAPConfigProperties|undefined} eapProperties
   * @private
   */
  setEap_(eapProperties) {
    switch (this.mojoType_) {
      case mojom.NetworkType.kWiFi:
        this.configProperties_.typeConfig.wifi.eap = eapProperties;
        break;
      case mojom.NetworkType.kEthernet:
        this.configProperties_.typeConfig.ethernet.eap = eapProperties;
        break;
    }
    this.set('eapProperties_', eapProperties);
  },

  /**
   * @param {!mojom.ManagedProperties} managedProperties
   * @return {?mojom.ManagedEAPProperties}
   * @private
   */
  getManagedEap_(managedProperties) {
    let managedEap;
    switch (managedProperties.type) {
      case mojom.NetworkType.kWiFi:
        managedEap = managedProperties.typeProperties.wifi.eap;
        break;
      case mojom.NetworkType.kEthernet:
        managedEap = managedProperties.typeProperties.ethernet.eap;
        break;
      case mojom.NetworkType.kVPN:
        if (managedProperties.typeProperties.vpn.ipSec) {
          managedEap = managedProperties.typeProperties.vpn.ipSec.eap;
        }
        break;
    }
    return managedEap || null;
  },

  /**
   * @param {!mojom.ConfigProperties} properties
   * @return {!VPNConfigType}
   * @private
   */
  getVpnTypeFromProperties_(properties) {
    const vpn = properties.typeConfig.vpn;
    assert(vpn);
    if (!!vpn.type && vpn.type.value === mojom.VpnType.kIKEv2) {
      return VPNConfigType.IKEV2;
    } else if (!!vpn.type && vpn.type.value === mojom.VpnType.kL2TPIPsec) {
      return VPNConfigType.L2TP_IPSEC;
    } else if (!!vpn.type && vpn.type.value === mojom.VpnType.kWireGuard) {
      return VPNConfigType.WIREGUARD;
    }
    return VPNConfigType.OPEN_VPN;
  },

  /**
   * @param {!mojom.ConfigProperties} properties
   * @return {!IpsecAuthType}
   * @private
   */
  getIpsecAuthTypeFromProperties_(properties) {
    const vpn = properties.typeConfig.vpn;
    assert(vpn);
    if (!vpn.type ||
        !(vpn.type.value === mojom.VpnType.kL2TPIPsec ||
          vpn.type.value === mojom.VpnType.kIKEv2)) {
      // This field will not be used by services other than IPsec-based VPN.
      // Initiate it to "PSK" for simplicity.
      return IpsecAuthType.PSK;
    }
    if (vpn.ipSec.authenticationType === IpsecAuthType.PSK) {
      return IpsecAuthType.PSK;
    } else if (vpn.ipSec.authenticationType === IpsecAuthType.CERT) {
      return IpsecAuthType.CERT;
    } else if (vpn.ipSec.authenticationType === IpsecAuthType.EAP) {
      return IpsecAuthType.EAP;
    }
    assertNotReached();
  },

  /** @private */
  updateWireGuardKeyType_() {
    return this.wireguardKeyType_ === WireGuardKeyConfigType.USER_INPUT;
  },

  /** @private */
  updateCertItems_() {
    if (this.configProperties_ === undefined ||
        this.cachedServerCaCerts_ === undefined ||
        this.cachedUserCerts_ === undefined) {
      return;
    }

    const isOpenVpn = this.vpnType_ === VPNConfigType.OPEN_VPN;
    const isIpsec = this.vpnType_ === VPNConfigType.L2TP_IPSEC ||
        this.vpnType_ === VPNConfigType.IKEV2;
    let caCerts = this.cachedServerCaCerts_.slice();
    if (!isOpenVpn && !isIpsec) {
      // 'Default' is the same as 'Do not check' except that 'Default' sets
      // eap.useSystemCas (which does not apply to OpenVPN and IPsec-based
      // VPNs).
      caCerts.unshift(this.getDefaultCert_(
          chromeos.networkConfig.mojom.CertificateType.kServerCA,
          this.i18n('networkCAUseDefault'), DEFAULT_HASH));
    }
    if (!isIpsec) {
      // For IPsec-based VPNs, it is mandatory to verify the server.
      caCerts.push(this.getDefaultCert_(
          chromeos.networkConfig.mojom.CertificateType.kServerCA,
          this.i18n('networkCADoNotCheck'), DO_NOT_CHECK_HASH));
    }
    if (!caCerts.length) {
      caCerts = [this.getDefaultCert_(
          chromeos.networkConfig.mojom.CertificateType.kServerCA,
          this.i18n('networkCertificateNoneInstalled'), NO_CERTS_HASH)];
    }
    this.set('serverCaCerts_', caCerts);

    let userCerts = this.cachedUserCerts_.slice();
    // Only certs available for network authentication can be used.
    userCerts.forEach(function(cert) {
      if (!cert.availableForNetworkAuth) {
        cert.hash = '';
      }  // Clear the hash to invalidate the certificate.
    });

    const isEap = this.securityType_ === mojom.SecurityType.kWpaEap;
    const isEapTls = isEap && this.eapProperties_.outer === 'EAP-TLS';

    // User certificate is allowed but not required for OpenVPN and
    // EAP protocols except EAP-TLS (required for EAP-TLS)
    const isUserCertOptional = isOpenVpn || (isEap && !isEapTls);

    if (isUserCertOptional) {
      userCerts.unshift(this.getDefaultCert_(
          chromeos.networkConfig.mojom.CertificateType.kUserCert,
          this.i18n('networkNoUserCert'), NO_USER_CERT_HASH));
    }
    if (!userCerts.length) {
      userCerts = [this.getDefaultCert_(
          chromeos.networkConfig.mojom.CertificateType.kUserCert,
          this.i18n('networkCertificateNoneInstalled'), NO_CERTS_HASH)];
    }
    this.set('userCerts_', userCerts);

    this.updateSelectedCerts_();
    this.updateCertError_();
  },

  /** @private */
  updateVpnType_() {
    if (this.configProperties_ === undefined || this.vpnType_ === undefined) {
      return;
    }

    const vpn = this.configProperties_.typeConfig.vpn;
    if (!vpn) {
      this.showVpn_ = null;
      this.updateCertError_();
      return;
    }
    switch (this.vpnType_) {
      case VPNConfigType.IKEV2:
        vpn.type = {value: mojom.VpnType.kIKEv2};
        if (!vpn.ipSec) {
          this.ipsecAuthType_ = IpsecAuthType.EAP;
          vpn.ipSec = {
            authenticationType: this.ipsecAuthType_,
            ikeVersion: 2,
            saveCredentials: false,
          };
        }
        if (this.ipsecAuthType_ === IpsecAuthType.EAP && !vpn.ipSec.eap) {
          vpn.ipSec.eap = {
            domainSuffixMatch: [],
            outer: 'MSCHAPv2',
            saveCredentials: false,
            subjectAltNameMatch: [],
            useSystemCas: false,
          };
          this.eapProperties_ = vpn.ipSec.eap;
        }
        break;
      case VPNConfigType.L2TP_IPSEC:
        vpn.type = {value: mojom.VpnType.kL2TPIPsec};
        if (this.ipsecAuthType_ !== IpsecAuthType.PSK &&
            this.ipsecAuthType_ !== IpsecAuthType.CERT) {
          // This will happen if user changes the VPN type to IKEv2 where the
          // default value of auth type is EAP, and then changes the VPN type to
          // L2TP/IPsec.
          this.ipsecAuthType_ = IpsecAuthType.PSK;
        }

        if (!vpn.ipSec) {
          vpn.ipSec = {
            authenticationType: this.ipsecAuthType_,
            ikeVersion: 1,
            saveCredentials: false,
          };
        }
        break;
      case VPNConfigType.OPEN_VPN:
        vpn.type = {value: mojom.VpnType.kOpenVPN};
        vpn.openVpn = vpn.openVpn || {saveCredentials: false};
        break;
      case VPNConfigType.WIREGUARD:
        vpn.type = {value: mojom.VpnType.kWireGuard};
        vpn.wireguard = vpn.wireguard || {peers: [{}]};
        break;
      default:
        assertNotReached();
    }

    const isIpsec = this.vpnType_ === VPNConfigType.L2TP_IPSEC ||
        this.vpnType_ === VPNConfigType.IKEV2;
    const ipsecAuthIsPsk = this.ipsecAuthType_ === IpsecAuthType.PSK;
    const ipsecAuthIsEap = this.ipsecAuthType_ === IpsecAuthType.EAP;
    const ipsecAuthIsCert = this.ipsecAuthType_ === IpsecAuthType.CERT;
    const isOpenvpn = this.vpnType_ === VPNConfigType.OPEN_VPN;
    this.showVpn_ = {
      IPsec: isIpsec,
      IPsecPSK: isIpsec && ipsecAuthIsPsk,
      IPsecEAP: isIpsec && ipsecAuthIsEap,
      IKEv2: this.vpnType_ === VPNConfigType.IKEV2,
      OpenVPN: isOpenvpn,
      WireGuard: this.vpnType_ === VPNConfigType.WIREGUARD,
      ServerCA: (isIpsec && !ipsecAuthIsPsk) || isOpenvpn,
      UserCert: (isIpsec && ipsecAuthIsCert) || isOpenvpn,
    };

    if (vpn.type.value === mojom.VpnType.kL2TPIPsec && !vpn.l2tp) {
      vpn.l2tp = {
        lcpEchoDisabled: false,
        password: '',
        saveCredentials: false,
        username: '',
      };
    }
    if (vpn.type.value !== mojom.VpnType.kL2TPIPsec &&
        vpn.type.value !== mojom.VpnType.kIKEv2) {
      delete vpn.ipSec;
    }
    if (vpn.type.value !== mojom.VpnType.kL2TPIPsec) {
      delete vpn.l2tp;
    }
    if (vpn.type.value !== mojom.VpnType.kOpenVPN) {
      delete vpn.openVpn;
    }
    if (vpn.type.value !== mojom.VpnType.kWireGuard) {
      delete vpn.wireguard;
    }
    this.updateCertError_();
  },

  /** @private */
  updateVpnIPsecAuthTypeItems_() {
    this.ipsecAuthTypeItems_ = [
      IpsecAuthType.PSK,
      IpsecAuthType.CERT,
    ];
    if (this.vpnType_ === VPNConfigType.IKEV2) {
      this.ipsecAuthTypeItems_.push(IpsecAuthType.EAP);
    }
  },

  /** @private */
  updateVpnIPsecCerts_() {
    if (this.vpnType_ !== VPNConfigType.L2TP_IPSEC &&
        this.vpnType_ !== VPNConfigType.IKEV2) {
      return;
    }
    if (this.ipsecAuthType_ === IpsecAuthType.PSK) {
      return;
    }
    const ipSec = this.configProperties_.typeConfig.vpn.ipSec;
    if (!ipSec) {
      return;
    }
    const pem = ipSec.serverCaPems ? ipSec.serverCaPems[0] : undefined;
    const certId =
        ipSec.clientCertType === 'PKCS11Id' ? ipSec.clientCertPkcs11Id : '';
    this.setSelectedCerts_(pem, certId);
  },

  /** @private */
  updateOpenVPNCerts_() {
    if (this.vpnType_ !== VPNConfigType.OPEN_VPN) {
      return;
    }
    const openVpn = this.configProperties_.typeConfig.vpn.openVpn;
    if (!openVpn) {
      return;
    }
    const pem = openVpn.serverCaPems ? openVpn.serverCaPems[0] : undefined;
    const certId =
        openVpn.clientCertType === 'PKCS11Id' ? openVpn.clientCertPkcs11Id : '';
    this.setSelectedCerts_(pem, certId);
  },

  /** @private */
  updateCertError_() {
    // If |this.error| was set to something other than a cert error, do not
    // change it.
    /** @const */ const noCertsError = 'networkErrorNoUserCertificate';
    /** @const */ const noValidCertsError =
        'networkErrorNotAvailableForNetworkAuth';
    if (this.error && this.error !== noCertsError &&
        this.error !== noValidCertsError) {
      return;
    }

    const requireCerts = (this.showEap_ && this.showEap_.UserCert) ||
        (this.showVpn_ && this.showVpn_.UserCert);
    if (!requireCerts) {
      this.setError_('');
      return;
    }
    if (!this.userCerts_.length || this.userCerts_[0].hash === NO_CERTS_HASH) {
      this.setError_(noCertsError);
      return;
    }
    const validUserCert = this.userCerts_.find(function(cert) {
      return !!cert.hash;
    });
    if (!validUserCert) {
      this.setError_(noValidCertsError);
      return;
    }
    this.setError_('');
    return;
  },

  /**
   * Sets the selected cert if |pem| (serverCa) or |certId| (user) is specified.
   * Otherwise sets a default value if no certificate is selected.
   * @param {string|undefined} pem
   * @param {string|undefined} certId
   * @private
   */
  setSelectedCerts_(pem, certId) {
    if (pem) {
      const serverCa = this.serverCaCerts_.find(function(cert) {
        return cert.pemOrId === pem;
      });
      if (serverCa) {
        this.selectedServerCaHash_ = serverCa.hash;
      }
    }

    if (certId) {
      // |certId| is in the format |slot:id| for EAP and IPSec and |id| for
      // OpenVPN certs.
      // |userCerts_[i].pemOrId| is always in the format |slot:id|.
      // Use a substring comparison to support both |certId| formats.
      const userCert = this.userCerts_.find(function(cert) {
        return cert.pemOrId.indexOf(/** @type {string} */ (certId)) >= 0;
      });
      if (userCert) {
        this.selectedUserCertHash_ = userCert.hash;
      }
    }
    this.updateSelectedCerts_();
    this.updateIsConfigured_();
  },

  /**
   * @param {!Array<!chromeos.networkConfig.mojom.NetworkCertificate>} certs
   * @param {string|undefined} hash
   * @private
   * @return {!chromeos.networkConfig.mojom.NetworkCertificate|undefined}
   */
  findCert_(certs, hash) {
    if (!hash) {
      return undefined;
    }
    return certs.find((cert) => {
      return cert.hash === hash;
    });
  },

  /**
   * Called when the certificate list or a selected certificate changes.
   * Ensures that each selected certificate exists in its list, or selects the
   * correct default value.
   * @private
   */
  updateSelectedCerts_() {
    if (!this.serverCaCerts_.length || !this.userCerts_.length) {
      return;
    }
    const eap = this.eapProperties_;

    // Only device-wide certificates can be used for shared networks that
    // require a certificate.
    this.deviceCertsOnly_ =
        this.shareNetwork_ && !!eap && eap.outer === 'EAP-TLS';

    // Validate selected Server CA.
    const caCert =
        this.findCert_(this.serverCaCerts_, this.selectedServerCaHash_);
    if (!caCert || (this.deviceCertsOnly_ && !caCert.deviceWide)) {
      this.selectedServerCaHash_ = undefined;
    }
    if (!this.selectedServerCaHash_) {
      if (eap && eap.useSystemCas) {
        this.selectedServerCaHash_ = DEFAULT_HASH;
      } else if (!this.guid && this.serverCaCerts_[0]) {
        // For unconfigured networks, default to the first available
        // certificate and fallback to DEFAULT_HASH. See
        // onNetworkCertificatesChanged() for how certificates are added.
        let cert = this.serverCaCerts_[0];
        if (cert.hash === DEFAULT_HASH &&
            this.isRealCertUsableForNetworkAuth_(this.serverCaCerts_[1])) {
          cert = this.serverCaCerts_[1];
        }
        this.selectedServerCaHash_ = cert.hash;
      } else {
        this.selectedServerCaHash_ = DO_NOT_CHECK_HASH;
      }
    }

    // Validate selected User cert.
    const userCert =
        this.findCert_(this.userCerts_, this.selectedUserCertHash_);
    if (!userCert || (this.deviceCertsOnly_ && !userCert.deviceWide)) {
      this.selectedUserCertHash_ = undefined;
    }
    if (!this.selectedUserCertHash_) {
      for (let i = 0; i < this.userCerts_.length; ++i) {
        const userCert = this.userCerts_[i];
        if (userCert && (!this.deviceCertsOnly_ || userCert.deviceWide)) {
          this.selectedUserCertHash_ = userCert.hash;
          break;
        }
      }
    }
  },
  /**
   * Checks that the hash of the certificate is set and not one of the default
   * special strings.
   * @param {chromeos.networkConfig.mojom.NetworkCertificate|undefined} cert
   * @return {boolean}
   * @private
   */
  isRealCertUsableForNetworkAuth_(cert) {
    return !!cert && cert.hash !== DO_NOT_CHECK_HASH &&
        cert.hash !== DEFAULT_HASH;
  },
  /**
   * @return {boolean}
   * @private
   */
  getIsConfigured_() {
    if (this.securityType_ === undefined || !this.configProperties_) {
      return false;
    }

    const typeConfig = this.configProperties_.typeConfig;
    if (typeConfig.vpn) {
      if (this.vpnType_ === VPNConfigType.IKEV2 && !this.isIkev2Supported_()) {
        return false;
      }
      return this.vpnIsConfigured_();
    }

    if (typeConfig.wifi) {
      if (!typeConfig.wifi.ssid) {
        return false;
      }
      if (this.configRequiresPassphrase_) {
        const passphrase = typeConfig.wifi.passphrase;
        if (!passphrase || passphrase.length < this.MIN_PASSPHRASE_LENGTH) {
          return false;
        }
      }
    }
    if (this.securityType_ === mojom.SecurityType.kWpaEap) {
      return this.eapIsConfigured_();
    }
    return true;
  },

  /** @private */
  updateIsConfigured_() {
    this.isConfigured_ = this.getIsConfigured_();
  },

  /**
   * @param {mojom.NetworkType} networkType
   * @return {boolean}
   * @private
   */
  isWiFi_(networkType) {
    return networkType === mojom.NetworkType.kWiFi;
  },

  /** @private */
  setEnableSave_() {
    this.enableSave = this.isConfigured_ && !!this.managedProperties_;
  },

  /** @private */
  setEnableConnect_() {
    this.enableConnect = this.isConfigured_ && !this.propertiesSent_;
  },

  /**
   * @param {mojom.NetworkType} networkType
   * @return {boolean}
   * @private
   */
  securityIsVisible_(networkType) {
    return networkType === mojom.NetworkType.kWiFi ||
        networkType === mojom.NetworkType.kEthernet;
  },

  /**
   * @return {boolean}
   * @private
   */
  securityIsEnabled_() {
    // WiFi Security type cannot be changed once configured.
    return !this.guid || this.mojoType_ === mojom.NetworkType.kEthernet;
  },

  /**
   * @return {boolean}
   * @private
   */
  shareIsVisible_() {
    if (!this.managedProperties_) {
      return false;
    }
    return this.managedProperties_.source === mojom.OncSource.kNone &&
        this.managedProperties_.type === mojom.NetworkType.kWiFi;
  },

  /**
   * @return {boolean}
   * @private
   */
  shareIsEnabled_() {
    if (!this.managedProperties_) {
      return false;
    }
    if (!this.shareAllowEnable ||
        this.managedProperties_.source !== mojom.OncSource.kNone) {
      return false;
    }

    // Insecure WiFi networks are always shared.
    if (this.mojoType_ === mojom.NetworkType.kWiFi &&
        this.securityType_ === mojom.SecurityType.kNone) {
      return false;
    }
    return true;
  },

  /**
   * @return {boolean}
   * @private
   */
  configCanAutoConnect_() {
    // Only WiFi can choose whether or not to autoConnect.
    return loadTimeData.getBoolean('showHiddenNetworkWarning') &&
        this.mojoType_ === mojom.NetworkType.kWiFi;
  },

  /**
   * @return {boolean}
   * @private
   */
  autoConnectDisabled_() {
    return this.isAutoConnectEnforcedByPolicy_();
  },

  /**
   * @return {boolean}
   * @private
   */
  isAutoConnectEnforcedByPolicy_() {
    return !!this.globalPolicy_ &&
        !!this.globalPolicy_.allowOnlyPolicyNetworksToAutoconnect;
  },

  /**
   * @return {boolean}
   * @private
   */
  showHiddenNetworkWarning_() {
    Polymer.dom.flush();
    return loadTimeData.getBoolean('showHiddenNetworkWarning') &&
        this.autoConnect_ && !this.hasGuid_();
  },

  /**
   * @private
   */
  updateHiddenNetworkWarning_() {
    this.hiddenNetworkWarning_ = this.showHiddenNetworkWarning_();
  },

  /**
   * @return {boolean}
   * @private
   */
  selectedServerCaHashIsValid_() {
    return !!this.selectedServerCaHash_ &&
        this.selectedServerCaHash_ !== NO_CERTS_HASH;
  },

  /**
   * @return {boolean}
   * @private
   */
  selectedUserCertHashIsValid_() {
    return !!this.selectedUserCertHash_ &&
        this.selectedUserCertHash_ !== NO_CERTS_HASH;
  },

  /**
   * @return {boolean}
   * @private
   */
  eapIsConfigured_() {
    if (!this.configProperties_) {
      return false;
    }
    const eap = this.getEap_(this.configProperties_);
    if (!eap) {
      return false;
    }
    if (eap.outer !== 'EAP-TLS') {
      return true;
    }
    // EAP TLS networks can be shared only for device-wide certificates.
    if (this.deviceCertsOnly_) {  // network is shared
      let cert = this.findCert_(this.userCerts_, this.selectedUserCertHash_);
      if (!cert || !cert.deviceWide) {
        return false;
      }
      cert = this.findCert_(this.serverCaCerts_, this.selectedServerCaHash_);
      if (!cert.deviceWide) {
        return false;
      }
    }

    return this.selectedUserCertHashIsValid_();
  },

  /**
   * @return {boolean}
   * @private
   */
  ikev2IsConfigured_() {
    const vpn = this.configProperties_.typeConfig.vpn;
    switch (this.ipsecAuthType_) {
      case IpsecAuthType.PSK:
        return !!vpn.ipSec.psk;
      case IpsecAuthType.CERT:
        // TODO(b/206722135): Show proper error message in the UI if server CA
        // is invalid.
        return this.selectedServerCaHashIsValid_() &&
            this.selectedUserCertHashIsValid_();
      case IpsecAuthType.EAP:
        return this.selectedServerCaHashIsValid_() &&
            !!this.eapProperties_.identity;
      default:
        assertNotReached();
    }
  },


  /**
   * @return {boolean}
   * @private
   */
  l2tpIpsecIsConfigured_() {
    const vpn = this.configProperties_.typeConfig.vpn;
    switch (this.ipsecAuthType_) {
      case IpsecAuthType.PSK:
        return !!vpn.l2tp.username && !!vpn.ipSec.psk;
      case IpsecAuthType.CERT:
        // TODO(b/206722135): Show proper error message in the UI if server CA
        // is invalid.
        return !!vpn.l2tp.username && this.selectedServerCaHashIsValid_() &&
            this.selectedUserCertHashIsValid_();
      default:
        assertNotReached();
    }
  },

  /**
   * @param {string|null|undefined} input
   * @return {boolean}
   * @private
   */
  isValidWireGuardKey_(input) {
    // A base64 translation of a 32 byte string is 44 bytes with '=' ending
    return !!input && input.length === 44 && input.charAt(43) === '=' &&
        !!input.match(/^[a-z0-9+/=]+$/i);
  },

  /**
   * @param {mojom.WireGuardConfigProperties|null|undefined} wireguard
   * @param {string|undefined} ipAddress
   * @return {boolean}
   * @private
   */
  isWireGuardConfigurationValid_(wireguard, ipAddress) {
    if (!wireguard) {
      return false;
    }
    // ipAddress should be a valid IPv4 address
    if (!ipAddress || !ipAddress.match(/^([0-9]+\.){3}[0-9]+$/i)) {
      return false;
    }
    if (this.isWireGuardUserPrivateKeyInputActive_ &&
        !this.isValidWireGuardKey_(wireguard.privateKey)) {
      return false;
    }
    // TODO: Current UI only supports configuring a single peer.
    const peer = wireguard.peers[0];
    if (!this.isValidWireGuardKey_(peer.publicKey)) {
      return false;
    }
    if (!!peer.presharedKey && peer.presharedKey !== PLACEHOLDER_CREDENTIAL &&
        !this.isValidWireGuardKey_(peer.presharedKey)) {
      return false;
    }
    // endpoint should be the form of IP:port or hostname:port
    if (!peer.endpoint || !peer.endpoint.match(/^[a-zA-Z0-9\-\.]+:[0-9]+$/i)) {
      return false;
    }
    // allowedIps should be comma-seperated list of IP/cidr
    if (!peer.allowedIps ||
        !peer.allowedIps.match(
            /^([0-9\.]+(\/[0-9]+)?,)*[0-9\.]+(\/[0-9]+)?$/i)) {
      return false;
    }
    return true;
  },

  /**
   * @return {boolean}
   * @private
   */
  vpnIsConfigured_() {
    const vpn = this.configProperties_.typeConfig.vpn;
    if (!this.configProperties_.name || !vpn ||
        (!vpn.host && this.vpnType_ !== VPNConfigType.WIREGUARD)) {
      return false;
    }

    switch (this.vpnType_) {
      case VPNConfigType.IKEV2:
        return this.ikev2IsConfigured_();
      case VPNConfigType.L2TP_IPSEC:
        return this.l2tpIpsecIsConfigured_();
      case VPNConfigType.OPEN_VPN:
        // OpenVPN should require username + password OR a user cert. However,
        // there may be servers with different requirements so err on the side
        // of permissiveness.
        return true;
      case VPNConfigType.WIREGUARD:
        return this.isWireGuardConfigurationValid_(
            vpn.wireguard, this.ipAddressInput_);
    }
    return false;
  },

  /** @private */
  getPropertiesToSet_() {
    const propertiesToSet =
        /** @type{!mojom.ConfigProperties}*/ (
            Object.assign({}, this.configProperties_));
    // Do not set AutoConnect by default, the connection manager will set
    // it to true on a successful connection.
    delete propertiesToSet.autoConnect;
    if (this.guid) {
      propertiesToSet.guid = this.guid;
    }
    const eap = this.getEap_(propertiesToSet);
    if (eap) {
      this.setEapProperties_(eap);
    }
    if (this.mojoType_ === mojom.NetworkType.kVPN) {
      const vpnConfig = propertiesToSet.typeConfig.vpn;
      // VPN.Host can be an IP address but will not be recognized as such if
      // there is initial whitespace, so trim it.
      if (vpnConfig.host !== undefined) {
        vpnConfig.host = vpnConfig.host.trim();
      }
      const vpnType = vpnConfig.type.value;
      if (vpnType === mojom.VpnType.kOpenVPN) {
        this.setOpenVPNProperties_(propertiesToSet);
      } else {
        delete propertiesToSet.typeConfig.vpn.openVpn;
      }
      if (vpnType === mojom.VpnType.kIKEv2) {
        this.setVpnIkev2Properties_(propertiesToSet);
      } else if (vpnType === mojom.VpnType.kL2TPIPsec) {
        this.setVpnL2tpIpsecProperties_(propertiesToSet);
      } else {
        delete propertiesToSet.typeConfig.vpn.ipSec;
        delete propertiesToSet.typeConfig.vpn.l2tp;
      }
      if (vpnType === mojom.VpnType.kWireGuard) {
        this.setWireGuardProperties_(propertiesToSet);
      } else {
        delete propertiesToSet.typeConfig.vpn.wireguard;
      }
    }
    return propertiesToSet;
  },

  /**
   * @return {!Array<string>}
   * @private
   */
  getServerCaPems_() {
    const caHash = this.selectedServerCaHash_ || '';
    if (!caHash || caHash === DO_NOT_CHECK_HASH || caHash === DEFAULT_HASH) {
      return [];
    }
    const serverCa = this.findCert_(this.serverCaCerts_, caHash);
    return serverCa && serverCa.pemOrId ? [serverCa.pemOrId] : [];
  },

  /**
   * @return {string}
   * @private
   */
  getUserCertPkcs11Id_() {
    const userCertHash = this.selectedUserCertHash_ || '';
    if (!this.selectedUserCertHashIsValid_() ||
        userCertHash === NO_USER_CERT_HASH) {
      return '';
    }
    const userCert = this.findCert_(this.userCerts_, userCertHash);
    return (userCert && userCert.pemOrId) || '';
  },

  /**
   * @param {!mojom.EAPConfigProperties} eap
   * @private
   */
  setEapProperties_(eap) {
    eap.useSystemCas = this.selectedServerCaHash_ === DEFAULT_HASH;

    eap.serverCaPems = this.getServerCaPems_();

    const pkcs11Id = this.getUserCertPkcs11Id_();
    eap.clientCertType = pkcs11Id ? 'PKCS11Id' : 'None';
    eap.clientCertPkcs11Id = pkcs11Id || '';
  },

  /**
   * @param {!mojom.ConfigProperties} propertiesToSet
   * @private
   */
  setVpnIkev2Properties_(propertiesToSet) {
    const ipsec = propertiesToSet.typeConfig.vpn.ipSec;
    assert(!!ipsec);

    ipsec.authenticationType = this.ipsecAuthType_;
    if (ipsec.authenticationType !== IpsecAuthType.PSK) {
      // Set psk to empty string to make sure the value is cleared.
      ipsec.psk = '';
      // For non-PSK auth method, server CA is mandatory.
      ipsec.serverCaPems = this.getServerCaPems_();
    }

    if (ipsec.authenticationType === IpsecAuthType.CERT) {
      ipsec.clientCertType = 'PKCS11Id';
      ipsec.clientCertPkcs11Id = this.getUserCertPkcs11Id_();
    } else {
      delete ipsec.clientCertType;
      delete ipsec.clientCertPkcs11Id;
    }

    if (ipsec.authenticationType === IpsecAuthType.EAP) {
      // Not all fields in eap are used by IKEv2, so create a new object here.
      const eap = ipsec.eap;
      ipsec.eap = {
        domainSuffixMatch: [],
        identity: eap.identity,
        outer: 'MSCHAPv2',
        password: eap.password,
        saveCredentials: this.vpnSaveCredentials_,
        subjectAltNameMatch: [],
        useSystemCas: false,
      };
    } else {
      delete ipsec.eap;
    }

    ipsec.ikeVersion = 2;
    ipsec.saveCredentials = this.vpnSaveCredentials_;
  },

  /**
   * @param {!mojom.ConfigProperties} propertiesToSet
   * @private
   */
  setOpenVPNProperties_(propertiesToSet) {
    const openVpn = propertiesToSet.typeConfig.vpn.openVpn;
    assert(!!openVpn);

    openVpn.serverCaPems = this.getServerCaPems_();

    const pkcs11Id = this.getUserCertPkcs11Id_();
    openVpn.clientCertType = pkcs11Id ? 'PKCS11Id' : 'None';
    openVpn.clientCertPkcs11Id = pkcs11Id || '';

    if (openVpn.password) {
      openVpn.userAuthenticationType =
          openVpn.otp ? 'PasswordAndOTP' : 'Password';
    } else if (openVpn.otp) {
      openVpn.userAuthenticationType = 'OTP';
    } else {
      openVpn.userAuthenticationType = 'None';
    }

    openVpn.saveCredentials = this.vpnSaveCredentials_;
    propertiesToSet.typeConfig.vpn.openVpn = openVpn;
  },

  /**
   * @param {!mojom.ConfigProperties} propertiesToSet
   * @private
   */
  setWireGuardProperties_(propertiesToSet) {
    const wireguard = propertiesToSet.typeConfig.vpn.wireguard;
    assert(!!wireguard);
    propertiesToSet.typeConfig.vpn.host = 'wireguard';
    propertiesToSet.ipAddressConfigType = 'Static';
    propertiesToSet.staticIpConfig = {
      gateway: this.ipAddressInput_,
      ipAddress: this.ipAddressInput_,
      routingPrefix: 32,
      type: chromeos.networkConfig.mojom.IPConfigType.kIPv4,
    };
    if (this.nameServersInput_) {
      propertiesToSet.nameServersConfigType = 'Static';
      propertiesToSet.staticIpConfig.nameServers =
          this.nameServersInput_.split(',');
    }
    if (this.wireguardKeyType_ === WireGuardKeyConfigType.USE_CURRENT) {
      delete wireguard.privateKey;
    } else if (this.wireguardKeyType_ === WireGuardKeyConfigType.GENERATE_NEW) {
      wireguard.privateKey = '';
    }
    assert(!!wireguard.peers);
    for (const peer of wireguard.peers) {
      if (peer.presharedKey === PLACEHOLDER_CREDENTIAL) {
        delete peer.presharedKey;  // No modification
      } else if (peer.presharedKey === undefined) {
        peer.presharedKey = '';  // Explicitly removed
      }
    }
  },

  /**
   * @param {!mojom.ConfigProperties} propertiesToSet
   * @private
   */
  setVpnL2tpIpsecProperties_(propertiesToSet) {
    const vpn = propertiesToSet.typeConfig.vpn;
    assert(vpn.ipSec);
    assert(vpn.l2tp);

    vpn.ipSec.authenticationType = this.ipsecAuthType_;
    if (vpn.ipSec.authenticationType === IpsecAuthType.CERT) {
      vpn.ipSec.clientCertType = 'PKCS11Id';
      vpn.ipSec.clientCertPkcs11Id = this.getUserCertPkcs11Id_();
      vpn.ipSec.serverCaPems = this.getServerCaPems_();
    }
    vpn.ipSec.ikeVersion = 1;
    vpn.ipSec.saveCredentials = this.vpnSaveCredentials_;
    vpn.l2tp.saveCredentials = this.vpnSaveCredentials_;

    // Clear IPsec fields which are only for IKEv2.
    delete vpn.ipSec.eap;
    delete vpn.ipSec.localIdentity;
    delete vpn.ipSec.remoteIdentity;
  },

  /**
   * @param {boolean} success
   * @param {string} errorMessage
   * @param {boolean} connect If true, connect after save.
   * @private
   */
  setPropertiesCallback_(success, errorMessage, connect) {
    if (!success) {
      console.warn(
          'Unable to set properties for: ' + this.guid +
          ' Error: ' + errorMessage);
      this.propertiesSent_ = false;
      this.setError_(errorMessage);
      this.focusPassphrase_();
      return;
    }

    // Only attempt a connection if the network is not yet connected.
    if (connect &&
        this.managedProperties_.connectionState ===
            mojom.ConnectionStateType.kNotConnected) {
      this.startConnect_(this.guid);
    } else {
      this.close_();
    }
  },

  /**
   * @param {?string} guid
   * @param {string} errorMessage
   * @param {boolean} connect If true, connect after save.
   * @private
   */
  createNetworkCallback_(guid, errorMessage, connect) {
    if (!guid) {
      console.warn(
          'Unable to configure network: ' + guid + ' Error: ' + errorMessage);
      this.propertiesSent_ = false;
      this.setError_(errorMessage);
      this.focusPassphrase_();
      return;
    }

    if (connect) {
      this.startConnect_(guid);
    } else {
      this.close_();
    }
  },

  /**
   * @param {string} guid
   * @private
   */
  startConnect_(guid) {
    this.networkConfig_.startConnect(guid).then(response => {
      const result = response.result;
      if (result === mojom.StartConnectResult.kSuccess ||
          result === mojom.StartConnectResult.kInvalidGuid ||
          result === mojom.StartConnectResult.kInvalidState ||
          result === mojom.StartConnectResult.kCanceled) {
        // Connect succeeded, or is in progress completed or canceled.
        // Close the dialog.
        this.close_();
        return;
      }
      this.setError_(response.message);
      console.warn(
          'Error connecting to network: ' + guid + ': ' + result.toString() +
          ' Message: ' + response.message);
      this.propertiesSent_ = false;
    });
  },

  /**
   * @param {chromeos.networkConfig.mojom.NetworkType|undefined} mojoType
   * @param {chromeos.networkConfig.mojom.SecurityType|undefined} securityType
   * @return {boolean}
   * @private
   */
  computeConfigRequiresPassphrase_(mojoType, securityType) {
    // Note: 'Passphrase' is only used by WiFi; Ethernet uses EAP.Password.
    return mojoType === mojom.NetworkType.kWiFi &&
        (securityType === mojom.SecurityType.kWepPsk ||
         securityType === mojom.SecurityType.kWpaPsk);
  },

  /**
   * @param {string} outer
   * @return {!Array<string>}
   * @private
   */
  getEapInnerItems_(outer) {
    if (outer === 'PEAP') {
      return this.eapInnerItemsPeap_;
    }
    if (outer === 'EAP-TTLS') {
      return this.eapInnerItemsTtls_;
    }
    return [];
  },

  /**
   * @param {string|undefined} error
   * @private
   */
  setError_(error) {
    this.error = error || '';
  },

  /**
   * Returns a managed property for policy controlled networks.
   * @param {!mojom.ManagedProperties} managedProperties
   * @return {mojom.ManagedString|undefined}
   * @private
   */
  getManagedSecurity_(managedProperties) {
    const policySource =
        OncMojo.getEnforcedPolicySourceFromOncSource(managedProperties.source);
    if (policySource === mojom.PolicySource.kNone) {
      return undefined;
    }
    switch (managedProperties.type) {
      case mojom.NetworkType.kWiFi:
        return {
          activeValue: OncMojo.getSecurityTypeString(
              managedProperties.typeProperties.wifi.security),
          policySource: policySource,
        };
        break;
      case mojom.NetworkType.kEthernet:
        return {
          activeValue: OncMojo.getActiveString(
              managedProperties.typeProperties.ethernet.authentication),
          policySource: policySource,
        };
        break;
    }
    return undefined;
  },

  /**
   * @param {!mojom.ManagedProperties} managedProperties
   * @return {!mojom.ManagedBoolean|undefined}
   * @private
   */
  getManagedVpnSaveCredentials_(managedProperties) {
    const vpn = managedProperties.typeProperties.vpn;
    switch (vpn.type) {
      case mojom.VpnType.kIKEv2:
        return vpn.ipSec.saveCredentials || OncMojo.createManagedBool(false);
      case mojom.VpnType.kOpenVPN:
        return vpn.openVpn.saveCredentials || OncMojo.createManagedBool(false);
      case mojom.VpnType.kL2TPIPsec:
        return vpn.ipSec.saveCredentials || vpn.l2tp.saveCredentials ||
            OncMojo.createManagedBool(false);
      case mojom.VpnType.kWireGuard:
        return OncMojo.createManagedBool(true);
    }
    assertNotReached();
    return undefined;
  },

  /**
   * @param {!mojom.ManagedProperties} managedProperties
   * @return {?mojom.ManagedStringList|undefined}
   * @private
   */
  getManagedVpnServerCaRefs_(managedProperties) {
    const vpn = managedProperties.typeProperties.vpn;
    switch (vpn.type) {
      case mojom.VpnType.kOpenVPN:
        return vpn.openVpn.serverCaRefs;
      case mojom.VpnType.kIKEv2:
      case mojom.VpnType.kL2TPIPsec:
        return vpn.ipSec.serverCaRefs;
    }
    assertNotReached();
    return undefined;
  },

  /**
   * @param {!mojom.ManagedProperties} managedProperties
   * @return {!mojom.ManagedString|undefined}
   * @private
   */
  getManagedVpnClientCertType_(managedProperties) {
    const vpn = managedProperties.typeProperties.vpn;
    switch (vpn.type) {
      case mojom.VpnType.kOpenVPN:
        return vpn.openVpn.clientCertType || OncMojo.createManagedString('');
      case mojom.VpnType.kIKEv2:
      case mojom.VpnType.kL2TPIPsec:
        return vpn.ipSec.clientCertType || OncMojo.createManagedString('');
    }
    assertNotReached();
    return undefined;
  },

  /** @private */
  onWifiPasswordInputKeypress_() {
    // bad-passphrase corresponds to kErrorBadPassphrase in shill
    if (this.error === 'bad-passphrase') {
      // Reset error if user starts typing new password.
      this.setError_('');
    }
  },
});
