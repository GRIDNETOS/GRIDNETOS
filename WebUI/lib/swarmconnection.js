'use strict';
import {
  CVirtualCamDev,
  CVirtualAudioDev
} from './SwarmManager.js'

import {
  CNetMsg
} from '/lib/NetMsg.js'

import {
  CDataConcatenator
} from '/lib/tools.js'

import {
  CSwarmMsg
} from '/lib/swarmmsg.js'
import {
  CSDPEntity,
} from './SDPEntity.js'

import {
  CWindow
} from '/lib/window.js'


import {
  CSwarm
} from '/lib/swarm.js'

const isZKPAuthMask = 0b00000001;
const isDedicatedPSKMask = 0b00000001; //otherwise based on image(True Swarm ID | IV time-based),

export class CSwarmAuthData {

  constructor() {
    this.mData = new ArrayBuffer();
    this.mFlags = new ArrayBuffer(1);
    this.mVersion = 1;
  }

  //Flags - BEGIN
  set isDedicatedPSK(isIt = true) {
    let view = new Uint8Array(this.mFlags);
    if (isIt)
      view[0] |= isDedicatedPSKMask;
    else {
      view[0] &= ~isDedicatedPSKMask;
    }
    this.mFlags = view.buffer;
  }

  get isDedicatedPSK() { //otherwise based on image(True Swarm ID | IV time-based),
    let view = new Uint8Array(this.mFlags);
    return (view[0] & isDedicatedPSKMask) != 0;
  }

  set isZKP(isIt = true) {
    let view = new Uint8Array(this.mFlags);
    if (isIt)
      view[0] |= isZKPAuthMask;
    else {
      view[0] &= ~isZKPAuthMask;
    }
    this.mFlags = view.buffer;
  }

  get isZKP() {
    let view = new Uint8Array(this.mFlags);
    return (view[0] & isZKPAuthMask) != 0;
  }

  //Flags - END

  //(de)- serialization - Begin
  getPackedData() {
    var sequence = new asn1js.Sequence({
      value: [
        new asn1js.Integer({
          value: this.mVersion
        }), //Version

        new asn1js.Sequence({
          value: [
            new asn1js.OctetString({
              valueHex: this.mFlags
            }), //mFlags
            new asn1js.OctetString({
              valueHex: this.mData
            }) //mFlags
          ]
        })
      ]
    });

    let bytes = sequence.toBER(false);
    let length = bytes.byteLength;
    return bytes;
  }
  //takes ArrayBuffer
  static instantiate(data) {
    if (data.byteLength == 0)
      return null;

    let toRet = new CSwarmAuthData();

    let decoded_asn1 = asn1js.fromBER(data);
    if (decoded_asn1.offset === (-1))
      return null; // Error during decoding

    let decoded_sequence1 = decoded_asn1.result;
    if (decoded_sequence1.valueBlock.value == null) {
      console.log("Invalid Swarm-Auth-Data received.");
      return null;
    }

    if (decoded_sequence1.valueBlock.value.length < 2) {
      console.log("Invalid Swarm-Auth-Data received.");
      return null;
    }
    toRet.mVersion = decoded_sequence1.valueBlock.value[0].valueBlock.valueDec;

    if (toRet.mVersion == 1) {
      let decoded_sequence2 = decoded_sequence1.valueBlock.value[1];

      if (decoded_sequence2.valueBlock.length < 2) {
        console.log("Invalid Swarm-Auth-Data received.");
        return null;
      }

      toRet.mFlags = decoded_sequence2.valueBlock.value[0].valueBlock.valueHex;
      toRet.mData = decoded_sequence2.valueBlock.value[1].valueBlock.valueHex;
    }

    return toRet;
  }
  //(de)- serialization - END

  get data() {
    return this.mData;
  }

  set data(bytes) {
    this.mData = bytes;
  }

}


export class CSwarmConnection { //Swarm Connection - BEGIN
  constructor(swarm, rtcPeerConnection, peerID = new ArrayBuffer(), capabilities = eConnCapabilities.audioVideo) {
    this.mRTCConnection = rtcPeerConnection;
    this.mSwarm = swarm;

    this.mVideoActive = false;
    this.mNotifyAboutMediaOnAuth = true;
    this.mAudioActive = false;
    this.mSSActive = false;
    this.mTools = CTools.getInstance();
    this.mSDPSessionID = this.mTools.numberToArrayBuffer(this.mTools.getTime());// can be overriden ONLY by an accepted offer.
    //^thus SDP session ID is always shared between the two endpoints.
    this.mSignalingStateChangeTimestamp = this.mTools.getTime();
    this.mPeerStatus = eSwarmPeerStatus.notReachable;
    this.mConnectionQuality = eConnQuality.none;
    this.mICENegotiationReqCounter = 0;
    this.mLastKeepAliveReceivedMS = 0;
    this.mIsPolite = false;
    this.mLastPeerStatusChangeMS = 0;
    //Zero Knowledge Proofs support - BEGIN
    //Constants - BEGIN
    this.mClockDrift = 2000;
    this.mZKPTimer1ExpMS = 10000; // the time span after which validator CAN propose mZKPNonce1Local; a candidate MUST reject IV proposed any time sooner.
    this.mZKPTimer2ExpMS = 3000; // the time in which validator expects the ZKP to be delivered in.
    //Constants - END
    this.mZKPTimer1StartMS = 0;
    this.mZKPTimer2LocalStartMS = 0;
    this.mZKPNonce2Local = new ArrayBuffer(); //IV2 provided by a local candidate.
    this.mZKPNonce2Remote = new ArrayBuffer(); //IV2 provided by a (remote) candidate
    this.mZKPNonce1Remote = new ArrayBuffer();
    this.mZKPNonce1Local = new ArrayBuffer(); //provided by a validator. IV vector used with Zero Knowledge Proofs - for authentication purposes.
    //^ the the ZKP nonce is a PUBLIC value. It is delivered to the other peer who performs a one-way transformation on [mZKPNonce1Local | PSK]
    //the result of which is returned.
    //Important: mZKPNonce1Local MUST be unique on a per-conncetion basis. Value of a nonce CANNOT be resued. Value of a nonce MUST be re-generated each time it is validated.
    //Zero Knowledge Proofs support - END
    this.mIngressVideoTrack = null;
    this.mIngressAudioTrack = null;
    this.mIngressStream = null;
    this.mAuthenticated = false; // whether peer provided valid credentials (if required). Otherwise peer wouldn't be receiving data (audio/video/data).
    this.mStatusChangeTimestamp = gTools.getTime();
    this.mRecentLocalICE = null;
    this.mRecentRemoteICE = null;
    this.mPeerID = peerID;
    this.mDataChannel = null;
    this.mWasConnecting = false;
    this.mLocalDummyStream = this.mSwarm.getLocalDummyStream; // negotiate connection based on it, then we would be replacing senders' tracks on demand.
    this.mLocalStream = null; //real audio/video stream from local computer; we take tracks from it and replace on domand.
    this.mLocalRTCRtpSenders = [];
    this.mRemoteTracks = [];
    this.mLocalRTCRtpSenders = [];
    this.mPeerIP = null;
    this.mNativeConnectionID = 0; //i.e. RTCDataChannel.id
    this.mIPAddress = ''; //unknown; set as soon as ICE candidates are delivered.
    this.mConnectionID; // SHA3([mIPAddress,mNativeConnectionID]) - the actual connection identifier - MIGHT change is source IP changes and still the
    //webrtc stream is renegotiated.


    //External Event Handlers - BEGIN
    //Do note that external UI dApps can now subscribe for aggregated events by simply not sepcifying the particular connection ID.
    //Such events would be then fired for all new peers without the need to resubscribe.

    this.mAllowedCapabilities = capabilities; // The initial negotiated capabilities are always based on what the joining peer requested.
    //There might be nodes not willing to receive video streams due to bandwidth limitations etc.
    //Still, the mAllowedCapabilities is taken into account dynamically during data-transmission i.e. by adding and/or removing video/audio tracks on demand
    //as per what is being allowed by the user at the current moment.

    //higher level listeners
    //low-level API calls are translated into these
    this.mSwarmMessageEventListeners = []; // Swarm Data API Level 2
    this.mMessageEventListeners = []; // Swarm  Data API Level 1
    this.mSwarmConnectionStateChangeEventListeners = [];
    this.mPeerStatusEventListeners = [];
    this.mConnectionQualityEventListeners = [];
    //lower-level API listeners
    this.mTrackEventListeners = [];
    this.mPeerAuthenticationResultListeners = [];
    this.mConnectionStateChangeEventListeners = [];
    this.mICEConnectionStateChangeEventListeners = [];
    this.mSignalingStateChangeEventListeners = [];

    //data-channel listerers - Begin
    this.mDataChannelStateChangeEventListeners = [];
    this.mDataChannelMessageEventListeners = []; // Swarm  Data API Level 0
    //data-channel listereners - End
    //External Event Handlers - END

    ///!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //IMPORTANT: when adding qeueus, upgrade unregisterEventListenersByAppID()
    ///!!!!!!!!!!!!!!!!!!!!!!!!!!!!

  }

  genSDPSessionID()
  {
    this.SDPSessionID  = this.mTools.numberToArrayBuffer(this.mTools.getTime());
    return this.mSDPSessionID;
  }
  get SDPSessionID() {
    return this.mSDPSessionID;
  }

  set SDPSessionID(id) {
    this.mSDPSessionID = id;
  }


  set peerStatus(value) {

    //let nowMS = this.mTools.getTime(true);

    if (value == this.mPeerStatus)
      return;

    let nowMS = this.mTools.getTime(true);
    this.mLastPeerStatusChangeMS = nowMS;

    this.mPeerStatus = value;

    //Dispatch Notifications - BEGIN
    this.onPeerStatusChange(value);
    //Dispatch Notifications - END
  }

  set connectionQuality(value) {

    if (value == this.mConnectionQuality)
      return;


    this.mConnectionQuality = value;

    //Dispatch Notifications - BEGIN
    this.onConnQualityChange(value);
    //Dispatch Notifications - END
  }

  get peerStatusChangeTimestamp() {
    return this.mLastPeerStatusChangeMS;
  }

  get peerStatus() {
    return this.mPeerStatus;
  }

  get connectionQuality() {
    return this.mConnectionQuality;
  }
  set ingressAudioTrack(track) {
    this.mIngressAudioTrack = track;
  }

  get ingressAudioTrack() {
    return this.mIngressAudioTrack;
  }

  set ingressVideoTrack(track) {
    this.mIngressVideoTrack = track;
  }

  get ingressVideoTrack() {
    return this.mIngressVideoTrack;
  }

  set isPolite(isIt) {
    this.mIsPolite = isIt;
  }

  //Checks whether local peer is dominated by the other one.
  //The calculation is based on peers' identifiers.
  get isPolite() {

    let cf = CVMContext.getInstance().getCryptoFactory;
    //fetch peers' identifiers.
    let myID = this.mSwarm.getMyID;
    let peerID = this.mPeerID;

    //convert these identifiers into 32-byte numbers.
    let mT = cf.getSHA2_256Vec(gTools.convertToArrayBuffer(myID));
    let pT = cf.getSHA2_256Vec(gTools.convertToArrayBuffer(peerID));

    //compare these 32byte numbers, owner of a higher number dominates the other peer..
    if (gTools.arrayBufferToBigInt(pT) > gTools.arrayBufferToBigInt(mT)) {
      console.log(gTools.arrayBufferToString(peerID) + " dominated me. I'll be polite..");
      return true;
    } else {
      console.log("I've dominated " + gTools.arrayBufferToString(peerID) + "..");
      return false;
    }

    return false;
    //return this.mIsPolite;
  }

  get getProtocolID() {
    return 1; //standard CSwarmMsg requests
  }
  get getDummyAudioTrack() {
    let toRet = null;
    this.getLocalDummyStream.getTracks().every(function(track) {
      if (track.readyState == 'live' && track.kind === 'audio') {
        toRet = track;
        return false; //i.e. break the loop
      }
    });
    return toRet;
  }
  set ZKPNonce1Remote(value) {
    this.mZKPNonce1Remote = value;
    console.log('[- ZKP IV1 Remote SET -]: ' + (gTools.lengthOf(value) ? gTools.encodeBase58Check(value) : 'cleared'));
  }
  get ZKPNonce1Remote() {
    return this.mZKPNonce1Remote;
  }

  set ZKPNonce1Local(value) {
    this.mZKPNonce1Local = value;
    console.log('[- ZKP IV1 Local SET -]: ' + (gTools.lengthOf(value) ? gTools.encodeBase58Check(value) : 'cleared'));
  }
  get ZKPNonce1Local() {
    return this.mZKPNonce1Local;
  }

  set ZKPNonce2Local(value) {
    this.mZKPNonce2Local = value;
    console.log('[- ZKP IV2 Local SET -]: ' + (gTools.lengthOf(value) ? gTools.encodeBase58Check(value) : 'cleared'));
  }
  get ZKPNonce2Local() {
    return this.mZKPNonce2Local;
  }

  set ZKPNonce2Remote(value) {
    this.mZKPNonce2Remote = value;
    console.log('[- ZKP IV2 Remote SET -]: ' + (gTools.lengthOf(value) ? gTools.encodeBase58Check(value) : 'cleared'));
  }
  get ZKPNonce2Remote() {
    return this.mZKPNonce2Remote;
  }

  startZKPTimer1Local() {
    this.mZKPTimer1LocalStartMS = gTools.getTime(true);
  }
  startZKPTimer1Remote() {
    this.mZKPTimer1RemoteStartMS = gTools.getTime(true);
  }

  startZKPTimer2() {
    this.mZKPTimer2LocalStartMS = gTools.getTime(true);
  }

  isTimer1LocalExpired() { //Local peer acting as a candidate. The timer which indicates whether we can accept IV1 from a validator.
    let now = gTools.getTime(true);
    if ((now - this.mZKPTimer1LocalStartMS) > (this.mZKPTimer1ExpMS)) { //DO *NOT* account for clock drift here.
      return true;
    }
    return false;
  }

  isTimer1RemoteExpired() { //Local peer acting as validator. The timer which indicates whether validator can dispatch IV1.
    if (this.mZKPTimer1StartMS == 0) {
      return false;
    }

    let now = gTools.getTime(true);
    if ((now - this.mZKPTimer1RemoteStartMS) > (this.mZKPTimer1ExpMS + this.mClockDrift)) { //*DO* account for that time at the other end might be flowing little bit faster. Notice that we're little bit behind already due to network propgaation times.
      return true;
    }
    return false;
  }

  isTimer2Expired() { //Local peer acting as validator. Indicated whether we can still accept a ZKP from client.
    //Notice: there's no need to distinguis between isTimer2Remote and isTimer2Remote since a candidate simply strives to deliver a ZKP, ASAP.
    if (this.mZKPTimer2LocalStartMS == 0) {
      return false;
    }

    let now = gTools.getTime(true);
    if ((now - this.mZKPTimer2LocalStartMS) > this.mZKPTimer2ExpMS) {
      return true;
    }
    return false;
  }


  get getDummyVideoTrack() {
    let toRet = null;
    this.getLocalDummyStream.getTracks().every(function(track) {
      if (track.readyState == 'live' && track.kind === 'video') {
        toRet = track;
        return false; //i.e. break the loop
      }
    });
    return toRet;
  }
  get ZKPNonce() {
    return this.mZKPNonce1Local; //the OTHER peer needs to compute ZKP = SHA3([this.mZKPNonce1Local | this.Swarm.getPreSharedKey(true)]) in order to be authenticated.
    //that is to protect against reply attacks. The password itself also is not leaked.
  }

  //Processes datagrams received from a validator.
  //The stage intended by validator is detected automatically (based on the presence of IV1).
  processAuthRequestVal(authData) //nonce MUST be provided by the other peer A piori. BUT only in Phase 2. Candidate MUST REJECT
  { //IV2 which arrives before Timer1 expires.
    //Notice: **-> this function is executed ONLY when acted as a *CANDIDATE*  <--** in response to a 'authenticationRequestVal' datagram.
    let phase = 0;
    let result = false;
    let nonce = authData.data;

    //check the PROPOSED phase - BEGIN
    if (gTools.getLength(nonce)) {
      console.log('[- ZKP Stage Request -]: "' + gTools.arrayBufferToString(this.getPeerID) + '" wants to transition to Phase 2..');
      phase = 2;
    } else {
      console.log('[- ZKP Bootstrap Request -]: "' + gTools.arrayBufferToString(this.getPeerID) + '" wants to initiate a new Zero-Knowledge Proof Handshake..')
      phase = 1;
    }
    //check the PROPOSED phase - END

    //Phase 1 Support - BEGIN
    //in this phase, validator simply lets us know that we need to be authorized.
    //we respond with IV2 and start Timer1 (~3 sec).
    if (phase == 1) {
      //acting as a candidate.

      //generate IV2
      this.genZKPNonce2();
      let nonce2Local = this.mZKPNonce2Local;
      //now - prepare and issue a 'authRequestCand' datagram.

      //deliver ZKP to the other peer.
      let msg = new CSwarmMsg(eSwarmMsgType.authenticationRequestCand, this.mSwarm.getMyID, this.getPeerID, nonce2Local);
      let wrapper = new CNetMsg(this.getProtocolID, eNetReqType.request, msg.getPackedData());
      let serializedNetMsg = wrapper.getPackedData();

      //start Timer 1
      this.startZKPTimer1Local(); // so we can accept IV1 from validator *ONLY* after it expires.
      this.send(serializedNetMsg, false); //dispatch even if not authenticated yet.
      console.log('[- Local ZKP candidate -]: dispatched a Phase 1 response to "' + gTools.arrayBufferToString(this.getPeerID) + '". IV2 used: "' + gTools.encodeBase58Check(nonce2Local) + '"');
      result = true;
      return result;

      //Notice: nothing can go wrong here; we simply generate IV2 and deliver - acting as a candidate.

    } //Phase 1 Support - END
    //Phase 2 Support - BEGIN
    //during this stage we generate the actual Zero-Knowledge Proof.
    else if (phase == 2) {
      //acting as a candidate (also).
      if (this.isTimer1LocalExpired() == false) { //~10 seconds (started above).
        //*EXTREMELY IMPORTANT* otherwise we open up to reply attacks.
        console.log('[- ZKP Error -]:' + gTools.arrayBufferToString(this.getPeerID) + ' does not follow the protocol! Local Timer-1 not yet expired (required for Phase 2). Aborting.');
        this.resetZKPState(true, false); // reset local candidacy state alone. Does not affect the other peers' candidacy.
        return result;
      }

      if (gTools.lengthOf(nonce) != 32) {
        console.log("[- ZKP Error -]: won't attempt to athenticate, invalid IV1 provided by validator.");
        this.resetZKPState(true, false); // reset local candidacy state alone. Does not affect the other peers' candidacy.
        return result;
      }

      if (gTools.lengthOf(this.mSwarm.getPreSharedKey(true)) != 32) {
        console.log("[- ZKP Error -]: won't attempt to athenticate, pre-shared key is unknown.");
        this.resetZKPState(true, false); // reset local candidacy state alone. Does not affect the other peers' candidacy.
        return result;
      }
      this.ZKPNonce1Remote = nonce;
      return this.prepareAndDispatchZKP();
    }

    console.log('[- ZKP Error -]:' + gTools.arrayBufferToString(this.getPeerID) + ' does not follow the protocol! Unknown state during processAuthRequestVal(). Aborting.');
    this.resetZKPState(true, false); // reset local candidacy state alone. Does not affect the other peers' candidacy.
    return result;
    //Phase 2 Support - END
  }

  //Prepares and dispatches a Zero-Knowledge Proof.
  //Data is valdiated beforehand.
  prepareAndDispatchZKP() {
    //Operational Logic - BEGIN
    //let nonce = this.ZKPNonce; <- NO. this.ZKPNonce is the nonce we expect the REMOTE endpoint to use. (not the one we are supposed to use).
    //thus, for a mutual authentication TWO nonces are in use. The nonce required by the other peer is stored nowhere.

    //Local Variables - BEGIN
    let result = false;
    let psk = this.mSwarm.getPreSharedKey(true);
    let dc = new CDataConcatenator();
    let IV1 = this.mZKPNonce1Remote; //the value was provided by a validator.
    let IV2 = this.mZKPNonce2Local; // value generated by local node.
    //Local Variables - END

    //validate data - BEGIN
    if (gTools.getLength(IV1) != 32 || gTools.getLength(IV2) != 32 || gTools.getLength(psk) != 32) {
      console.log('[- ZKP Error -]: insufficient data to prepare an outgress Zero-Knowledge Proof. Aboritng.')
      this.resetZKPState(true, false); // reset local candidacy state alone. Does not affect the other peers' candidacy.
      return result;
    }
    //validate data - END

    //Calculate the ZKP - BEGIN
    dc.add(psk);
    dc.add(IV1);
    dc.add(IV2)
    let ephemeralZKP = dc.getData();
    ephemeralZKP = sha3_256.arrayBuffer(ephemeralZKP);
    //Calculate the ZKP - END

    //deliver ZKP to the other peer.
    let msg = new CSwarmMsg(eSwarmMsgType.zeroKnowledgeProof, this.mSwarm.getMyID, this.getPeerID, ephemeralZKP);
    let wrapper = new CNetMsg(this.getProtocolID, eNetReqType.request, msg.getPackedData());
    let serializedNetMsg = wrapper.getPackedData();
    this.send(serializedNetMsg, false); //dispatch even if not authenticated yet.
    console.log('[- ZKP OUT -]: [IV1]: "' + gTools.encodeBase58Check(IV1) + '" [IV2]: "' + gTools.encodeBase58Check(IV2) + '" [ZKP]: "' + gTools.encodeBase58Check(ephemeralZKP) + '"');
    result = true;
    return result
    //Operational Logic - END
  }
  /*
  The function verifies a Zero-Knowledge Proof delivered by a candidate.
  *IMPORTANT*: any zero-knowledge proof delivered after Timer1 has expires - it MUST be rejected.
  */
  async processZKP(ZKP) // ZKP = SHA3([this.mZKPNonce1Local | this.Swarm.getPreSharedKey(true)])
  {
    let result = false;
    //Important: notice that ZKP actually is considered as ZKP'. Now, ZKP' is compared against the expected ZKP.
    //Should there be a discrepancy, ZKP' is REJECTED and the protocol aborts.
    //Should anything go sideways, the procotol abotys as well.
    //Whenever the procol aborts, the internal Zero-Knowledge-Proof state-machinery is brought back to its initial state (all internal fields cleared).
    let peeriIDStr = gTools.arrayBufferToString(this.getPeerID);

    // Timing Security - BEGIN
    if (this.isTimer2Expired()) {
      //EXTREMELY IMPORTANT
      //otherwise we open up to reply attacks.
      console.log('[- ZKP Error -]: peer "' + peeriIDStr + '" delivered a ZKP too late. Aborting silently.')
      this.resetZKPState(false, true); // reset local candidacy state alone. Does not affect the other peers' candidacy.
      return result;
    }
    //Timing Security - END

    console.log('[- ZKP IN -]: {pre-processing} a Zero-Knowledge Proof from "' + peeriIDStr + '"');
    //this.genZKPNonce1();//IMPORTANT: generate new nonce asap for me requests.
    //Remote Execution - BEGIN
    //this command comes from a remote node.

    //Local Variables - BEGIN
    let dc = new CDataConcatenator();
    let psk = this.mSwarm.getPreSharedKey(true);
    let IV1 = this.mZKPNonce1Local; // the IV1 value was provided by local node.
    let IV2 = this.mZKPNonce2Remote; //IMPORTANT: notice that we're using IV2 provided by the remote peer. (not the one we generate and use to authorize ourselves as a candidate).
    //Local Variables - END

    //validate data - BEGIN
    if (gTools.getLength(IV1) != 32 || gTools.getLength(IV2) != 32 || gTools.getLength(psk) != 32) {
      //that might only happen if peers do not follow the protocol or under extreme adverse networking conditions.
      console.log('[- ZKP Error -]: insufficient data to prepare for verification of a Zero-Knowledge Proof from "' + peeriIDStr + '". Aboritng silently.');
      this.resetZKPState(false, true); // reset local validator state alone. Does not affect the other peer's validator.
      return result;
    }
    //validate data - END

    //Calculate the Verification ZKP - BEGIN
    dc.add(psk);
    dc.add(IV1);
    dc.add(IV2)
    let expectedZKP = dc.getData();
    expectedZKP = sha3_256.arrayBuffer(expectedZKP);
    //Calculate the Verification ZKP - END

    console.log('[- ZKP IN -]: {processing} [Expected IV1]: "' + gTools.encodeBase58Check(this.mZKPNonce1Local) + '" [Expected IV2]: "' + gTools.encodeBase58Check(this.mZKPNonce2Remote) +
      '" [Received ZKP]: "' + gTools.encodeBase58Check(ZKP) + '" [Expected ZKP] : "' + gTools.encodeBase58Check(expectedZKP) + '"');

    //Authorization Processing - BEGIN
    if (gTools.compareByteVectors(expectedZKP, ZKP)) {
      console.log('[- ZKP Success -]: peer ("' + gTools.arrayBufferToString(this.getPeerID) + '") successfully authenticated.');
      this.resetZKPState(false, true); // reset local validator state alone. Does not affect the other peer's validator.
      result = true;
      this.isAuthenticated = true; //peer is now authenticated.
      this.notifyAuthenticationSuccess(); //dispatch notificaiton to the remote peer.
    } else {
      console.log('[- ZKP *INVALID* -]: peer ("' + gTools.arrayBufferToString(this.getPeerID) + '") provided an invalid Zero-Knowledge Proof.');
      this.resetZKPState(false, true); // reset local validator state alone. Does not affect the other peer's validator.
      result = false;
      this.isAuthenticated = false; //even if previously authenticated.
      this.notifyAuthenticationFailure(); //dispatch notificaiton to the remote peer.
    }

    this.onPeerAuth(result, eSwarmAuthMode.preSharedSecret); //fire events (Swamrs API compatible).
    /*
    ^ notice: the event would fire at the connection level, later to propagate back the the Swarm level (nested events' processing).
    */

    //Authorization Processing - END

    return result;

    //Remote Execution - END
  }

  genZKPNonce1() {
    this.mZKPNonce1Local = gTools.getRandomVector(32);
  }

  genZKPNonce2() {
    this.mZKPNonce2Local = gTools.getRandomVector(32);
  }

  notifyAuthenticationSuccess() {
    console.log('Notifying ' + gTools.bytesToString(this.getPeerID) + ' about a successful authentication.. ');
    let msg = new CSwarmMsg(eSwarmMsgType.authenticationSuccess, this.mSwarm.getMyID);
    let wrapper = new CNetMsg(this.getProtocolID, eNetReqType.notification, msg.getPackedData());
    let serializedNetMsg = wrapper.getPackedData();
    this.send(serializedNetMsg, false); //dispatch even if not authenticated yet.
  }

  notifyAuthenticationFailure() {
    console.log('Notifying ' + gTools.bytesToString(this.getPeerID) + ' about invalid AUTH credentials..');
    let msg = new CSwarmMsg(eSwarmMsgType.authenticationFailure, this.mSwarm.getMyID);
    let wrapper = new CNetMsg(this.getProtocolID, eNetReqType.notification, msg.getPackedData());
    let serializedNetMsg = wrapper.getPackedData();
    this.send(serializedNetMsg, false); //dispatch even if not authenticated yet.
  }

  //Resests the local ZKP state-machine.
  resetZKPState(myCandState = true, myValState = true) {

    //IMPORTANT: we emply local and remote values, to allow for parallel, simultaneous, mutual ZKP authorization.
    if (myCandState) { //local node acting as a candidate.

      console.log('[- ZKP CANDIDATE RESET -]: resetting local Zero-Knowledge Proof state-machine..')
      this.ZKPNonce1Remote = new ArrayBuffer(); //clear IV1 delivered from a validator (phase 2).
      this.ZKPNonce2Local = new ArrayBuffer(); //clear IV2 local node proposed to remote validator (phase 1).
      this.mZKPTimer1LocalStartMS = 0; //when acting as a candidate - the time at which IV2 was dispatched to a validator.
      //notice: there's no need for this.mZKPTimer2RemoteStartMS as we simly strive to respond ASAP.
    }

    if (myValState) { //local node acting as a validator.

      console.log('[- ZKP VALIDATOR RESET -]: resetting local Zero-Knowledge Proof state-machine..')
      this.ZKPNonce1Local = new ArrayBuffer(); // clear IV1 which was proposed to a remote a candidate (phase 2).
      this.ZKPNonce2Remote = new ArrayBuffer(); //clear IV2 which was proposed by a remote a candidate (phase 1).
      this.mZKPTimer1RemoteStartMS = 0; //when acting as a validator - the time at which IV2 was received from a candidate.
      this.mZKPTimer2LocalStartMS = 0; //timestamp when a validator (local node) began to wait for a ZKP (the end of phase 2). ZKP - it MUST be received BEFORE the timer expires.
      //notice: there's no need for this.mZKPTimer2RemoteStartMS
    }

  }

  //An entry point to the ZKP authorization protocol.
  //Invoked only by a validator.
  requestAuthentication(isForced) {

    if (this.isAuthenticated && !isForced) {
      console.log('[- ZKP NOT NEEDED -]: peer already authenticated (AUTH was not to be enforced).')
      return false;
    }
    return this.dispatchAuthRequestVal(1); //initiate the ZKP protocol. (Phase 1).
  }

  //Invoked after validator requested for us to authenticate.
  //We provide IV2 which is to be used by us and the validator for computation of the ZKP.
  requestAuthenticationCand() { //send after requestAuthenticationVal received from validator.
    let msg = null;

    this.genZKPNonce2(); //IV provided by a candidate.

    console.log('Zero-Knowledge Proof candidate offering IV2 to ' + gTools.bytesToString(this.getPeerID) + ' (ZKP IV2 vector: ' +
      gTools.encodeBase58Check(this.ZKPNonce2) + ").");

    msg = new CSwarmMsg(eSwarmMsgType.authenticationRequestCand, this.mSwarm.getMyID, this.getPeerID, this.mZKPNonce2Local);
    let wrapper = new CNetMsg(this.getProtocolID, eNetReqType.request, msg.getPackedData());
    let serializedNetMsg = wrapper.getPackedData();
    return this.send(serializedNetMsg, false); //dispatch even if not authenticated yet.
  }

  //Invoked by validator once 'authenticationRequestCand' arrives from a candidate.
  processAuthRequestCand(data) {
    //Validation - BEGIN
    if (gTools.getLength(data) != 32) {
      console.log("Invalid IV2 received from candidate '" + gTools.bytesToString(this.getPeerID) + "'.");
      this.resetZKPState(false, true); // reset local validator state alone. Does not affect the other peer's validator.
      return false;
    }
    //Validation - END

    //Operational Logic - BEGIN
    this.mZKPNonce2Remote = data;
    this.startZKPTimer1Remote(); //only after it expires would we accept IV1 from the verifier.
    //notice that there would always be a slight discrepancy between timestamp at local node and remote due to network propagation times
    //and clock drift (of least significance).

    //schedule IV1 to be delivered. This can happen only after Timer1 expires.
    //notice that this would happen a few MS earlier at the remote end (network propagation times, timedrift, CPU)
    setTimeout(function() {
      //provide IV to a candidate
      this.dispatchAuthRequestVal(2);
    }.bind(this), this.mZKPTimer1ExpMS);

    //IMPORTANT: notice that due to a use a of setTimeout(), here we do not need to actually check for isTimer1RemoteExpired().
    //Were we in the future not to rely on setTimeout() (and say on gTools.sleeper() and Intents) we might need to be checking for isTimer1RemoteExpired();
    //in any case the timer is being as as of now, as it is.
    //Notice: the remote candidate keeps track of time-flow through Timer1Local. The client would NOT accept IV1 unless Timer1Local had expired.

    //Operational Logic - END
    return true;
  }

  //Orders for a ZKP authorization request to be dispatched to the remote endpoint.
  //This effectively initiates the Zero-Knowledge Proof authorization protocol.
  dispatchAuthRequestVal(phase = 1) { //called by a validator only.

    //Local Variabels - BEGIN
    let msg = null;
    let peerIDStr = gTools.bytesToString(this.getPeerID);
    let nowMS = gTools.getTime(true);


    let lIV1ToBeSent = new ArrayBuffer();
    //Local Variabels - END

    //Operational Logic - BEGIN
    if (phase == 1) { //initiates the protocol.

      if ((nowMS - this.mLastTimeOutgressAuthInitTimestamp) < ((this.mZKPTimer1ExpMS + this.mZKPTimer2ExpMS + this.mClockDrift))) {
        console.log("warning: won't attempt new AUTH with '" + peerIDStr + "' -  it's too early!")
        return false;
      }

      this.mLastTimeOutgressAuthInitTimestamp = nowMS;
      console.log('Requesting Zero-Knowledge Proof Authentication (Phase 1) from "' + peerIDStr + '"');
    } else if (phase == 2) { //validator transitions to Phase 2 - provides IV1 to a candidate.
      this.genZKPNonce1(); //generate a new nonce (IV1 vector).
      lIV1ToBeSent = this.mZKPNonce1Local;
      console.log('Requesting Zero-Knowledge Proof Authentication (Phase 2) from "' + peerIDStr + '" (ZKP IV vector: "' +
        gTools.encodeBase58Check(this.mZKPNonce1Local) + '").');
    }
    let authMsg = new CSwarmAuthData();
    authMsg.isZKP = true;
    authMsg.isDedicatedPSK = this.mSwarm.isDedicatedPSK;
    authMsg.data = lIV1ToBeSent;
    msg = new CSwarmMsg(eSwarmMsgType.authenticationRequestVal, this.mSwarm.getMyID, this.getPeerID, authMsg.getPackedData());
    let wrapper = new CNetMsg(this.getProtocolID, eNetReqType.request, msg.getPackedData());
    let serializedNetMsg = wrapper.getPackedData();
    this.send(serializedNetMsg, false); //dispatch even if not authenticated yet.

    //SECURITY - *Extremely Important* - BEGIN
    if (phase == 2) {
      this.startZKPTimer2(); // any ZKP arriving after the timer has expired - it MUST be rejected.
    }
    return true;
    //SECURITY - *Extremely Important* - END

    //Operational Logic - END
  }

  activateDummyTracks() {
    let dummyStream = this.getLocalDummyStream;

    if (!dummyStream)
      return false;

    //'muting' is about letting the Dummy Video track play.
    //changing the connection's effective capabilities is about disabling the lowest-level data streams.
    //we usually first 'mute' and then (after ~1s disable all the corresponding (audio/video) data-flows).
    let videoTracks = dummyStream.getVideoTracks();
    let audioTracks = dummyStream.getAudioTracks();

    if (audioTracks.length == 0 || videoTracks.length == 0)
      return false;

    let videoTrack = videoTracks[0];
    let audioTrack = audioTracks[0];
    videoTrack.enabled = true;
    audioTrack.enabled = true;
    return true;
  }

  hasTrack(track) {
    if (this.mRTCConnection == null)
      return false;
    let senderList = this.mRTCConnection.getSenders();

    for (let i = 0; i < senderList.length; i++) {
      if ((senderList[i].track != null) && (senderList[i].track.id === track.id))
        return true;
    }
    return false;
  }
  //Mutes audio/video tracks. Does NOT mute the data channel.
  //Notiece: the function is availble both at a Swarm and Connection level.
  async mute(audio = true, video = true, outgress = true, autoStopDummyTrack = false, autoStopDummyTrackAfterMS = 1000) {
    //todo: add support for ingress tracks
    let dummyStream = this.getLocalDummyStream;

    if (!dummyStream)
      return false;

    //'muting' is about letting the Dummy Video track play.
    //changing the connection's effective capabilities is about disabling the lowest-level data streams.
    //we usually first 'mute' and then (after ~1s disable all the corresponding (audio/video) data-flows).
    let videoTracks = dummyStream.getVideoTracks();
    let audioTracks = dummyStream.getAudioTracks();

    if (audioTracks.length == 0 || videoTracks.length == 0)
      return false;

    let videoTrack = videoTracks[0];
    let audioTrack = audioTracks[0];

    if (video) {
      videoTrack.enabled = true; //make sure the dummy video track can play.
    }

    if (audio) {
      audioTrack.enabled = true; //make sure the dummy audio track can play.
    }

    let res = null;
    if (audio) {
      res = await this.replaceAudioTrack(audioTrack);
      if (!res)
        return false;
    }

    if (video) {
      res = await this.replaceVideoTrack(videoTrack);
      if (!res)
        return false;
    }

    if (autoStopDummyTrack) {
      await gTools.sleeper(autoStopDummyTrackAfterMS).then(async function() {
        let dummyAudio = this.getDummyAudioTrack;
        let dummyVideo = this.getDummyVideoTrack;
        if (dummyAudio) {
          dummyAudio.enabled = false; //stop the buzzing.
        }
        if (dummyVideo) {
          dummyVideo.enabled = false; //stop the unneeded data flow entirely (black pixels).
        }
      }.bind(this));
    }

    return true;
  }

  //Un-mutes audio/video tracks. Does NOT (re)enable the data channel.
  //Notiece: the function is availble both at a Swarm and Connection level.
  async unmute(audio = true, video = true, outgress = true) {
    //todo: add support for ingress tracks
    //Security - BEGIN
    if (this.mSwarm.isPrivate && !this.isAuthenticated) {
      return false;
    }
    //Security - END

    //'muting' is about letting the Dummy Video track play.
    //changing the connection's effective capabilities is about disabling the lowest-level data streams.
    //we usually first 'mute' and then (after ~1s disable all the corresponding (audio/video) data-flows).
    //Notice: we thus never 'unmute' a Dummy track. We can unmute a connection only IF a LIVE video/audio track had been set in the first place.
    let liveVideoTrack = this.mSwarm.getLIVEVideoTrack;
    let liveAudioTrack = this.mSwarm.getLIVEAudioTrack;
    let res = null;

    let replaced = false;

    if (audio && liveAudioTrack && liveAudioTrack.readyState == 'live') {
      console.log('unmuting LIVE audio track for ' + gTools.arrayBufferToString(this.getPeerID));
      res = await this.replaceAudioTrack(liveAudioTrack);
      if (!res) {
        return false;
      }
      replaced = true;
    } else if (audio) {
      console.log("won't unmute audio as no LIVE audio track available.")
    }

    if (video && liveVideoTrack && liveVideoTrack.readyState == 'live') {
      console.log('unmuting LIVE video track for ' + gTools.arrayBufferToString(this.getPeerID));
      res = await this.replaceVideoTrack(liveVideoTrack);
      if (!res) {
        return false;
      }
      replaced = true;
    } else if (video) {
      console.log("won't unmute video as no LIVE video track available.")
    }

    return replaced;
  }

  set setNativeConnectionID(id) {
    this.mNativeConnectionID = id;
  }

  get getNativeConnectionID() {
    return this.mNativeConnectionID;
  }

  set setIPAddress(ip) {
    this.mIPAddress = ip;
  }

  //Computes the ⋮⋮⋮ Secure Connection Identifier.
  computeConnectionID() {
    let dc = new CDataConcatenator();
    dc.add(this.getNativeConnectionID);
    dc.add(this.getIPAddress);
    let concat = dc.getData();
    this.setConnectionID = sha3_256.arrayBuffer(concat);
  }

  get getIPAddress() {
    return this.mIPAddress;
  }

  set setConnectionID(connID) {
    this.mConnectionID = connID;
  }
  get getConnectionID() {
    return this.mConnectionID;
  }


  set isAuthenticated(isIt) {
    //Internal Processing - BEGIN
    this.mAuthenticated = isIt;
    if (this.mAuthenticated) {

    }

    if (this.mSwarm.isPrivate) {
      //console.log('muting ' + gTools.arrayBufferToString(this.mPeerID) +' as peer now needs to authenticate first.')
      //this.mute(true, true, true);
    }
    //Internal Processing - END

    //Dispatch Events - BEGIN
    //Dispatch Events - END

  }

  get isAuthenticated() {
    return this.mAuthenticated;
  }


  //Replace one of the streamed video tracks.
  async replaceVideoTrack(track, honorSecurity = true) {

    if (gTools.isNull(track)) {
      console.log('Error: there was an attempt to set a NULL video track. Aborting');
      return;
    }
    let didIt = false;
    if (this.hasTrack(track))
      return true;

    if (this.mRTCConnection == null)
      return false;

    //Security - BEGIN
    if (honorSecurity && !track.getVirtualDev && (this.mSwarm.isPrivate && !this.isAuthenticated)) { //allow for an exchange to a Virtual Device. That is needed for cold-muting.
      return false; //skipping
    }
    //Security - END

    let senders = this.mRTCConnection.getSenders();
    if (senders.length == 0)
      return false;

    let res = null;
    for (let i = 0; i < senders.length; i++) {

      if ((senders[i].track && senders[i].track.kind == 'video') && !(senders[i].track.id === track.id)) { //|| (senders[i].track==null && i==1)
        res = await senders[i].replaceTrack(track);
        didIt = true;
      }
    }
    return didIt;
  }

  //Replace one of the streamed video tracks.
  async replaceAudioTrack(track, honorSecurity = true) {

    if (gTools.isNull(track)) {
      console.log('Error: there was an attempt to set a NULL audio track. Aborting');
      return;
    }

    let didIt = false;
    if (this.hasTrack(track))
      return true;

    if (this.mRTCConnection == null)
      return false;

    //Security - BEGIN
    //the track needs to be produced by a Virtual Device or Authentication needed.
    if (honorSecurity && !track.getVirtualDev && (this.mSwarm.isPrivate && !this.isAuthenticated)) { //allow for an exchange to a Virtual Device. That is needed for cold-muting.
      return false; //skipping
    }
    //Security - END

    let senders = this.mRTCConnection.getSenders();
    if (senders.length == 0)
      return false;
    let res = null;
    for (let i = 0; i < senders.length; i++) {
      if ((senders[i].track && senders[i].track.kind == 'audio') && !(senders[i].track.id === track.id)) { //|| (senders[i].track==null && i==1)
        let res = await senders[i].replaceTrack(track);
        didIt = true;
      }
    }
    return didIt;
  }


  get getID() {
    return this.mPeerID;
  }

  getAllowedMedia() {
    return this.mSwarm.mSwarmManager.getAllowedMedia();
  }
  addLocalRTCRtpSender(sender) {
    this.mLocalRTCRtpSenders.push(sender);
  }

  createDataChannel(reliable = true) {
    if (this.mRTCConnection == null)
      return false;
    this.setDataChannel(this.mRTCConnection.createDataChannel('data', {
      reliable: reliable
    }));
    return true;
  }

  getEffectiveCapabilities(outgress = true) {
    if (this.mRTCConnection == null)
      return false;

    let audioPlaying = false;
    let videoPlaying = false;

    let objects = outgress ? this.mRTCConnection.getSenders() : this.mRTCConnection.getReceivers();
    if (objects != null)
      for (let i = 0; i < objects.length; i++) {
        if (objects[i].track == null)
          continue;
        if (objects[i].track.kind == 'audio') {
          if (objects[i].track.enabled)
            audioPlaying = true;
        } else {
          if (objects[i].track.kind == 'video') {
            if (objects[i].track.enabled)
              videoPlaying = true;
          }
        }
      }

    if (audioPlaying && videoPlaying)
      return eConnCapabilities.audioVideo;
    else if (videoPlaying)
      return eConnCapabilities.video;
    else if (audioPlaying)
      return eConnCapabilities.audio;
    else return eConnCapabilities.data;


  }


  //Sets connections's effective capabilities (outgress or ingress).
  //IF requested effective capabilities are higher than the current allowed apabilities, the latter will be updated
  //automatically, in which case renegotiation of the connection's parameters through SDP will occur automatically.

  //Flow: during normal operation, when video/audio outputs are to be muted, the idea is for the corresponding track(s) to be replaced
  //with a dynamically geenrated Dummy Track, to let it flow for 1-2 seconds, after which, the Effeective Capabilities are to be modified
  //and the corresponding Senders disabled. That is to account for a variety of Chromium bugs and inconsistancies.
  setEffectiveCapabilities(capabilities, outgress = true, unmuteVirtualTracks = false, honorSecurity = true) {

    let applied = false;
    if (this.mRTCConnection == null)
      return false;

    if (capabilities > this.mAllowedCapabilities)
      return false;

    //this.setAllowedCapabilities(this.mEffectiveCapabilities);//do NOT upgrade what allowed for (had been negotiated with remote peer in advance)

    //the value of effective capablities is reflected by the current state of attached media-tracks.

    let objects = outgress ? this.mRTCConnection.getSenders() : this.mRTCConnection.getReceivers();

    if (objects != null)
      for (let i = 0; i < objects.length; i++) {
        if (objects[i].track == null)
          continue;
        switch (capabilities) {
          case eConnCapabilities.data:
            if (objects[i].track.kind == 'audio') {
              objects[i].track.enabled = false;
              applied = true;
            }

            if (objects[i].track.kind == 'video') {
              objects[i].track.enabled = false;
              applied = true;
            }


            break;
          case eConnCapabilities.audio:
            if (objects[i].track.kind == 'audio') {

              if (objects[i].track.getVirtualDev && !unmuteVirtualTracks) {
                continue;
              }
              objects[i].track.enabled = true;
              applied = true;
            }
            if (objects[i].track.kind == 'video') {
              objects[i].track.enabled = false;
              applied = true;
            }

            break;
          case eConnCapabilities.video:
            if (objects[i].track.kind == 'audio') {
              objects[i].track.enabled = false;
              applied = true;
            }
            if (objects[i].track.kind == 'video') {
              if (objects[i].track.getVirtualDev && !unmuteVirtualTracks) {
                continue;
              }
              objects[i].track.enabled = true;
              applied = true;
            }

            break;
          case eConnCapabilities.audioVideo:
            if (objects[i].track.kind == 'audio') {
              if (objects[i].track.getVirtualDev && !unmuteVirtualTracks) {
                continue;
              }
              objects[i].track.enabled = true;
              applied = true;
            }
            if (objects[i].track.kind == 'video') {
              if (objects[i].track.getVirtualDev && !unmuteVirtualTracks) {
                continue;
              }
              applied = true;
              objects[i].track.enabled = true;

            }

            break;
        }
      }

    return true;
  }


  //Sets effective ingress capabilities.
  //Can be used to mute/unmute tracks (video/audio) without causing the underlying connection's renegotiation.
  setEffectiveIngressCapabilities(allowed) {
    if (this.mLocalStream == null)
      return false;

    return this.setEffectiveCapabilities(allowed);
  }
  //Sets effective capabilities for the entire Stream (BOTH ingress and outgress).
  //Can be used to mute/unmute tracks (video/audio) without causing the underlying connection's renegotiation.
  setLocalStreamEffectiveCapabilities(allowed) {
    if (allowed == eConnCapabilities.data) {
      this.mLocalStream.getAudioTracks().forEach(track => {
        track.enabled = false;
      });
      this.mLocalStream.getVideoTracks().forEach(track => {
        track.enabled = false;
      });
    } else if (allowed == eConnCapabilities.audio) {
      this.mLocalStream.getVideoTracks().forEach(track => {
        track.enabled = false;
      });
      this.mLocalStream.getAudioTracks().forEach(track => {
        track.enabled = true;
      });
    } else if (allowed == eConnCapabilities.video) {
      this.mLocalStream.getVideoTracks().forEach(track => {
        track.enabled = true;
      });
      this.mLocalStream.getAudioTracks().forEach(track => {
        track.enabled = false;
      });
    } else if (allowed == eConnCapabilities.audioVideo) {
      this.mLocalStream.getVideoTracks().forEach(track => {
        track.enabled = true;
      });
      this.mLocalStream.getAudioTracks().forEach(track => {
        track.enabled = true;
      });
    }
  }


  get getAllowedCapabilities() {
    return this.mAllowedCapabilities;
  }
  //The ceiling capabilities allowed by the local user.
  //Note the difference between setCapabilities which is limited by the remote peer. (the union of local and remote capabilities)
  //Note the difference between setEffectiveOutgressCapabilities which can be used to mute/unmute tracks (video/audio) without causing renegotiation.
  //Note that changing this might cause connection's renogotiation and firing of onTrack events on the other peers.
  //updateMedia is set to false especially during initial connection formation not to disturb the traditional sequence of events.
  setAllowedCapabilities(allowed, updateMedia = true) {
    //refresh local-stream and add/remove tracks

    if (this.mAllowedCapabilities != allowed) {
      this.mAllowedCapabilities = allowed;
    }
  }

  ping() {
    let msg = new CSwarmMsg(eSwarmMsgType.keepAlive, this.mSwarm.getMyID);
    this.sendSwarmMessage(msg, 0, false); //also if not authenticated.
  }
  //Sends a CNetMessage container.
  sendMessage(message, onlyIfAuthenticated = true) {

    if (!this.mTools.isInstanceOf(message, CNetMsg))
      return false;

    return this.send(message.getPackedData(), onlyIfAuthenticated);
  }

  //Sends a CSwarmMsg container.
  sendSwarmMessage(message, protocolID, onlyIfAuthenticated = true) {

    if (!this.mTools.isInstanceOf(message, CSwarmMsg))
      return false;


    let wrapper = new CNetMsg(protocolID, eNetReqType.notify, message.getPackedData());

    return this.send(wrapper.getPackedData(), onlyIfAuthenticated);
  }

  //send RAW bytes.
  send(data, protocolID = 0, onlyIfAuthenticated) {

    //Security - BEGIN
    if (onlyIfAuthenticated && (this.mSwarm.isPrivate && !this.isAuthenticated)) {
      return false; //skipping
    }
    //Security - END
    if (!(data instanceof ArrayBuffer)) {
      if (gTools.isInstanceOf(data, CNetMsg)) {
        return this.sendNetMsg(data, onlyIfAuthenticated);
      } else if (gTools.isInstanceOf(data, CSwarmMsg)) {
        return this.sendSwarmMessage(data, protocolID, onlyIfAuthenticated);
      }
    }

    try {
      this.mDataChannel.send(data);
      return true;
    } catch (error) {
      console.log('Error while sending data');
      return false;
    }
  }

  initEvents() {
    //Kindly do note: Peer B, the one receiving the connection (in our Swarm's API terms and the one establishing the connection in WebRTC terms)
    //will have the data-channel object created right away and attached to the Swarm's Connection member field.
    //This enables for initialization of the data-channel-related events right away.
    //Still, in a case where it is Peer A, it is NOT possible to subscribe for events UNTIL the onDataChannel event has fired, after which we were given the data-channel object.
    //Thus below we shall proceed even if mDataChannel member field is empty all we need in such a case in terms of data-chanel is to subscribe for the ondata-channel event.
    //Note that at the end there's an attempt to subscribe for data-channel events should the function be invoked by Peer B.
    if (this.mRTCConnection == null) //|| this.mDataChannel == null)
      return false;
    //sign-up for WebRTC-Connection events - BEGIN
    if (this.mRTCConnection.onicecandidate == null)
      this.mRTCConnection.onicecandidate = this.onLocalICECandidateEvent.bind(this);
    if (this.mRTCConnection.ontrack == null)
      this.mRTCConnection.ontrack = this.onTrackEvent.bind(this);
    if (this.mRTCConnection.ondatachannel == null)
      this.mRTCConnection.ondatachannel = this.onDataChannelEvent.bind(this);
    if (this.mRTCConnection.onnegotiationneeded == null)
      this.mRTCConnection.onnegotiationneeded = this.onNegotiatioNeeded.bind(this);
    if (this.mRTCConnection.oniceconnectionstatechange == null)
      this.mRTCConnection.oniceconnectionstatechange = this.onICEConnectionStateChangeEvent.bind(this);
    if (this.mRTCConnection.onsignalingstatechange == null)
      this.mRTCConnection.onsignalingstatechange = this.onHandleSignalingStateChangeEvent.bind(this);
    if (this.mRTCConnection.onconnectionstatechange == null)
      this.mRTCConnection.onconnectionstatechange = this.onConnectionStateChangeEvent.bind(this);
    //sign-up for WebRTC-Connection events - END

    if (this.mDataChannel != null) { //client A would wait for an onDataChannel event and initialize events over there
      this.initDataChannelEvents();
    }
    return true;
  }

  initDataChannelEvents() { //CORE
    if (this.mDataChannel == null)
      return false;
    //subscribe for WebRTC DataChannel events - BEGIN
    if (this.mDataChannel.onopen == null)
      this.mDataChannel.onopen = this.onDataChannelOpenEvent.bind(this);
    if (this.mDataChannel.onclose == null)
      this.mDataChannel.onclose = this.onDataChannelCloseEvent.bind(this);
    if (this.mDataChannel.onclosing == null)
      this.mDataChannel.onclosing = this.onDataChannelClosingEvent.bind(this);
    if (this.mDataChannel.onerror == null)
      this.mDataChannel.onerror = this.onDataChannelErrorEvent.bind(this);
    if (this.mDataChannel.onmessage == null)
      this.mDataChannel.onmessage = this.onDataChannelMessageEvent.bind(this);
    return true;
    //subscribe for WebRTC DataChannel events - END
  }

  //Event Handlers' Subscribers Addition - BEGIN

  //Note: this is different from RAW RTC socket-related events provided by
  //addConnectionStateChangeEventListener()


  //Swarm-Connection Events - BEGIN
  addSwarmConnectionStateChangeEventListener(eventListener, appID = 0) {
    this.mSwarmConnectionStateChangeEventListeners.push({
      handler: eventListener,
      appID
    });
  }

  addPeerStatusChangeEventListener(eventListener, appID = 0) {
    this.mPeerStatusEventListeners.push({
      handler: eventListener,
      appID
    });
  }

  addConnectionQualityChangeEventListener(eventListener, appID = 0) {
    this.mConnectionQualityEventListeners.push({
      handler: eventListener,
      appID
    });
  }

  addSwarmMessageListener(eventListener, appID = 0) {
    this.mSwarmMessageEventListeners.push({
      handler: eventListener,
      appID
    });
  }
  addMessageListener(eventListener, appID = 0) {
    this.mMessageEventListeners.push({
      handler: eventListener,
      appID
    });
  }
  //Swarm-Connection Events - END

  //Low-level WebRTC RTCPeerConnection connection events - BEGIN
  addConnectionStateChangeEventListener(eventListener, appID = 0) {
    this.mConnectionStateChangeEventListeners.push({
      handler: eventListener,
      appID
    });
  }
  addTrackEventListener(eventListener, appID = 0) {
    this.mTrackEventListeners.push({
      handler: eventListener,
      appID
    });
  }

  addPeerAuthResultListener(eventListener, appID = 0) {
    this.mPeerAuthenticationResultListeners.push({
      handler: eventListener,
      appID
    });
  }
  addICEConnectionStateChangeEventListener(eventListener, appID = 0) {
    this.mICEConnectionStateChangeEventListeners.push({
      handler: eventListener,
      appID
    });
  }

  addSignalingStateChangeEventListener(eventListener, appID = 0) {
    this.mSignalingStateChangeEventListeners.push({
      handler: eventListener,
      appID
    });
  }

  //Low-level WebRTC RTCPeerConnection connection events - END


  //Low level WebRTC RTCDataChannel events - BEGIN
  addDataChannelStateChangeEventListener(eventListener, appID = 0) {
    this.mDataChannelStateChangeEventListeners.push({
      handler: eventListener,
      appID
    });
  }
  addDataChannelMessageEventListener(eventListener, appID = 0) {
    this.mDataChannelMessageEventListeners.push({
      handler: eventListener,
      appID
    });
  }


  //Low level WebRTC RTCDataChannel events - END

  //Event Handler Subscribers Addition - END


  //WebRTC Native-events support  (CORE)- BEGIN
  //includes dispatchment of events to other UI dApps / handlers

  onDataChannelOpenEvent(event) {

      console.log('data channel is opening for a connection with ' +gTools.arrayBufferToString(this.mPeerID) )
    //core processing - Begin
    //the specification says that might be fired also when conneciton is 're-established' thus we transfer the connection to
    //the active connections' pool only upon verification that it's not already there
    this.setStatus(eSwarmConnectionState.active);

    //Authentication - BEGIN
    if (this.mSwarm.isPrivate) {
      this.requestAuthentication(true);
    }
    //Authentication - END
    this.setNativeConnectionID = event.target.id; //first part to construct a ⋮⋮⋮ Secure Connection ID.
    let newEvent = {
      swarmID: this.mSwarm.getID,
      peerID: this.mPeerID,
      event: event,
      connection: this
    }
    this.mSwarm.transferConnToActive(this.getID); //the peer ID as assumed as connection ID;todo: reconsider supporting mutliple swarm connections with the same Peer
    //this seems like not necessary due to the possibility of multiple channels at the upper-data later. Thus, for now mitple data-streams are multiplexed over same connection which seems Fine.
    //Note that Tor employs the same methodology. Tor used to achieve poor performance with multiple TCP/IP connections among peers.

    this.mSwarm.onDataChannelStateChange(newEvent); //notify aggregator
    //core processing - End
    for (let i = 0; i < this.mDataChannelStateChangeEventListeners.length; i++) {
      try {
        this.mDataChannelStateChangeEventListeners[i].handler(newEvent);
      } catch (error) {
        console.log('UI dApp failed to process event. App ID: ' + this.mDataChannelStateChangeEventListeners[i].appID)
      }
    }

    this.unmute();
  }

  onDataChannelCloseEvent(event) {
    //core processing - Begin - Part1
    //todo: handle this in the actual connection-changed event
    this.setStatus(eSwarmConnectionState.closed);
    console.log('Connection with peer ' + gTools.arrayBufferToString(this.mPeerID) + ' closed.');

    //core processing - End
    for (let i = 0; i < this.mDataChannelStateChangeEventListeners.length; i++) {
      try {
        this.mDataChannelStateChangeEventListeners[i].handler({
          event: event,
          peerID: this.mPeerID,
          swarmID: this.mSwarm.getID,
          connection: this
        });
      } catch (error) {
        console.log('UI dApp failed to process event. App ID: ' + this.mDataChannelStateChangeEventListeners[i].appID)
      }
    }
    //core processing - Begin - Part 2
    this.unregisterEventListenersByAppID(); //unregister all event listeners.
    this.mSwarm.removeConnection(this.getID);
    //core processing - End - Part 2
  }

  onDataChannelClosingEvent(event) {
    for (let i = 0; i < this.mDataChannelStateChangeEventListeners.length; i++) {
      try {
        this.mDataChannelStateChangeEventListeners[i].handler({
          event: event,
          peerID: this.mPeerID,
          swarmID: this.mSwarm.getID,
          connection: this
        });
      } catch (error) {
        console.log('UI dApp failed to process event. App ID: ' + this.mDataChannelStateChangeEventListeners[i].appID)
      }
    }
  }


  unregisterEventListenersByAppID(appID = 0, eventListener = null) {
    let result = false;

    //if (appID == 0 && eventListener == null) <- then unregister all
    //  {
    //  eventQueues.clear();
    //  }

    let eventQueues = [
      this.mSwarmMessageEventListeners,
      this.mMessageEventListeners,
      this.mSwarmConnectionStateChangeEventListeners,
      this.mTrackEventListeners,
      this.mPeerAuthenticationResultListeners,
      this.mConnectionStateChangeEventListeners,
      this.mICEConnectionStateChangeEventListeners,
      this.mSignalingStateChangeEventListeners,
      this.mDataChannelStateChangeEventListeners,
      this.mDataChannelMessageEventListeners,
      this.mPeerStatusEventListeners,
      this.mConnectionQualityEventListeners
    ];

    for (let a = 0; a < eventQueues.length; a++) {
      for (let i = 0; i < eventQueues[a].length; i++) {
        if (eventListener == null) {
          if (appID == 0 || eventQueues[a][i].appID == appID) {
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

  get lastSeen() {
    return this.mLastKeepAliveReceivedMS;
  }

  get videoActive() {
    return this.mVideoActive;
  }

  set videoActive(isIt) {
    if (this.mVideoActive != isIt) {
      this.mVideoActive = isIt;
    }
  }

  get audioActive() {
    return this.mAudioActive;
  }

  set audioActive(isIt) {
    this.mAudioActive = isIt;
  }
  get screenSharingActive() {
    return !this.mSSActive;
  }

  set screenSharingActive(isIt) {
    this.mSSActive = isIt;
  }




  //Kernel-Mode data listener. High-level( 2/2). Fired only if data was found to comprise a valid CSwarmMsg container.
  onSwarmMessage(message) {
    //core processing - BEGIN


    //Even non-authenticated - BEGIN

    switch (message.type) {
      case eSwarmMsgType.keepAlive:
        this.mLastKeepAliveReceivedMS = gTools.getTime(true);
        break;

        //Security - BEGIN
      case eSwarmMsgType.authenticationFailure:
        break;

      case eSwarmMsgType.zeroKnowledgeProof:
        this.processZKP(message.dataBytes);
        break;

      case eSwarmMsgType.authenticationRequestCand:
        this.processAuthRequestCand(message.dataBytes);
        break;

      case eSwarmMsgType.authenticationRequestVal:
        // should we notify the user?
        //in any case - attempt to process the request.
        //  let nonce = msg.dataBytes; // ephemeral IV vector is delivered from the other peer.
        //  e.connection.authenticateThroughZKP(nonce);// the function would auto-detect the Phase.
        let swarmAuthData = CSwarmAuthData.instantiate(message.dataBytes);
        if (!swarmAuthData) {
          console.log('Invalid Swarm AUTH data received.');
          break;
        }
        this.processAuthRequestVal(swarmAuthData);
        break;

      case eSwarmMsgType.authenticationSuccess:
        break;
        //Security - END
      default:

    }

    //Even non-authenticated - END

    //Only Authenticated - Begin
    if ((this.isAuthenticated || this.mSwarm.isPrivate == false)) {
      switch (message.type) {
        case eSwarmMsgType.startedScreenSharing:
          this.screenSharingActive = true;
          break;
        case eSwarmMsgType.stoppedScreenSharing:

          this.screenSharingActive = false;
          break;
        case eSwarmMsgType.mutedCam:

          this.videoActive = false;
          break;
        case eSwarmMsgType.unmutedCam:

          this.videoActive = true;
          break;
        case eSwarmMsgType.mutedAudio:

          this.audioActive = false;
          break;
        case eSwarmMsgType.unmutedAudio:
          this.audioActive = true;
          break;


      }
    }
    //Only Authenticated - End


    //construct an event
    let e = { // API level 2/2
      swarmID: this.mSwarm.getID,
      peerID: this.getPeerID,
      message: message,
      connection: this
    };

    //notify the agregator (notice that we pass an Event, not the actual message - performance).
    this.mSwarm.onSwarmMessage(e);

    //core processing - END

    //User-Mode notifications - BEGIN
    for (let i = 0; i < this.mSwarmMessageEventListeners.length; i++) {
      try {
        this.mSwarmMessageEventListeners[i].handler(e);
      } catch (error) {
        console.log('UI dApp failed to process event. App ID: ' + this.mSwarmMessageEventListeners[i].appID)
      }
    }
    //User-Mode notifications - END
  }

  //Kernel-Mode data listener. High-level( 1/2). Fired only if data was found to comprise a valid CNetMsg container.
  onMessage(message) {
    //core processing - BEGIN

    //construct an event
    let e = { // API level 1/2
      swarmID: this.mSwarm.getID,
      peerID: this.getPeerID,
      message: message,
      connection: this
    };
    //notify the agregator (notice that we pass an Event, not the actual message - performance).
    this.mSwarm.onMessage(e);

    //core processing - END

    //User-Mode notifications - BEGIN
    for (let i = 0; i < this.mMessageEventListeners.length; i++) {
      try {
        this.mMessageEventListeners[i].handler(e);
      } catch (error) {
        console.log('UI dApp failed to process event. App ID: ' + this.mMessageEventListeners[i].appID)
      }
    }
    //User-Mode notifications - END
  }

  //Fed directly to WebRTC API. Lowest-Level (0)
  //Kernel-Mode data listener. Low-level. Determines whether data received maked a valid CSwarmMsg container.
  onDataChannelMessageEvent(event) {
    //core processing - BEGIN
    //core processing - END
    //  console.log('Datagram received from peer: ' + gTools.arrayBufferToString(this.mPeerID));

    this.mSwarm.onNewPeerData(this, event.data); // API level 0/2
    this.mSwarm.onDataChannelMessageEvent({
      swarmID: this.mSwarm.getID,
      peerID: this.getPeerID,
      event: event,
      connection: this
    }); //notify the agregator

    //CSwarmMsg Support - BEGIN

    //now, translate to  higher-level notifications (if applicable).

    //first, try to interpret as a CNetMsg.
    let netMsg = CNetMsg.instantiate(event.data);
    netMsg.layer0DeliverySource = this.getIPAddress;

    if (netMsg) {

      this.onMessage(netMsg);

      //second, try to interpret as a CSwarmMsg.
      let swarmMsg = CSwarmMsg.instantiate(netMsg.getData);
      swarmMsg.layer0Datagram = netMsg;
      swarmMsg.protocolID = netMsg.getEntityType;
      if (swarmMsg) {
        this.onSwarmMessage(swarmMsg);
      }
    }
    //CSwarmMsg Support - END

    //core processing - END

    //Dispatch User-mode notifications - BEGIN
    for (let i = 0; i < this.mDataChannelMessageEventListeners.length; i++) {
      try {
        this.mDataChannelMessageEventListeners[i].handler({
          event: event,
          peerID: this.mPeerID,
          swarmID: this.mSwarm.getID,
          connection: this
        });
      } catch (error) {
        console.log('UI dApp failed to process event. App ID: ' + this.mDataChannelMessageEventListeners[i].appID)
      }
    }
    //Dispatch User-mode notifications - END
  }

  onPeerStatusChange(status) {
    //core processing - END
    console.log('Peer: ' + gTools.arrayBufferToString(this.mPeerID) + ' status changed to ' + this.mTools.peerStatusToString(status));

    let e = {
      swarmID: this.mSwarm.getID,
      peerID: this.mPeerID,
      status: status,
      connection: this
    };

    this.mSwarm.onPeerStatusChange(e); // API level 2/2
    //core processing - END

    //Dispatch User-mode notifications - BEGIN
    for (let i = 0; i < this.mPeerStatusEventListeners.length; i++) {
      try {
        this.mPeerStatusEventListeners[i].handler({
          event: event,
          peerID: this.mPeerID,
          swarmID: this.mSwarm.getID,
          connection: this
        });
      } catch (error) {
        console.log('UI dApp failed to process event. App ID: ' + this.mPeerStatusEventListeners[i].appID)
      }
    }
    //Dispatch User-mode notifications - END
  }

  onConnQualityChange(quality) {
    //core processing - END
    //  console.log('Peer: ' + gTools.arrayBufferToString(this.mPeerID) + ' connection quality changed to ' + this.mTools.connQualityToString(quality));

    let e = {
      swarmID: this.mSwarm.getID,
      peerID: this.mPeerID,
      quality: quality,
      connection: this
    };

    this.mSwarm.onConnQualityChange(e); // API level 2/2
    //core processing - END

    //Dispatch User-mode notifications - BEGIN
    for (let i = 0; i < this.mConnectionQualityEventListeners.length; i++) {
      try {
        this.mConnectionQualityEventListeners[i].handler({
          event: event,
          peerID: this.mPeerID,
          swarmID: this.mSwarm.getID,
          connection: this
        });
      } catch (error) {
        console.log('UI dApp failed to process event. App ID: ' + this.mConnectionQualityEventListeners[i].appID)
      }
    }
    //Dispatch User-mode notifications - END
  }

  onSwarmConnectionStateChange(eventData) {
    //core processing - BEGIN
    //this.mSwarm.onSwarmConnectionNewData(eventData); //notify the agregator
    //core processing - END

    for (let i = 0; i < this.mSwarmConnectionStateChangeEventListeners.length; i++) {
      try {
        this.mSwarmConnectionStateChangeEventListeners[i].handler(eventData);
      } catch (error) {
        console.log('UI dApp failed to process event. App ID: ' + this.mSwarmConnectionStateChangeEventListeners[i].appID)
      }
    }
  }
  onDataChannelErrorEvent(event) {
    console.log('Connection errored (Peer ' + gTools.arrayBufferToString(this.mPeerID) + ': ' + event.error.message)
  }

  onDataChannelEvent(event) {

    this.mDataChannel = event.channel;
    this.setNativeConnectionID = event.target.id; //first part to construct a ⋮⋮⋮ Secure Connection ID.
    //core processing - BEGIN
    this.initDataChannelEvents();
    //core processing - END

    //notify external applications - BEGIN
    for (let i = 0; i < this.mDataChannelStateChangeEventListeners.length; i++) {
      try {
        this.mDataChannelStateChangeEventListeners[i].handler({
          event: event,
          peerID: this.mPeerID,
          swarmID: this.mSwarm.getID,
          connection: this
        });
      } catch (error) {
        console.log('UI dApp failed to process event. App ID: ' + this.mDataChannelStateChangeEventListeners[i].appID)
      }
    }
    //notify external applications - END
  }
  set signalingStateChangeTimestamp(time) {
    this.mSignalingStateChangeTimestamp = time;
  }

  get signalingStateChangeTimestamp() {
    return this.mSignalingStateChangeTimestamp;
  }
  onHandleSignalingStateChangeEvent(event) {

    this.signalingStateChangeTimestamp = this.mTools.getTime();

    try {
      for (let i = 0; i < this.mSignalingStateChangeEventListeners.length; i++) {
        this.mSignalingStateChangeEventListeners[i].handler({
          event: event,
          peerID: this.mPeerID,
          swarmID: this.mSwarm.getID,
          connection: this
        });
      }
    } catch (error) {
      console.log('UI dApp failed to process event. App ID: ' + this.mDataChannelStateChangeEventListeners[i].appID)
    }
  }

  //This handler is called whenever onicecandidate occures on the LOCAL WebRTC connection.
  //This is an indication that the local peer needs to deliver an ICE to the remote Peer.
  //It is NOT the event indicating that ICE candidate arrived from remote peer(onRemoteICECandidateEvent)
  onLocalICECandidateEvent(event) {
    if (this.mRTCConnection == null)
      return false;

    //Note: the event'scandidate property CAN be null. Rationale below.
    //If the event's candidate property is null, ICE gathering has finished. This message should not be sent to the remote peer. When this happens, the connection's iceGatheringState has also changed to complete.
    //Source:https://developer.mozilla.org/en-US/docs/Web/API/RTCPeerConnection/onicecandidate
    if (event.candidate == null)
      return false;
    this.onICEReadyForDispatchEvent(JSON.stringify(event.candidate)); //deliver ICE to remote peer

    return true;
  }

  //This handler is called whenever an ICE candidate arrives from a remote Peer. (delivered throug the signaling full-node sub-system)
  onRemoteICECandidateEvent(candidate) {

    if (this.mRTCConnection == null || candidate == null)
      return false;
    this.mRTCConnection.addIceCandidate(candidate)
      .catch(function(e)
      {
        console.log("Failure during addIceCandidate(): " + e.message);
      }.bind(this));
    return true;
  }


  onConnectionStateChangeEvent(e) {
    //Core Processing - BEGIN
    let state = this.getRTCConnectionState;
    console.log("RTC connnection state with '" + this.mTools.arrayBufferToString(this.mPeerID) + "' changed to " + this.mTools.RTCConnectionStateToString(state));
    switch (state) {
      case eRTCConnectionState.connected:

        this.setState = eSwarmConnectionState.connected;
        break;
      case eRTCConnectionState.connecting:
        this.setState = eSwarmConnectionState.negotiating;
        this.mWasConnecting = true;
        break;
      case eRTCConnectionState.closed:
        this.close();
        break;

      case eRTCConnectionState.failed:
        this.setState = eSwarmConnectionState.closed;
        break;
      case eRTCConnectionState.disconnected:
        this.setState = eSwarmConnectionState.closed;
        break;

      default:
        this.setState = eSwarmConnectionState.closed;

    }
    this.mSwarm.onConnectionStateChangeEvent({
      swarmID: this.mSwarm.getID,
      peerID: this.getPeerID,
      event: event,
      connection: this
    }); //notify the agregator

    //Translate to a higher-level event - BEGIN
    this.onSwarmConnectionStateChange({
      swarmID: this.mSwarm.getID,
      peerID: this.getPeerID,
      state: this.getStatus,
      connection: this
    })
    //Translate to a higher-level event - END

    //Core Processing - end

    //External Processing - BEGIN
    for (let i = 0; i < this.mConnectionStateChangeEventListeners.length; i++) {
      try {
        this.mConnectionStateChangeEventListeners[i].handler({
          event: event,
          peerID: this.mPeerID,
          swarmID: this.mSwarm.getID,
          connection: this
        });
      } catch (error) {
        console.log('UI dApp failed to process event. App ID: ' + this.mDataChannelStateChangeEventListeners[i].appID)
      }
    }
    //External Processing - END
  }

  renewRTConnObject() {
    this.mRTCConnection = new RTCPeerConnection();
  }

  onNegotiatioNeeded(event) {

    console.log('RTC full-negotiation is needed.')



    if (event.target.iceConnectionState !== "new") {

      /*
      Documentation: "If the session is modified in a manner that requires negotiation while a negotiation is already in progress,
       no negotiationneeded event will fire until negotiation completes, and only then if negotiation is still needed."

       *THUS*, we do NOT rely on negotiationneeded() for initial negotiation, as the negotiation would need to be taking place twice
       i.e. for each (audio/video/data) tracks as these are added to the connection.

       Here, we are to support consecutive negotiatons ONLY.
      */

      this.mMakingOffer = true;
      this.mRTCConnection.setLocalDescription().then(function(returned) {
          this.onOfferReadyForDispatchEvent(this.getAllowedCapabilities);
        }.bind(this))
        .catch(this.mSwarm.handleGetUserMediaError).finally(function() {
          this.mMakingOffer = false;
        }.bind(this));
    }
    this.mICENegotiationReqCounter++;
  }

  onICEConnectionStateChangeEvent(event) {
    //Core Processing - BEGIN
    console.log("ICE  state with '" + this.mTools.arrayBufferToString(this.mPeerID) + "' changed to " + this.mRTCConnection.iceConnectionState);

    if (this.mRTCConnection.iceConnectionState === "failed") {
      console.log('Error: ICE negotiation failed. Attempting a restart..');
      this.mRTCConnection.restartIce();
    }

    this.mSwarm.onICEConnectionStateChangeEvent({
      swarmID: this.mSwarm.getID,
      peerID: this.getPeerID,
      event: event
    }); //notify the agregator
    //Core Processing - end

    //External Processing - BEGIN

    for (let i = 0; i < this.mICEConnectionStateChangeEventListeners.length; i++) {
      try {
        this.mICEConnectionStateChangeEventListeners[i].handler({
          event: event,
          peerID: this.mPeerID,
          swarmID: this.mSwarm.getID,
          connection: this
        });
      } catch (error) {
        console.log('UI dApp failed to process event. App ID: ' + this.mDataChannelStateChangeEventListeners[i].appID)
      }
    }
    //External Processing - END
  }

  onPeerAuth(result = eSwarmAuthResult.failure, mode = eSwarmAuthMode.none) {
    //Core Processing - BEGIN
    this.mSwarm.onPeerAuth({
      swarmID: this.mSwarm.getID,
      peerID: this.getPeerID,
      event: event,
      connection: this,
      result: result,
      mode: mode
    }); //notify the agregator

    if (this.mNotifyAboutMediaOnAuth) {
      this.notifyAboutStateOfMedia();
    }
    //Core Processing - end


    for (let i = 0; i < this.mPeerAuthenticationResultListeners.length; i++) {
      try {
        this.mPeerAuthenticationResultListeners[i].handler({
          event: event,
          peerID: this.mPeerID,
          swarmID: this.mSwarm.getID,
          connection: this,
          result: result,
          mode: mode
        });
      } catch (error) {
        console.log('UI dApp failed to process event. App ID: ' + this.mPeerAuthenticationResultListeners[i].appID)
      }
    }

  }

  get ingressStream() {
    return this.mIngressStream;
  }

  set ingressStream(stream) {
    this.mIngressStream = stream;
  }

  onTrackEvent(event) {
    //Core Processing - BEGIN

    console.log('Applying ' + this.mSwarm.getEffectiveOutgressCapabilities + " initial effective capabilities for " + gTools.arrayBufferToString(this.getPeerID));
    this.setEffectiveCapabilities(this.mSwarm.getEffectiveOutgressCapabilities, true); //when a new peer connects, mute the Dummy Streams (prevent beeping on the other end).

    if (event.track.kind === 'audio') {
      this.ingressAudioTrack = event.track;
    } else if (event.track.kind === 'video') {
      this.ingressVideoTrack = event.track;
    } else {

    }

    if (event.streams.length) {
      this.ingressStream = event.streams[0];
    }
    this.mSwarm.onTrack({
      swarmID: this.mSwarm.getID,
      peerID: this.getPeerID,
      event: event,
      connection: this
    }); //notify the agregator
    //Core Processing - end



    for (let i = 0; i < this.mTrackEventListeners.length; i++) {
      try {
        this.mTrackEventListeners[i].handler({
          event: event,
          peerID: this.mPeerID,
          swarmID: this.mSwarm.getID,
          connection: this
        });
      } catch (error) {
        console.log('UI dApp failed to process event. App ID: ' + this.mDataChannelStateChangeEventListeners[i].appID)
      }
    }

  }

  // takes capabilities describing ceiling capabilities during the data-flow
  onOfferReadyForDispatchEvent(capabilities = eConnCapabilities.audioVideo) {
    if (this.mSwarm == null || this.mSwarm.mSwarmManager == null)
      return false;
    //notice that this SDP datagram is routed to a specific peer, only.
    let sdp = new CSDPEntity(gCrypto, eSDPEntityType.processOffer, this.mSwarm.getMyID, this.mPeerID, JSON.stringify(this.mRTCConnection.localDescription), this.SDPSessionID);
    sdp.setCapabilities = capabilities;
    sdp.setSwarmID = this.mSwarm.getID;
    return this.mSwarm.mSwarmManager.routeSDPEntity(sdp); //note, additional (optional) authentication will be provided through the swarm-manager);
  }

  onICEReadyForDispatchEvent(candidate) {
    if (this.mSwarm == null || this.mSwarm.mSwarmManager == null)
      return false;
    //notice that this SDP datagram is routed to a specific peer, only.
    let sdp = new CSDPEntity(gCrypto, eSDPEntityType.processICE, this.mSwarm.getMyID, this.mPeerID, candidate, this.SDPSessionID);
    sdp.setSwarmID = this.mSwarm.getID;
    return this.mSwarm.mSwarmManager.routeSDPEntity(sdp); //note, additional (optional) authentication will be provided through the swarm-manager);
  }

  onOfferAnswerReadyForDispatchEvent() {
    if (this.mSwarm == null || this.mSwarm.mSwarmManager == null)
      return false;
    //notice that this SDP datagram is routed to a specific peer, only.
    let sdp = new CSDPEntity(gCrypto, eSDPEntityType.processOfferResponse, this.mSwarm.getMyID, this.mPeerID, JSON.stringify(this.mRTCConnection.localDescription), this.SDPSessionID);
    sdp.setSwarmID = this.mSwarm.getID;
    return this.mSwarm.mSwarmManager.routeSDPEntity(sdp); //note, additional (optional) authentication will be provided through the swarm-manager);
  }

  //WebRTC Native-events support  (CORE)- END
  /*
  The aim of thus function is to infer state of current media-tracks and let the remote peer know.
  */
  notifyAboutStateOfMedia(protocolID = 0) {
    //Local Variables - BEGIN
    let liveVideoTrack = this.mSwarm.getLIVEVideoTrack;
    let liveAudioTrack = this.mSwarm.getLIVEAudioTrack;
    let realAudioOn = false;
    let realVideoOn = false;
    let msg, wrapper, serializedNetMsg;
    //Local Variables - END

    //Operational Logic - BEGIN
    if (liveAudioTrack && liveAudioTrack.readyState == 'live' && !liveAudioTrack.getVirtualDev) {
      realAudioOn = true;
    }
    if (liveVideoTrack && liveVideoTrack.readyState == 'live' && !liveVideoTrack.getVirtualDev) {
      realVideoOn = true;
    }

    msg = new CSwarmMsg(!realVideoOn ? eSwarmMsgType.mutedCam : eSwarmMsgType.unmutedCam, this.mSwarm.getMyID); //covers both camera and screen sharing.
    wrapper = new CNetMsg(protocolID, eNetReqType.notify, msg.getPackedData());
    serializedNetMsg = wrapper.getPackedData();
    this.send(serializedNetMsg, true);

    msg = new CSwarmMsg(!realAudioOn ? eSwarmMsgType.mutedAudio : eSwarmMsgType.unmutedAudio, this.mSwarm.getMyID);
    wrapper = new CNetMsg(protocolID, eNetReqType.notify, msg.getPackedData());
    serializedNetMsg = wrapper.getPackedData();
    this.send(serializedNetMsg, true);
    //Operational Logic - END

  }
  close() {


    if (this.mRTCConnection) {
      try {
        this.mRTCConnection.close();
      } catch (err) {}
      this.mRTCConnection.ontrack = null;
      this.mRTCConnection.onremovetrack = null;
      this.mRTCConnection.onremovestream = null;
      this.mRTCConnection.onicecandidate = null;
      this.mRTCConnection.oniceconnectionstatechange = null;
      this.mRTCConnection.onsignalingstatechange = null;
      this.mRTCConnection.onicegatheringstatechange = null;
      this.mRTCConnection.onnegotiationneeded = null;

      this.mRTCConnection = null;
    }
    this.setState = eSwarmConnectionState.closed;
    let connection = this.mSwarm.getConnection(this.mPeerID);
    if (connection != null) {
      this.mSwarm.removeConnection(this.mPeerID);
    }
    //Core Processing - END

  }

  setLocalDummyStream(stream) {
    this.mLocalDummyStream = stream;

    //notify
    //this.mSwarm.mSwarmManager.onLocalStream(stream, this.mSwarm.getID); ficilitated through SwarmManager itself;it already knows about it.
  }

  get getLocalDummyStream() {
    return this.mLocalDummyStream;
  }

  isDummyStreamReady() {
    let tracks = this.getLocalDummyStream.getTracks();
    if (tracks.length < 2)
      return false;

    for (let i = 0; i < tracks.length; i++) {
      if (tracks[i] == null || !(tracks[i].readyState == 'live'))
        return false;
    }
    return true;
  }

  setLocalStream(stream) {
    this.mLocalStream = stream;

    //notify
    //this.mSwarm.mSwarmManager.onLocalStream(stream, this.mSwarm.getID); ficilitated through SwarmManager itself;it already knows about it.
  }

  get getLocalStream() {
    return this.mLocalStream;
  }

  get getDataChannel() {
    return this.mDataChannel;
  }

  setDataChannel(c) {
    this.mDataChannel = c;
  }

  get getPeerID() {
    return this.mPeerID;
  }

  setPeerID(id) {
    this.mPeerID = id;
  }

  setStatus(status) {
    //Core processing - BEGIN
    this.mStatus = status;
    let newEvent = {
      peerID: this.mPeerID,
      swarmID: this.mSwarm.getID,
      status: this.mStatus,
      connection: this
    };

    this.mStatusChangeTimestamp = gTools.getTime();
    this.mSwarm.onSwarmConnectionStateChange(newEvent); //notifi the master-events handler

    //Core processing - END

    //notify

    for (let i = 0; i < this.mSwarmConnectionStateChangeEventListeners.length; i++) {
      try {
        this.mSwarmConnectionStateChangeEventListeners[i].handler(newEvent);
      } catch (error) {
        console.log('UI dApp failed to process event. App ID: ' + this.mSwarmConnectionStateChangeEventListeners[i].appID)
      }
    }
  }

  get getRTCConnection() {
    return this.mRTCConnection;
  }

  get getRTCConnectionState() {
    switch (this.mRTCConnection.connectionState) {
      case 'new':
        return eRTCConnectionState.new;
        break;
      case 'connecting':
        return eRTCConnectionState.connecting;
        break;
      case 'connected':
        return eRTCConnectionState.connected;
        break;
      case 'disconnected':
        return eRTCConnectionState.disconnected;
        break;
      case 'failed':
        return eRTCConnectionState.failed;
        break;
      case 'closed':
        return eRTCConnectionState.closed;
        break;
      default:
        return eRTCConnectionState.unknown;

    }
  }

  get getRTCSignalingState() {
    switch (this.mRTCConnection.signalingState) {
      case 'stable':
        return eRTCSignalingState.stable;
        break;
      case 'have-local-offer':
        return eRTCSignalingState.haveLocalOffer;
        break;
      case 'have-remote-offer':
        return eRTCSignalingState.haveRemoteOffer;
        break;
      case 'have-local-pranswer':
        return eRTCSignalingState.answerCreated;
        break;
      case 'have-remote-pranswer':
        return eRTCSignalingState.furtherNegotiation;
        break;
      default:
        return eRTCSignalingState.unknown;
    }
  }

  get getStatus() {
    return this.mStatus;
  }


  get getLocalSDPOffer() {
    return this.mRecentLocalICE;
  } //can't be set explicitly, (setter is a deleted funciton)

  /*  onLocalICECandidateEvent(e) { //the local machine generated an ICE candidate
      if (e.candidate == null) {
        let ice = JSON.stringify(this.mRTCConnection.localDescription);
        //the offer now needs to be delivered to the other Swarm-participant
        // constructor(cryptoFactory, type, source, destination, SIPData) {
        this.mRecentLocalICE = ice;
        let sdp = new CSDPEntity(gCrypto, eSDPEntityType.processICE, this.mSwarm.getMyID, this.getPeerID, ice)
        this.mSwarm.mSwarmManager.routeSDPEntity(sdp); //note, additional (optional) authentication will be provided through the swarm-manager
      }
    }*/

} //Swarm Connection - END
