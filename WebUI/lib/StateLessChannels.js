import {
  CVMMetaSection,
  CVMMetaEntry,
  CVMMetaGenerator,
  CVMMetaParser
} from './MetaData.js'

import { TokenPoolGeneratorProxy } from './TokenPoolGeneratorProxy.js'

//CStateLessChannelsManager is responsible for managment (including cashing-out and synchronizaiton) of State-Less Transmission Channels.
//use addChannel() to include a channel under its managment and  provisioning.
//Uses a Controller Thread of its own.
//Call initialize() to start the Operational Logic.
//Call destroy() to abort the Operational Logic.

//New information regarding Token Pools / Transmission Tokens / Transit Pools might be coming through
// 1) DFS sub-System
// 2) VM meta-data sub-subsystem
// the notifications might be triggered as a result of queries from State-Less Channel objects or be result of Manager's own inqueries
// In any case, the response is always processed first by the Manager, and MAY be dispatched to underlying objects with events fired for external applications.
export class CStateLessChannelsManager {

  static getInstance(vmContext) {
    if (CStateLessChannelsManager.sInstance == null) {
      CStateLessChannelsManager.sInstance = new CStateLessChannelsManager(vmContext);
    }

    return CStateLessChannelsManager.sInstance;
  }

  constructor(vmContext) {
    this.mChannels = [];
    this.mPendingReward = 0n;
    this.mVMContext = vmContext;
    this.mMetaParser = new CVMMetaParser();

    //automatic cash-out criterions - BEGIN
    this.mCashOutChannelsAtValue = 1000; //the amount of GBUs received PER Channel
    //that would cause it be cashed-out on-the-chain
    this.mCashOutChannelAfterTimout = 28800; //the amount of time in seconds
    //that would cause the state-less channel to be cashed-out autonomously on-the-chain (default - 8 hours)
    //automatic cash-out criterions - END
    this.mControler = null;
    this.mNetworkRequestIDs = [];
    this.mChannelSyncInterval = 600; //in sec every 10 minutes
    this.mEventListenerInPlace = false;
    this.mControllerThread = 0;
    this.mControllerThreadGuardian = false; //mutex
    this.mControllerThreadInterval = 2000;
    this.mNetworkRequestIDs = [];
    //Event Listeners - BEGIN
    this.mNewTTListeners = [];
    this.mNewTokenPoolListeners = [];
    this.mNewStateChannelListeners = [];
    this.mNewTransitPoolListeners = [];
    this.mNewReceiptListeners = [];
    //Event Listeners - END
    //agregators - BEGIN
    //Event Listeners - BEGIN
    this.mStateChannelNewStateListeners = [];
    this.mNewTransmissionTokenListeners = [];
    //Event Listeners - END
    //agregators - END

    // Token Pool Generation/Validation Event Listeners - BEGIN
    this.mPoolGenerationProgressListeners = [];
    this.mPoolGenerationCompleteListeners = [];
    this.mPoolValidationProgressListeners = [];
    this.mPoolValidationCompleteListeners = [];
    this.mSeedRecoveryProgressListeners = [];
    this.mSeedRecoveryCompleteListeners = [];
    // Token Pool Generation/Validation Event Listeners - END

    // Token Pool Generator Proxy (Web Worker) - lazy initialized
    this.mTokenPoolGeneratorProxy = null;

    // Identity tracking for system-wide seed management
    this.mCurrentUserID = vmContext ? vmContext.getUserID : null;
    this.mUserLoginListenerID = null;

    // Register for user login events to clear seeds on system-wide identity change
    this._registerUserLoginListener();

    this.mID = 'StateLessChannelsManager';
  }

  /**
   * @brief Register listener for VMContext user login events
   * @private
   *
   * When the system-wide user identity changes (via loginWithSubIdentity, QR login, etc.),
   * we clear all cached token pool seeds to prevent cross-identity leakage.
   * This is the ONLY place where seeds should be cleared system-wide.
   */
  _registerUserLoginListener() {
    if (!this.mVMContext || typeof this.mVMContext.addUserLogonListener !== 'function') {
      console.warn('[StateLessChannelsManager] VMContext not available for user login listener');
      return;
    }

    this.mUserLoginListenerID = this.mVMContext.addUserLogonListener((loginData) => {
      this._handleUserLogin(loginData);
    }, 'StateLessChannelsManager');

    console.log('[StateLessChannelsManager] Registered for user login events');
  }

  /**
   * @brief Handle user login event from VMContext
   * @private
   * @param {Object} loginData - Login event data (may contain address, method, etc.)
   *
   * Called when the system-wide identity changes. Clears all pool seeds to prevent
   * using keys derived from the previous identity.
   */
  _handleUserLogin(_loginData) {
    // Get the new user ID (we check VMContext directly rather than trusting loginData)
    const newUserID = this.mVMContext ? this.mVMContext.getUserID : null;

    // Only clear seeds if the identity actually changed
    if (newUserID && newUserID !== this.mCurrentUserID) {
      console.log(`[StateLessChannelsManager] System-wide identity changed: ${this.mCurrentUserID || 'none'} -> ${newUserID}`);
      console.log('[StateLessChannelsManager] Clearing pool seeds due to identity switch');

      // Clear all pool seeds
      this.clearPoolSeeds();

      // Update tracked identity
      this.mCurrentUserID = newUserID;
    } else if (!newUserID) {
      console.log('[StateLessChannelsManager] User logged out - clearing pool seeds');
      this.clearPoolSeeds();
      this.mCurrentUserID = null;
    }
  }


  //Retrieves channel-IDs as per the requsted directorion-property(mode).
  getChannelIDs(mode = eChannelDirection.both, allowFriendlyIDs = true) {
    let toRet = [];

    for (let i = 0; i < this.mChannels.length; i++) {
      if (mode != eChannelDirection.both) {
        if (mode == eChannelDirection.ingress && this.mChannels[i].getIsOutgress)
          continue;
        else if (mode == eChannelDirection.outgress && !this.mChannels[i].getIsOutgress)
          continue;
      }

      let friendly = this.mChannels[i].getFriendlyID;
      toRet.push((allowFriendlyIDs && friendly.length > 0) ? friendly : this.mChannels[i].getID);
    }

    return toRet;
  }
  addNewTTListener(eventListener, appID = 0) {

    if (eventListener == null)
      return false;

    this.mNewTTListeners.push({
      handler: eventListener,
      appID
    });

    return true;
  }

  get getNetworkRequestIDs() {
    this.mNetworkRequestIDs;
  }

  addNetworkRequestID(obj) {
    let id = 0;
    if (obj instanceof COperationStatus)
      id = obj.getReqID;
    else {
      id = obj;
    }

    this.mNetworkRequestIDs.push(id);
  }

  hasDFSRequestID(reqID) {
    for (var i = 0; i < this.mNetworkRequestIDs.length; i++) {
      if (this.mNetworkRequestIDs[i] == reqID)
        return true;
    }
    return false;
  }

  addNewTransitPoolListener(eventListener, appID = 0) {
    if (eventListener == null)
      return false;

    this.mNewTransitPoolListeners.push({
      handler: eventListener,
      appID
    });

    return true;
  }
  addNewReceiptListener(eventListener, appID = 0) {
    if (eventListener == null)
      return false;

    this.mNewReceiptListeners.push({
      handler: eventListener,
      appID
    });

    return true;
  }

  addNewTokenPoolListener(eventListener, appID = 0) {
    if (eventListener == null)
      return false;

    this.mNewTokenPoolListeners.push({
      handler: eventListener,
      appID
    });

    return true;
  }

  addNewStateChannelListener(eventListener, appID = 0) {
    if (eventListener == null)
      return false;

    this.mNewStateChannelListeners.push({
      handler: eventListener,
      appID
    });
    return true;
  }
  addStateChannelNewStateListener(eventListener, appID = 0) {
    if (eventListener == null)
      return false;


    this.mStateChannelNewStateListeners.push({
      handler: eventListener,
      appID
    });

    return true;
  }


  //Attempts to synchronize channels owned by specific Agent as seen by current full-node.
  //Adds channels locally under its supervision if missing.
  syncAgentChannels(agentID) {
    if (agentID == null)
      return false;
    //perform DFS command first,the particular files/pools will be fetched once (if) data arrives
    let path = '/' + gTools.arrayBufferToString(agentID) + '/TokenPools/';
    //IMPORTANT: Notice: we suppress throws on the following one:
    this.addNetworkRequestID(CVMContext.getInstance().getFileSystem.doCD(path, true, false, true).getReqID); //the second parameter tells to perform LS as a single atomic command combined with LS
    //will be taking place on the publicly accessible data thread
  }

  controllerThreadF() {
      //operational logic - BEGIN
      this.cashOutChannels();
      this.syncChannels();
      //operational logic - END
  }

  //One can sign up for events at the manager's level. If so then incoming blockchain-state-less channels' related data would be handed over to target channel (if found by ID) and processed there.
  //double-processing of events does not hurt (performance only).
  signUpForEvents() {
    if (!this.mEventListenerInPlace) {
      this.mVMContext.addVMMetaDataListener(this.newVMMetaDataCallback.bind(this), this.mID);
      this.mVMContext.addNewDFSMsgListener(this.newDFSMsgCallback.bind(this), this.mID);
      this.mEventListenerInPlace = true;
    }
  }

  //attempts to synchronize instance of managed channels with world-view presented by the current full-node.
  syncChannels(forceIt = false) {
    for (let i = 0; i < this.mChannels.length; i++) {
      if (forceIt || (this.mChannels[i].getTimeSinceLastSync > this.mChannelSyncInterval))
        this.mChannels[i].synchronize();
    }
  }
  get getNetworkRequestIDs() {
    this.mNetworkRequestIDs;
  }
  addNetworkRequestID(obj) {
    let id = 0;
    if (obj instanceof COperationStatus)
      id = obj.getReqID;
    else {
      id = obj;
    }

    this.mNetworkRequestIDs.push(id);
  }

  //fired when it's not yet decided whether it's new or not. vs onNewTokenPool()
  onTokenPoolReceived(pool, requestID = 0) {

    //Local Variables - BEGIN
    let localChannel = this.findChannelByID(pool.getID());
    let previouslySpendableAssets = -1;
    let previouslyAccumulatedAssets = -1;
    let previouslyAvailableAssets = -1;

    let currentlySpendableAssets = 0n;
    let currentlyAccumulatedAssets = 0n;
    let currentlyAvailableAssets = 0n;
    let previousStateAvailable = false;
    //Local Variables - END

    //Operational Logic - BEGIN

    /*
    When a State-Less channel coresponding to a given token-pool is not yet available
    we construct, register one and notify external components.
    */
    if (localChannel == null) {
      this.onNewTokenPool(pool, requestID);
      localChannel = new CStateLessChannel(pool.getID());
      localChannel.setTokenPool = pool;
      this.onNewChannel(localChannel, requestID);

    } else {
      previousStateAvailable = true;
      //Gather Information Regarding Previous State - BEGIN
      previouslySpendableAssets = localChannel.getLocallySpendableAssets;
      previouslyAvailableAssets = localChannel.getTokenPool.getValueLeft();
      previouslyAccumulatedAssets = localChannel.getAccumulatedValue;
      //Gather Information Regarding Previous State - END
    }
    /*
    Update the state of a token-pool if the received data represents a newer world-view.
    */

    //Gather Information Regarding Current State - BEGIN
    //Note: these could be collected by recipient himself, anyway for functionality consistency and uniformity
    //these are computed and provided here as well.
    currentlySpendableAssets = localChannel.getLocallySpendableAssets;
    currentlyAvailableAssets = localChannel.getTokenPool.getValueLeft();
    currentlyAccumulatedAssets = localChannel.getAccumulatedValue;
    //Gather Information Regarding Current State - END

    if (localChannel.updateTokenPool(pool)) {
      //only if really affected the current state
      let previousState = previousStateAvailable ? {
        availableAssets: previouslyAvailableAssets,
        spendableAssets: previouslySpendableAssets,
        accumulatedAssets: previouslyAccumulatedAssets
      } : null;
      let newState = {
        availableAssets: currentlyAvailableAssets,
        spendableAssets: currentlySpendableAssets,
        accumulatedAssets: currentlyAccumulatedAssets
      };
      this.onChannelNewState(channel, newState, previousState);
    }

    //Operational Logic - END
  }

  async newDFSMsgCallback(dfsMsg) {
    if (!this.hasDFSRequestID(dfsMsg.getReqID))
      return;
    if (dfsMsg.getData1.byteLength > 0) {

      let metaData = this.mMetaParser.parse(dfsMsg.getData1);

      if (metaData != 0) {

        let sections = this.mMetaParser.getSections;

        for (var i = 0; i < sections.length; i++) {
          let sType = sections[i].getType;
          let metaData = sections[i].getMetaData; //contains full explicit directorie's path for directory listings and particular file entry (not section) contains full file path.
          let entries = sections[i].getEntries;
          let entriesCount = entries.length;

          this.mVisibleFilesCount = 0;
          this.mVisibleFoldersCount = 0;
          this.mHtmlEntries = [];

          for (var a = 0; a < entriesCount; a++) {

            let dataFields = entries[a].getFields;

            if (dataFields.length < 3) {
              console.log('Invalid DFS message received (invalid data-fields count).');
              continue;
            }
            let name = gTools.uintToString(new Uint8Array(dataFields[0]));
            if (entries[a].getType == eDFSElementType.stateDomainEntry) {

            } else
            if (entries[a].getType == eDFSElementType.directoryEntry) {

            } else if (entries[a].getType == eDFSElementType.fileEntry) { //must have received entries from the token-pool's directory

              let nonRelativeDir = gTools.arrayBufferToString(metaData); //this non-relative path should contain both state-domain ID and file-name

              if (name.length > 0) {
                this.addNetworkRequestID(CVMContext.getInstance().getFileSystem.doGetFile(nonRelativeDir + name));
              }
            } else if (entries[a].getType == eDFSElementType.fileContent) { //we're interested only in token-pools
              //todo: verify file-extension
              let dataType = dataFields[0];
              let fileName = gTools.arrayBufferToString(dataFields[1]);
              let data = dataFields[2];
              switch (dataType) {
                case eDataType.bytes:

                  let pool = CTokenPool.instantiate(data);

                  //[todo:Paulix:medium]: replace all of the generic javascript logging with calls to gridnet-os intrinsic loggic apparatus.
                  if (pool == null) {
                    //console.log('There was an attempt to instantiate an invalid token pool data.');
                    return false;
                  }
                  this.onTokenPoolReceived(pool, dfsMsg.getReqID);
                  break;

                default:
                  console.log('Unsupported bytes received by StateLess Channels manager... =/')
                  return;
              }

              return;
            }
          }
        }

      } else {

        return;

      }
    } else {

    }

  }

  /*
  Invoked whenever a new Transmission Token arrives.
  */
  onTT(token, isBankUpdate = false) {

    //Core processing - BEGIN
    console.log('Processing new Transmission Token.' + (isBankUpdate ? " - It's a Bank Update." : ""));
    this.processTT(token, isBankUpdate);
    //Core processing - END

    //External notifications
    for (let i = 0; i < this.mNewTTListeners.length; i++) {
      try {

        if (!gTools.isAsync(this.mNewTTListeners[i].handler)) {
          console.log('Event handler needs to be async!');
          continue;
        }

        this.mNewTTListeners[i].handler({
          token,
          isBankUpdate
        }); //pack all these into a single field to support (this)
      } catch (error) {
        console.long('State-Less Channel Notification handling error in app:' + this.mNewTTListeners[i].appID);

      }
    }
  }

  /*
  Executed whenever new information becomes available.
  Note: previousState might be null if no previous data avaiable.
  */
  onChannelNewState(channel, newState, previousState = null) {

    console.log('Core: The state of StateChannel ' + gTools.base58CheckEncode(channel.getID) + ' changed.');

    for (let i = 0; i < this.mStateChannelNewStateListeners.length; i++) {
      try {
        if (!gTools.isAsync(this.mStateChannelNewStateListeners[i].handler)) {
          console.log('Event handler needs to be async!');
          continue;
        }

        this.mStateChannelNewStateListeners[i].handler({
          channel,
          previousState,
          newState
        }); //pack all these into a single field to support (this)
      } catch (error) {
        console.long('State-Less Channel Notification handling error in app:' + this.mStateChannelNewStateListeners[i].appID);

      }
    }
  }

  onNewChannel(channel, requestID) {
    if (channel == null || !(channel instanceof CStateLessChannel))
      return;

    //Core processing - BEGIN
    channel.ping();
    this.mChannels.push(channel);
    //Core processing - END

    //External notifications
    for (let i = 0; i < this.mNewStateChannelListeners.length; i++) {
      try {
        if (!gTools.isAsync(this.mNewStateChannelListeners[i].handler))
          continue;
        this.mNewStateChannelListeners[i].handler({
          channel,
          requestID
        }); //pack all these into a single field to support (this)
      } catch (error) {
        console.long('State-Less Channel Notification handling error in app:' + this.mNewStateChannelListeners[i].appID);

      }
    }
  }

  //onNewChannel will follow after it.
  onNewTokenPool(pool, requestID = 0) {
    //Core processing - BEGIN
    this.processTokenPool(pool);
    //Core processing - END

    //External notifications
    for (let i = 0; i < this.mNewTokenPoolListeners.length; i++) {
      try {

        if (!gTools.isAsync(this.mNewTokenPoolListeners[i].handler)) {
          console.log('Event handler needs to be async!');
          continue;
        }
        this.mNewTokenPoolListeners[i].handler({
          pool,
          requestID
        }); //pack all these into a single field to support (this)
      } catch (error) {
        console.long('State-Less Channel Notification handling error in app:' + this.mNewTokenPoolListeners[i].appID);

      }
    }
  }
  onNewReceipt(receipt) {
    //Core processing - BEGIN
    this.processReceipt(receipt);

    //Core processing - END

    //External notifications

    for (let i = 0; i < this.mNewRece.length; i++) {
      try {

        if (!gTools.isAsync(this.mNewTokenPoolListeners[i].handler)) {
          console.log('Event handler needs to be async!');
          continue;
        }

        this.mNewReceiptListeners[i].handler({
          receipt
        }); //pack all these into a single field to support (this)
      } catch (error) {
        console.long('State-Less Channel Notification handling error in app:' + this.mNewReceiptListeners[i].appID);

      }
    }
  }
  onNewTransitPool(token) {
    //Core processing - BEGIN
    this.processReceipt(receipt);

    //Core processing - END

    //External notifications

    for (let i = 0; i < this.mNewTransitPoolListeners.length; i++) {
      try {
        if (!gTools.isAsync(this.mNewTransitPoolListeners[i].handler)) {
          console.log('Event handler needs to be async!');
          continue;
        }

        this.mNewTransitPoolListeners[i].handler({
          token
        }); //pack all these into a single field to support (this)
      } catch (error) {
        console.long('State-Less Channel Notification handling error in app:' + this.mNewTransitPoolListeners[i].appID);

      }
    }
  }

  /*
  Invoked by the Manager whenever a Transmission Token's processing is needed.
  Note: The function returns false when the sub-system is not ready to process the request yet.
  Otherwise, the value of the Transmission Token is returned.
  */
  processTT(token, isBankUpdate = false) {

    //Local Variables - BEGIN
    let channel = this.findChannelByID(token.getTokenPoolID());
    let previouslySpendableAssets = -1;
    let previouslyAccumulatedAssets = -1;
    let previouslyAvailableAssets = -1;

    let currentlySpendableAssets = 0n;
    let currentlyAccumulatedAssets = 0n;
    let currentlyAvailableAssets = 0n;
    let previousStateAvailable = false;
    //Local Variables - END

    //Operational Logic - BEGIN

    if (isBankUpdate) { //we presume that the token was delivered through a mobile app. todo: check source.
      CVMContext.getInstance().notifyMobileToken(eOperationStatus.success, eOperationScope.peer, this.getRecentQRIntentTaskID);
    }

    if (channel == null) {
      console.log('New State-Channel created as a result of a received Transmission Token').
      //the StateLessChannel which the Transmission Token referenced is unknown => let us create it and register locally
      //Note: for now we return false, since it's certain that it would take some time for the Channel to initialize itself,
      //fetch state-information from the decentalized state-machine etc.
      this.onNewTokenPool(pool);
      let channel = new CStateLessChannel(token.getTokenPoolID());
      channel.setTokenPool = pool;
      this.onNewChannel(channel);
      return false;
    } else {
      previousStateAvailable = true;
      //Gather Information Regarding Previous State - BEGIN
      previouslySpendableAssets = channel.getLocallySpendableAssets;
      previouslyAvailableAssets = channel.getTokenPool.getValueLeft();
      previouslyAccumulatedAssets = channel.getAccumulatedValue;
      //Gather Information Regarding Previous State - END
    }

    channel.ping();

    let res = channel.validateTT(token, true, isBankUpdate);

    //Gather Information Regarding Current State - BEGIN
    //Note: these could be collected by recipient himself, anyway for functionality consistency and uniformity
    //these are computed and provided here as well.
    currentlySpendableAssets = channel.getLocallySpendableAssets;
    currentlyAvailableAssets = channel.getTokenPool.getValueLeft();
    currentlyAccumulatedAssets = channel.getAccumulatedValue;
    //Gather Information Regarding Current State - END

    if (res) //if processing affected  Token Pool's state
    {
      let previousState = previousStateAvailable ? {
        availableAssets: previouslyAvailableAssets,
        spendableAssets: previouslySpendableAssets,
        accumulatedAssets: previouslyAccumulatedAssets
      } : null;
      let newState = {
        availableAssets: currentlyAvailableAssets,
        spendableAssets: currentlySpendableAssets,
        accumulatedAssets: currentlyAccumulatedAssets
      };
      this.onChannelNewState(channel, newState, previousState);
    }
    //Operational Logic - END

    return res;
  }

  /*
  Invoked by the Manager whenever a Token Pool's processing is needed.
  */
  processTokenPool(pool) {
    let channel = this.findChannelByID(pool.getID());
    if (channel == null)
      return false;
  }

  processReceipt(receipt) {

  }

  //The following method listens for incoming VM-Meta messages in an attempt to come upon a stateLessChannels section.
  //When found, coresponding objects are instantiated and notifications issued to coresponding State-Less Channels (created on demand autonomously).
  //External applications are notified through Callback events as well.
  //It is an agregator (calls particular Channel's and hands data over if specific Channel available ~checked by ID)
  async newVMMetaDataCallback(metaMsg) { //Callback is called when new state-less blockchain channels'-related data arrived from either another web-peer or full-node
    //data shall be handed over to the state-less Channel of interest if available and registered within the Manager.

    if (metaMsg.getData.byteLength > 0) {
      let metaData = this.mMetaParser.parse(metaMsg.getData);
      //todo: look through meta-data, deserialize Token Pool, validate it.
      if (metaData != 0) {

        let sections = this.mMetaParser.getSections;

        for (var i = 0; i < sections.length; i++) {
          let sType = sections[i].getType;

          let entries = sections[i].getEntries;
          let entriesCount = entries.length;
          if (sType != eVMMetaSectionType.stateLessChannels) //we are only concerned with stateLessChannels-related data (pools/tokens)
            continue;

          this.mVisibleFilesCount = 0;
          this.mVisibleFoldersCount = 0;
          this.mHtmlEntries = [];

          for (var a = 0; a < entriesCount; a++) {

            let dataFields = entries[a].getFields;

            let name = gTools.uintToString(new Uint8Array(dataFields[0]));

            if (entries[a].getType == eVMMetaEntryType.StateLessChannelsElement) { //check if that's a StateLessChannels entry at all?

              if (dataFields.length < 3) {
                console.log('Invalid StateLessChannelsElement received.');
                break;
              }
              //State-less channel entry-specific processing - BEGIN
              let elementType = dataFields[0];
              let name = gTools.arrayBufferToString(dataFields[1]);
              let data = dataFields[2];

              if (elementType == eStateLessChannelsElementType.token || elementType == eStateLessChannelsElementType.tokenPool ||
                elementType == eStateLessChannelsElementType.transitPool || elementType == eStateLessChannelsElementType.bankUpdate) {

                switch (elementType) {
                  case eStateLessChannelsElementType.bankUpdate:

                    /*
                    The aim of a 'bankUpdate' is to make additional assets available at the disposal of the web-app.
                    i.e. additional parts of a specific multi-dimensional token-pool are being revealed by the mobile token app and thus made available for spending.
                    */

                    /*
                    Discussion: The secret hashes will be injected to coresponding dimensions. The indexes indicating
                    preCachedSeed'hashes will be updated as well. Thus rendering them available for spending by the local Web-Peer.
                    The local Web-Peer will be able to generate Transmission Tokens based on the provided data.
                    */
                    console.log('New Bank Update received.');

                    let token = CTransmissionToken.instantiate(data);

                    //it might contain invalid or missing data => perform formal checks
                    if (!token.validate()) {
                      console.log('Invalid Transmission Token received.');
                      return;
                    }

                    /*  //1) retrieve state-channel referenced by this bank-update
                    let channel = this.findChannelByID(token.getTokenPoolID());

                    //if does not exist,-> create one and register with the Manager (this).

                    if (!channel) { //let us create the autonomous State-Channel
                      channel = new CStateLessChannel(token.getTokenPoolID());
                      this.addChannel(channel); // the channel would be autonomously initialized, based on the data available from within of the decentralized state-machine.
                      //with updates coming in at the pre-specified intervals. Ownership of the channel, including mode of operaitng (ingress/outgress would be also detected autonomously
                      //based on the user's identifier, given at the logon-stage).
                    }

                    //2) invoke validateTT with updateState and isBankUpdate parameters set to True
                    channel.getTokenPool.validateTT(token, true, true);

                    if (token == null)
                      return false;
*/
                    this.onTT(token, true);

                    break;
                  case eStateLessChannelsElementType.tokenPool:
                    console.log('New Token Pool received.');
                    let pool = CTokenPool.instantiate(data);

                    if (pool == null)
                      return false;

                    this.onTokenPoolReceived(pool);

                    break;

                  case eStateLessChannelsElementType.token:
                    console.log('New Token received.');

                    token = CTransmissionToken.instantiate(data);

                    if (token == null)
                      return false;

                    this.onTT(token);

                    break;

                  case eStateLessChannelsElementType.transitPool:
                    console.log('New Transit Pool received.');
                    token = CTransmissionToken.instantiate(data);

                    if (token == null)
                      return false;

                    this.onTT(token);
                    break;

                  default:

                    return false;
                }

                return;
              }
              //State-less channel entry-specific processing - END
            }
          }

        }

      } else {

        return;

      }
    }
  }


  //Adds cash-out Consensus Tasks to the Consensus Processing Qeueue
  //If no token-pool/channel identifiers provided, does this for all current connections meeting timout-out AND/OR value criterions (if not forced).
  //IF forced, attempts to cash-out all available Channels.
  //Note: Critetions considered on a per-channel basis.
  cashOutChannels(forceIt = false, autoComit = true, channelIDsp = [], abortOnNotReady = true) {

    //local parameters - BEGIN
    let toCashOut = [];
    let channelIDs = []; //what to consider
    //local parameters - END

    if (channelIDsp.length > 0)
      channelIDs = channelIDsp; //use provided
    else {
      for (let i = 0; i < this.mChannels.length; i++) { //everything
        channelIDs.push(this.mChannels[i].getID);
      }
    }
    //Operational Logic - BEGIN

    //Filtration - BEGIN
    if (channelIDs.length > 0) {
      for (let i = 0; i < channelIDs.length; i++) {

        let found = this.findChannelByID(channelIDs[i]);

        if (found == null) {
          if (abortOnNotReady)
            return false; //abort if data in regard to at least one channel is missing
          else continue;
        }
        if (found.getRecentToken == null) {
          if (abortOnNotReady)
            return false; //abort if no token available for the particular channel
          else continue;
        }

        if (found.getValue == 0)
          continue; //nothing to cash-out

        if (forceIt)
          toCashOut.push(found);
        else {
          if (toCashOut.getTimeSinceLastUpdate > (this.mCashOutChannelAfterTimout * 1000)) //ms
            toCashOut.push(found);
        }
      }
      if (toCashOut.length == 0)
        return false;

      //Filtration - END

      //Actual cashing-out - BEGIN

      for (let i = 0; i < toCashOut.length; i++) {
        toCashOut[i].cashOut();
      }

      //Actual cashing-out - END

      //optional Transaction Comitment Trigger to the decentralized VM follows
      if (autoComit)
        this.mVMContext.comit();
      return true;

    } else return false;
    //Operational Logic - END
  }

  destroy() {
    if (this.mControllerThread > 0)
      CVMContext.getInstance().stopJSThread(this.mControllerThread);
    this.mControllerThread = 0;
  }

  initialize() {
    this.signUpForEvents();
    this.mControllerThread = CVMContext.getInstance().createJSThread(this.controllerThreadF.bind(this), 0, this.mControllerThreadInterval, true, true);
  }
  //Threshold at which to cash-out the blockchain channel automatically. (it is then delivered to VM)
  set setCashoutThreshold(value) {
    this.mCashOutChannelAt = value;
  }

  get getCashoutThreshold() {
    return this.mCashOutChannelAt;
  }

  //Make channel a managed one - under the Manager.
  addChannel(channel) {
    if (channel == null || !(channel instanceof CStateLessChannel))
      return false;

    this.onNewChannel(channel);
    return true;
  }

  //Retrieved Blockchain State-less Channel by ID
  findChannelByID(channelID, allowFriendlyID = true) {
    channelID = gTools.convertToArrayBuffer(channelID);

    if (channelID == null || channelID.byteLength == 0)
      return null;

    for (let i = 0; i < this.mChannels.length; i++) {
      if (gTools.compareByteVectors(this.mChannels[i].getID, channelID) ||
        (allowFriendlyID && (gTools.compareByteVectors(gTools.convertToArrayBuffer(this.mChannels[i].getFriendlyID), channelID))))
        return this.mChannels[i];
    }
    return null;
  }

  //Retrieved Blockchain State-less Channel by State Domain ID
  findChannelsByOwner(ownerID, allowFriendlyID = true) {

    let toRet = [];
    ownerID = gTools.convertToArrayBuffer(ownerID);

    if (ownerID == null || ownerID.byteLength == 0)
      return null;

    for (let i = 0; i < this.mChannels.length; i++) {
      if (gTools.compareByteVectors(this.mChannels[i].getTokenPool.getOwnerID, ownerID))
        toRet.push(this.mChannels[i]);
    }
    return toRet;
  }

  // ============================================================================
  // TOKEN POOL GENERATION/VALIDATION API
  // ============================================================================

  /**
   * @brief Get or initialize the TokenPoolGeneratorProxy (Web Worker)
   * @returns {TokenPoolGeneratorProxy} The proxy instance
   * @private
   */
  _getTokenPoolGeneratorProxy() {
    if (!this.mTokenPoolGeneratorProxy) {
      this.mTokenPoolGeneratorProxy = TokenPoolGeneratorProxy.getInstance();
    }
    return this.mTokenPoolGeneratorProxy;
  }

  // ============================================================================
  // EVENT LISTENER REGISTRATION - Token Pool Generation
  // ============================================================================

  /**
   * @brief Register listener for token pool generation progress events
   * @param {Function} eventListener - Callback(progressData) with progress info
   * @param {string|number} appID - Application identifier for filtering
   * @returns {boolean} Success
   */
  addPoolGenerationProgressListener(eventListener, appID = 0) {
    if (eventListener == null) return false;
    this.mPoolGenerationProgressListeners.push({ handler: eventListener, appID });
    return true;
  }

  /**
   * @brief Register listener for token pool generation completion events
   * @param {Function} eventListener - Callback({ pool, success, error }) on completion
   * @param {string|number} appID - Application identifier for filtering
   * @returns {boolean} Success
   */
  addPoolGenerationCompleteListener(eventListener, appID = 0) {
    if (eventListener == null) return false;
    this.mPoolGenerationCompleteListeners.push({ handler: eventListener, appID });
    return true;
  }

  /**
   * @brief Register listener for pool validation progress events
   * @param {Function} eventListener - Callback(progressData) with validation progress
   * @param {string|number} appID - Application identifier for filtering
   * @returns {boolean} Success
   */
  addPoolValidationProgressListener(eventListener, appID = 0) {
    if (eventListener == null) return false;
    this.mPoolValidationProgressListeners.push({ handler: eventListener, appID });
    return true;
  }

  /**
   * @brief Register listener for pool validation completion events
   * @param {Function} eventListener - Callback({ result, success, error }) on completion
   * @param {string|number} appID - Application identifier for filtering
   * @returns {boolean} Success
   */
  addPoolValidationCompleteListener(eventListener, appID = 0) {
    if (eventListener == null) return false;
    this.mPoolValidationCompleteListeners.push({ handler: eventListener, appID });
    return true;
  }

  /**
   * @brief Register listener for seed recovery progress events
   * @param {Function} eventListener - Callback(progressData) with recovery progress
   * @param {string|number} appID - Application identifier for filtering
   * @returns {boolean} Success
   */
  addSeedRecoveryProgressListener(eventListener, appID = 0) {
    if (eventListener == null) return false;
    this.mSeedRecoveryProgressListeners.push({ handler: eventListener, appID });
    return true;
  }

  /**
   * @brief Register listener for seed recovery completion events
   * @param {Function} eventListener - Callback({ recoveredCount, totalCount, success }) on completion
   * @param {string|number} appID - Application identifier for filtering
   * @returns {boolean} Success
   */
  addSeedRecoveryCompleteListener(eventListener, appID = 0) {
    if (eventListener == null) return false;
    this.mSeedRecoveryCompleteListeners.push({ handler: eventListener, appID });
    return true;
  }

  // ============================================================================
  // EVENT FIRING HELPERS
  // ============================================================================

  /** @private */
  _firePoolGenerationProgress(progressData) {
    for (const listener of this.mPoolGenerationProgressListeners) {
      try { listener.handler(progressData); } catch (e) { console.error('[StateLessChannelsManager] Generation progress listener error:', e); }
    }
  }

  /** @private */
  _firePoolGenerationComplete(result) {
    for (const listener of this.mPoolGenerationCompleteListeners) {
      try { listener.handler(result); } catch (e) { console.error('[StateLessChannelsManager] Generation complete listener error:', e); }
    }
  }

  /** @private */
  _firePoolValidationProgress(progressData) {
    for (const listener of this.mPoolValidationProgressListeners) {
      try { listener.handler(progressData); } catch (e) { console.error('[StateLessChannelsManager] Validation progress listener error:', e); }
    }
  }

  /** @private */
  _firePoolValidationComplete(result) {
    for (const listener of this.mPoolValidationCompleteListeners) {
      try { listener.handler(result); } catch (e) { console.error('[StateLessChannelsManager] Validation complete listener error:', e); }
    }
  }

  /** @private */
  _fireSeedRecoveryProgress(progressData) {
    for (const listener of this.mSeedRecoveryProgressListeners) {
      try { listener.handler(progressData); } catch (e) { console.error('[StateLessChannelsManager] Recovery progress listener error:', e); }
    }
  }

  /** @private */
  _fireSeedRecoveryComplete(result) {
    for (const listener of this.mSeedRecoveryCompleteListeners) {
      try { listener.handler(result); } catch (e) { console.error('[StateLessChannelsManager] Recovery complete listener error:', e); }
    }
  }

  // ============================================================================
  // ASYNC API - Token Pool Generation
  // ============================================================================

  /**
   * @brief Generate a new token pool using Web Worker
   *
   * This method offloads the computationally expensive hash chain generation
   * to a Web Worker, preventing main thread blocking. Progress callbacks and
   * events allow UI feedback during generation.
   *
   * @param {Object} params - Pool generation parameters
   * @param {number} params.dimensionsCount - Number of banks/dimensions
   * @param {BigInt|string} params.dimensionDepth - Tokens per bank (hash chain length)
   * @param {BigInt|string} params.totalValue - Total pool value in GBUs
   * @param {ArrayBuffer|Uint8Array} params.ownerID - Pool owner identifier
   * @param {ArrayBuffer|Uint8Array} params.receiptID - Sacrifice receipt ID
   * @param {string} params.friendlyID - Human-readable pool name
   * @param {ArrayBuffer|Uint8Array} [params.tokenPoolID] - Custom pool ID (auto-generated if not provided)
   * @param {ArrayBuffer|Uint8Array} [params.seed] - Deterministic seed (random if not provided)
   * @param {number} [params.slot] - Slot index for deterministic seed tracking
   * @param {boolean} [params.verify=true] - Verify hash chains after generation
   * @param {Function} [progressCallback] - Called with progress updates { progress, currentBank, totalBanks, phase, etaMs, hashRate }
   * @param {number} [timeoutMs=300000] - Generation timeout in milliseconds (default 5 minutes)
   * @returns {Promise<CTokenPool>} The generated token pool
   * @throws {Error} If generation fails or times out
   *
   * @example
   * const pool = await channelsManager.generateTokenPool({
   *   dimensionsCount: 4,
   *   dimensionDepth: 1000n,
   *   totalValue: 10000000n,
   *   ownerID: ownerBuffer,
   *   receiptID: receiptBuffer,
   *   friendlyID: 'My Pool',
   *   seed: keyChain.deriveTokenPoolSeed(slot),
   *   slot: slot
   * }, (progress) => {
   *   console.log(`${progress.progress}% - Bank ${progress.currentBank}/${progress.totalBanks}`);
   * });
   */
  async generateTokenPool(params, progressCallback = null, timeoutMs = 300000) {
    const proxy = this._getTokenPoolGeneratorProxy();

    // Combined progress handler: fires events AND calls user callback
    const combinedProgressCallback = (progressData) => {
      // Fire event for all listeners
      this._firePoolGenerationProgress(progressData);
      // Call user's callback if provided
      if (progressCallback) {
        try { progressCallback(progressData); } catch (e) { console.error('[StateLessChannelsManager] Progress callback error:', e); }
      }
    };

    try {
      console.log('[StateLessChannelsManager] Starting token pool generation...');
      const pool = await proxy.generatePool(params, combinedProgressCallback, timeoutMs);

      // Fire completion event
      this._firePoolGenerationComplete({ pool, success: true, error: null });

      console.log('[StateLessChannelsManager] Token pool generation complete');
      return pool;

    } catch (error) {
      console.error('[StateLessChannelsManager] Token pool generation failed:', error);
      // Fire completion event with error
      this._firePoolGenerationComplete({ pool: null, success: false, error: error.message });
      throw error;
    }
  }

  // ============================================================================
  // ASYNC API - Token Pool Validation
  // ============================================================================

  /**
   * @brief Validate a token pool seed by recomputing hash chains in Web Worker
   *
   * This method validates that a given seed (typically regenerated from keychain)
   * produces the correct final hashes for all banks. Used during seed recovery
   * to verify the correct slot was found.
   *
   * @param {Object} params - Validation parameters
   * @param {ArrayBuffer|Uint8Array} params.seed - Master seed to validate (32 bytes)
   * @param {Array<Object>} params.banks - Bank data with finalHash for each bank
   * @param {BigInt|number} params.dimensionDepth - Number of hashes per bank
   * @param {number} params.dimensionsCount - Number of banks
   * @param {Function} [progressCallback] - Called with progress updates
   * @param {number} [timeoutMs=300000] - Validation timeout in milliseconds
   * @returns {Promise<Object>} Validation result { valid, validBanks, totalBanks, errors, bankResults, elapsedMs }
   * @throws {Error} If validation fails or times out
   *
   * @example
   * const result = await channelsManager.validatePoolSeed({
   *   seed: keyChain.deriveTokenPoolSeed(slot),
   *   banks: pool.getBanks().map((b, i) => ({ bankID: i, finalHash: b.getFinalHash(false) })),
   *   dimensionDepth: pool.getDimensionDepth(),
   *   dimensionsCount: pool.getDimensionsCount()
   * });
   * if (result.valid) {
   *   console.log('Seed is valid!');
   * }
   */
  async validatePoolSeed(params, progressCallback = null, timeoutMs = 300000) {
    console.log('[ Token Pool Handling ] validatePoolSeed called, timeout:', timeoutMs, 'banks:', params.banks?.length || 0);

    const proxy = this._getTokenPoolGeneratorProxy();
    console.log('[ Token Pool Handling ] Got proxy, proxy ready:', !!proxy);

    // Combined progress handler
    const combinedProgressCallback = (progressData) => {
      this._firePoolValidationProgress(progressData);
      if (progressCallback) {
        try { progressCallback(progressData); } catch (e) { console.error('[ Token Pool Handling ] Validation progress callback error:', e); }
      }
    };

    try {
      console.log('[ Token Pool Handling ] Starting Web Worker pool seed validation...');
      const validationStart = performance.now();

      const result = await proxy.validatePool(params, combinedProgressCallback, timeoutMs);

      const validationTime = performance.now() - validationStart;
      console.log(`[ Token Pool Handling ] Web Worker validation complete in ${validationTime.toFixed(0)}ms: ${result.validBanks}/${result.totalBanks} banks valid`);

      // Fire completion event
      this._firePoolValidationComplete({ result, success: true, error: null });

      return result;

    } catch (error) {
      console.error('[ Token Pool Handling ] Pool seed validation FAILED:', error);
      this._firePoolValidationComplete({ result: null, success: false, error: error.message });
      throw error;
    }
  }

  // ============================================================================
  // ASYNC API - Pool Slot Finding
  // ============================================================================

  /**
   * @brief Find the correct slot index for a token pool by validating against final hashes
   *
   * Iterates through slot indices (0 to maxSlots-1), deriving seed for each slot
   * from the keychain and validating against the pool's stored final hashes.
   * Returns the first slot where all banks validate successfully.
   *
   * @param {CTokenPool} pool - The token pool to find slot for
   * @param {Object} keyChain - Unlocked keychain with deriveTokenPoolSeed() method
   * @param {Function} [progressCallback] - Called with progress updates { currentSlot, maxSlots, progress, ... }
   * @param {number} [maxSlots=100] - Maximum slots to search
   * @param {number} [timeoutPerSlot=120000] - Timeout per slot validation (ms)
   * @returns {Promise<number|null>} Slot index if found, null otherwise
   *
   * @example
   * const slot = await channelsManager.findPoolSlot(pool, activeKeyChain, (progress) => {
   *   console.log(`Checking slot ${progress.currentSlot}...`);
   * });
   * if (slot !== null) {
   *   pool.regenerateSeedFromKeyChain(activeKeyChain, slot);
   * }
   */
  async findPoolSlot(pool, keyChain, progressCallback = null, maxSlots = 100, timeoutPerSlot = 120000) {
    console.log('[ Token Pool Handling ] findPoolSlot called, maxSlots:', maxSlots, 'timeout per slot:', timeoutPerSlot);

    if (!pool || !keyChain) {
      console.warn('[ Token Pool Handling ] findPoolSlot ABORT: pool or keyChain is null');
      return null;
    }

    const banks = pool.getBanks ? pool.getBanks() : [];
    console.log('[ Token Pool Handling ] Pool has', banks.length, 'banks');

    if (banks.length === 0) {
      console.warn('[ Token Pool Handling ] findPoolSlot ABORT: no banks');
      return null;
    }

    const dimensionDepth = pool.getDimensionDepth ? pool.getDimensionDepth() : pool.mDimensionDepth;
    const dimensionsCount = banks.length;

    console.log('[ Token Pool Handling ] dimensionDepth:', dimensionDepth?.toString?.() || dimensionDepth, 'dimensionsCount:', dimensionsCount);

    // Prepare bank data for validation
    const bankData = banks.map((bank, index) => ({
      bankID: index,
      finalHash: bank.getFinalHash ? bank.getFinalHash(false) : bank.mFinalHash
    }));

    console.log(`[ Token Pool Handling ] Searching for pool slot (max ${maxSlots} slots), bankData prepared:`, bankData.length, 'banks');

    // Try each slot until we find a match
    for (let slot = 0; slot < maxSlots; slot++) {
      console.log(`[ Token Pool Handling ] Trying slot ${slot}/${maxSlots}...`);
      const seedBuffer = keyChain.deriveTokenPoolSeed(slot);

      try {
        const result = await this.validatePoolSeed({
          seed: seedBuffer,
          banks: bankData,
          dimensionDepth: dimensionDepth,
          dimensionsCount: dimensionsCount
        }, (progress) => {
          if (progressCallback) {
            progressCallback({
              ...progress,
              currentSlot: slot,
              maxSlots: maxSlots,
              slotProgress: ((slot / maxSlots) * 100)
            });
          }
        }, timeoutPerSlot);

        console.log(`[ Token Pool Handling ] Slot ${slot} validation result: valid=${result.valid}, banks=${result.validBanks}/${result.totalBanks}`);

        if (result.valid) {
          console.log(`[ Token Pool Handling ] FOUND matching slot ${slot}!`);
          return slot;
        }
      } catch (error) {
        console.warn(`[ Token Pool Handling ] Validation error for slot ${slot}:`, error.message);
      }

      // Report slot completion progress
      if (progressCallback) {
        progressCallback({
          progress: ((slot + 1) / maxSlots) * 100,
          currentSlot: slot + 1,
          maxSlots: maxSlots,
          phase: 'searching',
          slotComplete: true
        });
      }
    }

    console.warn('[ Token Pool Handling ] No matching slot found after checking all', maxSlots, 'slots');
    return null;
  }

  // ============================================================================
  // ASYNC API - Seed Recovery for Multiple Pools
  // ============================================================================

  /**
   * @brief Regenerate seeds for all loaded token pools using the active keychain
   *
   * Iterates through all outgress channels, finding the correct slot for each
   * pool that needs seed recovery, and regenerates the seed from the keychain.
   *
   * @param {Object} keyChain - Unlocked keychain with deriveTokenPoolSeed() method
   * @param {Function} [progressCallback] - Called with overall progress updates
   * @param {number} [maxSlotsPerPool=100] - Maximum slots to search per pool
   * @returns {Promise<Object>} Recovery result { recoveredCount, totalCount, alreadyValid, failed, details }
   *
   * @example
   * const activeKeyChain = await keyChainManager.getActiveKeyChain(this);
   * const result = await channelsManager.regeneratePoolSeeds(activeKeyChain, (progress) => {
   *   updateProgressUI(progress);
   * });
   * console.log(`Recovered ${result.recoveredCount}/${result.totalCount} pools`);
   */
  async regeneratePoolSeeds(keyChain, progressCallback = null, maxSlotsPerPool = 100) {
    if (!keyChain || typeof keyChain.deriveTokenPoolSeed !== 'function') {
      console.error('[StateLessChannelsManager] Invalid keychain for seed recovery');
      return { recoveredCount: 0, totalCount: 0, alreadyValid: 0, failed: 0, details: [] };
    }

    // Get all outgress channels
    const channelIDs = this.getChannelIDs(eChannelDirection.outgress);
    const details = [];
    let alreadyValid = 0;
    let recoveredCount = 0;
    let failed = 0;

    // Filter to pools needing recovery
    const poolsNeedingRecovery = [];
    for (const channelID of channelIDs) {
      const channel = this.findChannelByID(channelID);
      if (!channel) continue;

      const pool = channel.getTokenPool;
      if (!pool) continue;

      if (pool.getIsMasterSeedHashAvailable && pool.getIsMasterSeedHashAvailable() && pool.canSpendTokens()) {
        alreadyValid++;
        details.push({ poolName: pool.getFriendlyID || 'Unknown', status: 'already_valid', slot: pool.getSlot ? pool.getSlot() : null });
        continue;
      }

      poolsNeedingRecovery.push({ channel, pool, channelID });
    }

    const totalNeedingRecovery = poolsNeedingRecovery.length;
    console.log(`[StateLessChannelsManager] ${totalNeedingRecovery} pools need seed recovery, ${alreadyValid} already valid`);

    // Fire initial progress
    this._fireSeedRecoveryProgress({
      phase: 'starting',
      totalPools: totalNeedingRecovery,
      currentPool: 0,
      progress: 0
    });

    // Process each pool
    for (let i = 0; i < poolsNeedingRecovery.length; i++) {
      const { channel, pool, channelID } = poolsNeedingRecovery[i];
      const poolName = pool.getFriendlyID || 'Unnamed Pool';

      // Fire progress for this pool
      this._fireSeedRecoveryProgress({
        phase: 'finding_slot',
        totalPools: totalNeedingRecovery,
        currentPool: i + 1,
        currentPoolName: poolName,
        progress: (i / totalNeedingRecovery) * 100
      });

      if (progressCallback) {
        progressCallback({
          phase: 'finding_slot',
          totalPools: totalNeedingRecovery,
          currentPool: i + 1,
          currentPoolName: poolName,
          progress: (i / totalNeedingRecovery) * 100
        });
      }

      try {
        // Find the correct slot
        const slot = await this.findPoolSlot(pool, keyChain, (slotProgress) => {
          const overallProgress = ((i + (slotProgress.progress / 100)) / totalNeedingRecovery) * 100;
          this._fireSeedRecoveryProgress({
            phase: 'validating',
            totalPools: totalNeedingRecovery,
            currentPool: i + 1,
            currentPoolName: poolName,
            currentSlot: slotProgress.currentSlot,
            progress: overallProgress,
            ...slotProgress
          });
          if (progressCallback) {
            progressCallback({
              phase: 'validating',
              totalPools: totalNeedingRecovery,
              currentPool: i + 1,
              currentPoolName: poolName,
              currentSlot: slotProgress.currentSlot,
              progress: overallProgress,
              ...slotProgress
            });
          }
        }, maxSlotsPerPool);

        if (slot !== null) {
          // Apply the validated seed
          const seedBuffer = keyChain.deriveTokenPoolSeed(slot);
          pool.setMasterSeedHash(seedBuffer);
          pool.setSlot(slot);

          // Regenerate bank seeds
          if (pool.regenerateBankSeeds) {
            pool.regenerateBankSeeds();
          }

          recoveredCount++;
          details.push({ poolName, status: 'recovered', slot });
          console.log(`[StateLessChannelsManager] Recovered seed for "${poolName}" at slot ${slot}`);
        } else {
          failed++;
          details.push({ poolName, status: 'not_found', slot: null });
          console.warn(`[StateLessChannelsManager] Could not find slot for "${poolName}"`);
        }
      } catch (error) {
        failed++;
        details.push({ poolName, status: 'error', error: error.message });
        console.error(`[StateLessChannelsManager] Error recovering "${poolName}":`, error);
      }
    }

    const result = {
      recoveredCount,
      totalCount: totalNeedingRecovery,
      alreadyValid,
      failed,
      details
    };

    // Fire completion event
    this._fireSeedRecoveryComplete(result);
    if (progressCallback) {
      progressCallback({
        phase: 'complete',
        ...result,
        progress: 100
      });
    }

    console.log(`[StateLessChannelsManager] Seed recovery complete: ${recoveredCount}/${totalNeedingRecovery} recovered, ${alreadyValid} already valid, ${failed} failed`);
    return result;
  }

  /**
   * @brief Check if Web Worker proxy is ready
   * @returns {boolean} True if worker is initialized and ready
   */
  isGeneratorReady() {
    return this.mTokenPoolGeneratorProxy && this.mTokenPoolGeneratorProxy.ready;
  }

  /**
   * @brief Get the next available slot for a new token pool
   * Scans existing pools to find the first unused slot index
   * @returns {number} The next available slot index
   */
  getNextAvailablePoolSlot() {
    const usedSlots = new Set();

    const channelIDs = this.getChannelIDs(eChannelDirection.outgress);
    for (const channelID of channelIDs) {
      const channel = this.findChannelByID(channelID);
      if (channel) {
        const pool = channel.getTokenPool;
        if (pool) {
          const slot = pool.getSlot ? pool.getSlot() : null;
          if (slot !== null && slot !== undefined) {
            usedSlots.add(slot);
          }
        }
      }
    }

    // Find first unused slot
    let nextSlot = 0;
    while (usedSlots.has(nextSlot)) {
      nextSlot++;
    }

    console.log(`[StateLessChannelsManager] Next available slot: ${nextSlot} (used: ${Array.from(usedSlots).join(', ') || 'none'})`);
    return nextSlot;
  }

  /**
   * @brief Clear all cached token pool seeds
   * Called when identity switches to prevent cross-identity seed leakage
   */
  clearPoolSeeds() {
    console.log('[StateLessChannelsManager] Clearing cached seeds');

    const channelIDs = this.getChannelIDs(eChannelDirection.both);
    for (const channelID of channelIDs) {
      const channel = this.findChannelByID(channelID);
      if (channel) {
        const pool = channel.getTokenPool;
        if (pool) {
          // Clear the master seed hash
          pool.mMasterSeedHash = new ArrayBuffer(0);
          pool.mSlot = undefined;

          // Clear bank seeds
          const banks = pool.getBanks ? pool.getBanks() : [];
          for (const bank of banks) {
            bank.mSeedHash = new ArrayBuffer(0);
          }
        }
      }
    }
  }
}




//Provides Autonomous State-Less Blockchain Channels capabilities.
//The class effectively agregates usage of Multi-Dimensional Token Pools, Transmision Tokens, Token Pool Banks and Transit Pool.
//Requires a Token-Pool to be already registered within the Decentralized VM, one specified by its ID.
//There's a 1-1 logical relationship between a Token-Pool and CStateLessChannel. The class provides additional state-descriptors
export class CStateLessChannel {

  constructor(tokenPoolID = null) {
    this.mTokenPool = null; //web-browser verifies Transmission Tokens on its own to save on full-node utilization.
    //such capability is implemented within CTokenPool itself. Note that first the internal mTokenPool object
    //needs to be synchronized with what is available within the Decentralized State-machine
    //thus, user of this class needs to call  synchronize() after constructing the Object.

    //This class(CStateLessChannel) will then listen for and await a network event containing
    //an asynchronous response with a BER-encoded TokenPool, deserialize it and store it within the internal mTokenPool cache.
    //User should call getIsReadyForVerification() to check if the Token Pool has been already fetched and thus if Transmission Token
    //verification is already possible. This StateChannel automatically synchronizes
    // state of the cached TokenPool with the Network.
    this.mVMContext = gVMContext;
    this.mNetworkRequestIDs = []; //used to associate network-requests with incomming BER-encoded tokenpools for instances
    //similar mechanics is used for UI dApps. still we want State-Channels to be independent from UI applications (and fully autonomous/easy to use)
    //thus we implement these mechanics also here.
    this.mTokenPoolID = tokenPoolID;
    this.mEventListenerInPlace = false;
    this.mRecentToken = null;

    //timestamps - BEGIN
    this.mLastUpdate = 0;
    this.mLastUsed = 0;
    this.mLastCashOutAttempt = 0;
    this.mLastTimeSynced = 0;
    //timestamps - END

    this.mRecipientsToBanks = new Map(); // creates a mapping between recipients and Token Pool's dimensions
    this.mIsOutGress = -1; //unknown by now

    this.loadBankAssignments();
  }

  isBankInUse(bankID) {
    return Array.from(this.mRecipientsToBanks.values()).includes(bankID);
  }

  /*
  Retrieves/decides upon a Bank/Dimension to be used for rewarding of a given user specified by peerID.
  */
  findBankForUser(peerID) {
    if (this.mTokenPool == null)
      return null;

    peerID = gTools.arrayBufferToString(peerID);
    if (peerID.length == 0)
      return null;

    let bankID = this.mRecipientsToBanks.get(peerID);
    if (bankID !== 'undefined') {
      return this.mTokenPool.getBankById(bankID);
    } else return null;
  }

  loadBankAssignments() {
    //[todo]:implement
    //retrieve bank-assignments from local storage

  }

  /*
  Explicitly assigns bank/dimension to a given peerID.
  */
  assignPeerToBank(peerID, bankID) {
    peerID = gTools.arrayBufferToString(peerID);
    if (peerID.length == 0 || bankID < 0)
      return false;

    //todo: update local storage

    this.mRecipientsToBanks.set(peerID, bankID);
    return true;
  }

  /*
  Frees a given bank/dimension i.e. makes it available for other peers.
  */
  freeBank(bankID) {
    for (const [key, value] of this.mRecipientsToBanks.entries()) {
      {
        if (bankID == value) {
          this.mRecipientsToBanks.delete(key);
          return;
        }
      }
    }
  }

  /*
  Returns the last the StateChannel was used.
  */
  getLastTimeUsed() {
    return this.mLastUsed;
  }

  /*
  Returns the last the StateChannel was updates.
  */
  getLastTimeUpdated() {
    return this.mLastUpdate;
  }

  /*
  Returns the last the StateChannel was cashed-out.
  */
  getLastTimeCashedOut() {
    return this.mLastCashOutAttempt;
  }
  /*
  Resets information regarding ownership of the StateChannel.
  i.e. the ingress/outgress relation.
  */
  resetOwner() {
    this.mIsOutGress = -1;
  }

  /*
  Returns information in regards whether the StateChannel is ingress or outgress.
  It is decided on demand based on the logged-in user's ID. The decision is cached once made.
  */
  get getIsOutgress() {

    if (this.mIsOutGress >= 0)
      return this.mIsOutGress;

    if (this.mTokenPool == null)
      return false;

    if (this.mIsOutGress == -1) {
      //we dunno gotta check if we can decide and report
      if (CVMContext.getInstance().getUserID.length > 0) {
        if (gTools.compareByteVectors(this.mTokenPool.getOwnerID, CVMContext.getInstance().getUserID)) {
          this.mIsOutGress = true;
          return this.mIsOutGress;
        }
      }

      if (CVMContext.getInstance().getUserFullID.length > 0) {
        if (gTools.compareByteVectors(this.mTokenPool.getOwnerID, CVMContext.getInstance().getUserFullID)) {
          this.mIsOutGress = true;
          return this.mIsOutGress;
        }
      }
    }

    return this.mIsOutGress;
  }

  /*
  Explicitly sets the StateChannel as either ingress or outgress.
  */
  set setIsOutgress(isIt = true) {
    this.mIsOutGress = isIt;
  }
  /*
    Returns time elapsed (in seconds) since the last cash-out attempt.
  */
  get getTimeSinceLastCashoutAttempt() {
    return gTools.getTime() - this.mLastCashOutAttempt;
  }

  /*
    Returns time elapsed (in seconds) since the recent synchronization of the StateChannel.
  */
  get getTimeSinceLastSync() {
    return gTools.getTime() - this.mLastTimeSynced;
  }
  /*
    Sets the recent Transmission Token.
  */
  set setRecentToken(token) {
    this.mRecentToken = token;
  }
  /*
    Returns the recent Transmission Token.
  */

  get getRecentToken() {
    return this.mRecentToken;
  }

  /*
    Returns the total value accumulated through the recently received Transmission Tokens.
    Would return 0 if no Tokens have been received so far since the local system went online.
  */
  get getAccumulatedValue() {
    if (this.mTokenPool == null || this.mRecentToken == null)
      return 0n; //nothing to validate against

    return this.mTokenPool.validateTT(this.mRecentToken, false); //do NOT update state
  }

  /*
  Sets the StateChannel's Token Pool.
  The token pool is used for all of its internal operations.
  The State Channel's identifier coresponds to the Token Pool's identifier.
  */
  set setTokenPool(pool) {
    this.mLastUpdate = gTools.getTime();
    this.mTokenPool = pool;
  }

  /*
  Updates the time at which the StateChannel was used.
  */
  ping(type = eChannelLocalPingType.lastUpdate) {

    let time = gTools.getTime();
    switch (type) {
      case eChannelLocalPingType.update:
        this.mLastUpdate = time;

        break;

      case eChannelLocalPingType.lastUsed:
        this.mLastUsed = time;

        break;

      case eChannelLocalPingType.lastSync:
        this.mLastTimeSynced = time;

        break;

      case eChannelLocalPingType.lastCashOut:
        this.mLastCashOutAttempt = time;

        break;
      default:
        this.mLastUpdate = time;
    }

  }
  /*
   Returns the most recent timestamp at which the StateChannel was updated.
  */
  get getLastUpdate() {
    return this.mLastUpdate;
  }
  /*
   Returns time (in seconds) since the recent update.
  */
  get getTimeSinceLastUpdate() {
    return gTools.getTime() - this.mLastUpdate;
  }
  /*
  Checks if the StateChannel handles a given Decentralized File System request - one specified by its identifier.
  */
  hasDFSRequestID(reqID) {
    for (var i = 0; i < this.mNetworkRequestIDs.length; i++) {
      if (this.mNetworkRequestIDs[i] == reqID)
        return true;
    }
    return false;
  }

  /*
  Returns the amount of assets, within the StateChannel, available to be spent, as seen by the local Peer.
  The value returned would be equal to 0 if either no resourced are left OR if the state-channel is an ingress one i.e. outside control of the local peer.
  */
  get getLocallySpendableAssets() {
    if (this.mTokenPool == null)
      return 0n;
    return this.mTokenPool.getLocallySpendableAssets();
  }

  //Returns random immutable identifier of this StateLessChannel
  get getID() {
    return this.mTokenPoolID; //the same as TP's ID indeed, one-to-one relationship anyway
  }

  //Returns a StateChannel's friendly identifier.
  get getFriendlyID() {
    if (this.mTokenPool != null) {
      return this.mTokenPool.getFriendlyID;
    } else return '';

  }

  /*
  Returns True if the StateChannel is ready to verify incomming Transmission Tokens; returns False otherwise.
  */
  get getIsReadyForVerification() {
    return this.mTokenPool != null;
  }

  //Returns the StateChannel's underlying TokenPool object.
  get getTokenPool() {
    return this.mTokenPool;
  }

  /*Updates the state of the internal token-pool, taking into account its updated version,- one delivered from the Network.
  By default updates only dimensions whose utilization state is lower than the best known one (as indicated by the locally cached dimensons/banks).
  Returns True if update succeedes, False otherwise.
  */
  updateTokenPool(pool, forceUpdate = false) {

    let updated = false;

    if (pool == null)
      return false;

    if (this.mTokenPool == null) {
      this.mTokenPool = pool;
      return true;
    }

    if (!gTools.compareByteVectors(pool.getID(), this.mTokenPool.getID()))
      return false;

    if (forceUpdate) {
      this.setTokenPool(pool); //replace the object entirely
    } else {

      //proceed with updating only increased usage of select dimensions

      let updatedPool = this.mTokenPool;

      if (pool.getDimensionsCount() != updatedPool.getDimensionsCount())
        return; //should not happen

      let dimensionsCount = pool.getDimensionsCount();

      //Now, we do update the token Pool ONLY if:
      //1) The newly spotted token pool is more depleted than the currently known.
      //that is because, states among peers are updated through Transmission Tokens not Token Pools.
      //Thus, IF such a Token Pools is spotted it means that it was delivered from a full-node and in such a case
      //we want to protect the user against possible double-spends.
      //Note that authentication took place earlier.
      for (let a = 0; a < dimensionsCount; a++) {
        if (pool.getBankById(a).getCurrentDepth() > updatedPool.getBankById(a).getCurrentDepth()) {
          updatedPool.updateBank(pool.getBankById(a));
          updated = true;
        }
      }

      return updated;
    }
  }

  //Attempts to generate a Transmission Token with sufficient resources (value) for a given Peer, one provided by peerID.
  //The function attempts to facilitate autonomous Token Pool's dimensions' management and data-type conversions (peer's ID).
  //If reassignDimensionsIfNeeded is set, it will attempt to reasign dimensions and find a more suitable one, if needed.
  //The function returns null on failure (insufficient resources, all dimensions occupied etc.)
  getTTForPeer(value, peerID = '', markUsed = true, reassignDimensionsIfNeeded = true) {
    //attempt type conversion
    peerID = gTools.arrayBufferToString(peerID);

    if (this.mTokenPool == null)
      return null;

    if (peerID.length == 0) //peer ID not provided
    { //we shall attempt to use the first active dimension, one with sufficient resources, if available
      this.ping(eChannelLocalPingType.lastUsed);
      return this.getTTWorthValue(value, -1, markUsed);
    } else { //peer ID provided

      let bank = this.findBankForUser(peerID);

      if (bank == null) {
        //no bank assigned to Peer, let us find a suitable one
        for (let i = 0; i < this.mTokenPool.mBanks.length; i++) {
          //iterate over banks in an attempt to find a suitable one
          if (this.mTokenPool.getBankStatus(i) == eTokenPoolBankStatus.active) { //omit depleted banks. This allows
            //for assignment of a new bank to a peer which owned and used up a previous one, right away.
            if (this.isBankInUse(i))
              continue; //omit if used by another peer

            if (this.mTokenPool.getValueLeftInBank(i) >= value) {
              this.assignPeerToBank(peerID, i); //looks Good =>assign dimension to peer
              this.ping(eChannelLocalPingType.lastUsed);
              return this.mTokenPool.getTTWorthValue(i, value, markUsed);
            }
          }
        }
      } else { //a Token Pool's dimension is already assigned to the particular Peer, let us try to use it shall we
        if (bank.getStatus() != eTokenPoolBankStatus.active) {
          if (reassignDimensionsIfNeeded)
            this.freeBank(bank.getID);
          else return null;
        }
        if (this.mTokenPool.getValueLeftInBank(bank.getID) >= value) {
          //attempt to get a TT
          this.ping(eChannelLocalPingType.lastUsed);
          return this.mTokenPool.getTTWorthValue(bank.getID, value, markUsed);
        } else {
          if (!reassignDimensionsIfNeeded) //are we allowed to re-assign dimensions? and find a more suitable one?
            return null; //if not => Abort.
          //no sufficient assets within the dimension. free Bank and attempt to find and assign a new one
          for (let i = 0; i < this.mTokenPool.mBanks.length; i++) {
            if (this.mTokenPool.getBankStatus(i) == eTokenPoolBankStatus.active) { //omit depleted banks. This allows
              //to assign new bank to a peer which owned and used up a previous one right away
              if (this.isBankInUse(i))
                continue; //omit if assigned to another peer
              if (this.mTokenPool.getValueLeftInBank(i) >= value) {
                this.assignPeerToBank(peerID, i); //assign dimension to peer
                this.ping(eChannelLocalPingType.lastUsed);
                return this.mTokenPool.getTTWorthValue(i, value, markUsed);
              }
            }
          }
        }
      }
    }
    return null;
  }

  /*
  Attempts to generate a Transmission Token of a given value.
  If recipient's ID is known, use getTTForPeer() for autonomous recipients <-> dimensions management.
  Particular bank/dimension can be specified through the bankID parameter.
  If no bank chosen a priori, it will choose the first one available with sufficient resources.
  */
  getTTWorthValue(value, bankID = -1, markUsed = true) {
    if (this.mTokenPool == null)
      return null;

    if (bankID == -1) { //user did not specify a particular dimension

      for (let i = 0; i < this.mTokenPool.mBanks.length; i++) {

        if (this.isBankInUse(i))
          continue; //some peer is being rewarded from it

        if (this.mTokenPool.getBankStatus(i) == eTokenPoolBankStatus.active) {
          // BUG FIX: Was calling .getValueLeftInBank() on status enum instead of pool
          if (this.mTokenPool.getValueLeftInBank(i) >= value) {
            this.ping(eChannelLocalPingType.lastUsed);
            // BUG FIX: Arguments were swapped - bankID should come first
            return this.mTokenPool.getTTWorthValue(i, value, markUsed);
          }
        }
      }
    } else
      return this.mTokenPool.getTTWorthValue(bankID, value, markUsed);

    return null;
  }

  //Queries full-node for the most recent version of a TokenPool.
  //Do NOT call this function too often otherwise the Full-Node (in order not to suffocate) would throttle down on processing and eventually block
  //queries from the local IP address.
  //The TokenPool is asynchronously delivered through a VMMetaData event.
  //The event will be fired even if the token pool does not exist, since even THEN
  //the full node delivers (empty) file-entry meta-data.
  //These can be then associated through the regular requestID-responseID mechanics
  //The response is processed and possibly propagated by the StateLessChannelsManager instead.
  synchronize() {
    if (this.mTokenPoolID == null)
      return false;


    //Operational Logic - Begin
    let cmd = "getPool " + gTools.encodeBase58Check(this.mTokenPoolID); //a #GridScript command
    let metaGen = new CVMMetaGenerator();
    metaGen.reset(); //todo: consider using a generator of its own
    metaGen.addRAWGridScriptCmd(cmd, eVMMetaCodeExecutionMode.GUI, this.mID);

    if (CVMContext.getInstance().processVMMetaDataKF(metaGen)) {
      this.ping(eChannelLocalPingType.lastSync); //ping only if we managed to push the request through.
    }

    return true;
    //Operational Logic - END
  }

  //Validates a Transmission Token.
  //Returns token's value; 0 if invalid. -1 if not ready.
  validateTT(token, updateState = true, isBankUpdate = false) {
    if (this.mTokenPool == null || token == null)
      return -1;

    return this.mTokenPool.validateTT(token, updateState, isBankUpdate);

  }

  //Adds a Cashing-out Consensus task into the processing qeueu. Task instructs cashing-out of the accumulated rewards, on-the-chain.
  //Note that the latest Transmission Token can be considered as a Transit-Pool.
  //Thus, the last token MIGHT and most often will describe a bigger part of a hash-chain.
  //Orders full-node to issue reward to a specified awardee based on the provided data. The task will be visible within the UI
  //through the 'Magic Button'
  //Important: note that only Authenticated Transmission Tokens (the ones containing a signature) are immune against malicious full-nodes.
  cashOut(token, awardeeID, autoComit = false) {

    if (this.mTokenPoolID == null || awardeeID == null || awardeeID.byteLength == 0)
      return false;

    this.ping(eChannelLocalPingType.lastCashOut);
    //this.mLastCashOutAttempt = gTools.getTime();

    //Operational Logic - Begin
    let cmd = "xTT " + gTools.encodeBase58Check(token.getPackedData()); //a #GridScript command delivering a BER-encoded Transmission Token
    //comprising the de'facto Transit-Pool

    let metaGen = new CVMMetaGenerator();

    metaGen.addRAWGridScriptCmd(cmd, eVMMetaCodeExecutionMode.GUI, this.mID);

    if (!CVMContext.getInstance().processVMMetaDataKF(metaGen))
      return false;

    let task = new CConsensusTask('Token Cash-Out (' + value + 'GBUs to ' + awardeeID + ')');
    this.mVMContext.addConsensusTask(task);

    if (autoComit)
      this.mVMContext.comit();
    return true;
    //Operational Logic - END

  }
}

//Represent a single dimension of a multi-dimensional Token Pool.
//Note that only a single recipient can be rewarded from a single Bank at once (i.e. untill reward from a single bank is cashed-out on the chain by recipient).
export class CTokenPoolBank {
  constructor(bankID = 0, pool = null, finalHash = new ArrayBuffer(), currentDepth = 0, seedHash = new ArrayBuffer()) {
    this.initFields();
    this.mID = bankID;
    this.mPool = pool;
    this.mFinalHash = finalHash;
    this.mCurrentDepth = currentDepth;
    this.mSeedHash = seedHash;
    this.mCurrentFinalHash = new ArrayBuffer();
    this.mPreCachedSeedHash = new ArrayBuffer();
  }

  get getID() {
    return this.mID;
  }
  initFields() {
    this.mCurrentDepth = 0n;
    this.mPreCachedSeedHashDepth = 0n;
    this.mCurrentIndex = 0n;
    this.mStatus = eTokenPoolBankStatus.active;
  }

  setPreCachedSeedHash(seed, depth) {
    if (seed.byteLength != 32)
      return false;
    this.mPreCachedSeedHash = seed;
    this.mPreCachedSeedHashDepth = depth;
    return true;
  }

  get getPreCachedSeedHashDepth() {
    return this.mPreCachedSeedHashDepth;
  }

  //Returns the amount of locally spendable assets.
  //That means the amount which can be released from a State-Channel without requesting an update from a Security Token (over a QR-Intent).
  //The value is based on the depth of a pre-cached-seed hash, previously made available by a call to setPreCachedSeedHash().
  //The returned value is in GBUs.
  getLocallySpendableAssets() {
    if (this.mPreCachedSeedHashDepth <= this.mCurrentDepth)
      return 0n;

    if (this.mSeedHash.byteLength == 32) {
      //master seed-hash available
      return (BigInt(this.mPool.getDimensionDepth()) - BigInt(this.mCurrentDepth)) * BigInt(this.mPool.getSingleTokenValue());
    } else //base our reponse on the mPreCachedSeedHash instead
      return (BigInt(this.mPreCachedSeedHashDepth) - BigInt(this.mCurrentDepth)) * BigInt(this.mPool.getSingleTokenValue());
  }

  setPool(pool) {
    this.mPool = pool;
  }

  /**
   * Sets the seed hash for this bank (used during seed regeneration from keychain)
   * @param {ArrayBuffer} seedHash - 32-byte seed hash
   * @returns {boolean} true on success
   */
  setSeedHash(seedHash) {
    if (!seedHash || seedHash.byteLength !== 32) {
      console.error('[CTokenPoolBank] Invalid seed hash - must be 32 bytes');
      return false;
    }
    this.mSeedHash = seedHash;
    return true;
  }

  /**
   * Gets the seed hash for this bank
   * @returns {ArrayBuffer} The 32-byte seed hash
   */
  getSeedHash() {
    return this.mSeedHash;
  }

  setStatus(status) {
    let statusTxt;

    switch (status) {
      case eTokenPoolBankStatus.active:
        statusTxt = 'active';
        break;
      case eTokenPoolBankStatus.depleted:
        statusTxt = 'depleted';
        break;
      default:

    }
    console.log('Marking dimension ' + this.mID + ' as ' + statusTxt);
    this.mStatus = status;
    //notify the Token-Pool itself about status change
    //so that it can update the status of itself if needed
    let pool;
    if (pool = this.mPool) {
      pool.notifyBankStatusChanged(status);
    }
  }
  getStatus() {
    return this.mStatus;
  }
  getNextHash(markUsed = true) {
    this.getHashAtDepth(this.mCurrentDepth + 1n, markUsed);
  }
  genTokenWorthValue(value, markUsed = true) {
    let hash = new ArrayBuffer();
    let hashesConsumedCount = 0n;
    if (value == 0 || value > this.getValueLeft())
      return null;

    if (this.mPool != null) {
      let tokenValue = this.mPool.getSingleTokenValue();
      if (tokenValue == 0)
        return toRet;
      //token-value in each bank is the same
      hashesConsumedCount = BigInt(1n + ((BigInt(value) - 1n) / BigInt(tokenValue))); // if value != 0 ; gets ceiling of division

      //the following would return an empty vector if value exceeds Pool-size
      hash = this.getHashAtDepth(this.mCurrentDepth + hashesConsumedCount, markUsed);
    } else return null;
    if (hash == null)
      return null;
    return {
      hash,
      hashesConsumedCount
    };
  }

  genTTWorthValue(value, markUsed) {
    let hashesToBeConsumedCount = 0n;

    let rawToken = this.genTokenWorthValue(value, markUsed);
    if (rawToken == null)
      return null;

    let expectedDepth = this.mCurrentDepth + rawToken.hashesConsumedCount;
    if (rawToken.hash.byteLength != 32)
      return null;

    if (this.mPool != null) {
      //  constructor( bankID=0,  value=0,  revealedHash = new ArrayBuffer(),  revealedHashesCount=0, bankUsageDepth=0,  dataHash = new ArrayBuffer(),  tokenPoolID
      //   = new ArrayBuffer(), transmissionTokenID = new ArrayBuffer(), sig = new ArrayBuffer())
      let tt = new CTransmissionToken(this.mID, value, rawToken.hash, rawToken.hashesConsumedCount, expectedDepth, null, this.mPool.getID());
      return tt;
    }

    return null;
  }


  getFinalHash(currentOne = true) {
    if (!currentOne || this.mCurrentFinalHash.byteLength != 32)
      return this.mFinalHash;
    else return this.mCurrentFinalHash;

  }

  getHashAtDepth(depth, updateState = true) {
    //Local variables - BEGIN
    let currentHash;
    let currentDepth = this.mCurrentDepth;
    let currentIndex = 0n;
    //Local variables - END

    if (this.mPool != null) {
      currentIndex = this.mPool.getDimensionDepth() - currentDepth;

      if (depth == 0)
        return this.mFinalHash;

      if (depth > this.mPool.getDimensionDepth()) //request is 'out of scope'
        return null;

      //first, check if the requested sub-chain has been revealed already
      if (depth <= currentDepth && this.mCurrentFinalHash.byteLength == 32) { //if so, no need for secrets
        currentHash = this.mCurrentFinalHash;
        currentDepth = this.mCurrentDepth;
      } else //then try to use a pre-cached seed-hash if available for the requested depth
        //we try this first as it's an optimization (by limiting the search-space)
        if (depth <= this.mPreCachedSeedHashDepth && this.mPreCachedSeedHash.byteLength == 32) //we're not in posession of the required secret
      { //the pre-cached secret might have been provided by the mobile app
        //the mobile app shall provide just enough for the services to remain of enough quality (vidoes playing etc)
        //while not risking too much of the user's assets
        currentHash = this.mPreCachedSeedHash;
        currentDepth = this.mPreCachedSeedHashDepth;
      } else if (this.mSeedHash.byteLength == 32) { //finally fallback to seed-hash for this dimension, if available
        currentHash = this.mSeedHash;
        currentDepth = this.mPool.getDimensionDepth();
        currentIndex = 0n;
      } else return null; //we cannot deliver

      //let us retrieve the hash-value based on known data
      for (let i = currentDepth; i > depth; i--) {
        currentHash = gCrypto.getSHA2_256Vec(currentHash);
      }

      //update bank's utilization - the token-range is assumed as used-up
      if (updateState && depth > this.mCurrentDepth) {
        this.mCurrentDepth = depth;
        this.mCurrentFinalHash = currentHash;

        if (this.getValueLeft() == 0)
          this.setStatus(eTokenPoolBankStatus.depleted);
      }

    }
    return currentHash;

  }
  setCurrentHash(currentHash) {
    this.mCurrentFinalHash = currentHash;
  }
  getCurrentHash() {
    return this.mCurrentFinalHash;
  }
  getCurrentDepth() {
    return this.mCurrentDepth;
  }
  setCurrentDepth(index) {
    this.mCurrentDepth = index;
  }
  getValueLeft() {
    let dimensionDepth = 0n;
    if (this.mStatus == eTokenPoolBankStatus.depleted)
      return 0n;

    if (this.mPool != null) {
      dimensionDepth = this.mPool.getDimensionDepth();

      if (dimensionDepth == 0 || this.mCurrentDepth == dimensionDepth)
        return 0n;

      return (BigInt(dimensionDepth) - BigInt(this.mCurrentDepth)) * BigInt(this.mPool.getSingleTokenValue());
    }

    return 0n;
  }
  getTokensLeft() {
    let dimensionDepth = 0n;
    if (this.mStatus == eTokenPoolBankStatus.depleted)
      return 0n;

    if (this.mPool != null) {
      dimensionDepth = this.mPool.getDimensionDepth();

      if (dimensionDepth == 0 || this.mCurrentDepth == dimensionDepth)
        return 0n;

      return (dimensionDepth - this.mCurrentDepth);
    }

    return 0n;
  }

  getTotalValue() {
    if (this.mPool != null) {
      return BigInt(this.mPool.getDimensionDepth()) * BigInt(this.mPool.getSingleTokenValue());
    }
    return 0n;
  }

}

//Represents a Multi-Dimensional Token-Pool (with mutiple banks)
export class CTokenPool {
  constructor(cf = null, dimensionsCount = 1, ownerID = new ArrayBuffer(), receiptID = new ArrayBuffer(),
    valuePerToken = 1, totalValue = 0, currentIndex = 0, friendlyID = "",
    seedHash = new ArrayBuffer(), finalHash = new ArrayBuffer(), currentHash = new ArrayBuffer()) {
    this.initFields();
    this.mCryptoFactory = cf;
    this.mReceiptID = receiptID;
    this.mTotalValue = totalValue;
    this.mDimensionsCount = dimensionsCount;
    let totalTokensCount = BigInt(totalValue) / BigInt(valuePerToken);
    //we'll distribute tokens among the available dimensions (banks)

    //the number of dimensions should be set accordinly with the expected numerosity of pending, parallel
    //State-Less Blockchain Channel transactions
    this.mDimensionDepth = BigInt(totalTokensCount) / BigInt(dimensionsCount);
    this.mFriendlyID = friendlyID;
    this.mMasterSeedHash = seedHash;
    this.mPubKey = new ArrayBuffer();
    this.mOwnerID = ownerID;
  }
  static getNew(cf = null, dimensionsCount = 0, ownerID = new ArrayBuffer(), receiptID = new ArrayBuffer(), valuePerToken = 1, totalValue = 0, currentIndex = 0,
    friendlyID = '', seedHash = new ArrayBuffer(), finalHash = new ArrayBuffer(), currentHash = new ArrayBuffer()) { //this way there's always at least one shared_ptr to (this)
    return new CTokenPool(cf, dimensionsCount, ownerID, receiptID, valuePerToken, totalValue, currentIndex, friendlyID, seedHash, finalHash, currentHash);
  }

  notifyBankStatusChanged(status) {

    let allBanksUsedUp = true;

    switch (status) {
      case eTokenPoolBankStatus.active:
        break;
      case eTokenPoolBankStatus.depleted:
        for (let i = 0; i < this.mBanks.length; i++) {
          if (this.mBanks[i].getStatus() != eTokenPoolBankStatus.depleted)
            return;
        }
        //all banks depleted, update the status of my own
        this.setStatus(eTokenPoolStatus.depleted);
        break;
      default:
        break;
    }
  }

  getValueLeftInBank(bankID) {
    //we do not want to return inner bank-objects to avoid state-synchronization between the two
    let bank = this.getBankById(bankID);
    if (bank != null) {
      return bank.getValueLeft();
    }
    return 0n;
  }

  getBankById(id) {
    for (let i = 0; i < this.mBanks.length; i++)
      if (i == id)
        return this.mBanks[i];
    return null;
  }
  getBankStatus(bankID) {
    //we do not want to return inner bank-objects to avoid state-synchronization between the two
    let bank = this.getBankById(bankID);
    if (bank != null) {
      return bank.getStatus();
    }
    return eTokenPoolBankStatus.depleted;
  }

  getDimensionDepth() {
    return this.mDimensionDepth;
  }
  getID() {
    return this.mTokenPoolID;
  }

  initFields() {
    this.mCryptoFactory = null;
    this.mDimensionsCount = 0;
    this.mDimensionDepth = 0n;
    this.mTotalValue = 0n;
    this.mVersion = 2;
    this.mBanks = [];
    this.mMasterSeedHash = new ArrayBuffer();
    this.mOwnerID = new ArrayBuffer();
    this.mPubKey = new ArrayBuffer();
    this.mTokenPoolID = gTools.getRandomVector(32);
    this.mStatus = eTokenPoolStatus.active;
  }

  updateBank(bank) {
    if (bank == null)
      return false;
    if (this.mBanks.length == 0)
      return false;

    if (bank.getID() > this.mBanks.length - 1)
      return false;

    this.mBanks[bank.getID()] = bank;
    return true;
  }

  /// <summary>
  /// Retrieves seeding hash for a given dimension (if master-seed-hash available)
  /// </summary>
  /// <param name="dimensionID"></param>
  /// <returns></returns>
  getSeedingHashForDimension(dimensionID) {
    //local variables - BEGIN
    let toRet = new ArrayBuffer();
    let bankSeed = new ArrayBuffer();
    let masterNr = 0;
    let temp = new ArrayBuffer(64);
    //local variables - END

    if (this.mMasterSeedHash.byteLength != 32)
      return toRet;

    masterNr = gTools.arrayBufferToBigInt(this.mMasterSeedHash, false); //intepret hash as a number

    for (let a = 0; a < this.mDimensionsCount; a++) {
      //Dimension Switch - BEGIN
      //Note: the dimension's secret is never revealed.
      let t = new Uint8Array(masterNr); //optimization to avoid mem-copies
      temp.set(t, 0);
      temp.set(t, 32);
      masterNr = gCrypto.getSHA2_256Vec(temp.buffer); //switch to a new dimension
      bankSeed = gCrypto.getSHA2_256Vec(masterNr);
      //Dimension Switch - END

      if (a == dimensionID)
        return bankSeed;
    }
    return toRet;
  }
  /// <summary>
  /// Generates dimensions / independant banks.
  /// </summary>
  /// <param name="reportStatus">Whether to report status during generation</param>
  /// <param name="verify">Whether to verify hash chain integrity</param>
  /// <param name="externalSeed">Optional: 32-byte ArrayBuffer seed derived from keychain (deterministic).
  ///                           If not provided, a random seed is generated (legacy behavior).</param>
  /// <returns>true on success, false on failure</returns>
  generateDimensions(reportStatus = true, verify = true, externalSeed = null) {

    //local variables - BEGIN
    let bankSeed;
    let bank;
    let currentHash;
    let masterNr = new ArrayBuffer();
    //local variables - END

    if (this.mMasterSeedHash.byteLength != 0 || this.mBanks.length != 0)
      return false;

    // Generate or use provided master seed hash
    if (externalSeed && externalSeed.byteLength === 32) {
      // Use deterministic seed derived from keychain
      currentHash = externalSeed;
      console.log('[CTokenPool] Using deterministic seed from keychain');
    } else {
      // Legacy behavior: generate random seed (WARNING: seed will be lost after on-chain registration)
      currentHash = gTools.getRandomVector(32);
      console.warn('[CTokenPool] Using random seed - seed will be LOST after on-chain registration!');
    }
    this.mMasterSeedHash = currentHash;
    masterNr = this.mMasterSeedHash;
    this.mBanks = [];

    let temp = new Uint8Array(64);
    for (let a = 0; a < this.mDimensionsCount; a++) {

      //Dimension Switch - BEGIN
      //Note: the dimension's secret is never revealed.
      let t = new Uint8Array(masterNr); //optimization to avoid mem-copies
      temp.set(t, 0);
      temp.set(t, 32);
      masterNr = gCrypto.getSHA2_256Vec(temp.buffer); //switch to a new dimension
      bankSeed = gCrypto.getSHA2_256Vec(masterNr);
      //Dimension Switch - END

      //  masterNr++; //generate new seed-hash for each bank based of the master-seed-hash
      //bankSeed = masterNr;//gTools.bigIntToArrayBuffer(masterNr);
      currentHash = bankSeed;

      //let us proceed with token-generation
      for (let i = 0; i < this.mDimensionDepth; i++) {
        currentHash = gCrypto.getSHA2_256Vec(currentHash);
      }
      let ending = currentHash;
      //verify - BEGN
      currentHash = bankSeed;
      if (verify) {
        for (let i = this.mDimensionDepth; i > 0; i--) // !tools->compareByteVectors(currentHash, ending) && iterations < (mDimensionDepth + 10))
        {
          currentHash = gCrypto.getSHA2_256Vec(currentHash);
        }
        if (!gTools.compareByteVectors(currentHash, ending))
          return false;
      }
      //verify - END

      //CTokenPoolBank(a,shared_from_this(), currentHash, 0, Botan::secure_vector<uint8_t>(bankSeed.begin(), bankSeed.end()));
      bank = new CTokenPoolBank(a, this, ending, 0, bankSeed);
      this.mBanks.push(bank);
    }

    return true;
  }

  getDimensionsCount() {
    return this.mDimensionsCount;
  }
  setDimensionsCount(count) {

    this.mDimensionsCount = count;
  }
  getIsMasterSeedHashAvailable() {

    return (this.mMasterSeedHash.byteLength == 32) ? true : false;
  }

  /**
   * Gets the token pool slot index (used for deterministic seed derivation)
   * @returns {number|null} The slot index or null if not set
   */
  getSlot() {
    return this.mSlot !== undefined ? this.mSlot : null;
  }

  /**
   * Sets the token pool slot index
   * @param {number} slot - The slot index for this pool
   */
  setSlot(slot) {
    this.mSlot = slot;
  }

  /**
   * Sets the master seed hash directly
   * @param {ArrayBuffer} seedHash - The 32-byte seed hash
   */
  setMasterSeedHash(seedHash) {
    if (seedHash && seedHash.byteLength === 32) {
      this.mMasterSeedHash = seedHash;
    }
  }

  /**
   * Regenerate all bank seeds from the master seed hash.
   * This must be called after setMasterSeedHash to enable spending.
   */
  regenerateBankSeeds() {
    if (!this.mMasterSeedHash || this.mMasterSeedHash.byteLength !== 32) {
      console.error('[CTokenPool] Cannot regenerate bank seeds - no master seed');
      return false;
    }

    try {
      // Use gCrypto or global crypto factory
      const crypto = this.mCryptoFactory || (typeof gCrypto !== 'undefined' ? gCrypto : null);
      if (!crypto || typeof crypto.getSHA2_256Vec !== 'function') {
        console.error('[CTokenPool] No crypto factory available for seed regeneration');
        return false;
      }

      // Regenerate bank seeds using the same algorithm as generateDimensions
      let masterNr = this.mMasterSeedHash;
      let temp = new Uint8Array(64);

      for (let a = 0; a < this.mBanks.length; a++) {
        // Dimension Switch - same algorithm as generateDimensions
        let t = new Uint8Array(masterNr);
        temp.set(t, 0);
        temp.set(t, 32);
        masterNr = crypto.getSHA2_256Vec(temp.buffer);
        let bankSeed = crypto.getSHA2_256Vec(masterNr);

        // Store seed in bank
        this.mBanks[a].mSeedHash = bankSeed;
      }

      console.log(`[CTokenPool] Regenerated ${this.mBanks.length} bank seeds from master seed`);
      return true;
    } catch (error) {
      console.error('[CTokenPool] Error regenerating bank seeds:', error);
      return false;
    }
  }

  /**
   * Regenerates the master seed and all bank seeds from a keychain.
   * This is used for pools loaded from blockchain that don't have the seed stored.
   *
   * @param {CKeyChain} keyChain - The unlocked keychain to derive seed from
   * @param {number} slot - The slot index for this pool (0, 1, 2, ...)
   * @returns {boolean} true on success, false on failure
   */
  regenerateSeedFromKeyChain(keyChain, slot) {
    if (!keyChain || typeof keyChain.deriveTokenPoolSeed !== 'function') {
      console.error('[CTokenPool] Invalid keychain provided for seed regeneration');
      return false;
    }

    if (typeof slot !== 'number' || slot < 0) {
      console.error('[CTokenPool] Invalid slot number for seed regeneration');
      return false;
    }

    try {
      // Derive deterministic seed from keychain
      const seedBuffer = keyChain.deriveTokenPoolSeed(slot);
      if (!seedBuffer || seedBuffer.byteLength !== 32) {
        console.error('[CTokenPool] Failed to derive seed from keychain');
        return false;
      }

      // Store the slot for future reference
      this.mSlot = slot;

      // Set the master seed hash
      this.mMasterSeedHash = seedBuffer;
      console.log(`[CTokenPool] Regenerated master seed for pool "${this.mFriendlyID}" at slot ${slot}`);

      // Regenerate bank seeds using the same algorithm as generateDimensions
      let masterNr = this.mMasterSeedHash;
      let temp = new Uint8Array(64);

      for (let a = 0; a < this.mBanks.length; a++) {
        // Dimension Switch - same algorithm as generateDimensions
        let t = new Uint8Array(masterNr);
        temp.set(t, 0);
        temp.set(t, 32);
        masterNr = gCrypto.getSHA2_256Vec(temp.buffer);
        let bankSeed = gCrypto.getSHA2_256Vec(masterNr);

        // Update the bank's seed hash
        if (this.mBanks[a]) {
          this.mBanks[a].setSeedHash(bankSeed);
        }
      }

      console.log(`[CTokenPool] Regenerated seeds for ${this.mBanks.length} banks`);

      // Validate regenerated seed against stored final hashes
      const validation = this.validateSeedAgainstBanks(true);
      if (!validation.valid) {
        console.error(`[CTokenPool] SEED VALIDATION FAILED for pool "${this.mFriendlyID}" at slot ${slot}`);
        console.error(`[CTokenPool] Valid banks: ${validation.validBanks}/${validation.totalBanks}`);
        console.error(`[CTokenPool] Errors:`, validation.errors);
        // Clear the invalid seed
        this.mMasterSeedHash = new ArrayBuffer();
        this.mSlot = undefined;
        // Clear bank seeds
        for (let a = 0; a < this.mBanks.length; a++) {
          if (this.mBanks[a]) {
            this.mBanks[a].mSeedHash = new ArrayBuffer();
          }
        }
        return false;
      }

      console.log(`[CTokenPool] Seed validation PASSED for pool "${this.mFriendlyID}" (${validation.validBanks}/${validation.totalBanks} banks)`);
      return true;

    } catch (error) {
      console.error('[CTokenPool] Error regenerating seed from keychain:', error);
      return false;
    }
  }

  /**
   * Checks if this pool can spend tokens (has valid seed)
   * @returns {boolean} true if pool has seed and can generate tokens
   */
  canSpendTokens() {
    return this.mMasterSeedHash.byteLength === 32 && this.mBanks.length > 0;
  }

  /**
   * Validates the current master seed against all bank final hashes.
   * This verifies that the regenerated/provided seed is correct by computing
   * the hash chain for each bank and comparing the final hash with the stored one.
   *
   * @param {boolean} verbose - If true, logs detailed validation info
   * @returns {Object} { valid: boolean, validBanks: number, totalBanks: number, errors: string[] }
   */
  validateSeedAgainstBanks(verbose = false) {
    const result = {
      valid: false,
      validBanks: 0,
      totalBanks: this.mBanks.length,
      errors: []
    };

    // Check preconditions
    if (this.mMasterSeedHash.byteLength !== 32) {
      result.errors.push('No master seed hash available (must be 32 bytes)');
      return result;
    }

    if (this.mBanks.length === 0) {
      result.errors.push('No banks present in token pool');
      return result;
    }

    if (this.mDimensionDepth <= 0n) {
      result.errors.push('Invalid dimension depth');
      return result;
    }

    try {
      // Regenerate bank seeds using the same algorithm as generateDimensions
      let masterNr = this.mMasterSeedHash;
      let temp = new Uint8Array(64);

      for (let bankIndex = 0; bankIndex < this.mBanks.length; bankIndex++) {
        const bank = this.mBanks[bankIndex];

        // Dimension Switch - same algorithm as generateDimensions
        let t = new Uint8Array(masterNr);
        temp.set(t, 0);
        temp.set(t, 32);
        masterNr = gCrypto.getSHA2_256Vec(temp.buffer);
        let bankSeed = gCrypto.getSHA2_256Vec(masterNr);

        // Generate the final hash by computing the full hash chain
        let computedFinalHash = bankSeed;
        for (let i = 0n; i < this.mDimensionDepth; i++) {
          computedFinalHash = gCrypto.getSHA2_256Vec(computedFinalHash);
        }

        // Get stored final hash from bank
        const storedFinalHash = bank.getFinalHash(false); // false = get original final hash, not current

        if (!storedFinalHash || storedFinalHash.byteLength !== 32) {
          result.errors.push(`Bank ${bankIndex}: No valid stored final hash`);
          continue;
        }

        // Compare computed vs stored
        if (gTools.compareByteVectors(computedFinalHash, storedFinalHash)) {
          result.validBanks++;
          if (verbose) {
            console.log(`[CTokenPool] Bank ${bankIndex}: Final hash MATCHES ✓`);
          }
        } else {
          result.errors.push(`Bank ${bankIndex}: Final hash MISMATCH - seed may be incorrect`);
          if (verbose) {
            console.log(`[CTokenPool] Bank ${bankIndex}: Final hash MISMATCH ✗`);
            console.log(`  Computed: ${gTools.arrayBufferToHex(computedFinalHash)}`);
            console.log(`  Stored:   ${gTools.arrayBufferToHex(storedFinalHash)}`);
          }
        }
      }

      // Pool is valid only if ALL banks match
      result.valid = (result.validBanks === result.totalBanks);

      if (verbose) {
        console.log(`[CTokenPool] Validation result: ${result.validBanks}/${result.totalBanks} banks valid`);
      }

    } catch (error) {
      result.errors.push(`Validation error: ${error.message}`);
      console.error('[CTokenPool] Error during seed validation:', error);
    }

    return result;
  }

  /**
   * Validates the entire token pool structure and optionally validates seed against banks.
   * This is a comprehensive validation that checks all pool properties.
   *
   * @param {boolean} validateSeed - If true and seed is present, validates seed against bank final hashes
   * @returns {boolean} true if pool is structurally valid (and seed matches if validateSeed=true)
   */
  validate(validateSeed = false) {
    // Basic structural validation
    if (this.mVersion < 1) return false;
    if (this.mDimensionsCount <= 0) return false;
    if (this.mDimensionDepth <= 0n) return false;
    if (this.mBanks.length !== this.mDimensionsCount) return false;
    if (this.mTokenPoolID.byteLength !== 32) return false;
    if (this.mOwnerID.byteLength === 0) return false;

    // Validate each bank has a final hash
    for (let i = 0; i < this.mBanks.length; i++) {
      const bank = this.mBanks[i];
      if (!bank) return false;
      const finalHash = bank.getFinalHash(false);
      if (!finalHash || finalHash.byteLength !== 32) return false;
    }

    // Optionally validate seed against banks
    if (validateSeed && this.mMasterSeedHash.byteLength === 32) {
      const seedValidation = this.validateSeedAgainstBanks(false);
      if (!seedValidation.valid) {
        console.warn('[CTokenPool] Seed validation failed:', seedValidation.errors);
        return false;
      }
    }

    return true;
  }

  /**
   * Gets all banks (dimensions) of this token pool
   * @returns {Array<CTokenPoolBank>} Array of bank objects
   */
  getBanks() {
    return this.mBanks;
  }

  getStatus() {
    return this.mStatus;
  }
  setStatus(status) {

    let statusTxt;

    switch (status) {
      case eTokenPoolStatus.active:
        statusTxt = 'active';
        break;
      case eTokenPoolStatus.depleted:
        statusTxt = 'depleted';
        break;
      case eTokenPoolStatus.banned:
        statusTxt = 'banned';
        break;
      default:

    }
    console.log('Marking pool as ' + statusTxt);

    this.mStatus = status;
  }


  getSeedHash() {
    return this.mMasterSeedHash;
  }
  getInfo() {
    return ''; //todo
  }

  //Validates Transmission Token against the locally available world-view.
  //The isBankUpdate parameter is set when the Token has been released by owner to web-ui to to make additional assets available from the Web-UI.
  //In such a case, the particular bank's utilizaiton state does not increase but - the pre-cached seed hash is updated and more tokens are marked as available/spendable.
  validateTT(token, updateState = true, isBankUpdate = false) {

    if (token == null || token == 'undefined')
      return 0n;

    //local-variables - BEGIN

    //**WARNING*** <--------------------
    let worthValue = 0n; //*UTMOST* importance is for this NOT to change until decision is made and everything IS VERIFIED
    //**WARNING*** <--------------------
    let bank;
    let startPoint;
    let checkPoint;
    let iterationsCount = 0;

    startPoint = token.getRevealedHash(); //that's the secret revealed by this very Transmission Token
    //local-variables - END

    //Operational Logic - BEGIN ----------------------

    //Security-Checks - BEGIN
    if (this.mStatus != eTokenPoolStatus.active)
      return 0n;

    if (this.mPubKey.byteLength == 32 && token.getSig().byteLength > 0) {
      if (!gCrypto.verify(this.mPubKey, token.getPackedData(false), token.getSig()))
        return false;
    }

    if (token.getRevealedHashesCount() == 0) //worthless
      return 0n;

    if (token.getValue() > this.getTotalValue()) //impossible
    {
      return 0n;
    }

    //note that the order of checks suits performance optimization
    bank = this.getBankById(token.getBankID());
    if (bank == null)
      return 0n;

    if (bank.getStatus() == eTokenPoolBankStatus.depleted)
      return 0n;

    if (token.getValue() > bank.getValueLeft()) //impossible
    {
      return 0n;
    }

    if (token.getCurrentDepth() <= bank.getCurrentDepth()) //possibly a double-spend attempt
    //(still, LEGIT behaviour IF data arrived through multiple paths though. Recognition of that NoT suported in this universial implementation)
    //Note that doule-spends would NOT succeeed anyway.
    {
      return 0n;
    }

    //Security-Checks - END

    //check tokens in between - BEGIN ----------------------
    startPoint = token.getRevealedHash();
    //we'll be verifying hashes from this point up to the current hash of the Token

    checkPoint = bank.getFinalHash(); //gets CURRENT ceiling hash the token-poor interval represented by this token would NEED to end at it.
    //mCurrentHash will be used solely for optimization purposes IF available

    //let's go with verifying tokens in between:
    //this should result in the final hash OR the intermediary hash IF available within the Token Pool (optimization)

    //check just in case
    if (startPoint.byteLength != 32 || checkPoint.byteLength != 32)
      return 0n;

    iterationsCount = token.getRevealedHashesCount();

    //hashing
    let currentHash = startPoint;
    for (let i = 0; i < iterationsCount; i++) {
      currentHash = gCrypto.getSHA2_256Vec(currentHash);
    }
    if (!gTools.compareByteVectors(currentHash, checkPoint))
      return false;

    //check if checkpoint reached
    if (!gTools.compareByteVectors(currentHash, checkPoint))
      return false;

    //check tokens in between - END ----------------------

    //calculate total value
    worthValue = BigInt(this.getSingleTokenValue()) * BigInt(iterationsCount);

    if (updateState) {
      //update pool-usage (internal state only)
      if (isBankUpdate) {
        //in such a case the utilization state is not updated but for the pre-caches Seed hash (which is the seed hash revealed by the particular Token)
        //this makes additional assets available for the Web-UI dApps.
        bank.setPreCachedSeedHash(token.getRevealedHash(), token.getCurrentDepth());
      } else {
        //if so, then the utilization state is updated.No additional assets are being made available. While the earlier is used by token pool's owner to allow
        //for releasing of assets from the web-ui,this might be used by recipient to track validity of received Tokens and to keep track of what's left within
        //a given State-Less Blockchain Channel.
        bank.setCurrentHash(token.getRevealedHash());
        bank.setCurrentDepth(token.getCurrentDepth());
      }
      //depletion check
      if (updateState && bank.getCurrentDepth() >= this.mDimensionDepth) {
        bank.setStatus(eTokenPoolBankStatus.depleted);
      }
    }

    //** DECISION MADE - BEGIN **

    //** DECISION MADE - END **

    //checks passed

    //Operational Logic - END ----------------------

    return worthValue;
  }

  getTTWorthValue(bankID, value, markUsed = true) {
    // Verify pool has valid seed before attempting TT generation
    if (!this.canSpendTokens()) {
      console.error('[CTokenPool.getTTWorthValue] Cannot generate TT - pool seed not available');
      return null;
    }

    if (this.mBanks.length == 0 || bankID > this.mBanks.length - 1)
      return null;
    return this.mBanks[bankID].genTTWorthValue(value, markUsed);
  }

  //Token-Pool meta-data retrieval
  get getOwnerID() {
    return this.mOwnerID;

  }
  get getFriendlyID() {
    return this.mFriendlyID;
  }
  setFriendlyID(id) {
    this.mFriendlyID = id;
  }
  get getTokensCount() {
    return BigInt(this.mDimensionDepth) * BigInt(this.mDimensionsCount);
  }


  get getReceiptID() {
    return this.mReceiptID;
  }
  get getTokenPoolID() {
    return this.mTokenPoolID;
  }

  setPubKey(pubKey) {
    if (!(pubKey == null || pubKey.byteLength == 0 || pubKey.byteLength == 32))
      return false;
    this.mPubKey = pubKey;
    return true;
  }
  get getPubKey() {

    return this.mPubKey;
  }

  //serialization
  getPackedData(includeSeed = true) //retrieve BER-encoded packed data
  {
    // DEBUG: Log key values being serialized
    console.log('[CTokenPool.getPackedData] Serializing pool:');
    console.log('  mDimensionDepth:', this.mDimensionDepth?.toString());
    console.log('  mDimensionsCount:', this.mDimensionsCount);
    console.log('  mTotalValue:', this.mTotalValue?.toString());
    console.log('  mFriendlyID:', this.mFriendlyID);

    ///if (this.mBanks.length != this.mDimensionsCount || this.mBanks.length == 0) id zero banks then it's a Request to generate the pool
    //  return null;
    let seed = includeSeed ? this.mMasterSeedHash : new ArrayBuffer();

    //we need to construct encoding iteratively
    let wrapperSeq = new asn1js.Sequence();
    wrapperSeq.valueBlock.value.push(new asn1js.Integer({
      value: this.mVersion
    }));

    let mainDataSeq = new asn1js.Sequence();
    let dimensionsDataSeq = new asn1js.Sequence();

    mainDataSeq.valueBlock.value.push(new asn1js.OctetString({
      valueHex: gTools.convertToArrayBuffer(this.mOwnerID)
    }));
    mainDataSeq.valueBlock.value.push(new asn1js.OctetString({
      valueHex: seed
    }));

    // CRITICAL: Use little-endian encoding to match C++ BigIntToBytes (export_bits msv_first=false)
    const dimensionDepthBytes = gTools.bigIntToArrayBufferNew(this.mDimensionDepth, true);
    const totalValueBytes = gTools.bigIntToArrayBufferNew(this.mTotalValue, true);

    // DEBUG: Show actual bytes being serialized
    console.log('[CTokenPool.getPackedData] BigInt byte encoding - START');
    console.log('[CTokenPool.getPackedData] dimensionDepthBytes:', dimensionDepthBytes);
    console.log('[CTokenPool.getPackedData] totalValueBytes:', totalValueBytes);
    console.log('[CTokenPool.getPackedData] dimensionDepthBytes byteLength:', dimensionDepthBytes ? dimensionDepthBytes.byteLength : 'NULL');
    console.log('[CTokenPool.getPackedData] totalValueBytes byteLength:', totalValueBytes ? totalValueBytes.byteLength : 'NULL');
    if (dimensionDepthBytes && dimensionDepthBytes.byteLength > 0) {
      console.log('[CTokenPool.getPackedData] dimensionDepth hex:', Array.from(new Uint8Array(dimensionDepthBytes)).map(b => b.toString(16).padStart(2, '0')).join(''));
    }
    if (totalValueBytes && totalValueBytes.byteLength > 0) {
      console.log('[CTokenPool.getPackedData] totalValue hex:', Array.from(new Uint8Array(totalValueBytes)).map(b => b.toString(16).padStart(2, '0')).join(''));
    }
    console.log('[CTokenPool.getPackedData] BigInt byte encoding - END');

    mainDataSeq.valueBlock.value.push(new asn1js.OctetString({
      valueHex: dimensionDepthBytes
    }));
    mainDataSeq.valueBlock.value.push(new asn1js.Integer({
      value: this.mDimensionsCount
    }));
    mainDataSeq.valueBlock.value.push(new asn1js.OctetString({
      valueHex: totalValueBytes
    }));

    mainDataSeq.valueBlock.value.push(new asn1js.OctetString({
      valueHex: this.mReceiptID
    }));
    mainDataSeq.valueBlock.value.push(new asn1js.OctetString({
      valueHex: this.mTokenPoolID
    }));
    mainDataSeq.valueBlock.value.push(new asn1js.OctetString({
      valueHex: gTools.convertToArrayBuffer(this.mFriendlyID)
    }));

    mainDataSeq.valueBlock.value.push(new asn1js.Integer({
      value: this.mStatus
    }));

    //construct the dimensions-sequence
    for (let i = 0; i < this.mBanks.length; i++) {
      //new sequence for each dimension
      let dimension = new asn1js.Sequence();
      // IMPORTANT: Use getFinalHash(false) during serialization to match C++ reference implementation
      dimension.valueBlock.value.push(new asn1js.OctetString({
        valueHex: this.mBanks[i].getFinalHash(false)
      }));
      dimension.valueBlock.value.push(new asn1js.OctetString({
        valueHex: this.mBanks[i].getCurrentHash()
      }));
      // CRITICAL: Use little-endian encoding to match C++ BigIntToBytes
      dimension.valueBlock.value.push(new asn1js.OctetString({
        valueHex: gTools.bigIntToArrayBufferNew(this.mBanks[i].getCurrentDepth(), true)
      }));
      dimensionsDataSeq.valueBlock.value.push(dimension);
    }
    //fix-sequences into their slots

    mainDataSeq.valueBlock.value.push(dimensionsDataSeq);


    //the below optional for authenticated TokenPools
    //authenticated token pools support both authenticated Tokens and non-authenticated tokens once the secret has been revealed
    //both would be accepted.
    //still , if a token IS authenticated then full-node woouldn't be able to 'steal it away'.
    //usage of authenticated tokens is more expensive due to their bigger size
    if (this.mPubKey.byteLength > 0) {
      mainDataSeq.valueBlock.value.push(new asn1js.OctetString({
        valueHex: this.mPubKey
      }));
    }
    wrapperSeq.valueBlock.value.push(mainDataSeq);

    var bytes = wrapperSeq.toBER(false);
    var length = bytes.byteLength;
    return bytes;

  }

  //deserialization
  static instantiate(packedData) {
    try {
      if (packedData.byteLength == 0)
        return 0;
      //local variables - BEGIN
      let pool = new CTokenPool();
      let usageDepth = 0n;
      let bankID = 0;
      let friendlyID, wrapperSeq, mainSeq, dimensionSeq, decodedASN1, dimensionsWrapperSeq;
      //local variables - END

      decodedASN1 = asn1js.fromBER(packedData);

      if (decodedASN1.offset === (-1))
        return 0; // Error during decoding

      wrapperSeq = decodedASN1.result;
      pool.mVersion = wrapperSeq.valueBlock.value[0].valueBlock.valueDec;

      if (pool.mVersion == 2) {
        mainSeq = wrapperSeq.valueBlock.value[1];
        pool.mOwnerID = mainSeq.valueBlock.value[0].valueBlock.valueHex;
        pool.mMasterSeedHash = mainSeq.valueBlock.value[1].valueBlock.valueHex;
        pool.mDimensionDepth = gTools.arrayBufferToBigInt(mainSeq.valueBlock.value[2].valueBlock.valueHex);
        pool.mDimensionsCount = BigInt(mainSeq.valueBlock.value[3].valueBlock.valueDec);
        pool.mTotalValue = gTools.arrayBufferToBigInt(mainSeq.valueBlock.value[4].valueBlock.valueHex);
        pool.mReceiptID = mainSeq.valueBlock.value[5].valueBlock.valueHex;
        pool.mTokenPoolID = mainSeq.valueBlock.value[6].valueBlock.valueHex;
        friendlyID = mainSeq.valueBlock.value[7].valueBlock.valueHex;
        pool.mFriendlyID = gTools.arrayBufferToString(friendlyID);
        pool.mStatus = mainSeq.valueBlock.value[8].valueBlock.valueDec;
      } else {
        if (pool.mVersion < 2) {} //console.log('This token-pool version is no longer supported by the Web-UI.')
        if (pool.mVersion > 2) {} //console.log('You might need to upgrade GRIDNET-OS web-software.')
        return null; //unsupported
      }
      //decode dimensions - BEGIN
      if (mainSeq.valueBlock.value.length > 9) {
        dimensionsWrapperSeq = mainSeq.valueBlock.value[9];
        for (let i = 0; i < dimensionsWrapperSeq.valueBlock.value.length; i++) { //iterate over all Dimensions

          dimensionSeq = dimensionsWrapperSeq.valueBlock.value[i];
          let finalHash = dimensionSeq.valueBlock.value[0].valueBlock.valueHex;
          let currentHash = dimensionSeq.valueBlock.value[1].valueBlock.valueHex;
          let usageDepth = gTools.arrayBufferToBigInt(dimensionSeq.valueBlock.value[2].valueBlock.valueHex);

          let bank = new CTokenPoolBank(bankID, pool, finalHash, usageDepth, pool.getSeedingHashForDimension(bankID));
          pool.mBanks.push(bank);
          if (gTools.compareByteVectors(currentHash, finalHash))
            bank.setStatus(eTokenPoolBankStatus.depleted); //serializing this would be a waste of storage; we can infer its value
          bankID++;
        }

      }

      if (friendlyID.byteLength > 0)
        pool.setFriendlyID(gTools.arrayBufferToString(friendlyID));

      if (mainSeq.valueBlock.value.length > 10) {
        pool.mPubKey = mainSeq.valueBlock.value[10].valueBlock.valueHex;
      }
      //decode dimensions - END

      return pool;
    } catch (error) {
      return null;
    }
  }

  getSingleTokenValue() {
    if (this.mDimensionDepth == 0 || this.mDimensionsCount == 0)
      return 0n; //no tokens

    return BigInt(this.mTotalValue) / ((BigInt(this.mDimensionDepth) * BigInt(this.mDimensionsCount)));
  }
  getTotalValue() {
    return this.mTotalValue;
  }
  getValueLeft() {

    let valueLeft = 0n;

    for (let i = 0; i < this.mBanks.length; i++) {
      valueLeft += this.mBanks[i].getValueLeft();
    }

    return valueLeft;
  }

  getLocallySpendableAssets() {
    let spendable = 0n;

    for (let i = 0; i < this.mBanks.length; i++) {
      spendable += this.mBanks[i].getLocallySpendableAssets();
    }

    return spendable;
  }


  getTokensLeft() {

    let tokensLeft = 0n;

    for (let i = 0; i < this.mBanks.length; i++) {
      tokensLeft += this.mBanks[i].getTokensLeft();
    }

    return tokensLeft;
  }
  getVersion() {
    return this.mVersion;
  }
}

//Represents a Transmission token, which represents the amount of value released from a Token Pool.
//When delivered to the decentralized data-store, it constitutes the Transit Pool i.e. the total amout assigned to recipient.
//After Transit Pool delivered to the decentralized data-store, the bank it referenced can be re-used.
export class CTransmissionToken {
  initFields() {
    this.mBankUsageDepth = 0n;
    this.mRevealedHashesCount = 0n;
    this.mValue = 0n;
    this.mVersion = 1;
    this.mBankIndex = 0n;
  }

  constructor(bankID = 0, value = 0, revealedHash = new ArrayBuffer(), revealedHashesCount = 0, bankUsageDepth = 0, dataHash = new ArrayBuffer(), tokenPoolID = new ArrayBuffer(), transmissionTokenID = new ArrayBuffer(), sig = new ArrayBuffer()) {
    this.initFields();

    this.mBankIndex = bankID;
    this.mRevealedHash = revealedHash;
    this.mRevealedHashesCount = revealedHashesCount;
    this.mBankUsageDepth = bankUsageDepth;
    this.mDataHash = dataHash;
    this.mTokenPoolID = tokenPoolID;
    this.mSig = sig;
    this.mValue = value;
    this.mRecipient = new ArrayBuffer();

    if (this.mDataHash == null)
      this.mDataHash = new ArrayBuffer();
    if (this.mSig == null)
      this.mSig = new ArrayBuffer();
    if (this.mDataHash == null)
      this.mDataHash = new ArrayBuffer();
    if (this.mTokenPoolID == null)
      this.mTokenPoolID = new ArrayBuffer();

  }

  getRecipient() {
    return this.mRecipient;
  }

  setRecipient(recipient) {
    this.mRecipient = recipient;
  }
  getCurrentDepth() {

    return this.mBankUsageDepth;
  }
  getVersion() {
    return this.mVersion;
  }

  sign(privKey) {

    let sig = gCrypto.sign(privKey, this.getPackedData(false));
    if (sig.byteLength == 64) {
      this.mSig = sig;
      return true;
    } else
      return false;
  }

  verifySignature(pubKey) {
    if (gCrypto.verify(pubKey, this.getPackedData(false), mSig))
      return true;
    else
      return false;
  }
  getBankID() {
    return this.mBankIndex;
  }
  set setBankID(bankID) {
    this.mBankIndex = bankID;
  }
  getRevealedHash() {
    return this.mRevealedHash;
  }
  getRevealedHashesCount() {
    return this.mRevealedHashesCount;
  }

  getValue() {
    return this.mValue;
  }
  getDataHash() {
    return this.mDataHash;
  }
  getTokenPoolID() {
    return this.mTokenPoolID;
  }
  getSig() {
    return this.mSig;
  }

  validate() {
    return this.mBankUsageDepth > 0 && this.mRevealedHashesCount > 0 && this.mVersion > 0 && this.mTokenPoolID.byteLength == 32;
  }

  getPackedData(includeSig = true) {

    let temp = new ArrayBuffer();
    //we need to construct encoding iteratively due to optional fields
    let wrapperSeq = new asn1js.Sequence();
    wrapperSeq.valueBlock.value.push(new asn1js.Integer({
      value: this.mVersion
    }));

    let mainDataSeq = new asn1js.Sequence();

    mainDataSeq.valueBlock.value.push(new asn1js.OctetString({
      valueHex: this.mRevealedHash
    }));

    mainDataSeq.valueBlock.value.push(new asn1js.Integer({
      value: this.mBankIndex
    }));

    // CRITICAL: Use little-endian encoding to match C++ BigIntToBytes (export_bits msv_first=false)
    mainDataSeq.valueBlock.value.push(new asn1js.OctetString({
      valueHex: gTools.bigIntToArrayBufferNew(this.mBankUsageDepth, true)
    }));

    mainDataSeq.valueBlock.value.push(new asn1js.OctetString({
      valueHex: gTools.bigIntToArrayBufferNew(this.mRevealedHashesCount, true)
    }));

    mainDataSeq.valueBlock.value.push(new asn1js.OctetString({
      valueHex: this.mTokenPoolID
    }));
    mainDataSeq.valueBlock.value.push(new asn1js.OctetString({
      valueHex: this.mDataHash
    }));
    mainDataSeq.valueBlock.value.push(new asn1js.OctetString({
      valueHex: gTools.bigIntToArrayBufferNew(this.mValue, true)
    }));

    if (includeSig) {
      mainDataSeq.valueBlock.value.push(new asn1js.OctetString({
        valueHex: this.mSig
      }));

      if (this.mRecipient.byteLength > 0) {
        mainDataSeq.valueBlock.value.push(new asn1js.OctetString({
          valueHex: this.mRecipient
        }));
      }
    }

    wrapperSeq.valueBlock.value.push(mainDataSeq);

    var bytes = wrapperSeq.toBER(false);
    var length = bytes.byteLength;
    return bytes;

  }

  /*
    Instantiates a BER-encoded Transmission Token.
    Note: this function does NOT perform any kind of formal verification.
  */
  static instantiate(packedData) {
    try {

      let temp = new ArrayBuffer();
      if (packedData.byteLength == 0)
        return 0;
      //local variables - BEGIN
      let toRet = new CTransmissionToken();
      let decoded_sequence1, decoded_sequence2, decoded_asn1;
      //local variables - END

      decoded_asn1 = asn1js.fromBER(packedData);

      if (decoded_asn1.offset === (-1))
        return 0; // Error during decoding

      decoded_sequence1 = decoded_asn1.result;
      toRet.mVersion = decoded_sequence1.valueBlock.value[0].valueBlock.valueDec;

      if (toRet.mVersion == 1) {
        decoded_sequence2 = decoded_sequence1.valueBlock.value[1];
        toRet.mRevealedHash = decoded_sequence2.valueBlock.value[0].valueBlock.valueHex;
        toRet.mBankIndex = BigInt(decoded_sequence2.valueBlock.value[1].valueBlock.valueDec);

        toRet.mBankUsageDepth = gTools.arrayBufferToBigInt(decoded_sequence2.valueBlock.value[2].valueBlock.valueHex);
        toRet.mRevealedHashesCount = gTools.arrayBufferToBigInt(decoded_sequence2.valueBlock.value[3].valueBlock.valueHex);
        toRet.mTokenPoolID = decoded_sequence2.valueBlock.value[4].valueBlock.valueHex;
        toRet.mDataHash = decoded_sequence2.valueBlock.value[5].valueBlock.valueHex;
        toRet.mValue = gTools.arrayBufferToBigInt(decoded_sequence2.valueBlock.value[6].valueBlock.valueHex);

        //decode the optional Variables
        if (decoded_sequence2.valueBlock.value.length > 7)
          toRet.mSig = decoded_sequence2.valueBlock.value[7].valueBlock.valueHex;
        if (decoded_sequence2.valueBlock.value.length > 8)
          toRet.mRecipient = decoded_sequence2.valueBlock.value[8].valueBlock.valueHex;
      }

      return toRet;

    } catch (error) {
      return false;
    }
  }
}
