'use strict';
import {
  CNetMsg,
  CDFSMsg
} from './NetMsg.js'

import {
  CSDPEntity,
} from './SDPEntity.js'

import {
  CSwarm,
} from './swarm.js'

var displayMediaOptions = {
  video: {
    cursor: "always"
  },
  audio: false
};

export class CVirtualCamDev {
  constructor(widthP = 640, heightP = 480) {
    this.mEnabled = true;
    this.mTrack = ({
      width = widthP,
      height = heightP
    } = {}) => {
      let canvas = Object.assign(document.createElement("canvas"), {
        width,
        height
      });
      canvas.getContext('2d').fillRect(0, 0, width, height);
      let stream = canvas.captureStream(25);
      return Object.assign(stream.getVideoTracks()[0], {
        enabled: this.mEnabled
      });
    }
    this.mTrack = this.mTrack();
    this.mTrack.getVirtualDev = this;

    Object.defineProperty(this.mTrack, "enabled", {
      get: function() {
        return this.enabled
      }.bind(this),
      set: function(isIt) {
        this.enabled = isIt
      }.bind(this)
    });

  }

  set enabled(isIt) {
    this.mEnabled = isIt;
  }
  get enabled() {
    return this.mEnabled;
  }

  get getTrack() {
    return this.mTrack;
  }
}
export class CVirtualAudioDev {
  constructor() {
    this.mMuted = false;

    this.mWasOscillatorEverStarted = false; //an oscillator can be started *ONLY ONCE*. Then it CAN be attached/detached.
    // We instance the class, create the context
    this.mAudioCtx = new AudioContext();
    // Create the oscillator
    this.mOscillator = this.mAudioCtx.createOscillator();
    // Define type of wave
    this.mOscillator.type = 'sine';
    // We create a gain intermediary
    this.mVolumeDev = this.mAudioCtx.createGain();
    this.setVolume = 0.1; //only now can we set the volume.
    // We connect the oscillator with the gain knob

    // Then connect the volume to the context destination
    //  this.mVolumeDev.connect(this.mAudioCtx.destination);
    // We can set & modify the gain knob
    this.mVolumeDev.gain.value = this.mVolumeVal;

    //We can test it with some frequency at current time
    this.mOscillator.frequency.setValueAtTime(440.0, this.mAudioCtx.currentTime);

    let dst = this.mVolumeDev.connect(this.mAudioCtx.createMediaStreamDestination());

    if (!this.startOscillator()) //it would be connected over here as well.
      return;

    this.mTrack = Object.assign(dst.stream.getAudioTracks()[0], {
      enabled: this.mEnabled
    });


    Object.defineProperty(this.mTrack, "enabled", {
      get: function() {
        return this.enabled
      }.bind(this),
      set: function(isIt) {
        this.enabled = isIt
      }.bind(this)
    });

    this.mTrack.getVirtualDev = this;

  }

  startOscillator() {

    if (this.mEnabled) {
      CTools.getInstance().logEvent('an attempt to initialize an already active oscillator!', eLogEntryCategory.localSystem, 1, eLogEntryType.warning);
      return false;
    }

    this.mOscillator.connect(this.mVolumeDev);

    if (!this.mWasOscillatorEverStarted) { //this can be done ONLY ONCE. Then we can only attach/detach.
      if (this.mOscillator.start) {
        this.mOscillator.start();
        this.mWasOscillatorEverStarted = true;
      } else if (this.mOscillator.noteOn) {
        this.mOscillator.noteOn(0);
        this.mWasOscillatorEverStarted = true;
      } else {
        CTools.getInstance().logEvent('Critical error: unable to initialize the Microphone Virtual Device!', eLogEntryCategory.localSystem, 1, eLogEntryType.error);
        return false;
      }
    }

    this.mEnabled = true;
    return true;
  }

  stopOscillator(onlyDetach = true) {
    if (!this.mEnabled) {
      CTools.getInstance().logEvent('an attempt to stop an inactive oscillator!', eLogEntryCategory.localSystem, 1, eLogEntryType.warning);
      return true;
    }
    if (!onlyDetach) { //we usually want to ONLY detach the oscillator. As start() can be invoked only ONCE per its lifetime.
      if (this.mOscillator.stop) {
        this.mOscillator.stop(0);
      } else if (this.mOscillator.noteOff) {
        this.mOscillator.noteOff(0);
      } else {
        CTools.getInstance().logEvent('unable to communicate with the Virtual Microphone Device!', eLogEntryCategory.localSystem, 1, eLogEntryType.error);
        return false;
      }
    }

    this.mOscillator.disconnect();
    this.mEnabled = false;

    return true;
  }

  //after a track is 'stopped', there's no way of ressuscitating it.
  renew() {
    this.mTrack.stop();
    this.mTrack = Object.assign(dst.stream.getAudioTracks()[0], {
      enabled: this.mEnabled
    });
    this.mTrack.getVirtualDev = this;

  }
  get muted() {
    return this.mMuted;
  }

  set muted(isIt) {
    this.mMuted = isIt;
    if (isIt) {
      this.setVolume = 0;
    } else {
      this.setVolume = this.mVolumeValBeforeMuted;
    }

  }

  get getTrack() {
    return this.mTrack
  }
  set setVolume(val) {
    this.mVolumeVal = val;
    this.mVolumeValBeforeMuted = val;
    this.mVolumeDev.gain.value = val;
    if (val == 0) {
      this.mMuted = true;
    }
  }

  get getVolume() {
    return this.mVolumeVal;
  }

  //Either enables or disables the internal oscillator.

  get enabled() {
    return this.mEnabled;
  }
  set enabled(isIt) {
    if (isIt) {
      CTools.getInstance().logEvent('enabling the Virtual Microphone device..', eLogEntryCategory.localSystem, 1, eLogEntryType.notification);
      this.startOscillator();
    } else {
      CTools.getInstance().logEvent('disabling the Virtual Microphone device..', eLogEntryCategory.localSystem, 1, eLogEntryType.notification);
      this.stopOscillator();
    }

  }
}

export class CSwarmsManager {

  static getInstance(vmContext) {
    if (CSwarmsManager.sInstance == null) {
      CSwarmsManager.sInstance = new CSwarmsManager(vmContext);
    }

    return CSwarmsManager.sInstance;
  }

  constructor(vmContext) {
    this.mVMContext = vmContext;
    this.mSwarms = [];
    this.mTools = CTools.getInstance();
    this.mSwarmDataListeners = [];
    this.mLatestSeqNr = 0;
    this.mTokenPool = null; //token pool used for authentication
    this.mPrivKey = null; //private key used for authentication
    this.mDoAuth = true;
    this.mTPBankIDToUse = 0;
    this.mLocalStreamListeners = [];
    this.mLocalStream = null;
    //  this.mAllowedByCurrentLocalStream = 0;
    this.mDefaultOutgressCapabilities = eConnCapabilities.data;
    //  this.mCurrentCeilingCapabilities = this.mDefaultOutgressCapabilities;
    this.mScreenStream = null;

  }

  get getDefaultOutgressCapabilities() {
    return this.mDefaultOutgressCapabilities;
  }


  get getLocalStream() {
    return this.mLocalStream;
  }
  set setLocalStream(stream) {
    this.mLocalStream = stream;
  }

  addLocalStreamEventListener(eventListener, appID = 0) {
    this.mLocalStreamListeners.push({
      handler: eventListener,
      appID
    });
  }

  unregisterEventListenersByAppID(appID = 0, eventListener = null) {
    let result = false;


    //Core Dependancies - BEGIN

    //Swarm Connectins - BEGIN
    for (let i = 0; i < this.mSwarms.length; i++) {
      let connections = this.mSwarms[i].peers;
      for (let y = 0; y < connections.length; y++) {
        connections[y].unregisterEventListenersByAppID(appID, eventListener);
      }
    }
    //Swarm Connectins - END

    //Swarms - BEGIN
    for (let i = 0; i < this.mSwarms.length; i++) {
      this.mSwarms[i].unregisterEventListenersByAppID(appID, eventListener);
    }
    //Swarms - END

    //Core Dependancies - END

    //Swarm Manager Callbacks - BEGIN
    if (appID == 0 && eventListener == null)
      return result;
    let eventQueues = [
      this.mSwarmDataListeners,
      this.mLocalStreamListeners
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
    //Swarm Manager Callbacks - END
    return result;
  }

  //Sets effective outgress capabilities.
  //Can be used to mute microphone or disable video output to all immediate peers.
  //It does not affect the presence of tracks themselves.
  setEffectiveOutgressCapabilities(allowed, swarmID = null) {
    let swarm = null;

    if (swarmID != null) {
      swarm = this.findSwarmByID(swarmID);
      if (swarm != null)
        swarm.setEffectiveOutgressCapabilities(allowed);
    } else {
      for (let i = 0; i < this.mSwarms.length; i++) {
        this.mSwarms[i].setEffectiveOutgressCapabilities(allowd);
      }
    }
  }

  //Releasing Resources Support - BEGIN
  //Notice: these functions are meant to actually STOP i.e. relase particular device fromt the web-browser.
  // stop both mic and camera
  stopBothVideoAndAudio(stream) {
    if (!stream)
      return;
    stream.getTracks().forEach(function(track) {
      if (track.readyState == 'live') {
        track.stop();
      }
    });
  }

  // stop only camera.
  stopVideoOnly(stream) {
    if (!stream)
      return;
    stream.getTracks().forEach(function(track) {
      if (track.readyState == 'live' && track.kind === 'video') {
        track.stop();
      }
    });
  }

  // stop only mic.
  stopAudioOnly(stream) {
    if (!stream)
      return;
    stream.getTracks().forEach(function(track) {
      if (track.readyState == 'live' && track.kind === 'audio') {
        track.stop();
      }
    });
  }


  //stop SCREEN capture.
  stopCapture() {
    if (this.mScreenStream == null)
      return;
    let tracks = this.mScreenStream.getTracks();

    tracks.forEach(track => track.stop());

  }

  //Releasing Resources Support - END


  //(Re)Attaching of Resources Support - BEGIN

  async startCapture() {

    try {
      let alreadyActive = false;
      //check if screen capture already active
      let tracks = [];
      if (this.mScreenStream) {
        tracks = this.mScreenStream.getVideoTracks();
      }
      if (tracks.length > 0) {
        for (let i = 0; i < tracks.length; i++) {
          if (tracks[i].kind == 'video' && tracks[i].readyState == "live") {
            alreadyActive = true;
            CTools.getInstance().logEvent(`won't ask for a Screen Device as device already in use..`, eLogEntryCategory.localSystem, 1, eLogEntryType.notification);

            break;
          }
        }
      }

      if (!alreadyActive) {
        //this.stopCapture();
        CTools.getInstance().logEvent(`I'm about to ask for a Screen Device..`, eLogEntryCategory.localSystem, 1, eLogEntryType.notification);

        let stream = await navigator.mediaDevices.getDisplayMedia(displayMediaOptions);

        if (this.mScreenStream) {
          //clean it - BEGIN
          let newTracks = stream.getTracks();
          let oldTracks = this.mScreenStream.getTracks();

          if (oldTracks.length) {
            let index = oldTracks.length - 1;
            while (index >= 0) {
              if (oldTracks[index].readyState != 'live') {
                this.mScreenStream.removeTrack(oldTracks[index]);
                oldTracks.splice(index, 1);
              }
              index -= 1;
            }
          }
          //clean it - END

          //add new tracks - BEGIN
          newTracks.forEach(function(track) {
            CTools.getInstance().logEvent('adding a new ' + track.kind + ' track' + ' ("' + track.label + '")' + ' to the screen data feed.', eLogEntryCategory.localSystem, 1, eLogEntryType.notification);

            this.mScreenStream.addTrack(track); //notice - multiple tracks - even of same kind are thus possible. We do our best to elimiante inactive tracks though.
          }.bind(this));
          //add new tracks - END
        } else {
          this.mScreenStream = stream;
        }

      }

      if (this.mScreenStream != null && this.mScreenStream.getVideoTracks().length > 0) {
        return this.mScreenStream;
      }
      return null;

    } catch (err) {
      this.mScreenStream = null;
      return Promise.reject(new Error("Unable to start Screen Capture."));
    }
  }
  //Notice: these functions are meant to actually STOP i.e. relase particular device fromt the web-browser.
  // stop both mic and camera
  async startBothMicAndCam() {
    this.stopBothVideoAndAudio(this.mLocalStream); //release the resources, just in case.prevent zombie handles.
    let stream = await this.getAllowedMedia(eConnCapabilities.audioVideo);
    return stream;
  }

  // resume only camera.
  async startCamOnly() {
    this.stopVideoOnly(this.mLocalStream); //release the resources, just in case.prevent zombie handles.
    let stream = await this.getAllowedMedia(eConnCapabilities.video);
    return stream;
  }

  // resume only mic.
  async startMicOnly() {
    this.stopAudioOnly(this.mLocalStream); //release the resources, just in case.prevent zombie handles.
    let stream = await this.getAllowedMedia(eConnCapabilities.audio);
    return stream;
  }

  //(Re)Attaching of Resources Support - END

  //The aim of the function is to optimize resources (video/audio feeds - being requested from the underlying web-browser).
  //(ex. to release the camera entirely,- when it is not in use by any UI dApp). Note that this might be of a paramount importance
  //to the user's overall privacy-related wellbeing (i.e. web-cam LED indicator turned off when webcam is not supposed to be in use).
  optimizeRequestedResources() {
    CTools.getInstance().logEvent("Optimizing media hardware utilization..", eLogEntryCategory.localSystem, 1, eLogEntryType.notification);
    //Local Variables - BEGIN
    let activeHardwareCapabilities = this.getActiveHardwareCapabilities(); //*IMPORTANT* - check currently active hardware handles.
    let audioRequested = false;
    let videoRequested = false;
    let audioAvailable = false;
    let videoAvailable = false;
    //Local Variables - END

    //now check what's actually being requested by Swarms, in the end we need to satify dApps' requirements BUT ensure That
    //EXCESSIVE hardware resouces are NOT being provided and/or occupied.
    for (let i = 0; i < this.mSwarms.length; i++) {
      //capabilitiesRequestedBySwarm = this.mSwarms[i].getEffectiveOutgressCapabilities;
      //^ Notice: the current effective outgress capabilities should be of no significance (ex. since a Dummy Track might be playing.)
      // what DOES matter is whether the particular Swarm indicates its will to use either a webcam, microphone or both.
      // Here, we optimize hardware utilizatian, NOT swarms' CAPABILITIES (effective or allowed).
      if (this.mSwarms[i].getCamInUse) {
        videoRequested = true;
      }

      if (this.mSwarms[i].getMicInUse) {
        audioRequested = true;
      }
    }

    //translate, i.e. decouple active hardware capabilities into seperate boolean flags.
    switch (activeHardwareCapabilities) {
      case eConnCapabilities.audio:
        audioAvailable = true;
        break;
      case eConnCapabilities.video:
        videoAvailable = true;
        break;
      case eConnCapabilities.audioVideo:
        audioAvailable = true;
        videoAvailable = true;
        break;
      default:
        audioAvailable = false;
        videoAvailable = false;
    }

    //compare hardware currently in use against hardware that was actually requested and - and optimize hardware utilization..
    if (audioAvailable && !audioRequested) {
      //^ we make the decision based on actual usage of hardware resources - i.e. not the webrtc capabilities:  if (audioInUse && !audioRequested) {
      //release microphone -  from the web-browser as it is not needed as of now.
      this.stopAudioOnly(this.mLocalStream);
      CTools.getInstance().logEvent("Releasing microphone as it is no longer needed..", eLogEntryCategory.localSystem, 1, eLogEntryType.notification);
      //return true; //we've optimized access to native resources.
    }

    if (videoAvailable && !videoRequested) {
      //^ we make the decision based on actual usage of hardware resources - i.e. not webrtc capabilities:  if (videoInUse && !videoRequested) {
      //release web-cam - from the web-browser as it is not needed as of now.
      this.stopVideoOnly(this.mLocalStream);
      CTools.getInstance().logEvent("Releasing web-cam as it is no longer needed..", eLogEntryCategory.localSystem, 1, eLogEntryType.notification);
      //return true; //we've optimized access to native resources.
    }
  }
  //Sets effective ingress capabilities.
  //Can be used to mute microphone or disable video output to all immediate peers.
  //It does not affect the presence of tracks themselves.
  setEffectiveIngressCapabilities(allowed, swarmID = null) {
    let swarm = null;

    if (swarmID != null) {
      swarm = this.findSwarmByID(swarmID);
      if (swarm != null) {
        swarm.setEffectiveIngressCapabilities(allowed);
      }
    } else {
      for (let i = 0; i < this.mSwarms.length; i++) {
        this.mSwarms[i].setEffectiveIngressCapabilities(allowd);
      }
    }
  }

  //Set ceiling capabilities to either a single swarm or all of them (swarmID=null)
  //takes eConnCapabilities. These capabilities affect connections with all immediate swarms'-peers.
  setAllowedCapabilities(allowed, swarmID = null) {
    let swarm = null;

    if (swarmID != null) {
      swarm = this.findSwarmByID(swarmID);
      if (swarm != null)
        swarm.setAllowedCapabilities(allowed);
    } else {
      for (let i = 0; i < this.mSwarms.length; i++) {
        this.mSwarms[i].setAllowedCapabilities(allowd);
      }
    }
  }



  async getAllowedMedia(allowed) {
    if (allowed == 0)
      return null; //only DATA channel is being requested thus we DONT perform media queries

    if (this.getActiveHardwareCapabilities == allowed)
      return this.mLocalStream;
    CTools.getInstance().logEvent("Requesting media hardware..", eLogEntryCategory.localSystem, 1, eLogEntryType.notification);

    let allowedRTCCapabilities = gTools.swarmCapabilitiestoRTCCapabilities(allowed);
    try {
      let stream = await navigator.mediaDevices.getUserMedia(allowedRTCCapabilities);

      //Update the local stream - BEGIN (if present)
      //Rationale: we NEED to keep handles to previously requested and received hardware resources so that we can diables these when no longer needed.

      if (this.mLocalStream) {

        //clean it - BEGIN
        let newTracks = stream.getTracks();
        let oldTracks = this.mLocalStream.getTracks();

        if (oldTracks.length) {
          let index = oldTracks.length - 1;
          while (index >= 0) {
            if (oldTracks[index].readyState != 'live') {
              this.mLocalStream.removeTrack(oldTracks[index]);
              oldTracks.splice(index, 1);
            }
            index -= 1;
          }
        }
        //clean it - END

        //add new tracks - BEGIN
        newTracks.forEach(function(track) {
          if (track.muted) {
            CTools.getInstance().logEvent("[WARNING]: " + (track.kind === 'video' ? "camera" : "microphone") + " is MUTED in local system settings!", eLogEntryCategory.localSystem, 1, eLogEntryType.warning);
          }
          CTools.getInstance().logEvent('adding a new ' + track.kind + ' track' + ' ("' + track.label + '")' + ' to the local data feed.', eLogEntryCategory.localSystem, 1, eLogEntryType.notification);

          this.mLocalStream.addTrack(track); //notice - multiple tracks - even of same kind are thus possible. We do our best to elimiante inactive tracks though.
        }.bind(this));
        //add new tracks - END
      }
      //Update the local stream - END

      if (this.mLocalStream)
        this.onLocalStream(this.mLocalStream, null);
      else {
        this.onLocalStream(stream, null);
      }
      return this.mLocalStream;
    } catch (err) {
      /* handle the error */

      throw new Error('web-browser did not agree to provide the requested media devices.');
    }

  }

  /*  async getAllowedMedia(allowed) {
    if (allowed == 0)
      return null; //only DATA channel is being requested thus we DONT perform media queries

    if (this.getActiveHardwareCapabilities == allowed)
      return this.mLocalStream;
    //return navigator.mediaDevices.getUserMedia(gTools.swarmCapabilitiestoRTCCapabilities(allowed))
    navigator.mediaDevices.enumerateDevices()
    .then(
      function (devices) {
        let videoDevices = [];
        let videoDeviceIndex = 0;
        devices.forEach(function(device) {
        //console.log(device.kind + ": " + device.label +
        //    " id = " + device.deviceId);
        if (device.kind == "videoinput") {

            videoDevices.push({deviceId: device.deviceId, label:device.label});
            videoDeviceIndex++;
        }
      }.bind(this));

    this.mLastCamIndex = 0;
    console.log('Selecting ' + videoDevices[this.mLastCamIndex].label + ' for video input.');
    return navigator.mediaDevices.getUserMedia(gTools.swarmCapabilitiestoRTCCapabilities(allowed,videoDevices[this.mLastCamIndex].deviceId));

  }.bind(this))
    .then(function(stream) {
      this.onLocalStream(stream, null);
      return stream;
    }.bind(this));
  }
  */
  onLocalStream(stream, swarmID) {

    //if local stream exists then ONLY UPDATE tracks?..no.. we replace the entire object.. there's no 'removeTrack' function...
    //no listeners are supposed to listen to a muted track anyway...
    this.setLocalStream = stream;
    if (this.mLocalStream != null) {
      this.setLocalStreamEffectiveCapabilities();
      this.mLocalStream.onaddtrack = this.onLocalTrackHandler.bind(this);
    }

    for (let i = 0; i < this.mLocalStreamListeners.length; i++) {
      try {
        this.mLocalStreamListeners[i].handler({
          swarmID: swarmID,
          stream
        });
      } catch {
        CTools.getInstance().logEvent('UI dApp failed to process event. App ID: ' + this.mLocalStreamListeners[i].appID, eLogEntryCategory.localSystem, 1, eLogEntryType.error);

      }
    }

  }
  onLocalTrackHandler(event) {
    this.setLocalStreamEffectiveCapabilities();
  }


  getEffectiveOutgressCapabilities() {
    let audioPlaying = false;
    let videoPlaying = false;
    if (this.mLocalStream == null)
      return 0;
    let tracks = this.mLocalStream.getTracks();
    if (tracks.length == 0)
      return 0;

    for (let i = 0; i < tracks.length; i++) {
      if (tracks[i] == null)
        continue;
      if (tracks[i].kind == 'audio') {
        if (tracks[i].enabled)
          audioPlaying = true;
      } else {
        if (tracks[i].kind == 'video') {
          if (tracks[i].enabled)
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

  getActiveHardwareCapabilities() {
    let micEnabled = false;
    let camEnabled = false;
    if (this.mLocalStream == null)
      return 0;
    let tracks = this.mLocalStream.getTracks();
    if (tracks.length == 0)
      return 0;

    for (let i = 0; i < tracks.length; i++) {
      if (tracks[i] == null)
        continue;
      if (tracks[i].kind == 'audio') {
        if (tracks[i].readyState == "live")
          micEnabled = true;
      } else {
        if (tracks[i].kind == 'video') {
          if (tracks[i].readyState == "live")
            camEnabled = true;
        }
      }
    }

    if (micEnabled && camEnabled)
      return eConnCapabilities.audioVideo;
    else if (camEnabled)
      return eConnCapabilities.video;
    else if (micEnabled)
      return eConnCapabilities.audio;
    else return eConnCapabilities.data;
  }
  setLocalStreamEffectiveCapabilities(capabilities = -1) {
    let applied = false;
    if (this.mLocalStream == null)
      return;
    capabilities = (capabilities == -1) ? this.getCapabilitiesRequestedBySwarms : capabilities;

    let tracks = this.mLocalStream.getTracks();

    for (let i = 0; i < tracks.length; i++) {
      if (tracks[i] == null)
        continue;
      switch (capabilities) {
        case eConnCapabilities.data:
          if (tracks[i].kind == 'audio')
            tracks[i].enabled = false;
          if (tracks[i].kind == 'video')
            tracks[i].enabled = false;
          applied = true;
          break;
        case eConnCapabilities.audio:
          if (tracks[i].kind == 'audio')
            tracks[i].enabled = true;
          if (tracks[i].kind == 'video')
            tracks[i].enabled = false;
          applied = true;
          break;
        case eConnCapabilities.video:
          if (tracks[i].kind == 'audio')
            tracks[i].enabled = false;
          if (tracks[i].kind == 'video')
            tracks[i].enabled = true;
          applied = true;
          break;
        case eConnCapabilities.audioVideo:
          if (tracks[i].kind == 'audio')
            tracks[i].enabled = true;
          if (tracks[i].kind == 'video')
            tracks[i].enabled = true;
          applied = true;
          break;
      }
    }


  }

  addSwarm(swarm, doInit = true) {

    if (swarm == null)
      return false;

    if (this.findSwarmByID(swarm.getID))
      return false;
    CTools.getInstance().logEvent("Signed up for Swarm ''" + gTools.arrayBufferToString(swarm.trueID) + "'", eLogEntryCategory.localSystem, 1, eLogEntryType.notification);
    this.mSwarms.push(swarm);

    if (doInit)
      swarm.initialize();
    return true;
  }

  get getSwarmsCount() {
    return this.mSwarms.length;
  }
  //if recipientID == null then delivers to all members
  //if swarmID == null, then delivers to all active swarms
  sendData(data, recipientID, swarmID) {
    if (data == null)
      return false;

    if (swarmID == null) {
      for (let i = 0; i < this.mSwarms.length; i++) {
        this.mSwarms[i].sendData(data, recipientID);
      }
    } else {
      let swarm = this.findSwarmByID(swarmID);
      if (swarm == null)
        return false;

      swarm.sendData(data, recipientID);
    }
  }


  findSwarmByID(id) {
    if (id == null)
      return nullptr;
    id = gTools.convertToArrayBuffer(id);
    for (let i = 0; i < this.mSwarms.length; i++) {
      if (gTools.compareByteVectors(this.mSwarms[i].getID, id) || gTools.compareByteVectors(this.mSwarms[i].trueID, id))
        return this.mSwarms[i];
    }
    return null;
  }

  set setTPBankIDToUse(id) {
    this.mTPBankIDToUse = id;
  }

  get getTPBankIDToUse() {
    return this.mTPBankIDToUse;
  }

  addSwarmDataListener(listener) {

    if (eventListener == null)
      return false;

    this.mSwarmDataListeners.push({
      handler: eventListener,
      appID
    });

    return true;
  }

  set setTokenPool(pool) {
    this.mTokenPool = pool;
  }

  get getTokenPool() {
    return this.mTokenPool;
  }

  set setPrivKey(privKey) {
    this.mPrivKey = privKey;
  }

  get getPrivKey() {
    return this.mPrivKey;
  }

  get getDummyVideoStream() {
    return this.mDummyStream;
  }

  initialize() {
    this.mVMContext.addNewSwarmSDPMsgListener(this.newSwarmSDPMsgCallback.bind(this), this.mID);
    this.mVMContext.addConnectionStatusChangedListener(this.connectionStatusChangedCallback.bind(this));
    this.mVirtualAudioDevice = new CVirtualAudioDev();
    this.mVirtualCamDevice = new CVirtualCamDev(640, 480);

    let track = null;
    let args = [];
    this.mVirtualAudioDevice = new CVirtualAudioDev();
    this.mVirtualCamDevice = new CVirtualCamDev(640, 480);
    CTools.getInstance().logEvent('Initializing new Virtual Audio/Video stream..', eLogEntryCategory.localSystem, 1, eLogEntryType.notification);
    this.mDummyStream = new MediaStream([this.mVirtualAudioDevice.getTrack, this.mVirtualCamDevice.getTrack]);
  }


  async connectionStatusChangedCallback(status) {

    switch (status) {
      case eConnectionState.disconnected:
        CTools.getInstance().logEvent('Dissociating all active ⋮⋮⋮ Swarms due to a lost connection with a ⋮⋮⋮ signaling node.', eLogEntryCategory.localSystem, 1, eLogEntryType.notification);

        for (let i = 0; i < this.mSwarms.length; i++) {
          this.mSwarms[i].joinConfirmed = false;
          this.mSwarms[i].mLastJoinAttempt = 0;
        }
        break;
      case eConnectionState.connecting:

        break;
      case eConnectionState.connected:

        break;
      case eConnectionState.aboutToShutDown:

        break;

      default:

    }
  }

  newSwarmSDPMsgCallback(sdp) {

    if (sdp == null)
      return;

    let swarm = this.findSwarmByID(sdp.getSwarmID);

    if (swarm == null)
      return; //we have no idea where to route it

    swarm.processSignalEntity(sdp);
  }

  //Provides  agregated ability to subscribe for data-events from all active Swarms
  onNewSwarmData(swarmID, peerID, data) {

    for (let i = 0; i < this.mSwarmDataListeners.length; i++) {
      if (gTools.compareByteVectors(this.mSwarmDataListeners[i].swarmID, swarmID)) {
        try {
          this.mSwarmDataListeners[i].handler({
            swarmID,
            peerID,
            data
          }); //pack all these into a single field to support (this)
        } catch (error) {
          console.long('Invalid swarm-msg handler by ' + this.mSwarmDataListeners[i].id);

        }
      }
    }
  }

  get getSwarmIDs() {
    let toRet = [];

    for (let i = 0; i < this.mSwarms.length; i++) {
      toRet.push(this.mSwarms[i].getID);
    }
    return toRet;
  }

  get getCurrentSwarmID() {
    return this.mCurrentSwarmID;
  }

  set setCurrentSwarmID(swarmID) {
    this.mCurrentSwarmID = swarmID;
  }

  get getIsPrivKeyAvailable() {
    if (this.mPrivKey != null && this.mPrivKey.byteLength == 32)
      return true;

    return false;
  }

  get getIsTokenPoolAvailable() {
    if (this.mTokenPool != null && this.mTokenPool.getStatus() == eTokenPoolStatus.active)
      return true;

    return false;
  }

  //Takes an instance of CRTCExtraData
  routeExtraData(data = null, swarmID = null, source = null, destination = null, onlyImmediatePeers = true) {
    if (data == null || swarmID == null)
      return false;

    let sourceL = (source == null || source.byteLength == 0) ? this.mMyID : source;
    let destinationL = destination != null ? destination : new ArrayBuffer();
    //constructor(cryptoFactory, type, source, destination, SIPData) {
    let wrapper = new CSDPEntity(gCrypto, eSDPEntityType.data, sourceL, destinationL);
    wrapper.setSwarmID = swarmID;
    // check if target is within the immediate Swarm-memebrs, otherwise attempt to route data through the full-node  network
    //(effectively constitutes STUN-server capability for WebRTCstream-external data)

    let swarm = this.findSwarmByID(swarmID)
    if (swarm == null)
      return false;


    if (!swarm.sendDataToTarget(data, destination)) {
      if (!onlyImmediatePeers) {
        //route through full-node network
        return routeSDPEntity(wrapper);
      }

    } else return true;

    return true;

  }

  //Takes an SDP entity, authenticates it when possible and delivers to full-node for
  //further routing to destination
  routeSDPEntity(sdp) {
    if (sdp == null)
      return false;

    //preliminaries
    this.mLatestSeqNr++;
    sdp.setSeqNr = this.mLatestSeqNr;

    if (this.mDoAuth)
      this.authenticateSDP(sdp);
    //Data delivery - Begin
    let serializedCmd = sdp.getPackedData();
    if (serializedCmd == null || serializedCmd.byteLength == 0)
      return false; //something went wrong during data serialization

    if (sdp.getTT == null && sdp.getSig == null)
      CTools.getInstance().logEvent('Warning: WebRTC signaling is NOT authenticated/incentivized. Full-node might reject.', eLogEntryCategory.localSystem, 1, eLogEntryType.notification);


    let netMsg = new CNetMsg(eNetEntType.Swarm, eNetReqType.request, serializedCmd); //Note: it's a Request for full-node (always).
    //the full-node will then generate a new CNetMsg with a possibly different NetReqType
    //the authentication on SDPEntity level would persist and be unaffected.
    if (!this.mVMContext.sendNetMsg(netMsg, false))
      return false;

    return true;
    //Data delivery - End
  }

  get getCapabilitiesRequestedBySwarms() {
    let capabilities = eConnCapabilities.data;
    let videoNeeded = false;
    let audioNeeded = false;

    for (let i = 0; i < this.mSwarms.length; i++) {
      if (this.mSwarms[i].getCamInUse) {
        videoNeeded = true;
      }
      if (this.mSwarms[i].getMicInUse) {
        audioNeeded = true;
      }
    }

    if (videoNeeded && audioNeeded) {
      capabilities = eConnCapabilities.audioVideo;
    } else if (videoNeeded) {
      capabilities = eConnCapabilities.video;
    } else if (audioNeeded) {
      capabilities = eConnCapabilities.audio;
    } else {
      capabilities = eConnCapabilities.data;
    }

    return capabilities;
  }

  ///Authenticates a control datagram
  authenticateSDP(sdp) {
    //authentication - Begin

    let ttInPlace = false;
    let sigInPlace = false;

    if (this.getIsTokenPoolAvailable) //Transmission Token provides both authentication and incentivization
    {
      let tt = this.getTTWorthValue(this.mTPBankIDToUse, 1, true);
      if (!tt)
        return false; //token pool's bank depleted

      sdp.setTT(tt);
      ttInPlace = true;
    }
    //provide additional strong-signature authenticationw when privKey available
    //both types of authentication are reduntant yet provide stronger authentication
    //especialy in protection against reply-attacks/ the signature authenticates the entier datagram
    //some full-nodes might require both.
    if (this.getIsPrivKeyAvailable) {
      if (!sdp.sign(this.mPrivKey))
        return false; //should not happen if priv-key valid
      sigInPlace = true;
    }

    if (ttInPlace || sigInPlace)
      return true;
    else return false;
    //authentication - End
  }

  syncSwarm(swarmID = null, userID = new ArrayBuffer(), privKey = new ArrayBuffer()) {
    return joinSwarm(swarmID, userID, privKey);
  }

  bootstrapMedia(capabilities = this.mDefaultOutgressCapabilities) {
    CTools.getInstance().logEvent('Attempting to bootstrap Media Devices..', eLogEntryCategory.localSystem, 1, eLogEntryType.notification);
    this.getAllowedMedia(capabilities);
  }

  //The aim of this function is to prepare all the hardware resources so that the Allowed Capabilities are satisfied.
  async prepareForCapabilities() {
    CTools.getInstance().logEvent("Preparing requested media devices if any..", eLogEntryCategory.localSystem, 1, eLogEntryType.notification);
    //Local Variables - START
    let activeHardwareCapabilities = this.getActiveHardwareCapabilities(); //*IMPORTANT* - check currently active hardware handles.
    let audioAvailable = false;
    let videoAvailable = false;
    let videoNeeded = false;
    let audioNeeded = false;
    let stream = null;
    //Local Variables - END

    //Operational Logic - BEGIN

    //translate, i.e. decouple active hardware capabilities into seperate boolean flags.
    switch (activeHardwareCapabilities) {
      case eConnCapabilities.audio:
        audioAvailable = true;
        break;
      case eConnCapabilities.video:
        videoAvailable = true;
        break;
      case eConnCapabilities.audioVideo:
        audioAvailable = true;
        videoAvailable = true;
        break;
      default:
        audioAvailable = false;
        videoAvailable = false;
    }

    //now, check what's actually being requested by swarms
    for (let i = 0; i < this.mSwarms.length; i++) {
      if (this.mSwarms[i].getCamInUse) {
        videoNeeded = true;
      }
      if (this.mSwarms[i].getMicInUse) {
        audioNeeded = true;
      }
    }

    //compare hardware available against hardware requested and - optimize hardware utilization..
    //if ((!audioAvailable && audioNeeded) && (!videoAvailable && videoNeeded)) {
    //^ notice that we always need to request union of all of the required hardware capabilities.
    //i.e. we need to request BOTH microphone and camera EVEN if camera is already available.
    if ((audioNeeded && videoNeeded) && (!audioAvailable && !videoAvailable)) {
      CTools.getInstance().logEvent("Requesting both web-cam and microphone as these are needed..", eLogEntryCategory.localSystem, 1, eLogEntryType.notification);
      stream = await this.startBothMicAndCam();
    } else if (audioNeeded && !audioAvailable) {
      //request microphone -  from the web-browser as it is needed as of now.
      CTools.getInstance().logEvent("Requesting microphone as it is needed..", eLogEntryCategory.localSystem, 1, eLogEntryType.notification);
      stream = await this.startMicOnly();

    } else if (videoNeeded && !videoAvailable) {
      //request web-cam - from the web-browser as it is needed as of now.
      CTools.getInstance().logEvent("Requesting web-cam as it is needed..", eLogEntryCategory.localSystem, 1, eLogEntryType.notification);
      stream = await this.startCamOnly();

    }
    //Operational Logic - END
    return stream;
  }

  unregisterProcessFromSwarms(processID) {
    for (let i = 0; i < this.mSwarms.length; i++) {
      this.mSwarms[i].removeClientProcess(processID);
    }

  }
  //Provides an indication to a full-node (signaling server) of our will to join a particular (if swarmID provided)
  //or a most 'suitable' swarm - decided at full-node's discretion.
  //capabilities indicates ceiling capabilities supported during participation within Swarm. (MIGHT be renegotiated BUT better to state a ceiling upfront value).
  joinSwarm(trueSwarmID = new ArrayBuffer(), userID = new ArrayBuffer(), privKey = new ArrayBuffer(), capabilities = eConnCapabilities.audioVideo, appInstance) {
    if (this.mJoiningSwarmMutex) {
      CTools.getInstance().logEvent('Already attempting to join a Swarm. Aborting..', eLogEntryCategory.localSystem, 1, eLogEntryType.warning);
      return false;
    }

    try {
      this.mJoiningSwarmMutex = true;
      if (this.mVMContext.getUserID.byteLength == 0) {
        CTools.getInstance().logEvent('Users need to provide credentials.', eLogEntryCategory.localSystem, 1, eLogEntryType.notification);
        return false;
      }
      let trueIDTxt = this.mTools.arrayBufferToString(trueSwarmID);
      let ctx = CVMContext.getInstance();
      userID = gTools.convertToArrayBuffer(userID);
      trueSwarmID = gTools.convertToArrayBuffer(trueSwarmID);
      let now = this.mTools.getTime();
      let swarmID = sha3_256.arrayBuffer(trueSwarmID);
      swarmID = gTools.base58CheckEncode(swarmID);
      swarmID = gTools.convertToArrayBuffer(swarmID);

      let swarm = this.findSwarmByID(swarmID);

      if (!swarm) {
        CTools.getInstance().logEvent("Attempting to join " + trueIDTxt + "' Swarm..", eLogEntryCategory.localSystem, 1, eLogEntryType.notification);
        swarm = new CSwarm(this, userID, swarmID, trueSwarmID); //    constructor(swarmManager, agentID = new ArrayBuffer(), swarmID = new ArrayBuffer())
        swarm.setAllowedCapabilities(capabilities);
        swarm.canBeOperational = true;
        this.addSwarm(swarm);
      } else {
        if ((now - swarm.joinAttemptTimestamp) < 30) {
          CTools.getInstance().logEvent("Cannot attempt to join Swarm '" + trueIDTxt + "' that often. Aborting request.'", eLogEntryCategory.localSystem, 1, eLogEntryType.notification);
          return;
        }
        swarm.joinConfirmed = false;
        swarm.initialize();

        CTools.getInstance().logEvent("Attempting to rejoin an active '" + trueIDTxt + "' Swarm..", eLogEntryCategory.localSystem, 1, eLogEntryType.notification);
      }
      swarm.addClientAppInstance(appInstance);
      swarm.pingJoinAttempt();
      let result = new COperationStatus(eOperationScope.dataTransit, eOperationStatus.success, ctx.getFileSystem.genRequestID()); //network operation status (just in terms in CNetMsg delivery through a web socket)
      let sdp = new CSDPEntity(gCrypto, eSDPEntityType.joining, userID);
      sdp.setCapabilities = capabilities; //represents MAXIMUM capabilities (ingress and/or outgress to be supported by the local node;common part of two sets)
      //thus, other peers won't try to negotiate more than that.
      sdp.setSwarmID = swarmID;
      sdp.setSeqNr = result.getReqID;

      if (this.mDoAuth)
        if (!this.authenticateSDP(sdp))
          this.mVMContext.writeToLog('Authentication of SDP failed.');

      let serializedSDP = sdp.getPackedData();

      let netMsg = new CNetMsg(eNetEntType.Swarm, eNetReqType.request, serializedSDP);

      if (ctx.getIsEncryptionAvailable()) {
        result.setIsSuccess = ctx.sendNetMsg(netMsg, false);
      } else {
        CTools.getInstance().logEvent("Cannot connect to Swarm at this time (session key not available). Will keep trying..", eLogEntryCategory.localSystem, 1, eLogEntryType.warning);
        result = false;
      }

      this.optimizeRequestedResources();
      return result; //Note: the status of joining the Swarm will be indicated asynchronously by eSDPControlStatus within returned CSDPentity
      //the sequence Nr will match in the response-datagram
    } finally {
      this.mJoiningSwarmMutex = false;
    }
  }

  //Provides an indication to full-node (signaling server) that the Client still requires its singaling/data routing support.
  //Note that the server might require a reward (through Transmission Token) for doing so.
  pingFullNode(swarmID = new ArrayBuffer(), userID = new ArrayBuffer(), privKey = new ArrayBuffer()) {
    let result = new COperationStatus(eOperationScope.dataTransit, eOperationStatus.success, CVMContext.getInstance().getFileSystem.genRequestID()); //network operation status(
    if (this.mVMContext.getUserID.byteLength == 0) {
      CTools.getInstance().logEvent('Users need to provide credentials.', eLogEntryCategory.localSystem, 1, eLogEntryType.notification);
      return false;
    }

    if (this.mVMContext.getConnectionState != eConnectionState.connected) {
      result.setIsSuccess = false;
      return result;
    }
    swarmID = gTools.convertToArrayBuffer(swarmID);

    //just in terms in CNetMsg delivery through a web socket)
    let sdp = new CSDPEntity(gCrypto, eSDPEntityType.pingFullNode, this.mVMContext.getUserID);
    sdp.setSwarmID = swarmID;
    sdp.setSeqNr = result.getReqID;

    if (this.mDoAuth)
      if (!this.authenticateSDP(sdp))
        this.mVMContext.writeToLog('Authentication of SDP failed.');

    let serializedSDP = sdp.getPackedData();

    let netMsg = new CNetMsg(eNetEntType.Swarm, eNetReqType.request, serializedSDP);
    result.setIsSuccess = CVMContext.getInstance().sendNetMsg(netMsg, false);
    return result; //Note: the status of joining the Swarm will be indicated asynchronously by eSDPControlStatus within returned CSDPentity
    //the sequence Nr will match in the response-datagram
  }

  //Provides an indication to signaling server that we're to leave a given swarm
  leaveSwarm(trueSwarmID = new ArrayBuffer()) {

    let swarmID = sha3_256.arrayBuffer(trueSwarmID);
    swarmID = gTools.base58CheckEncode(swarmID);
    swarmID = gTools.convertToArrayBuffer(swarmID);

    let swarm = this.findSwarmByID(swarmID);
    if (!swarm)
      return false;

    swarm.close();

    if (this.mVMContext.getUserID.byteLength == 0) {
      CTools.getInstance().logEvent('Users need to provide credentials.', eLogEntryCategory.localSystem, 1, eLogEntryType.notification);
      return false;
    }
    let result = new COperationStatus(eOperationScope.dataTransit, eOperationStatus.success, CVMContext.getInstance().getFileSystem.genRequestID()); //network operation status(just in terms in CNetMsg delivery through a web socket)
    let sdp = new CSDPEntity(eSDPEntityType.bye, this.mVMContext.getUserID);
    sdp.setSwarmID = swarmID;
    sdp.setSeqNr = result.getReqID;

    if (this.mDoAuth)
      this.authenticateSDP(sdp);

    let serializedSDP = sdp.getPackedData();

    let netMsg = new CNetMsg(eNetEntType.Swarm, eNetReqType.notify, serializedSDP);
    result.setIsSuccess = CVMContext.getInstance().sendNetMsg(netMsg, false);
    return result; //Note: the status of joining the Swarm will be indicated asynchronously by eSDPControlStatus within returned CSDPentity
    //the sequence Nr will match in the response-datagram
  }

}
