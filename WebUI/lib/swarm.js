'use strict';
import {
  CVirtualCamDev,
  CVirtualAudioDev
} from './SwarmManager.js'

import {
  CWindow
} from '/lib/window.js'

import {
  CNetMsg
} from '/lib/NetMsg.js'

import {
  CDataConcatenator
} from '/lib/tools.js'

import {
  CSwarmConnection
} from '/lib/swarmconnection.js'

import {
  CSwarmMsg
} from '/lib/swarmmsg.js'
import {
  CSDPEntity,
} from './SDPEntity.js'


var con = {
  'optional': [{
    'DtlsSrtpKeyAgreement': true
  }]
}




class CRTCExtraData {
  constructor(type = eRTCExtraDataType.sdp, data = new ArrayBuffer()) {
    this.mVersion = 1;
    this.mType = type;
    this.mData = data; //might encapsulate arbitrary protocol data ex. an SDP datagram or chat-dApp message.
  }

  get getData() {
    return this.mData;
  }

  get getType() {
    return this.mType;
  }

  set setType(type) {
    this.mType = type;
  }

  set setData(data) {
    this.mData = data;
  }



  getPackedData() {
    let wrapperSeq = new asn1js.Sequence();
    wrapperSeq.valueBlock.value.push(new asn1js.Integer({
      value: this.mVersion
    }));

    let mainDataSeq = new asn1js.Sequence();

    wrapperSeq.valueBlock.value.push(new asn1js.Integer({
      value: this.mType
    }));
    mainDataSeq.valueBlock.value.push(new asn1js.OctetString({
      valueHex: this.mData
    }));

    wrapperSeq.valueBlock.value.push(mainDataSeq);

    var bytes = wrapperSeq.toBER(false);
    var length = bytes.byteLength;
    return bytes;
  }

  static instantiate(packedData) {
    try {
      if (packedData.byteLength == 0)
        return 0;
      //local variables - BEGIN
      let toRet = new CRTCExtraData();
      let decoded_sequence1, decoded_sequence2, decoded_asn1;
      //local variables - END

      decoded_asn1 = asn1js.fromBER(packedData);

      if (decoded_asn1.offset === (-1))
        return 0; // Error during decoding

      decoded_sequence1 = decoded_asn1.result;
      toRet.mVersion = decoded_sequence1.valueBlock.value[0].valueBlock.valueDec;

      if (toRet.mVersion == 1) {
        decoded_sequence2 = decoded_sequence1.valueBlock.value[1];
        toRet.mType = decoded_sequence2.valueBlock.value[0].valueBlock.valueDec;
        toRet.mData = decoded_sequence2.valueBlock.value[1].valueBlock.valueHex;
      }

      return toRet;
    } catch (error) {
      return false;
    }
  }
}

//Instance of a WebRTC-Swarm.
//Swarm (in terms of proposed participants) is managed by the signaling full-node through a web-socket bankSeed communication.
//Control packets are echanged through CSDPEntity BER-encoded messages (also encapsulating JSON encoded ICEs)
export class CSwarm {

  //Swarm-Connection Events - BEGIN
  //Supporting agregated subscriptions

  //Swarm-Connection Events - END
  constructor(swarmManager, agentID = new ArrayBuffer(), swarmID = new ArrayBuffer(), trueID = new ArrayBuffer()) {
    console.log('New WebRTC Swarm created..');
    this.mRTCCfg = { //todo: replace with full-nodes automatically

    };

    this.mVMContext = CVMContext.getInstance();
    this.mRTCCfg.iceServers = this.mVMContext.ICEServers;

    this.mPeerReachableTimeoutMS = 5000; //MS
    this.mIsDedicatedPSK = false;
    this.mLastNewConnectionWithPeers = {};
    //Coonection Quality Thresholds - BEGIN
    this.mConnQualityMaxThreshold = 1000; //MS
    this.mConnQualityHighThreshold = 1500; //MS
    this.mConnQualityMediumThreshold = 2000; //MS
    this.mConnQualityLowThreshold = 3500; //MS
    //Coonection Quality Thresholds - END
    this.mTools = CTools.getInstance();
    this.mMyID = (agentID != null && agentID.byteLength > 0) ? agentID : gTools.convertToArrayBuffer(gTools.encodeBase58Check(gTools.getRandomVector(16)));
    this.mID = (swarmID != null && swarmID.byteLength > 0) ? gTools.convertToArrayBuffer(swarmID) : gTools.convertToArrayBuffer(CVMContext.getInstance().getMainSwarmID); //fallback to the main global GRIDNET-OS swarm
    this.mLastFNPingTimestamp = 0;
    this.mPeersPingIntervalMS = 250; //MS
    this.mLastOutgressPeersPingMS = 0;
    this.mJoinConfirmed = false;
    this.mLastGlobalAuthAttempt = 0;
    this.mLastTimeOutgressAuthInitTimestamp = 0;
    this.mSwarmAuthReq = eSwarmAuthRequirement.open;
    //the below two indicators are used to indicate whether microphone or mic are truly being streamed at any given moment.
    /*
    That is decoupled from 'effective' capabilties of a WebRTC Swarm.
    ex. 'mStreamingCam' might be set to true, but a stub 'black screen' or other dynamically generated vidoe might be being used (for straming).
    In such a case, SwarmManager MAY decide to release the associated hardware resources (cam/mic).
    */
    this.mKillWhenNoProcesses = true;
    this.mClientProcesses = [];
    this.mMakingOffer = false;
    this.mTrueID = trueID;
    this.mStreamingCam = false;
    this.mStreamingMic = false;

    //Security - BEGIN
    this.mPasswordImage = new ArrayBuffer();
    this.mCanBeOperational = true; //whether users agreed for the Swarm to become Operational.
    //i.e. when a Swarm id closed, we do not want to accet new getOffer inqueries delivered by means of the signaling server as
    //that would render participation in a given Swarm active indefinitely (and local user chose to quit).
    //Security - END
    this.mFNPingInterval = 30; //sec
    this.mSwarmManager = swarmManager;
    this.mControlerExecuting = false; //mutex
    this.mControllerThread = 0;
    this.mLIVEVideoTrack = null;
    this.mLIVEAudioTrack = null;
    this.mControllerThreadInterval = 100;
    this.mPendingConnections = [];
    this.mActiveConnections = [];
    this.mLastJoinAttempt = 0;
    this.mVirtualAudioDevice = new CVirtualAudioDev();
    this.mVirtualCamDevice = new CVirtualCamDev(640, 480);
    //this.mLIVEVideoTrack = this.mVirtualCamDevice.getTrack;
    this.mVirtualAudioDevice.getTrack.enabled = true;
    this.mVirtualCamDevice.getTrack.enabled = true;
    this.mLocalDummyStream = new MediaStream([this.mVirtualAudioDevice.getTrack, this.mVirtualCamDevice.getTrack]);
    //Default connections' capabilties - BEGIN
    this.mAllowedCapabilities = eConnCapabilities.audioVideo; //used as defaults by new connections
    this.mEffectiveOutgressCapabilities = eConnCapabilities.data; //used as defaults by new connections
    //^ notice that the above are typically overriden by mDefaultOutgressCapabilities of CSwarmsManager
    this.mEffectiveIngressCapabilities = eConnCapabilities.audioVideo; //used as defaults by new connections
    //Default connections' capabilties - END

    this.mState = eSwarmState.idle;

    //External Event handlers - BEGIN
    this.mSwarmStateChangeEventListeners = [];
    this.mSwarmAuthRequirementsChangedListeners = [];
    this.mSwarmRegistrationConfirmedListeners = [];
    this.mPeerStatusEventListeners = [];
    this.mConnectionQualityEventListeners = [];
    //Master-Events - Begin
    //These events aggregate individual events fired by individual connections.
    //Thanks to these external UI dApps can be notified about events related to newly connected/discovered peers without resubscribing.
    this.mTrackEventListeners = [];
    this.mPeerAuthenticationResultListeners = [];
    this.mMessageEventListeners = [];
    this.mSwarmMessageEventListeners = [];
    this.mDataChannelMessageEventListeners = [];
    this.mSwarmConnectionStateChangeEventListeners = [];
    this.mConnectionStateChangeEventListeners = [];
    this.mICEConnectionStateChangeEventListeners = [];
    this.mDataChannelStateChangeEventListeners = [];

    ///!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //IMPORTANT: when adding qeueus, upgrade unregisterEventListenersByAppID()
    ///!!!!!!!!!!!!!!!!!!!!!!!!!!!!


    //Master-Events - End
    //External Event handlers - END
  }

  get isDedicatedPSK() {
    return this.mIsDedicatedPSK;
  }

  set isDedicatedPSK(isIt) {
    this.mIsDedicatedPSK = isIt;
  }



  get canBeOperational() {
    return this.mCanBeOperational;
  }
  set canBeOperational(canIt) {
    this.mCanBeOperational = canIt;
    if (!canIt) {
      CTools.getInstance().logEvent("Swarm '" + this.mTools.arrayBufferToString(this.trueID) + "' can be Operational no more.", eLogEntryCategory.localSystem, 1, eLogEntryType.notification);
    } else {
      CTools.getInstance().logEvent("Swarm '" + this.mTools.arrayBufferToString(this.trueID) + "' can be now Operational.", eLogEntryCategory.localSystem, 1, eLogEntryType.notification);
    }
  }
  set killWhenNoProcesses(doIt) {
    this.mKillWhenNoProcesses = doIt;
  }

  get killWhenNoProcesses() {
    return this.mKillWhenNoProcesses;
  }

  addClientProcess(id) {
    let alreadyThere = false;

    for (let i = 0; i < this.mClientProcesses.length; i++) {
      if (this.mClientProcesses[i] == id)
        return false;
    }
    this.mClientProcesses.push(id);

    CTools.getInstance().logEvent('Registering process ' + id + " with Swarm '" + this.mTools.arrayBufferToString(this.trueID) + "'.", eLogEntryCategory.localSystem, 1, eLogEntryType.notification);
    return true;
  }

  addClientAppInstance(window) {
    if (!this.mTools.isInstanceOf(window, CWindow))
      return false;

    return this.addClientProcess(window.getProcessID);
  }

  removeClientProcess(id) {
    let alreadyThere = false;
    let removed = false;

    for (let i = 0; i < this.mClientProcesses.length; i++) {
      if (this.mClientProcesses[i] == id) {
        this.mClientProcesses.splice(i, 1);
        removed = true;
        break;
      }
    }

    if (removed) {
      CTools.getInstance().logEvent('Unregistering process ' + id + " for Swarm '" + gTools.bytesToString(this.trueID) + "'.", eLogEntryCategory.localSystem, 1, eLogEntryType.notification);

      if (this.mClientProcesses.length == 0 && this.killWhenNoProcesses) {
        CTools.getInstance().logEvent(`Killing Swarm '` + gTools.bytesToString(this.trueID) + "' all client apps have quit.", eLogEntryCategory.network, 1, eLogEntryType.notification);
        this.mSwarmManager.leaveSwarm(this.mTrueID); //let the singaling node know that we're leaving.
        this.close();
      }

    }
    return removed;
  }

  removeClientAppInstance(windowP) {
    if (!this.mTools.isInstanceOf(windowP, CWindow))
      return false;

    return this.removeClientProcess(windowP.getProcessID);
  }


  //Broadcasts a keep-alive datagram to all Swarm participants.
  ping() {
    for (let i = 0; i < this.mActiveConnections.length; i++) {
      this.mActiveConnections[i].ping();
    }
  }

  pingJoinAttempt() {
    this.mLastJoinAttempt = gTools.getTime();
  }
  set joinAttemptTimestamp(time) {
    this.mLastJoinAttempt = time;
  }
  get joinAttemptTimestamp() {
    return this.mLastJoinAttempt;
  }
  set authRequirement(req) {
    this.mSwarmAuthReq = req;
    this.onAuthReqChanged(req);
  }

  get authRequirement() {
    return this.mSwarmAuthReq;
  }
  //Issues a Swarm-wide authentication request.
  //Notice: notifyAuthenticationSuccess() and  notifyAuthenticationFailure()  are available from the scope of a paritcular connection only.
  requestAuthentication(forceIt = false) {
    if (forceIt) {
      console.log('Requesting authentication from all active peers..');
    } else {
      console.log('Requesting (re)authentication from already authenticated peers only..');
    }

    for (let i = 0; i < this.mActiveConnections.length; i++) {
      if (forceIt || !this.mActiveConnections[i].isAuthenticated) { //issue the request only to those who have not authenticated yet.
        this.mActiveConnections[i].requestAuthentication(forceIt);
      }
    }
  }

  get trueID() {
    return this.mTrueID;
  }
  set trueID(value) {
    this.mTrueID = value;
  }
  get getLocalDummyStream() {
    return this.mLocalDummyStream;
  }

  //Cammands' Processor.
  //Typically, commands are delivered throgh the data-track of a WebRTC stream.
  /*
  This function is responsible for processing of both local commands and those delivered from remote endpoints.
  If, latter is the case, the cnnection parameter MUST be set.
  The function returns eSwarmCmdProcessingResult .
  */


  async processCommand(cmd, connection = null) {
    let params = [];
    let result = eSwarmCmdProcessingResult.invalid;
    //pre-validation - BEGIN
    if (!gTools.isString(cmd) || cmd.length == 0)
      return result;

    if (!cmd[0] == "/")
      return result;

    cmd = cmd.substring(1); //remove the leading '/'

    let words = cmd.trim().split(/\s+/);

    let cmdWord = words.length ? words[0] : cmd;

    if (words.length > 1) { //retrieve parameters if any
      for (let i = 1; i < words.length; i++) {
        params.push(words[i]);
      }
    }

    //pre-validation - END

    //processing - BEGIN
    switch (cmdWord) {
      case 'setkey': //
        if (params.length == 0) {
          //effectively disables AUTH for this swarm.
          this.clearPSK();
          this.authRequirement = eSwarmAuthRequirement.open; //dispatches notifications
          result = eSwarmCmdProcessingResult.success;
          return result;
        }


        //Local Execution - BEGIN
        //this command was provided by the local user.
        if (!connection) {
          // if 'auth' is invoked in the context of a local connection, it actually sets the pre-sahred secret.
          //we employ the fact that the passwords is well, - shared and same on all nodes involved.
          //return eSwarmCmdProcessingResult.invalidInThisContext;
          let pass = params[0];
          if (this.isPasswordNew(pass)) {
            let setKeyRes = await this.setPreSharedKey(pass);
            if (setKeyRes) { //that would MUTE all the current connections.
              this.isDedicatedPSK = true; //i.e. not one based on sha3(Swarm's True ID | IV time-based)
              console.log('Security key for a Swarm changed.');
              this.authRequirement = eSwarmAuthRequirement.PSK_ZK; //dispatches notifications
              this.requestAuthentication(true); //force (re) authentication requirement
              //onto all active connections (even those that are currently authenticated - i.e. during a password change).
              result = eSwarmCmdProcessingResult.success;
            } else {
              result = eSwarmCmdProcessingResult.failure;
            }
          } else {
            console.log('Provided shared-secret is already set.');
          }

          return;
        }
        //Local Execution - END
        else {

        }
        break;

        /*case 'setkey':
          if (connection)
            return eSwarmCmdProcessingResult.invalidInThisContext; //external peers cannot execute this command locally.
          if (params.length == 0)
            return eSwarmCmdProcessingResult.invalid;
          if (this.setPreSharedKey(params[0])) {
            result = eSwarmCmdProcessingResult.success;
          } else {
            result = eSwarmCmdProcessingResult.failure;
          }
          console.log('Security key for a Swarm changed.');

          break;*/
      default:
        return eSwarmCmdProcessingResult.invalid;
    }
    //processing - END
    return result;
  }

  get isPrivate() {
    let keyImage = this.getPreSharedKey(true);
    if (gTools.isNull(keyImage) || keyImage.byteLength == 0) {
      return false;
    }
    return true;
  }

  //LIVE data getters and setters - BEGIN
  //UI dApp may set the active track.
  //the active tracks are used whenenever:
  /*
    1) the corrensponding CSwarmConnection is NOT muted ( through muteStreams(), which is when it is temporarly REPLACED with a Dummy Video/Audio Track).
    2) track has not been MUTED (i.e. *not* replaced) through a call to setEffectiveCapabilities().

    W need BOTH muting and replacing to first cover up for last-frame-freezed bugs in web-browsers and second, - to save on bandwidth utilization by not
    streaming Dummy Data indefinitely.

    IMPORTANT: In a PRIVATE Swarm, Dummy Tracks are not switched to LIVE Tracks until the other peer successfuly authenticates.

  */

  //Mutes audio/video tracks. Does NOT mute the data channel.
  //Notice: the function is availble both at a Swarm and Connection level.
  async mute(audio = true, video = true, outgress = true, autoStopDummyTrack = false, autoStopDummyTrackAfterMS = 1000) {
    let conns = this.getActiveConnection;
    let result = true;
    for (let i = 0; i < this.mActiveConnections.length; i++) {
      if ((await this.mActiveConnections[i].mute(audio, video, outgress, autoStopDummyTrack, autoStopDummyTrackAfterMS)) == false) {
        result = false;
      }
    }
    return result;
  }

  //Unmutes audio/video tracks. Does NOT mute the data channel.
  //Notice: the function is availble both at a Swarm and Connection level.
  async unmute(audio = true, video = true) {
    let conns = this.getActiveConnection;
    let result = true;
    for (let i = 0; i < this.mActiveConnections.length; i++) {
      if ((await this.mActiveConnections[i].unmute(audio, video)) == false) {
        result = false;
      }
    }
    return result;
  }

  get getLIVEVideoTrack() {
    return this.mLIVEVideoTrack;
  }

  async setLIVEVideoTrack(track, updateWithPeers = true) {
    if (!track) {
      console.log('error: there was an attempt to assign null to Live Video Track.');
      return;
    }
    if (track.getVirtualDev) {
      console.log('error: cannot assign a Virtual Stream to a Live Video Track.');
      return;
    }
    this.mLIVEVideoTrack = track;
    if (track.readyState != 'live') {
      console.log('warning: a zombie video track assigned to a LIVE session.');
    } else {
      console.log('Received a new LIVE video track.');
    }
    if (updateWithPeers && this.mActiveConnections.length) {
      console.log('Updating video stream with active peers..')
      await this.replaceVideoTrack(track);
    }
  }

  get getLIVEAudioTrack() {
    return this.mLIVEAudioTrack;
  }

  async setLIVEAudioTrack(track, updateWithPeers = true) {
    if (!track) {
      console.log('error: there was an attempt to assign null to Live Audio Track.');
      return;
    }
    if (track.getVirtualDev) {
      console.log('error: cannot assign a Virtual Stream to a Live Track.');
      return;
    }

    if (track.readyState != 'live') {
      console.log('warning: a zombie video track assigned to a LIVE session.');
    } else {
      console.log('Received a new LIVE audio track.');
    }

    this.mLIVEAudioTrack = track;
    if (updateWithPeers && this.mActiveConnections.length) {
      console.log('Updating audio stream with active peers..')
      await this.replaceAudioTrack(track);
    }
  }
  //LIVE data getters and setters - END

  addPeerAuthResultListener(eventListener, appID = 0) {
    this.mPeerAuthenticationResultListeners.push({
      handler: eventListener,
      appID
    });
  }

  clearPSK() {
    this.mPasswordImage = new ArrayBuffer();
  }
  //Sets a pre-shared secret (a password), the image of which is to be provided by each and every peer participating in the Swarm.
  //Notice: the function does not store the secret but an image of it. The secret itself is never stored or transmitted anywhere.
  //Should an invalid image of the password be provided by a peer,- other peers would refuse to provide real data (only the Dummy Data stream).
  //Only as soon as a valid image of the pre-shared secret is provided, would nodes switch to the actual real-time data-tracks (web-cam/microphone/chat).
  async setPreSharedKey(pass) {

    //Local Variables - BEGIN
    let previous = this.getPreSharedKey(false);
    let armingPrivacy = true;
    let now = gTools.getTime(false); //a timestamp in seconds.
    let IV = new ArrayBuffer();
    //  let dc = new CDataConcatenator();
    //Local Variables - END

    //Operational Logic - BEGIN
    if (gTools.isNull(pass) || pass.length == 0) {
      //make data available to everyone
      armingPrivacy = false;
      console.log('Making Swarm public..');

    } else {

      //IMPORTANT: IV can be computed only when value is READ.
      //that is to cover for a situation when peer A sets password long before peer B.
      //Compute Salt - BEGIN
      //we are to allow for a ~18-hour time-drift, since the beginning of 1970, as measured in seconds.
      //allow for 256/2 (average! till it wraps! i.e 128 sec) sec time-drift. - only the last byte is skipped.
      //  IV = gTools.numberToArrayBuffer(now, false).slice(4, 7); //convert time-tamp to its Big-Endian representation
      //  console.log('Pre-Shared Key IV: '+  gTools.arrayBufferToNumber(IV));
      //Compute Salt - END
      armingPrivacy = true;
      console.log('Making ' + gTools.arrayBufferToString(this.trueID) + ' Swarm Private..');
    }

    //Make the changes Operational - BEGIN
    if (armingPrivacy) {

      //  dc.add(pass); //add the actual password.
      //  dc.add(IV); //add some salt.
      this.mPasswordImage = sha3_256.arrayBuffer(pass); //transform through a one-way hash-function

      if (armingPrivacy && !gTools.compareByteVectors(this.getPreSharedKey(false), previous)) { //the key changed.
        let coolingDown = true;
        await this.mute(true, true, true, true);

        for (let i = 0; i < this.mActiveConnections.length; i++) {
          this.mActiveConnections[i].isAuthenticated = false; //require all peers to be reauthenticated.
          //the (re)authentication can happen asynchronously on per-peer basis. Some would be seeing current data earlier then others, as they authenticate
          //with new credentials.
        }
      }
      return armingPrivacy;
    } else {
      this.mPasswordImage = new ArrayBuffer();
    }
    //Make the changes Operational - END
    return false;
    //Operational Logic - END
  }

  get getSwarmsManager() {
    return this.mSwarmManager;
  }
  //Retrieves image of the pre-shared secret which is to be known among participants of a Private Swarm.
  getPreSharedKey(addNonce = true) {

    if (this.mPasswordImage.byteLength != 32)
      return new ArrayBuffer();

    if (!addNonce) {
      return this.mPasswordImage;
    }
    //Compute Salt - BEGIN
    //we are to allow for a ~18-hour time-drift, since the beginning of 1970, as measured in seconds.
    //allow for 256/2 (average! till it wraps! i.e 128 sec) sec time-drift. - only the last byte is skipped.
    let IV = gTools.numberToArrayBuffer(gTools.getTime(), false).slice(4, 7); //convert time-tamp to its Big-Endian representation
    //console.log('Pre-Shared Key IV: ' + gTools.arrayBufferToNumber(IV));
    let dc = new CDataConcatenator();
    dc.add(this.mPasswordImage);
    dc.add(IV);
    return sha3_256.arrayBuffer(dc.getData());

  }
  isPasswordNew(pass) {
    let img = sha3_256.arrayBuffer(gTools.convertToArrayBuffer(pass));
    return !this.verifyPreSharedKey(img);
  }
  //verifies the provided pre-shared key.
  verifyPreSharedKey(secretImg) {
    if (gTools.isNull(secretImg) || secretImg.byteLength == 0)
      return false;

    //let img = sha3_256.arrayBuffer(pass); <= nope, the IMAGE is provided.
    //the secret itself is NEVER expected.
    if (gTools.compareByteVectors(secretImg, this.getPreSharedKey(true))) {
      return true;
    }
    return false;
  }

  set setMicInUse(isIt = true) {
    this.mStreamingMic = isIt;
  }
  get getMicInUse() {
    return this.mStreamingMic;
  }

  set setCamInUse(isIt = true) {
    this.mStreamingCam = isIt;
  }
  get getCamInUse() {
    return this.mStreamingCam;
  }

  async replaceVideoTrack(track) { //notice the function is available both at the Swarm and Conneciton level.
    if (track == null)
      return;
    let conns = this.peers;

    for (let i = 0; i < conns.length; i++) {

      //Security - BEGIN
      if (this.isPrivate && !conns[i].isAuthenticated) {
        continue; //skipping
      }
      //Security - END
      await conns[i].replaceVideoTrack(track);
    }
  }

  async replaceAudioTrack(track) {
    if (track == null)
      return;

    let conns = this.peers;

    for (let i = 0; i < conns.length; i++) {

      //Security - BEGIN
      if (this.isPrivate && !conns[i].isAuthenticated) {
        continue; //skipping
      }
      //Security - END
      await conns[i].replaceAudioTrack(track);
    }
  }



  //Low-level (0/2) aggegarted event handlers called by underlying Swarm Connections - BEGIN
  //low level agregators simply pass on the event and provide conn ID and swarm ID fields.
  //higher-level aggregators provide additional translation.
  //Note: all data sent should be encapsulated within CNetMsg
  onNewPeerData(conn, data) {
    this.mSwarmManager.onNewSwarmData(conn.getID, conn.getPeerID, data);
  }


  //High level aggregators - Begin

  onDataChannelStateChange(event) //note that is our high-level aggregator; it aggregates a few WebRTC native events
  {
    for (let i = 0; i < this.mDataChannelStateChangeEventListeners.length; i++) {
      try {

        this.mDataChannelStateChangeEventListeners[i].handler(event);
      } catch (error) {
        console.log('UI dApp failed to process event. App ID: ' + this.mDataChannelStateChangeEventListeners[i].appID)
      }
    }
  }

  //1st level of encapsulation. (API Level 1/2)
  onMessage(event) {
    //core processing - BEGIN

    //core processing - END

    //User-Mode notifications - BEGIN
    for (let i = 0; i < this.mMessageEventListeners.length; i++) {
      try {
        this.mMessageEventListeners[i].handler(event);
      } catch (error) {
        console.log('UI dApp failed to process event. App ID: ' + this.mMessageEventListeners[i].appID)
      }
    }
    //User-Mode notifications - END
  }
  //2nd level of encapsulation.(API Level 2/2)
  onSwarmMessage(event) {

    //core processing - BEGIN

    //core processing - END

    //User-Mode notifications - BEGIN
    for (let i = 0; i < this.mSwarmMessageEventListeners.length; i++) {
      try {
        this.mSwarmMessageEventListeners[i].handler(event);
      } catch (error) {
        console.log('UI dApp failed to process event. App ID: ' + this.mSwarmMessageEventListeners[i].appID)
      }
    }

    //User-Mode notifications - END
  }

  onPeerStatusChange(event) {
    for (let i = 0; i < this.mPeerStatusEventListeners.length; i++) {
      try {
        this.mPeerStatusEventListeners[i].handler(event);
      } catch (error) {
        console.log('UI dApp failed to process event. App ID: ' + this.mPeerStatusEventListeners[i].appID)
      }
    }
  }

  onConnQualityChange(event) {
    for (let i = 0; i < this.mConnectionQualityEventListeners.length; i++) {
      try {
        this.mConnectionQualityEventListeners[i].handler(event);
      } catch (error) {
        console.log('UI dApp failed to process event. App ID: ' + this.mConnectionQualityEventListeners[i].appID)
      }
    }
  }



  onSwarmConnectionStateChange(event) {
    for (let i = 0; i < this.mSwarmConnectionStateChangeEventListeners.length; i++) {
      try {
        this.mSwarmConnectionStateChangeEventListeners[i].handler(event);
      } catch (error) {
        console.log('UI dApp failed to process event. App ID: ' + this.mSwarmConnectionStateChangeEventListeners[i].appID)
      }
    }
  }
  //High level aggregators - End
  //Low level aggregators - Begin
  onTrack(event) {
    for (let i = 0; i < this.mTrackEventListeners.length; i++) {
      try {
        this.mTrackEventListeners[i].handler(event);
      } catch (error) {
        console.log('UI dApp failed to process event. App ID: ' + this.mTrackEventListeners[i].appID)
      }
    }
  }

  onPeerAuth(event) {
    for (let i = 0; i < this.mPeerAuthenticationResultListeners.length; i++) {
      try {
        this.mPeerAuthenticationResultListeners[i].handler(event);
      } catch (error) {
        console.log('UI dApp failed to process event. App ID: ' + this.mPeerAuthenticationResultListeners[i].appID)
      }
    }
  }



  onConnectionStateChangeEvent(event) {
    for (let i = 0; i < this.mConnectionStateChangeEventListeners.length; i++) {
      try {
        this.mConnectionStateChangeEventListeners[i].handler(event);
      } catch (error) {
        console.log('UI dApp failed to process event. App ID: ' + this.mConnectionStateChangeEventListeners[i].appID)
      }
    }
  }
  onDataChannelMessageEvent(event) //low level (0/2) data-channel event
  {
    for (let i = 0; i < this.mDataChannelMessageEventListeners.length; i++) {
      try {
        this.mDataChannelMessageEventListeners[i].handler(event);
      } catch (error) {
        console.log('UI dApp failed to process event. App ID: ' + this.mDataChannelMessageEventListeners[i].appID)
      }
    }

  }
  onICEConnectionStateChangeEvent(event) {
    for (let i = 0; i < this.mICEConnectionStateChangeEventListeners.length; i++) {
      try {
        this.mICEConnectionStateChangeEventListeners[i].handler(event);
      } catch (error) {
        console.log('UI dApp failed to process event. App ID: ' + this.mICEConnectionStateChangeEventListeners[i].appID)
      }
    }
  }
  //Low level aggregators - End
  //High-level aggegarted event handlers called by underlying Swarm Connections - END

  sendSwarmMessage(message, protocolID, onlyIfAuthenticated = true) {
    for (let i = 0; i < this.mActiveConnections.length; i++) {
      this.mActiveConnections[i].sendSwarmMessage(message, protocolID, onlyIfAuthenticated);
    }
  }

  sendMessage(message, onlyIfAuthenticated = true) {
    for (let i = 0; i < this.mActiveConnections.length; i++) {
      this.mActiveConnections[i].sendSwarmMessage(message, onlyIfAuthenticated);
    }
  }

  //if target == null delivers to everyone
  sendData(data, target = null, onlyToAuthenticated = true) {
    if (data == null)
      return;

    if (target != null) {
      let dest = this.getConnection(target);
      if (dest == null)
        return false;
      return dest.send(data);
    } else {

      if (this.mActiveConnections.length == 0)
        return false;

      for (let i = 0; i < this.mActiveConnections.length; i++) {
        this.mActiveConnections[i].send(data, onlyToAuthenticated);
      }
      return true;
    }

  }
  //Returns an overall Swarm's connection state (eSwarmState) from the viewpoint of the current peer.
  //Returns 'active' if there's an active connection with at least one peer , returns 'idle' otherwise.
  get getState() {
    for (let i = 0; i < this.mActiveConnections.length; i++) {
      if (this.mActiveConnections[i].getStatus == eConnectionState.connected)
        return eSwarmState.active;
    }
    return eSwarmState.idle;
  }

  onSwarmRegistrationConfirmed(req) {
    //notify

    for (let i = 0; i < this.mSwarmRegistrationConfirmedListeners.length; i++) {
      try {
        this.mSwarmRegistrationConfirmedListeners[i].handler({
          timestamp: this.mTools.getTime(),
          peerID: this.mPeerID,
          swarmID: this.getID
        });
      } catch (error) {
        console.log('UI dApp failed to process event. App ID: ' + this.mSwarmRegistrationConfirmedListeners[i].appID)
      }
    }
  }


  onAuthReqChanged(req) {
    //notify

    for (let i = 0; i < this.mSwarmAuthRequirementsChangedListeners.length; i++) {
      try {
        this.mSwarmAuthRequirementsChangedListeners[i].handler({
          auth: req,
          peerID: this.mPeerID,
          swarmID: this.getID
        });
      } catch (error) {
        console.log('UI dApp failed to process event. App ID: ' + this.mSwarmAuthRequirementsChangedListeners[i].appID)
      }
    }
  }


  set setState(state) {
    this.mState = state;

    //notify

    for (let i = 0; i < this.mSwarmStateChangeEventListeners.length; i++) {
      try {
        this.mSwarmStateChangeEventListeners[i].handler({
          state: this.mState,
          peerID: this.mPeerID,
          swarmID: this.mSwarm.getID
        });
      } catch (error) {
        console.log('UI dApp failed to process event. App ID: ' + this.mSwarmStateChangeEventListeners[i].appID)
      }
    }
  }

  transferConnToActive(connID) {

    if (connID == null || connID.byteLength == 0)
      return false;

    let index = -1;
    let foundConn = null;

    //check if connection already is within theactive connections pool if so=>Abort.
    for (let i = 0; i < this.mActiveConnections.length; i++) {
      if (gTools.compareByteVectors(this.mActiveConnections[i].getID, connID)) {
        return false;
      }
    }

    //check if connection truly is withint the pending-connections pool
    for (let i = 0; i < this.mPendingConnections.length; i++) {
      if (gTools.compareByteVectors(this.mPendingConnections[i].getID, connID)) {
        index = i;
        foundConn = this.mPendingConnections[i];
        break;
      }
    }

    //do the xFer
    if (index >= 0 && foundConn != null) {
      //Found
      this.mPendingConnections.splice(index, 1);
      this.mActiveConnections.push(foundConn);
      return true;
    }

    return false;
  }

  get getAllowedCapabilities() {
    return this.mAllowedCapabilities;
  }
  get getEffectiveIngressCapabilities() {
    return this.mEffectiveIngressCapabilities;
  }
  get getEffectiveOutgressCapabilities() {
    return this.mEffectiveOutgressCapabilities;
  }


  getActiveConnectionsCount() {
    return this.mActiveConnections.length;
  }

  getPendingConnectionsCount() {
    return this.mPendingConnections.length;
  }



  //Agregator
  //The ceiling capabilities allowed by the local user for all connections.
  //Note the difference between setCapabilities which is limited by the remote peer. (the union of local and remote capabilities)
  //Note the difference between setEffectiveOutgressCapabilities which can be used to mute/unmute tracks (video/audio) without causing renegotiation.
  //Note that calling this might cause connetion's renogotiation and firing of onTrack events on the other peers.
  setAllowedCapabilities(allowed) {
    this.mAllowedCapabilities = allowed;

    for (let i = 0; i < this.mActiveConnections.length; i++) {
      this.mActiveConnections[i].setAllowedCapabilities(allowed);
    }

    for (let i = 0; i < this.mPendingConnections.length; i++) {
      this.mPendingConnections[i].setAllowedCapabilities(allowed);
    }

  }

  //Agregator
  //Sets effective ingress capabilities for all connection within the swarm.
  //Can be used to mute/unmute tracks (video/audio) without causing the underlying connection's renegotiation.
  //Also sets the DEFAULTS member field (mEffectiveIngressCapabilities).
  setEffectiveIngressCapabilities(allowed) {
    this.mEffectiveIngressCapabilities = allowed;

    for (let i = 0; i < this.mActiveConnections.length; i++) {
      this.mActiveConnections[i].setEffectiveIngressCapabilities(allowed);
    }

    for (let i = 0; i < this.mPendingConnections.length; i++) {
      this.mPendingConnections[i].setEffectiveIngressCapabilities(allowed);
    }
  }

  //Agregator
  //Sets effective outgress capabilities for all connection within the swarm.
  //Can be used to mute/unmute tracks (video/audio) without causing the underlying connection's renegotiation.
  //Also sets the DEFAULTS member field (mEffectiveOutgressCapabilities).
  setEffectiveOutgressCapabilities(allowed) {
    console.log('Setting effective outgress capabilities of a Swarm to ' + allowed);
    if (allowed != this.mSwarmManager.getEffectiveOutgressCapabilities()) { //i.e. '<' relation is not enough (for only-audio and only-video)
      this.mSwarmManager.setLocalStreamEffectiveCapabilities(allowed);
    }
    this.mEffectiveOutgressCapabilities = allowed;

    for (let i = 0; i < this.mActiveConnections.length; i++) {
      this.mActiveConnections[i].setEffectiveCapabilities(allowed, true);
    }

    for (let i = 0; i < this.mPendingConnections.length; i++) {
      this.mPendingConnections[i].setEffectiveCapabilities(allowed, true);
    }

    this.mSwarmManager.optimizeRequestedResources(); //allow Swarm Manager to optimize (i.e. eliminate handles to hardware resources
    //that are no longer needed.)
  }

  handleGetUserMediaError(e) {
    switch (e.name) {
      case "NotFoundError":
        alert("No camera and/or microphone were found.");
        break;
      case "SecurityError":
      case "PermissionDeniedError":
        alert("Camera / microphone permission denied.");
        break;
      default:
        console.log("WebRTC Error: " + e.message);
        break;
    }

    //this.closeVideoCall();
  }
  ///Process an incoming signaling message (SDP/ICE/Control)

  //Node A - the one joining the swarm
  //Node B - the one accepting on incomming client (Warning: here, it's the one preparing the actual WebRTC offer!)
  //Node S - the full-node (signaling server)


  set joinConfirmed(isIt) {
    if (isIt) { //we want full-node to conrim that local node was successfuly registered within its Swarm table.
      //local node would continue resuming 'join' attempts until it is so.
      console.log("⋮⋮⋮ Node " + CVMContext.getInstance().getCurrentNodeURI + " confirmed registration at ⋮⋮⋮ Swarm '" + gTools.bytesToString(this.trueID) + "'");
    }
    this.mJoinConfirmed = isIt;

    //Core Internal Processing - BEGIN
    this.onSwarmRegistrationConfirmed();
    //Core Internal Processing - END
  }

  get joinConfirmed() {
    return this.mJoinConfirmed;
  }


  createPendingConnWithPeer(peerID) {
    let now = this.mTools.getTime();

    let lastTry = this.mLastNewConnectionWithPeers[peerID];
    //if (lastTry && (now - lastTry) < 10) {
    //this.mTools.logEvent("Cannot create connections with same peer that often.", eLogEntryCategory.network, 1, eLogEntryType.warning);
    //  return;
    //}
    this.mLastNewConnectionWithPeers[peerID] = now;
    let rtcConn = new RTCPeerConnection({
      iceServers: this.mRTCCfg.iceServers,
      iceTransportPolicy: "all"
    });
    //Notice: 1) connection ID is to be inferred, as soon as, the Data-Channel is established from RTCDataChannel.id
    //        2) the IP address is to be retrived from RTCiceCandidate.address as soon as ICE candidates are delivered.
    let conn = new CSwarmConnection(this, rtcConn, peerID, this.getAllowedCapabilities);

    conn.initEvents(); //the function is immune to numerous invocations
    //will instantiate only what it can (no data-channel atm.) the data-channel would be filled and events subscribed for in the onDataChannel event

    this.mPendingConnections.push(conn); //push pending connection
    return conn;
  }

  //Update: we are to be negotiating connection solely based on the dynamically generated dummy audio/video.
  //Step 1 - happened when Node A sent JOINING to Node S
  async processSignalEntity(sdpE) {

    let tools = CTools.getInstance();
    let now = tools.getTime();
    if (!this.canBeOperational) {
      tools.logEvent("Won't process signaling entity. Swarm '" + this.mTools.arrayBufferToString(this.trueID) + "' is not allowed to be Operational.", eLogEntryCategory.network, 1, eLogEntryType.warning);
      return;
    }

    if (sdpE == null || !sdpE.validate())
      return false;
    let allowFor = eConnCapabilities.data;
    let conn = this.getConnection(sdpE.getSourceID);

    //Stale Connection Detection - BEGIN
    if (conn) {
      if (conn.mRTCConnection.connectionState !== "connected" && (now - conn.signalingStateChangeTimestamp) > 45) {
        tools.logEvent("Warning: removing stale connection with " + gTools.bytesToString(sdpE.getSourceID) + "@" + (conn ? conn.getIPAddress : "unknown") + ". ", eLogEntryCategory.network, 1, eLogEntryType.warning);
        conn.close(); //would dispatch notifications, close and remove.
        conn = null;
      }
    }
    //Stale Connection Detection - END

    //Ensure there's a connection to process the SDP - BEGIN
    if (conn == null && sdpE.getSourceID.byteLength > 0 //IMPORTANT: create pending connections only for Offer-Requests and Offers. why? connection might had been disconnected form singaling server in which case we need to neglect all the other ICEs and SDPs.
      &&
      (sdpType == eSDPEntityType.getOffer || sdpType == eSDPEntityType.processOffer)) { //check for source identifier as an SDP issued by full-node would have it empty.
      //Security UPDATE: Do create a connection with  a peer the full-node proposes.
      conn = this.createPendingConnWithPeer(sdpE.getSourceID); //will be created with swarm-specific default capabilities.
    }
    //Ensure there's a connection to process the SDP - END

    if (conn) {
      if (!(tools.compareByteVectors(sdpE.SDPSessionID, conn.SDPSessionID) || sdpType == eSDPEntityType.processOffer || sdpType == eSDPEntityType.getOffer)) {

        let got = tools.arrayBufferToNumber(sdpE.SDPSessionID);
        let expected = tools.arrayBufferToNumber(conn.SDPSessionID);
        let comparison = (got > expected ? "an older" : "a newer");
        tools.logEvent("Won't process signaling entity. Invalid SDP Session ID. Expected: " + expected + ", Received " + comparison + " SDP session:" + got + " .", eLogEntryCategory.network, 1, eLogEntryType.warning);
      }
    }

    tools.logEvent('-----\n [SDP Entity]: ' + sdpE.description + "\n [Layer-0 Info]-: \n [Source]: " + sdpE.layer0Datagram.layer0DeliverySource + sdpE.layer0Datagram.description + "\n-----", eLogEntryCategory.network, 1, eLogEntryType.notification);

    var sdpType = sdpE.getType;


    switch (sdpType) {

      //Control  Support - begin
      case eSDPEntityType.control:
        switch (sdpE.getStatus) {
          case eSDPControlStatus.ok:
            break;
          case eSDPControlStatus.joined:

            this.joinConfirmed = true;
            break;
          case eSDPControlStatus.kickedOut:
            break;
          case eSDPControlStatus.error:
            break;
          case eSDPControlStatus.invalidIdentity:
            break;
          case eSDPControlStatus.banned:
            break;
          case eSDPControlStatus.nodeLimitsExceeded:
            break;
          case eSDPControlStatus.swarmClosing:
            break;
          case eSDPControlStatus.xToOtherFullNode:
            break;
          default:

        }
        break;
        //Control  Support - end
        //the following is requested by full-node.
        //in simple terms the full-node asks US to INITIATE a new WebRTC connection with a new peer
        //IMPORTANT: that way it's safer since we do NOT need to analyze the incomming SDP invitation. Rather, the receiving party looks at the proposed
        //eConnCapabilities (explicit enum) within SDPEntity and decided for itself (and makes the actual SDP offering).
        //thus we're the ones creating an 'offer'.
        //NOTE: full-node might decide to reconfigure entire Swarm, in which case the request would be generated as well (again)
        //let us prepare an 'SDP-Offer' (asked for by full-node (signaling server) when needed i.e. when a new peer is about to join the swarm)
      case eSDPEntityType.getOffer: //Step 2 at peer B  - (issued by Node S after JOINING received on its end from Node A)
        //check in active, as well, as pending connections
        //note that this message will be generated by full-node when connecting for the first time with a particular peer but might be the case
        //in which full-nodes switch or full-swarm-maintenance is needed



        //IMPORTANT: notice that a local SDP session ID, associated with local instance of a connection would be used.
        //Thus, the local SDP session identifier can be overriden only by an accepted incoming SDP Offer.
        //Therefore, the SDP session identifier is always shared between the two peers.

        //Decide if we accept the connection

        if (conn == null) {
          //create the connection.
          //Note: the getOffer might be delivered multiple times during connection's lifetime shall the other peer want to renegotiate the ALLOWED for capabilities with the local node.
          conn = this.createPendingConnWithPeer(sdpE.getSourceID); //will be created with swarm-specific default capabilities
        } else {

          //connection object already in place (might not be active).
          if (conn.getStatus == eSwarmConnectionState.active) {
            tools.logEvent("Warning: won't process a ⋮⋮⋮ Swarm get-offer request - a connection is already active.", eLogEntryCategory.network, 1, eLogEntryType.notification);
            return;
          }

          //Synchronization - BEGIN

          //SDP Session ID Support - BEGIN
          conn.genSDPSessionID();
          //SDP Session ID Support - END

          //On Signaling State - BEGIN
          if (conn.mRTCConnection.signalingState !== "stable") { //There is no ongoing exchange of offer / answer underway.
            tools.logEvent("Warning: won't generate an Offer for " + gTools.bytesToString(sdpE.getSourceID) + "@" + conn.getIPAddress + "- the  connection is not stable (pending offer/response). ", eLogEntryCategory.network, 1, eLogEntryType.warning);
            return;
          }
          //On Signaling State - END

          //On Connection State - BEGIN
          if (conn.mRTCConnection.connectionState === "connected") {
            tools.logEvent("Warning: won't process Offer-Request from " + gTools.bytesToString(sdpE.getSourceID) + "@" + (conn ? conn.getIPAddress : "unknown") + "- already connected with peer. ", eLogEntryCategory.network, 1, eLogEntryType.warning);
            return;
          } else if (conn.mRTCConnection.connectionState === "connecting") {
            tools.logEvent("Warning: won't process Offer-Request from " + gTools.bytesToString(sdpE.getSourceID) + "@" + (conn ? conn.getIPAddress : "unknown") + "- the connection is already being formed. ", eLogEntryCategory.network, 1, eLogEntryType.warning);
            return;
          } else if (conn.mRTCConnection.connectionState === "closed") {
            tools.logEvent("Warning: Creating new connection object for " + gTools.bytesToString(sdpE.getSourceID) + "@" + (conn ? conn.getIPAddress : "unknown") + "- previous ended. ", eLogEntryCategory.network, 1, eLogEntryType.warning);
            //this.removeConnection(sdpE.getSourceID);
            conn.close() //would dispatch notifications, detach events, close and remove.
            conn = this.createPendingConnWithPeer(sdpE.getSourceID);
          } //else if (conn.mRTCConnection.connectionState === "failed" || conn.mRTCConnection.connectionState === "disconnected" {
          //  tools.logEvent("Warning: issuing ICE restart for " + gTools.bytesToString(sdpE.getSourceID) + "@" + (conn ? conn.getIPAddress : "unknown") + "- on get offer request. ", eLogEntryCategory.network, 1, eLogEntryType.warning);
          //  conn.mRTCConnection.restartIce();//IMPORTANT: notice that this is accomplished throguh onICEConnectionStateChangeEvent() in CSwarmConnection whenever needed.
          //  }
          //On      //On Connection State - END
          //Synchronization - END

        }
        //conn = this.getConnection(sdpE.getSourceID); //needed! might had been just created above.

        if (!conn) {
          tools.logEvent("Warning: cannot form connection with " + gTools.bytesToString(sdpE.getSourceID) + "@" + (conn ? conn.getIPAddress : "unknown") + " at this time.", eLogEntryCategory.network, 1, eLogEntryType.warning);
          return;
        }

        tools.logEvent("Received WebRTC Offer Request from " + gTools.bytesToString(sdpE.getSourceID) + "@" + (conn ? conn.getIPAddress : "unknown") + ". ", eLogEntryCategory.network, 1, eLogEntryType.notification);

        if (conn == null)
          return false;

        if (CVMContext.getInstance().isPeerBlackListed(sdpE.getSourceID)) //should have happened at the network engine-level; do again anyway
        {
          tools.logEvent("[SECURITY]: won't process a Swarm offer request from " + gTools.bytesToString(sdpE.getSourceID) + "@" + conn.getIPAddress + ' - peer is blacklisted.', eLogEntryCategory.network, 1, eLogEntryType.warning);

          return;
        }


        //Now, allow only for an INTERSECTION of the two sets of capabilities (local and remote)
        allowFor = (sdpE.getCapabilities < this.getAllowedCapabilities) ? sdpE.getCapabilities : this.getAllowedCapabilities;
        //do NOT try to negotiate more than the joining/remote node allowed for. What the local node allows for is represented by this.getAllowedCapabilities (Swarm's parameter).
        //which is the default parameter nogotiated by local peer when IT wills to join a Swarm.

        conn.setAllowedCapabilities(allowFor, false); //Important: based on what allowed for by node A. Local node will have the right to add/disable
        //outgress video/audio anyway. IF audio and/or video not enabled then an attempt to add these WOULD NOT be made (not to trigger pointless renegotiation)
        conn.setStatus(eSwarmConnectionState.negotiating);
        //create a binary data-channel

        //  if(conn.getDataChannel == null)
        //{
        if (!conn.createDataChannel()) //it's ALWAYS there. (not negotiated, forced upon among all nodes due to its low cost).
          return false;
        //  }

        if (!conn.initEvents())
          return false;

        //let us query browser for the required media as per the connection requirements
        //Note: user might NOT agree.

        //sdp.getCapabilities indicates ceiling capabilities desired BY THE OTHER PEER.
        //Note that the other peer might not have these capabilities fully utilized at the current moment
        //but the node might deem these to be useful.
        //In a way it is better to negotiated more useful capabilities at the start and then to mute the channels as per the Effective Capabilities' indication
        //since that does not require renegotiation later on. By default nodes attempt to initially negotiate the triplet of audio/video/data.
        //The actual data-exchange (audio/video) can be controlled through setEffectiveIngressCapabilities and setEffectiveOutgressCapabilities
        //  if (sdpE.getCapabilities != eConnCapabilities.data && this.mAllowedCapabilities != eConnCapabilities.data)
        //Note, each Track is muted by default ANYWAY, UNTIL one or more streams are added to the receiver by the remote peer.
        //https://developer.mozilla.org/en-US/docs/Web/API/RTCPeerConnection/addTrack
        //  {

        //this.mSwarmManager.getAllowedMedia(this.getAllowedCapabilities) // query for all allowed media as per the above ALREADY negotiated ALLOWED for stream-types.
        //These might include ingress/outgress streams to be MUTED in a moment by the local node.

        //as indiciatd by Swarm's default parameters                                                  ^
        //we shall mute/unmute video/audio tracks before these are added to the stream so that the engine can assess data-rate but no actual data begins to flow
        //to the other peer.
        //add all tracks local user agrees upon. Impose effective limitations (mute)
        //NOTE: localStream MIGHT be null if only data-stream was negotiated
        //  this.setLocalStream(localStream); //swarm-api specific
        //IMPORTANT: impose user's limitation onto the outgress stream BEFORE the data-transmission begins
        //  if (this.mDummyVideoStream != null) {
        //  this.setLocalStreamEffectiveCapabilities(this.mSwarm.getEffectiveOutgressCapabilities); //SWARM's DEFAULTS

        //clean up any existing senders - BEGIN
        //IMPORTANT: this needs to be done as we may be dealing with a MIDM attack.
        //need to disable any active data streams and await authentication.
        let senders = conn.mRTCConnection.getSenders();
        senders.forEach((sender) => {
          conn.mRTCConnection.removeTrack(sender)
        });
        //clean up any existing senders - END
        conn.activateDummyTracks();

        if (!conn.isDummyStreamReady()) {
          tools.logEvent("Error: there was an attempt to initiate a ⋮⋮⋮ Swarm connection but Dummy Tracks are not ready. Aborting.", eLogEntryCategory.network, 1, eLogEntryType.error);

          return;
        }

        conn.getLocalDummyStream.getTracks().forEach( //recall: the Dummy black-screen dynamic video-stream is used for both negotiation AND
          // when a 'real' audio/video data stream (track) needs to be muted through one of the dummy Tracks.
          track => {
            try {
              if (conn.hasTrack(track) == false) {


                conn.mRTCConnection.addTrack(track, conn.getLocalDummyStream) //the stream parameter is used to assure synchronization of audio/video.
              }
              //that holds true for when tracks are replaced.
            } catch (e) {
              tools.logEvent("Error adding track to a connection.", eLogEntryCategory.network, 1, eLogEntryType.error);

            }
          }
        );

        //account for a case in which a track is already being streamed (ex. during connection's renegotiation!)


        conn.mMakingOffer = true;
        tools.logEvent('Offering SDP Session ID: ' + this.mTools.arrayBufferToNumber(conn.SDPSessionID), eLogEntryCategory.network, 1, eLogEntryType.notification);
        conn.mRTCConnection.setLocalDescription().then(function(returned) {
            this.onOfferReadyForDispatchEvent(this.getAllowedCapabilities);
          }.bind(conn))
          .catch(conn.mSwarm.handleGetUserMediaError).finally(function() {
            conn.mMakingOffer = false;
          }.bind(conn));

        break;

      case eSDPEntityType.processOfferResponse: //Step 4 at Node A (AFTER processOffer)

        tools.logEvent("Received WebRTC Offer Response from " + gTools.bytesToString(sdpE.getSourceID) + "@" + (conn ? conn.getIPAddress : "unknown") + ". ", eLogEntryCategory.network, 1, eLogEntryType.notification);

        //find which pending connection the SDP-response targets
        if (conn != null) {
          let iceStr = gTools.arrayBufferToString(sdpE.getSDPData);
          let desc = JSON.parse(iceStr);
          if (desc == null)
            return null;

          //Synchronization - BEGIN

          //On Connection State - BEGIN
          if (conn.mRTCConnection.connectionState === "connected") {
            tools.logEvent("Warning: won't process Offer-Response, connection is already established. ", eLogEntryCategory.network, 1, eLogEntryType.warning);

            return;
          }
          //On Connection State - END

          //On Signaling State - BEGIN

          if (conn.mRTCConnection.signalingState === "stable") { //There is no ongoing exchange of offer and answer underway.
            tools.logEvent("Warning: won't process an Offer-Response from " + gTools.bytesToString(sdpE.getSourceID) + "@" + conn.getIPAddress + "- the  connection is stable (not awaiting an offer-response). ", eLogEntryCategory.network, 1, eLogEntryType.warning);
            return;
          }

          if (conn.mRTCConnection.signalingState !== "have-local-offer") {
            tools.logEvent("Warning: won't process Offer-Response, no offer was made. ", eLogEntryCategory.network, 1, eLogEntryType.warning);

            return;
          }
          //On Signaling State - END

          //Synchronization - END

          conn.getRTCConnection.setRemoteDescription(desc);
          return true;
        } else {
          return false; //unknown
        }
        break;

        break;

      case eSDPEntityType.joining:
        //full-node is more concorned with this type of a msg
        //note that the 'joining' message is issued each time peers needs to reconfigure connection
        //that would cause re-configuration with all the swarm-members
        break;
      case eSDPEntityType.bye:
        //indication that particular IS ABOUT to leave the swarm


        break;

      case eSDPEntityType.processICE: //ongoing,i.e. - an indefinite process.
        tools.logEvent("Received ICE from " + gTools.bytesToString(sdpE.getSourceID) + "@" + (conn ? conn.getIPAddress : "unknown") + ".", eLogEntryCategory.network, 1, eLogEntryType.notification);


        if (conn == null) {
          tools.logEvent("Won't process ICE from " + gTools.bytesToString(sdpE.getSourceID) + "@" + (conn ? conn.getIPAddress : "unknown") + " no connection to process.", eLogEntryCategory.network, 1, eLogEntryType.notification);
          return false; //should not happen
        }


        //Synchronization - BEGIN

        //On Signaling State - BEGIN

        if (conn.mRTCConnection.signalingState !== "stable") { //There is no ongoing exchange of offer and answer underway.
          tools.logEvent("Warning: won't process an ICE candidate  from " + gTools.bytesToString(sdpE.getSourceID) + "@" + conn.getIPAddress + "- the  connection is not stable (pending offer/response). ", eLogEntryCategory.network, 1, eLogEntryType.warning);
          return;
        }
        //On Signaling State - END

        //On ICE gathering state - BEGIN
        if (conn.mRTCConnection.iceGatheringState === "completed") {
          tools.logEvent("Warning: won't process an ICE candidate  from " + gTools.bytesToString(sdpE.getSourceID) + "@" + conn.getIPAddress + "- ICEs already collected. ", eLogEntryCategory.network, 1, eLogEntryType.warning);
          return;
        } //else if (conn.mRTCConnection.iceGatheringState !== "gathering") {
        //tools.logEvent("Warning: won't process an ICE candidate  from " + gTools.bytesToString(sdpE.getSourceID) + "@" + conn.getIPAddress + "- not in ICE gathering state. ", eLogEntryCategory.network, 1, eLogEntryType.warning);
        //  return;
        //  }
        else if (conn.mRTCConnection.iceGatheringState === "closed") {
          tools.logEvent("Warning: won't process an ICE candidate  from " + gTools.bytesToString(sdpE.getSourceID) + "@" + conn.getIPAddress + "- ICE sub-system is closed. ", eLogEntryCategory.network, 1, eLogEntryType.warning);
          return;
        }
        //On ICE gathering state - END

        if (!conn.mRTCConnection.remoteDescription) {
          tools.logEvent("Warning: won't process an ICE candidate  from " + gTools.bytesToString(sdpE.getSourceID) + "@" + conn.getIPAddress + "- remote description has not been delivered yet. ", eLogEntryCategory.network, 1, eLogEntryType.warning);
          return;
        }
        //Synchronization - END
        let iceStr = gTools.arrayBufferToString(sdpE.getSDPData); //JSON encoded

        let obj = JSON.parse(iceStr);
        let candidate = null;
        try {
          candidate = new RTCIceCandidate(obj); //ICE candidate encapsulated within the standard SDP field
          //retrieve the IP address, set it within the CSwarmConnection.
          conn.setIPAddress = candidate.address; // 2nd part needed to compute the ⋮⋮⋮ Secure Connection ID.
        } catch (error) {
          tools.logEvent('invalid ICE received:' + iceStr, eLogEntryCategory.network, 1, eLogEntryType.error);
          return false;
        }
        if (candidate == null) {
          tools.logEvent('invalid ICE received:' + iceStr, eLogEntryCategory.network, 1, eLogEntryType.error);
          return false;
        }

        conn.onRemoteICECandidateEvent(candidate);
        //  let connection = this.getConnection(sdpE.getSourceID); //check in active as well as pending connections


        break;

        //processing of WebRTC connection-offer
        //issued by peer after it was requested to do so by the signaling server(full-node)
        //now we process the generated offer.
        //Note that offers are generated on peer-to-peer basis (literally), thus the particular offer was generated *specifically* for US only.
        //authentication had been performed by a full-node (based either on a TransmissionToken or ECC signature)
      case eSDPEntityType.processOffer: //at node A STEP 3 (after getOffer and before processOfferResponse)
        conn = this.getConnection(sdpE.getSourceID); //check in active as well as pending connections
        //check if we've got a pending connection (negotiated) with peer if so process, otherwise establish a new one
        //create a pending connection - BEGIN
        if (conn == null) {
          //Security UPDATE: Do create a connection with  a peer the full-node proposes.

          conn = this.createPendingConnWithPeer(sdpE.getSourceID); //will be created with swarm-specific default capabilities.

        }
        //  conn = this.getConnection(sdpE.getSourceID); //check in active as well as pending connections.
        tools.logEvent("Received a WebRTC Offer from " + gTools.bytesToString(sdpE.getSourceID) + "@" + (conn ? conn.getIPAddress : "unknown") + ". ", eLogEntryCategory.network, 1, eLogEntryType.notification);


        if (conn == null)
          return false; //limits reached
        //create a pending connection - END

        //Extremely important: in order to ascertain Perfect Negotiation, processing of processOffer datagrams is most sophisticated.
        /*
        We employ the convention of a polite and impolite nodes. The polite node accepts and incoming offer even though it might had issued one itself.
        An inpolite node, on the other hand, - it would REJECT an incoming offer had it issued one itself. That is to mitigate deadlocks at the initial stage.
        To decide which node is impolite, peers' identifiers are used (IDs converted into numbers and compares against each other).

        Should anything go sideways- we've got a fallback timeout of 45 seconds after which,
        any connection whose SDP signaling state has not changed for this time-perios - it gets removed.
        */
        let offerCollision = (conn.mMakingOffer || conn.mRTCConnection.signalingState !== "stable");

        if (!conn.isPolite && offerCollision) {
          tools.logEvent('Ignored a WebRTC offer, waiting for a response ourselves already..', eLogEntryCategory.network, 1, eLogEntryType.notification);
          return;
        }

        conn.SDPSessionID = sdpE.SDPSessionID;

        tools.logEvent('Accepting SDP Session ID: ' + this.mTools.arrayBufferToNumber(conn.SDPSessionID), eLogEntryCategory.network, 1, eLogEntryType.notification);


        //Capabilities negotiation - BEGIN
        conn.setStatus(eSwarmConnectionState.negotiating); //notify external code about negotiation
        //processOffer is processed by the node who initially initiated the join-process
        //we LIMIT the allowed capabilities as per what was allowed for by the other Peer. Thus the result is a common part of two sets.
        //the other peer SHOULD NOT have offered more than we initially requested for. Abort connection otherwise.
        //i.e. The other peer should have done:  let allowFor = (sdpE.getCapabilities < this.getAllowedCapabilities) ? sdpE.getCapabilities : this.getAllowedCapabilities;

        //note that the ALLOWED FOR capabilities do NOT affect the current effective outbound data (cam/mic); only the possibilities.

        //still it MIGHT happen that the remote peer wants to offer LESS thant the local node, thus:
        allowFor = (sdpE.getCapabilities < this.getAllowedCapabilities) ? sdpE.getCapabilities : this.getAllowedCapabilities;
        conn.setAllowedCapabilities(allowFor, false);

        //  conn.setDataChannel = rtcConn.createDataChannel('data', {
        //    reliable: true //an ICE candidate will be generated asynchronously for the initiating peer (specifically)
        //the candidate shall be delivered to it
        //  });
        //Capabilities negotiation - END

        conn = this.getConnection(sdpE.getSourceID); //might be already active OR pending
        if (conn == null)
          return false; //limits reached?

        let offer = gTools.arrayBufferToString(sdpE.getSDPData); //JSON serialized object
        offer = JSON.parse(offer);

        let offerDesc = new RTCSessionDescription(offer);
        if (!conn.isDummyStreamReady()) {
          tools.logEvent("Error: there was an attempt to process an incomming ⋮⋮⋮ Swarm connection but local Dummy Tracks are not ready. Aborting.", eLogEntryCategory.network, 1, eLogEntryType.error);

          return;
        }
        conn.getRTCConnection.setRemoteDescription(offerDesc).then(function() {
            //  this.setLocalStream(stream);
            //provide only Tracks (audio/video) which user enabled  despite what had been requested by the other peer
            //later sample usage document.getElementById("local_video").srcObject = this.mLocalStream;

            //IMPORTANT: impose user's limitation onto the outgress stream BEFORE the data-transmission begins

            //Now,set effective ingress capabilities as per the Swarm's ceiling capabilities.
            //user might be then able to mute/unmute particular peers from within dApp's UI.
            //for now we allow for delivery as per the maxim allowe for capabilities.
            //this might be minimized to save on bandwidth udage.
            //this.setEffectiveCapabilities(this.mSwarm.getAllowedCapabilities, false);

            //clean up any existing senders - BEGIN
            let senders = conn.mRTCConnection.getSenders();
            senders.forEach((sender) => {
              conn.mRTCConnection.removeTrack(sender)
            });
            //clean up any existing senders - END

            this.activateDummyTracks();
            this.getLocalDummyStream.getTracks().forEach(
              track => {
                if (this.hasTrack(track) == false) {
                  this.mRTCConnection.addTrack(track, this.getLocalDummyStream)
                }
              }
            );

          }.bind(conn))
          .then(function() {
            return this.mRTCConnection.setLocalDescription();
          }.bind(conn))
          .then(function() {
            this.onOfferAnswerReadyForDispatchEvent();
            this.setEffectiveCapabilities(this.mSwarm.getEffectiveOutgressCapabilities, true); //SWARM's DEFAULTS
          }.bind(conn))
          .catch(this.handleGetUserMediaError.bind(this));

        break;
    }

    return false;
  }

  removeConnection(peerID) {


    if (peerID == null || peerID.byteLength == 0)
      return false;

    let index = -1;

    for (let i = 0; i < this.mPendingConnections.length; i++) {
      if (gTools.compareByteVectors(this.mPendingConnections[i].getPeerID, peerID)) {
        index = i;
        break;
      }
    }

    if (index > -1) {
      this.mTools.logEvent("Removing pending connection with " + gTools.bytesToString(peerID) + ". ", eLogEntryCategory.network, 1, eLogEntryType.warning);
      this.mPendingConnections.splice(index, 1);
    }

    index = -1;
    for (let i = 0; i < this.mActiveConnections.length; i++) {
      if (gTools.compareByteVectors(this.mActiveConnections[i].getPeerID, peerID)) {
        index = i;
        break;
      }
    }

    if (index > -1) {
      this.mTools.logEvent("Removing active connection with " + gTools.bytesToString(peerID) + ". ", eLogEntryCategory.network, 1, eLogEntryType.warning);
      this.mActiveConnections.splice(index, 1);
    }



    if (index > -1)
      return true;

    return false;
  }

  getActiveConnection(peerID) {
    if (peerID == null || peerID.byteLength == 0)
      return null;

    for (let i = 0; i < this.mActiveConnections.length; i++) {
      if (gTools.compareByteVectors(peerID, this.mActiveConnections[i].getPeerID))
        return this.mActiveConnections[i];

    }
    return null;
  }

  getConnection(peerID) {
    let connection = null;
    connection = this.getActiveConnection(peerID);
    if (connection != null)
      return connection;
    connection = this.getPendingConnection(peerID);
    return connection;
  }

  getPendingConnection(peerID) {
    if (peerID == null || peerID.byteLength == 0)
      return null;

    for (let i = 0; i < this.mPendingConnections.length; i++) {
      if (gTools.compareByteVectors(peerID, this.mPendingConnections[i].getPeerID))
        return this.mPendingConnections[i];

    }
    return null;
  }


  //https://stackoverflow.com/questions/6951727/setinterval-not-working-properly-on-chrome
  //thus, we avoid setInterval()
  controllerThreadF() {
    //operational logic - BEGIN
    this.maintenance();
    //operational logic - END
  }

  //  The controller will attempt to prelong communication with full-node as long as the client indicates its will to participate.
  // NOTE: client UI dApp SHOULD call Close(), as soon as, it does not need to participate within the Swarm anymore. In such a case
  // connection up-holding attempts would cease. The final decision is up to the full-node.

  maintenance() {
    let timeMS = gTools.getTime(true);
    let time = (timeMS / 1000);


    //Peers' Keep Alive Support - BEGIN
    if ((timeMS - this.mLastOutgressPeersPingMS) > this.mPeersPingIntervalMS) {
      this.mLastOutgressPeersPingMS = timeMS;
      this.ping();
    }

    let pingMS = 0;
    //Connection's State Assessment  - high-level - BEGIN
    for (let i = 0; i < this.mActiveConnections.length; i++) {
      pingMS = (timeMS - this.mActiveConnections[i].lastSeen);

      //Peer Status Support - BEGIN
      if (pingMS > this.mPeerReachableTimeoutMS) {
        this.mActiveConnections[i].peerStatus = eSwarmPeerStatus.notReachable; // that would dispatch notifications.
      } else {
        this.mActiveConnections[i].peerStatus = eSwarmPeerStatus.active;
      }
      //Peer Status Support - END


      //Connections' Quality Assessment Support - BEGIN
      if (pingMS > this.mConnQualityLowThreshold) {
        this.mActiveConnections[i].connectionQuality = eConnQuality.none; // that would dispatch notifications.
      } else if (pingMS < this.mConnQualityMaxThreshold) { //i.e. for maximum quality.
        this.mActiveConnections[i].connectionQuality = eConnQuality.max; // that would dispatch notifications.
      } else if (pingMS < this.mConnQualityHighThreshold) {
        this.mActiveConnections[i].connectionQuality = eConnQuality.high; // that would dispatch notifications.
      } else if (pingMS < this.mConnQualityMediumThreshold) {
        this.mActiveConnections[i].connectionQuality = eConnQuality.medium; // that would dispatch notifications.
      } else if (pingMS < this.mConnQualityLowThreshold) { //i.e. sifficeint for minumum quality.
        this.mActiveConnections[i].connectionQuality = eConnQuality.low; // that would dispatch notifications.
      }
      //Connections' Quality Assessment Support - END

    }
    //Connection's State Assessment  - high-level - END

    //Peers' Keep Alive Support - END

    //Signaling Peer (full-node) Keep Alive Support - BEGIN
    if ((time - this.mLastFNPingTimestamp) > this.mFNPingInterval) {
      this.mSwarmManager.pingFullNode(this.getID);
      this.mLastFNPingTimestamp = time;
    }
    //Signaling Peer (full-node) Keep Alive Support - END

    let connectedToFullNode = (CVMContext.getInstance().getConnectionState == eConnectionState.connected);

    if (connectedToFullNode) {
      //Re-attach - BEGIN

      //Make sure peers know about the local node - BEGIN
      if (this.joinAttemptTimestamp != 0 && this.joinConfirmed && this.mActiveConnections == 0 && (time - this.joinAttemptTimestamp) > 30) {
        console.log('Swarm "' + gTools.bytesToString(this.trueID) + '" has no peers, re-registering at ⋮⋮⋮ Node now.. ');
        this.reconnect();
        this.joinAttemptTimestamp = time;
      }
      //Make sure peers know about the local node - END

      //Make sure full-node registered local node within a Swarm table -  BEGIN
      if (this.joinAttemptTimestamp != 0 && !this.joinConfirmed && (time - this.joinAttemptTimestamp) > 10) {
        console.log('Re-submitting a registration request for a ⋮⋮⋮ Swarm "' + gTools.bytesToString(this.trueID) + '"');
        this.reconnect();
        this.joinAttemptTimestamp = time;
      }
      //Make sure full-node registered local node within a Swarm table -  END

      //Re-attach - END
    }

    //attempt to authenticate peers - BEGIN
    if (this.isPrivate && (time - this.mLastGlobalAuthAttempt) > 30) {

      let connectionsInNeedOfAuth = [];

      for (let i = 0; i < this.mActiveConnections.length; i++) {
        if (!this.mActiveConnections[i].isAuthenticated) {
          connectionsInNeedOfAuth.push(this.mActiveConnections[i]);
          this.mActiveConnections[i].requestAuthentication();
        }
      }
      if (connectionsInNeedOfAuth.length) {

        if (connectionsInNeedOfAuth.length > 1) {
          console.log(' [- Swarm "' + gTools.arrayBufferToString(this.trueID) + '"]: ' + connectionsInNeedOfAuth.length +
            " peers need to be authenticated..");
        } else {
          console.log(' [- Swarm "' + gTools.arrayBufferToString(this.trueID) + '"]: ' + connectionsInNeedOfAuth.length +
            " peer needs to be authenticated..");
        }

        for (let i = 0; i < connectionsInNeedOfAuth.length; i++) {
          connectionsInNeedOfAuth[i].requestAuthentication();
        }
      }


      this.mLastGlobalAuthAttempt = time;
    }

    //attempt to authenticate peers - END
  }

  unregisterEventListenersByAppID(appID = 0, eventListener = null) {
    let result = false;

    if (appID == 0 && eventListener == null)
      return result;

    let eventQueues = [
      this.mSwarmMessageEventListeners,
      this.mMessageEventListeners,
      this.mTrackEventListeners,
      this.mPeerAuthenticationResultListeners,
      this.mDataChannelMessageEventListeners,
      this.mSwarmConnectionStateChangeEventListeners,
      this.mConnectionStateChangeEventListeners,
      this.mICEConnectionStateChangeEventListeners,
      this.mDataChannelStateChangeEventListeners,
      this.mSwarmStateChangeEventListeners,
      this.mSwarmAuthRequirementsChangedListeners,
      this.mSwarmRegistrationConfirmedListeners,
      this.mPeerStatusEventListeners,
      this.mConnectionQualityEventListeners
    ];

    for (let a = 0; a < eventQueues.length; a++) {
      for (let i = 0; i < eventQueues[a].length; i++) {
        if (eventListener == null) {
          if (eventQueues[a][i].appID == appID) {
            eventQueues[a].splice(i, 1);
            result = true;

          }
        } else {
          if (eventQueues[a][i].handler == eventListener) {
            eventQueues[a].splice(i, 1);
            result = true;
          }
        }
      }
    }
    return result;
  }


  getSwarmSize() {
    return this.mActiveConnections.length;
  }

  get getMyID() {
    return this.mMyID;
  }
  reconnect() {
    console.log('Swarm "' + gTools.bytesToString(this.trueID) + '" attempting to reconnect.. ');
    this.mSwarmManager.joinSwarm(this.trueID, this.mMyID);
  }

  get peers() {
    return this.mActiveConnections;
  }
  get getID() {
    return this.mID;
  }
  initialize() {

    //https://stackoverflow.com/questions/6951727/setinterval-not-working-properly-on-chrome

    //bottom line: we use setTimeout instead of setInterval
    if (this.mControllerThread == 0)
      this.mControllerThread = CVMContext.getInstance().createJSThread(this.controllerThreadF.bind(this), 0, this.mControllerThreadInterval, true, true);
    //  this.mControllerThread = setTimeout(this.controllerThreadF.bind(this), this.mControllerThreadInterval);
    this.canBeOperational = true;
    this.mState = eSwarmState.idle;
  }

  close(killConnections = true) {
    try {
      this.canBeOperational = false;
      //shut down thread
      if (this.mControllerThread > 0)
        CVMContext.getInstance().stopJSThread(this.mControllerThread);
      this.mControllerThread = 0;

      if (killConnections) {
        //kill connections
        if (this.mActiveConnections.length) {
          CTools.getInstance().logEvent("Killing " + this.mActiveConnections.length + " connections associated with Swarm '" +
            this.mTools.arrayBufferToString(this.trueID) + ".", eLogEntryCategory.network, 1, eLogEntryType.notification);

          for (let i = 0; i < this.mActiveConnections.length; i++)
            this.mActiveConnections[i].close();
          for (let i = 0; i < this.mPendingConnections.length; i++)
            this.mPendingConnections[i].close();
        } else {
          CTools.getInstance().logEvent("Swarm '" + this.mTools.arrayBufferToString(this.trueID) + "' has no active connections to kill.", eLogEntryCategory.network, 1, eLogEntryType.notification);
        }
      }
    } finally {
      this.setState = eSwarmState.idle; //i.e. not connected.
    }
  }

  //External Events Support - BEGIN
  //Swarm-Events Subscribers - BEGIN
  addSwarmStateChangeEventListener(eventListener, appID = 0) {
    this.mSwarmStateChangeEventListeners.push({
      handler: eventListener,
      appID
    });
  }
  addSwarmRegistrationConfirmedEventListener(eventListener, appID = 0) {
    this.mSwarmRegistrationConfirmedListeners.push({
      handler: eventListener,
      appID
    });
  }
  addSwarmAuthRequirementsChangedEventListener(eventListener, appID = 0) {
    this.mSwarmAuthRequirementsChangedListeners.push({
      handler: eventListener,
      appID
    });
  }
  //Swarm-Events Subscribers- END

  //aggregated-subscribers - BEGIN
  //The below methods allow to subscribe for events related to a single or all (peerID == null) connected peers.
  //The peers constantly change thus the prior version would require client UI dAPP to resubscribe for events related to newly detected peers.
  //Instead, we refactor the code for the individual Swarm connections to trigger the 'master-event-listeners' within the CSwarm.
  //It is still perfectly valid to subscribe for individual Swarm connection's events though!

  addTrackEventListener(eventListener = null, appID = 0, peerID = null) {
    let subscribed = false;
    if (eventListener == null)
      return false;

    for (let i = 0; i < this.mActiveConnections.length; i++) {
      if (peerID != null && gTools.compareByteVectors(peerID, this.mActiveConnections[i].getPeerID)) {
        this.mActiveConnections[i].addTrackEventListener(eventListener, appID);
        subscribed = true;
      }

    }

    if (subscribed)
      return true;

    if (!subscribed && peerID == null) {
      this.mTrackEventListeners.push({
        handler: eventListener,
        appID
      });
      subscribed = true;
    }
    /*Prior version would do the following:
    for (let i = 0; i < this.mPendingConnections.length; i++) {
      if (peerID != null && gTools.compareByteVectors(peerID, this.mPendingConnections[i].getPeerID)) {
        this.mPendingConnections[i].addTrackEventListener(eventListener, appID);
        subscribed = true;
      }
    }
    NEW: instead, if no particular peer/connection was chosen, we subscribe caller for  the master-event.
    */
    return subscribed;

  }
  //API level 2/2
  addSwarmMessageListener(eventListener, appID = 0, peerID = null) {

    let subscribed = false;
    if (eventListener == null)
      return false;

    for (let i = 0; i < this.mActiveConnections.length; i++) {
      if (peerID != null && gTools.compareByteVectors(peerID, this.mActiveConnections[i].getPeerID)) {
        this.mActiveConnections[i].addSwarmMessageListener(eventListener, appID);
        subscribed = true;
      }

    }

    if (subscribed)
      return true;

    if (!subscribed && peerID == null) {
      this.mSwarmMessageEventListeners.push({
        handler: eventListener,
        appID
      });
      subscribed = true;
    }
    return subscribed;

  }


  ////API level 1/2
  addMessageListener(eventListener, appID = 0, peerID = null) {

    let subscribed = false;
    if (eventListener == null)
      return false;

    for (let i = 0; i < this.mActiveConnections.length; i++) {
      if (peerID != null && gTools.compareByteVectors(peerID, this.mActiveConnections[i].getPeerID)) {
        this.mActiveConnections[i].addSwarmMessageListener(eventListener, appID);
        subscribed = true;
      }

    }

    if (subscribed)
      return true;

    if (!subscribed && peerID == null) {
      this.mMessageEventListeners.push({
        handler: eventListener,
        appID
      });
      subscribed = true;
    }
    return subscribed;

  }
  ////API level 0/2
  addDataChannelMessageEventListener(eventListener, appID = 0, peerID = null) {

    let subscribed = false;
    if (eventListener == null)
      return false;

    for (let i = 0; i < this.mActiveConnections.length; i++) {
      if (peerID != null && gTools.compareByteVectors(peerID, this.mActiveConnections[i].getPeerID)) {
        this.mActiveConnections[i].addDataChannelMessageEventListener(eventListener, appID);
        subscribed = true;
      }

    }

    if (subscribed)
      return true;

    if (!subscribed && peerID == null) {
      this.mDataChannelMessageEventListeners.push({
        handler: eventListener,
        appID
      });
      subscribed = true;
    }
    return subscribed;

  }

  addPeerStatusChangeEventListener(eventListener, appID = 0, peerID = null) {

    let subscribed = false;
    if (eventListener == null)
      return false;

    for (let i = 0; i < this.mActiveConnections.length; i++) {
      if (peerID != null && gTools.compareByteVectors(peerID, this.mActiveConnections[i].getPeerID)) {
        this.mActiveConnections[i].addPeerStatusChangeEventListener(eventListener, appID);
        subscribed = true;
      }

    }

    if (subscribed)
      return true;

    if (!subscribed && peerID == null) {
      this.mPeerStatusEventListeners.push({
        handler: eventListener,
        appID
      });
      subscribed = true;
    }
    return subscribed;

  }

  addConnectionQualityChangeEventListener(eventListener, appID = 0, peerID = null) {

    let subscribed = false;
    if (eventListener == null)
      return false;

    for (let i = 0; i < this.mActiveConnections.length; i++) {
      if (peerID != null && gTools.compareByteVectors(peerID, this.mActiveConnections[i].getPeerID)) {
        this.mActiveConnections[i].addConnectionQualityChangeEventListener(eventListener, appID);
        subscribed = true;
      }

    }

    if (subscribed)
      return true;

    if (!subscribed && peerID == null) {
      this.mConnectionQualityEventListeners.push({
        handler: eventListener,
        appID
      });
      subscribed = true;
    }
    return subscribed;

  }
  //Note: If peer-id not provided then subscribed to each and every Swarm-channel
  addSwarmConnectionStateChangeEventListener(eventListener = null, appID = 0, peerID = null) {
    let subscribed = false;
    if (eventListener == null)
      return false;

    for (let i = 0; i < this.mActiveConnections.length; i++) {
      if (peerID != null && gTools.compareByteVectors(peerID, this.mActiveConnections[i].getPeerID)) {
        this.mActiveConnections[i].addSwarmConnectionStateChangeEventListener(eventListener, appID);
        subscribed = true;
      }

    }

    if (subscribed)
      return true;

    if (!subscribed && peerID == null) {
      this.mSwarmConnectionStateChangeEventListeners.push({
        handler: eventListener,
        appID
      });
      subscribed = true;
    }
    return subscribed;

  }
  addConnectionStateChangeEventListener(eventListener = null, appID = 0, peerID = null) {
    let subscribed = false;
    if (eventListener == null)
      return false;

    for (let i = 0; i < this.mActiveConnections.length; i++) {
      if (peerID != null && gTools.compareByteVectors(peerID, this.mActiveConnections[i].getPeerID)) {
        this.mActiveConnections[i].addConnectionStateChangeEventListener(eventListener, appID);
        subscribed = true;
      }

    }

    if (!subscribed && peerID == null) {
      this.mConnectionStateChangeEventListeners.push({
        handler: eventListener,
        appID
      });
      subscribed = true;
    }
    return subscribed;

  }

  addICEConnectionStateChangeEventListener(eventListener = null, appID = 0, peerID = null) {
    let subscribed = false;
    if (eventListener == null)
      return false;

    for (let i = 0; i < this.mActiveConnections.length; i++) {
      if (peerID != null && gTools.compareByteVectors(peerID, this.mActiveConnections[i].getPeerID)) {
        this.mActiveConnections[i].addICEConnectionStateChangeEventListener(eventListener, appID);
        subscribed = true;
      }

    }

    if (!subscribed && peerID == null) {
      this.mICEConnectionStateChangeEventListeners.push({
        handler: eventListener,
        appID
      });
      subscribed = true;
    }
    return subscribed;

  }

  addDataChannelStateChangeEventListener(eventListener = null, appID = 0, peerID = null) {
    let subscribed = false;
    if (eventListener == null)
      return false;

    for (let i = 0; i < this.mActiveConnections.length; i++) {
      if (peerID != null && gTools.compareByteVectors(peerID, this.mActiveConnections[i].getPeerID)) {
        this.mActiveConnections[i].addDataChannelStateChangeEventListener(eventListener, appID);
        subscribed = true;
      }

    }

    if (!subscribed && peerID == null) {
      this.mDataChannelStateChangeEventListeners.push({
        handler: eventListener,
        appID
      });
      subscribed = true;
    }
    return subscribed;

  }
  //aggregated-subscribers - BEGIN

  //aggregated-core-event handlers - BEGIN
  getTrackEventListeners() {
    return this.mTrackEventListeners;
  }

  getPeerAuthEventListeners() {
    return this.mPeerAuthenticationResultListeners;
  }
  getSwarmConnectionNewMessageListeners() {
    return this.mMessageEventListeners;
  }
  getSwarmConnectionNewSwarmMessageListeners() {
    return this.mSwarmMessageEventListeners;
  }


  getSwarmConnectionStateChangeEventListeners() {
    return this.mSwarmConnectionStateChangeEventListeners;
  }
  getConnectionStateChangeEventListeners() {
    return this.mConnectionStateChangeEventListeners
  }
  getICEConnectionStateChangeEventListeners() {
    return this.mICEConnectionStateChangeEventListeners;
  }

  getDataChannelStateChangeEventListeners() {
    return this.mDataChannelStateChangeEventListeners;
  }

  //aggregated-core-event handlers - END

  //External Events Support - END
}
