import {
  CVMMetaSection,
  CVMMetaEntry,
  CVMMetaGenerator,
  CVMMetaParser
} from './../../../../lib/MetaData.js'
import {
  CNetMsg
} from './../../../../lib/NetMsg.js'
import {
  CWindow
} from "/lib/window.js"

import {
  CSwarmsManager
} from '/lib/SwarmManager.js'


var meetingBody = `  <link rel="stylesheet" href="/css/main.css">
<link rel="stylesheet" href="/dApps/Meeting/style.css">
  <link rel="stylesheet" href="/css/sweetalert2.min.css">
<link rel="stylesheet" href="/css/animate.min.css" />
<link href="/css/fadumped.css" rel="stylesheet">
<link href="/css/faall.css" rel="stylesheet">
<link href="/dApps/Meeting/css/emoji.css" rel="stylesheet">

    <div id='videosHeader' class='noselect'>
      <div id="videosHeaderCoverD" class="videosHeaderCover"> <i class="fa fa-th"></i> LIVE Meeting </div>
      <div class="shadow"></div>
      <div id='actionsContainer' class="actionsContainer">
        <div class="menu_simple">
          <ul>
            <li style="margin-top:-1em ; "><a id='peersCount' href="#"></a> </li>

            <li onclick="alert('Option 1 Clicked')"><a href="#">
                <i class="fal fa-pause"></i> Pause</a></li>
            <li onclick="let window = gWindowManager.getWindowByID($($(this).closest('.idContainer')).find('#windowIDField').first().val()); window.quitMeeting();"><a href="#">
                <i class="far fa-sign-out-alt"></i> Quit</a></li>

          </ul>
        </div>

      </div>
    </div>

    <div id='joinMeetingView' class='show'>

      <div id='joinMeetingPoster'>

      </div>
      <div id='loginBox'>
        <div id='loginBoxMeetingID'>  <form onkeydown="if(event.key == 'Enter') {let window = gWindowManager.getWindowByID($($(this).closest('.idContainer')).find('#windowIDField').first().val()); if(!window.isSFOk){ window.animateCSS(this.id,'shakeX'); return}; window.meetingInitialization();  return false;} return true;">
          <label for="meetingIDTxt" style='margin-top: 1em; color: cyan; top: 0;
    bottom: 0;margin-bottom: auto; display: inline-block;'>Meeting I D:</label>

          <input type="text" autocomplete="off" style="    top: 0;
    bottom: 0;
    margin-top: auto;
    margin-bottom: auto;
    margin-left: 0.5rem;
    height: 2rem;
    display: inline-block; width: 36%;
    max-width: 13em;" class="neonicLiteTxtInput" id="meetingIDTxt" placeholder="provide ID.." value="" onclick="">

          <button type="button"  onclick="let window = gWindowManager.getWindowByID($($(this).closest('.idContainer')).find('#windowIDField').first().val()); if(!window.isSFOk){ window.animateCSS(this.id,'shakeX'); return}; window.meetingInitialization(); return false;" id="joinMeetingBtn" style="vertical-align: baseline;"class="buttonStd"><i class="fas fa-sign-in-alt fa-lg"></i>
          </button>
          <br>
              </form>
        </div>

      </div>
    </div>
    <div id='activeMeetingView'>

    <div id='emojisContainer' class='animate__animated'>
      <emoji-picker id ='picker'></emoji-picker>

    </div>

      <div id="videoFeedsContainer">



      </div>

      <div id='mediaControls' style="text-align:center">
        <span id='micBtnOuter' class="fa-stack redTxt">
          <i class="fal fa-circle fa-stack-2x"></i>
          <!-- microphone-alt -->
          <i id='micBtn' class="fal fa-microphone-alt-slash fa-stack-1x" onclick="let window = gWindowManager.getWindowByID($($(this).closest('.idContainer')).find('#windowIDField').first().val()); window.toggleMic();"></i>
        </span>
        <span id='camBtnOuter' class="fa-stack redTxt">
          <i class="fal fa-circle fa-stack-2x"></i>
          <!-- fa-webcam-slash-->
          <i id='camBtn' class="fal fa-webcam-slash fa-stack-1x " onclick="let window = gWindowManager.getWindowByID($($(this).closest('.idContainer')).find('#windowIDField').first().val()); window.toggleCam();"></i>
        </span>

        <span id='ssBtnOuter' class="fa-stack redTxt" onclick="let window = gWindowManager.getWindowByID($($(this).closest('.idContainer')).find('#windowIDField').first().val()); window.toggleSS();">
          <i class="fal fa-circle fa-stack-2x"></i>
          <!-- fa-webcam-slash-->
          <i id='ssBtn' class="fal fa-desktop fa-stack-1x " ></i>
        </span>
      </div>
      <div id='chat' ondblclick="let window = gWindowManager.getWindowByID($($(this).closest('.idContainer')).find('#windowIDField').first().val()); window.toggleChatSize();" class=''>


        <div id='chatHeader' class='noselect'>

        </div>
        <div id='chatMsgsContainer'>
          <ul id="innerChat">

          </ul>

        </div>

        <div style="position:absolute; bottom:0px;width:100%">
          <div id='containerTxt'>
            <div id="typingStates">
          <div id='John' class="typingState">
    <span class="who">John</span><img src="/dApps/Meeting/img/txtwait.gif"  class="typingSticker">
    </div>


    </div>
          <i onclick="let window = gWindowManager.getWindowByID($($(this).closest('.idContainer')).find('#windowIDField').first().val()); window.toggleEmojiPicker()" class="fal fa-smile fa-lg emojiPickerButton" ></i>
<form onkeydown=" let window = gWindowManager.getWindowByID($($(this).closest('.idContainer')).find('#windowIDField').first().val()); window.notifyTyping();if(event.key == 'Enter') {if(!window.getCheckSAOk(1)){ window.animateCSS(this.id,'shakeX'); return false}; window.newGlobalMsgFromMe(); return false;} else {return true;} ">
            <input id='chatInputTxt' autocomplete="off" type="text" ></div>
</form>
          <div id='containerBtn'>

            <input id='chatSendBtn' onclick="let window = gWindowManager.getWindowByID($($(this).closest('.idContainer')).find('#windowIDField').first().val()); if(!window.getCheckSAOk(1)){ window.animateCSS(this.id,'shakeX'); return}; window.newGlobalMsgFromMe();" type="button" value="Send"></div>

        </div>
      </div>
    </div>


`;

let typingStateTemplate = `<div id="[peerID]" class = 'typingState animate__animated'><span class="who">[peerID]</span><img src="/dApps/Meeting/img/txtwait.gif"  class="typingSticker"></div>`
let msgTemplate = `<li class="[originFlagField]"><div class="entete"><span class="[statusField]"></span><h2>[sourceField]</h2><h3>[timeField]</h3></div><div class="message"><div class="triangle"></div>[msgTxtField]</div></li>`;
let streamTemplate = `<div id='[FIELD_PEER_ID]' class = 'videoFeedContainer videoFeedResizable' onclick="let window = gWindowManager.getWindowByID($($(this).closest('.idContainer')).find('#windowIDField').first().val()); window.toggleFullScreenVideo(this.id);" onmouseout=""> </div>`;
let streamTitleTemplate = `<div class ='videoFeedTitle'><div class='feedTitleTxt'>[fieldTitle]</div></div>`;

const eView = {
  joinMeeting: 0,
  activeMeeting: 1
}

class CPeer {
  constructor(id, connection) {
    this.mID = id;
    this.mConnection = connection;
    this.mLastTyping = 0;
    this.mLastMsg = 0;
  }

  pingTyping() {
    this.mLastTyping = CTools.getInstance().getTime(true);
  }
  pingLastMsg() {
    this.mLastMsg = CTools.getInstance().getTime(true);
  }


  get getLastTyping() {
    return this.mLastTyping;
  }
  get getLastMsg() {
    return this.mLastMsg;
  }

  get getID() {
    return this.mID;
  }
  get getConnection() {
    return this.mConnection;
  }


}

class CMeeting extends CWindow {
  constructor(positionX, positionY, width, height) {
    super(positionX, positionY, width, height, meetingBody, "⋮⋮⋮ Meeting", CMeeting.getIcon(), true);
    this.mMetaParser = new CVMMetaParser();
    this.setProtocolID = 257;
    this.mTools = CTools.getInstance();
    this.mPeers = [];
    this.mMaxedVideo = null;
    this.mLastHeightRearangedAt = 0;
    this.mLastWidthRearangedAt = 0;
    this.mErrorMsg = "";
    this.mPrivKey = null;
    this.mEmojiPickerVisible = false;
    this.mLastAnonymousID = 0;
    //register for network events
    CVMContext.getInstance().addVMMetaDataListener(this.newVMMetaDataCallback.bind(this), this.mID);
    CVMContext.getInstance().addNewDFSMsgListener(this.newDFSMsgCallback.bind(this), this.mID);
    CVMContext.getInstance().addNewGridScriptResultListener(this.newGridScriptResultCallback.bind(this), this.mID);
    this.loadLocalData();
    this.controllerThreadInterval = 300;
    this.mControler = 0;
    this.mControlerExecuting = false;
    this.mOutBoundStream = null;
    //app specific logic - Begin
    this.mLastVidResize = this.mTools.getTime(true);
    this.mSwarm = null; //single swarm per Meeting
    this.mSwarmManager = CSwarmsManager.getInstance(this.mVMContext);
    this.mMicMuted = true;
    this.mCamMuted = true;
    this.mSSMuted = true;
    this.mScreenSharingEnabled = false;
    this.mChatMaxed = false;
    this.mActiveView = eView.joinMeeting;
    this.mStreams = [];
    this.mMyID = CVMContext.getInstance().getUserID;
    this.mTypingValidityThreshold = 4000; //ms


    //app specific login - End
  }
  static getDefaultCategory() {
    return 'productivity';
  }
  static getPackageID() {
    return "org.gridnetproject.UIdApps.meeting";
  }
  static getIcon() {
    return `data:image/png;base64,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`;
  }
  getTypingStickerRendering(peerID) {
    return typingStateTemplate.replaceAll('[peerID]', this.mTools.arrayBufferToString(peerID));
  }
  addPeer(peer) {
    if (peer == null || this.findPeerByID(peer.getID) != null)
      return;

    this.mPeers.push(peer);
  }

  whoIsIndicatedAsTyping() {
    let typers = $(this.mTypingStatesContainer).find('.typingState');
    let IDs = [];
    for (let i = 0; i < typers.length; i++) {
      IDs.push(this.mTools.convertToArrayBuffer($(typers[i]).attr('id')));
    }
    return IDs;
  }
  getRealTypers() {
    let toRet = [];
    let time = this.mTools.getTime(true);

    for (let i = 0; i < this.mPeers.length; i++) {
      if (time < this.mPeers[i].getLastTyping)
        continue; //might happen due to async
      if ((time - this.mPeers[i].getLastTyping) < this.mTypingValidityThreshold)
        toRet.push(this.mPeers[i].getID);
    }

    return toRet;
  }

  removeTyperFromUI(peerID) {

    if (peerID == null || peerID.length == 0) {
      $(this.mTypingStatesContainer).empty();
    } else {
      peerID = this.mTools.arrayBufferToString(peerID);
      this.animateCSS(peerID, 'fadeOut', 'animate__', null, true);
      $(this.mTypingStatesContainer).find('#' + peerID).remove();
    }
  }

  removeFromUIStrict(peerID) {
    if (peerID == null || peerID.length == 0) {
      $(this.mTypingStatesContainer).empty();
    } else {
      peerID = this.mTools.arrayBufferToString(peerID);

      $(this.mTypingStatesContainer).find('#' + peerID).remove();
    }
  }



  renderTypingPeers() {
    //get peers that should be indicated as typing as per our world-view - Set A
    let realTypers = this.getRealTypers();
    //get peers currently indicated as typing - Set B
    let claimedTypers = this.whoIsIndicatedAsTyping();
    //hide those from B that are not in A. (that way in order not to disrupt animations in UI)

    let time = this.mTools.getTime(true);

    for (let b = 0; b < claimedTypers.length; b++) {
      let found = false;
      let stillValid = false;
      let obj = this.findPeerByID(claimedTypers[b]);
      if (obj != null) {
        if ((obj.getLastTyping > time) || ((time - obj.getLastTyping) < this.mTypingValidityThreshold))
          stillValid = true;
      }
      if (!stillValid)
        for (let a = 0; a < realTypers.length; a++) {
          if (this.mTools.compareByteVectors(realTypers[a], claimedTypers[b])) {
            found = true;
            break;
          }
        }

      if (!found && !stillValid)
        this.removeTyperFromUI(claimedTypers[b]);
    }

    //Add missing ones from A

    for (let a = 0; a < realTypers.length; a++) {
      let found = false;
      for (let b = 0; b < claimedTypers.length; b++) {
        if (this.mTools.compareByteVectors(realTypers[a], claimedTypers[b])) {
          found = true;
          break;
        }
      }

      if (!found) {
        let r = this.getTypingStickerRendering(realTypers[a]);
        $(this.mTypingStatesContainer).append(r);
        this.animateCSS(this.mTools.arrayBufferToString(realTypers[a]), 'fadeIn', 'animate__', null);
      }
    }
  }

  findPeerByID(peerID) {
    if (peerID == null || peerID.byteLength == 0)
      return null;

    for (let i = 0; i < this.mPeers.length; i++) {
      if (this.mTools.compareByteVectors(this.mPeers[i].getID, peerID)) {
        return this.mPeers[i];
      }
    }
  }

  removePeer(peerID) {
    if (peerID == null || peerID.byteLength == 0)
      return false;

    for (let i = 0; i < this.mPeers.length; i++) {
      if (this.mTools.compareByteVectors(this.mPeers[i].getID, peerID)) {
        this.mPeers.splice(i);
        return true;
      }
    }
    return false;
  }
  toggleFullScreenVideo(id) {
    if (this.mMaxedVideo) {
      this.minVid(id);
    } else {
      this.maxVid(id);
    }
  }

  maxVid(id) {
    this.mMaxedVideo = id;
    //sec
    if ((this.mTools.getTime(true) - window.getLastVidResize) < 200) return;
    window.getLastVidResize = this.mTools.getTime(true);
    //logic
    let container = $(this.getBody).find('#' + id)[0];
    container.classList.remove('videoFeedContainer');
    container.classList.add('videoFeedResizableHovered');
  }

  minVid(id) {
    this.mMaxedVideo = null;
    //sec
    if ((this.mTools.getTime(true) - window.getLastVidResize) < 200) return;
    window.getLastVidResize = this.mTools.getTime(true);
    //logic
    let container = $(this.getBody).find('#' + id)[0];
    container.classList.remove('videoFeedResizableHovered');
    container.classList.add('videoFeedContainer');

  }
  set setMyID(id) {
    if (id == null)
      return false;
    this.mMyID = id;
    return true;
  }

  toggleEmojiPicker() {
    if (this.mEmojiPickerVisible) {
      $(this.mEmojisContainer).removeClass('animate__backInUp');
      $(this.mEmojisContainer).addClass('animate__backOutDown');

      this.mEmojiPickerVisible = false;
    } else {
      $(this.mEmojisContainer).removeClass('animate__backOutDown');
      $(this.mEmojisContainer).addClass('shown animate__backInUp');
      this.mEmojiPickerVisible = true;;
    }
  }

  meetingInitialization(id = null) {

    if (this.mVMContext.getConnectionState != eConnectionState.connected) {
      this.showMessageBox('Not conntected', "⋮⋮⋮ Network is unavailable please check your connectivity.", eNotificationType.notification);
    }

    if (id == null)
      id = this.getValue('meetingIDTxt');
    if (id.length == 0) {
      this.showMessageBox('Unknown conference ID', "Please provide Conference ID. Think one up if you create it!", eNotificationType.notification);
      return;
    }
    if (this.connectToSwarm(id)) {
      this.setMeetingView();
      return true;
    } else {
      this.showMessageBox('Unable to connect 🥺', "Unable to connect to ⋮⋮⋮ Swarm.", eNotificationType.error);
    }
    return false;
  }

  setLocalStreamInUI(stream) {
    if (stream == null)
      return;

    let videoTag = $(this.getBody).find('#Me_stream video');

    if (videoTag.length > 0) {
      videoTag[0].srcObject = stream;
    }
  }
  connectToSwarm(id) {
    if (this.mSwarm == null) {
      /*
      status: this.mStatus,
      peerID: this.mPeerID,
      swarmID: this.mSwarm.getID
      */
      if (CVMContext.getInstance().getSwarmsManager.joinSwarm(id, this.mMyID, this.mPrivKey, eConnCapabilities.audioVideo)) {
        this.mSwarm = CVMContext.getInstance().getSwarmsManager.findSwarmByID(id);
        this.updateEffectiveOutgressCapabilities();
        let ls = CVMContext.getInstance().getSwarmsManager.getLocalStream;
        if (ls != null) {
          if (!this.getStreamByID('Me'))
            this.addStream(ls, 'Me');

          this.setLocalStreamInUI(ls);
        }
        CVMContext.getInstance().getSwarmsManager.addLocalStreamEventListener(this.localStreamEventHandler.bind(this), this.mID);
        //Subscribe to high-level events
        this.mSwarm.addSwarmConnectionStateChangeEventListener(this.swarmConnectionChangedEventHandler.bind(this), this.mID);
        this.mSwarm.addDataChannelMessageEventListener(this.newSwarmDataEventHandler.bind(this), this.mID);
        this.mSwarm.addDataChannelStateChangeEventListener(this.dataChannelStateChangeEventhandler.bind(this), this.mID);
        this.mSwarm.addConnectionStateChangeEventListener(this.connectionStateChangedEventHandler.bind(this), this.mID);
        //subscribe to low-level WebRTC track-events (audio/video)

        this.mSwarm.addTrackEventListener(this.trackEventHandler.bind(this), this.mID);
        return true;
      } else return false;
    } else {
      return false;
    }
  }

  connectionStateChangedEventHandler(event) {
    if (event.state == eConnectionState.disconnected) {
      this.removeStreamByPeer(event.peerID);
    }
  }
  localStreamEventHandler(event) {
    this.setOutBoundStream(event.stream);
    //show the outgresss audio/video multiplexed stream to user
  }
  //Note: the onTrack event will  double fire for both the Audio and Video track.
  //Yet Still, these tracks are synchronized and multiplexed over a single stream.
  //Thus we keep track of the added streams and so avoid double-addition.
  trackEventHandler(e) {
    /*event: event,
    peerID: this.mPeerID,
    swarmID: this.mSwarm.getID*/
    let peerID = this.mTools.arrayBufferToString(e.peerID);
    if (e.event.track.kind != 'video')
      return;
    if (this.getStreamByID(e.peerID))
      this.removeStreamByPeer(e.peerID);

    this.addStream(e.event.streams[0], e.peerID)

  }
  setOutBoundStream(stream) {
    this.mOutBoundStream = stream;
    if (!this.getStreamByID('Me'))
      this.addStream(stream, 'Me');

    this.setLocalStreamInUI(stream);
  }

  swarmConnectionChangedEventHandler(event) {
    switch (event.status) {

      case eSwarmConnectionState.intial:
        break;
      case eSwarmConnectionState.negotiating:
        break;
      case eSwarmConnectionState.active:
        let peer = new CPeer(event.peerID, event.connection);
        this.addPeer(peer);
        break;
        break;
      case eSwarmConnectionState.closed:
        this.removeStream(event.peerID);
        this.removePeer(event.peerID);
        break;
    }
  }

  newSwarmDataEventHandler(e) {
    try {
      let data = e.event.data;
      let wrapper = CNetMsg.instantiate(data);

      switch (wrapper.getEntityType) {
        case this.getProtocolID: //eMeeting Protocol
          let msg = CChatMsg.instantiate(wrapper.getData);

          //Decide on the External Flag - BEGIN
          if (this.mTools.compareByteVectors(this.mMyID, msg.mFromID))
            msg.mExternal = false;
          else msg.mExternal = true;
          //Decide on the External Flag - END

          switch (msg.getType) {
            case eChatMsgType.text:
              this.addMessage(msg);
              break;

            case eChatMsgType.typing:
              let peer = this.findPeerByID(msg.getSourceID);
              if (peer != null)
                peer.pingTyping();
              break;
            default:

              //ignore otherwise
              break;
          }

      }

    } catch (error) {
      console.log('Invalid ChatMsg received');
    }
  }

  dataChannelStateChangeEventhandler(event) {

  }
  /*
  const eSwarmConnectionState = {
    intial: 0,
    negotiating: 1,
    active: 2,
    closed: 3
  };*/


  set setPrivKey(priv = null) {
    this.mPrivKey = priv;
  }


  get getMyID() {
    return this.mMyID;
  }

  get getLastVidResize() {
    return this.mLastVidResize;
  }
  set setLastVidResize(time) {
    this.mLastVidResize = time;
  }


  set getLastVidResize(last) {
    this.mLastVidResize = last;
  }

  initialize() //called only when app needs a thread/processing queue of its own, one seperate from the Window's
  //internal processing queue
  {
    this.mControler = setInterval(this.mControlerThreadF.bind(this), this.controllerThreadInterval);
  }

  pingFullNode() {

  }

  mControlerThreadF() {
    if (this.mControlerExecuting)
      return false;

    this.mControlerExecuting = true; //mutex protection
    try {
      this.redrawMe();
      this.updatePeersCount();
      this.renderTypingPeers();


      //operational logic - BEGIN

      //operational logic - END
    } catch (error) {

    }
    this.mControlerExecuting = false;

  }

  redrawMe() {
    //the size of the container is limited by max-height, still we want the bottom chat to fill the entire remaining space (by default)
    //-it will fill the entire screen IF clicked anyway.
    this.mVideosContainerHeight = this.mTools.getVisibleWithin(this.getBody, '#videoFeedsContainer');
    let windowHeight = this.getHeight;

    let newMaxChatHeight = windowHeight - this.mVideosContainerHeight;
    //we update the default (when not in the maximized state) max-height value of the bottom-chat accordingly

    $(this.getBody).find('#chat').css('height', 'calc(' + newMaxChatHeight + 'px - 9em)');
    //  this.refreshGlobalChat();
  }
  finishResize(isFallbackEvent) { //Overloaded window-resize Event
    //called on finish of resize-animation ie. maxWindow, minWindow
    this.redrawMe();
    super.finishResize(isFallbackEvent);
    this.refreshStreams();
  }

  stopResize(handle) { //fired when mouse-Resize ends
    super.stopResize(handle);

  }

  open() { //Overloaded Window-Opening Event
    this.mSwarmManager.bootstrapMedia();
    this.mContentReady = false;
    console.log('⋮⋮⋮Meeting booting up..')
    super.open();
    //modify content here
    this.preInit();
    this.initialize();
  }
  updatePeersCount() {
    if (this.mSwarm == null) {
      this.mPeersCountLbl.innerHTML = "Peers: 0";
    } else {
      this.mPeersCountLbl.innerHTML = "Peers: " + (this.mSwarm.getActiveConnectionsCount() + 1);
    }
  }

  preInit() {
    // Initializes and creates emoji set from sprite sheet
    this.setWindowOverflowMode = eWindowOverflowMode.hidden;
    //    if (window.emojiPicker == null)

    this.mPeersCountLbl = $(this.getBody).find('#peersCount')[0];
    this.mCamBtn = $(this.getBody).find('#camBtn')[0];
    this.mMicBtn = $(this.getBody).find('#micBtn')[0];
    this.mSSBtn = $(this.getBody).find('#ssBtn')[0];
    this.mInnerChat = $(this.getBody).find('#innerChat')[0];
    this.mVideoFeeds = $(this.getBody).find('#videoFeedsContainer')[0];
    this.mUserGlobalOutTxt = $(this.getBody).find('#chatInputTxt')[0];
    this.mEmojisContainer = $(this.getBody).find('#emojisContainer')[0];
    this.mTypingStatesContainer = $(this.getBody).find('#typingStates')[0];
    this.getBody.addEventListener('click', this.globalClickHandler.bind(this));
    // Finds all elements with `emojiable_selector` and converts them to rich emoji input fields
    // You may want to delay this step if you have dynamically created input fields that appear later in the loading process
    // It can be called as many times as necessary; previously converted input fields will not be converted again
    $(this.getBody).find('#containerTxt').dblclick(function() {
      return false;
    });
    $(this.getBody).find('#containerBtn').dblclick(function() {
      return false;
    });
    const style = document.createElement('style');
    style.textContent = `.roundedC
      {
        border-radius: 0.5em !important;
        box-shadow: -1px 5px 10px 5px #0dece6;
      }`
    let picker = $(this.getBody).find('#picker')[0];
    picker.shadowRoot.appendChild(style);
    $($(picker.shadowRoot).find('.picker')[0]).addClass('roundedC');
    picker.addEventListener('emoji-click', this.emojiClicked.bind(this));

  }

  globalClickHandler(event) {
    if (event.target.tagName != 'EMOJI-PICKER' && event.target.tagName.length != 1) {

      if (this.mEmojiPickerVisible)
        this.toggleEmojiPicker();
    }

  }

  emojiClicked(event) {

    this.mUserGlobalOutTxt.value += event.detail.emoji.unicode;
  }

  closeWindow() {

    if (this.mControler > 0)
      clearInterval(this.mControler); //shut-down the thread if active

    if (this.mSwarm != null)
      this.mSwarm.close();
    super.closeWindow();
  }

  loadLocalData() {
    return false;

  }

  newGridScriptResultCallback(result) {
    if (result == null)
      return;
    this.notifyResult(result);
    this.retrieveBalance();

  }

  notifyResult(result, immediateOne = false) {

  }
  newDFSMsgCallback(dfsMsg) {
    if (!this.hasNetworkRequestID(dfsMsg.getReqID))
      return;
    if (dfsMsg.getData1.byteLength > 0) {
      //this.writeToLog('<span style="color: blue;">DFS-data-field-1 (meta-data)contains data..</span>');

      let metaData = this.mMetaParser.parse(dfsMsg.getData1);

      if (metaData != 0) {

        let sections = this.mMetaParser.getSections;

        for (var i = 0; i < sections.length; i++) {
          let sType = sections[i].getType;
          if (sType != eVMMetaSectionType.fileContents)
            break;

          let entries = sections[i].getEntries;
          let entriesCount = entries.length;

          for (var a = 0; a < entriesCount; a++) {
            let dataFields = entries[a].getFields;

            if (entries[a].getType == eDFSElementType.fileContent) {
              let dataType = dataFields[0];
              let fileName = dataFields[1];
              if (this.mTools.arrayBufferToString(fileName) != "GNC")
                return;
              let data = dataFields[2];

              switch (dataType) {
                case eDataType.bytes:

                  break;

                case eDataType.signedInteger:
                  data = this.mTools.arrayBufferToNumber(data);
                  break;
                case eDataType.unsignedInteger:
                  data = this.mTools.arrayBufferToNumber(data); //here will be the GNC value
                  this.mBalance = data;
                  this.refreshBalance();
                  break;
                default:
                  return;

                  return;
              }

              return;
            } else break;
          }

        }

      } else {

        return;

      }
    } else {

    }

  }

  newVMMetaDataCallback(dfsMsg) { //callback called when new Meta-Data made available from full-node
    //this will contain the result of our #Crypto transfer
    if (!this.hasNetworkRequestID(dfsMsg.getReqID))
      return;
    if (dfsMsg.getData1.byteLength > 0) {
      let metaData = this.mMetaParser.parse(dfsMsg.getData1);
    }
  }

  //app specific member functions - Begin
  set setSwarm(swarm) {
    this.mSwarm = swarm;
  }

  deliverMsg(msg) {
    if (this.mSwarm == null)
      return false;
    this.mSwarm.getActiveConnection

  }

  get getIsScreenSharingEnabled() {
    return this.mScreenSharingEnabled;
  }

  refreshGlobalChat() {
    this.mInnerChat.scrollTop = this.mInnerChat.scrollHeight;
  }

  isOverflown(element) {
    return element.scrollHeight > element.clientHeight || element.scrollWidth > element.clientWidth;
  }

  refreshStreams() {
    //todo
    this.mVideoFeeds.scrollTop = this.mVideoFeeds.scrollHeight;

    let wrappers = $(this.getBody).find('.videoFeedTitle');


    for (let i = 0; i < wrappers.length; i++) {
      let wrap = wrappers.eq(i)[0];
      let wrapWidth = wrap.offsetWidth; // just in case it's not known and set by CSS

      let txtField = $(wrap).find('.feedTitleTxt')[0];

      if (this.isOverflown(wrap)) {
        txtField.classList.add('streamTxt');
      }

    }

  }

  setMeetingView() {
    $(this.getBody).find('#joinMeetingView').removeClass('show');
    $(this.getBody).find('#activeMeetingView').addClass('show');
    this.mActiveView = eView.activeMeeting;
    this.setTitle(this.mTools.bytesToString(this.mSwarm.getID) + ' - ⋮⋮⋮ Meeting');
  }

  setJoinMeetingView() {
    $(this.getBody).find('#activeMeetingView').removeClass('show');
    $(this.getBody).find('#joinMeetingView').addClass('show');
    this.mActiveView = eView.joinMeeting;
  }

  //Appends message to chat. Takes instance of CChatMsg.
  addMessage(msg) {
    let renderedMsg = msg.getRendering();
    if (renderedMsg == null) {
      console.log('invalid  msg');
      return false;
    }

    $(this.mInnerChat).append(renderedMsg);
    this.refreshGlobalChat();
    return true;
  }
  newGlobalMsgFromMe(txt) {

    //if (this.mSwarm == null || this.mSwarm.getState == eSwarmState.idle) {
    //  this.showMessageBox('Empty conference room', "Right now there's nobody to send to ;-(", eNotificationType.notification);
    //  return;
    //}

    if (txt == null || txt.length == 0)
      txt = this.mUserGlobalOutTxt.value;
    //this.addStream();
    if (txt.length == 0)
      return false;
    if (this.mSwarm != null) {

      let msg = new CChatMsg(eChatMsgType.text, this.mMyID, null, this.mUserGlobalOutTxt.value);
      $(this.mUserGlobalOutTxt).val('');

      let wrapper = new CNetMsg(this.getProtocolID, eNetReqType.request, msg.getPackedData());
      let serializedNetMsg = wrapper.getPackedData();
      this.mSwarm.sendData(serializedNetMsg);
      return this.addMessage(msg);

    }
  }

  notifyTyping() {

    let swarm = this.mSwarm;
    if (swarm != null) {
      if (!this.isSFOk)
        return; //not to spam the network
      let msg = new CChatMsg(eChatMsgType.typing, this.mMyID);

      let wrapper = new CNetMsg(this.getProtocolID, eNetReqType.request, msg.getPackedData());
      let serializedNetMsg = wrapper.getPackedData();
      swarm.sendData(serializedNetMsg);

    }

  }

  removeStream(peerID = null) {
    if (peerID == null)
      return false;
    let peerIDStr = this.mTools.arrayBufferToString(peerID);
    let titleTagID = peerIDStr + '_stream';
    $(this.mVideoFeeds).find('#' + titleTagID).remove();
    return true;
  }

  removeStreamByPeer(peerID = null) {
    if (peerID == null || peerID.byteLength == 0)
      return false;

    let index = -1;

    //check if connection truly is withint the pending-connections pool
    for (let i = 0; i < this.mStreams.length; i++) {
      if (this.mTools.compareByteVectors(this.mStreams[i].peerID, peerID)) {
        index = i;
        break;
      }
    }

    if (index >= 0) {
      this.mStreams.splice(index, 1);
      return true;
    }

    return false;
  }


  //Appends a video/audio-stream. The stream is played automatically.
  addStream(stream, peerID) {
    //try {
    if (stream == null || peerID == null)
      return false;

    let peerIDStr = this.mTools.arrayBufferToString(peerID);
    let video = null;
    let titleTagID = peerIDStr + '_stream';
    let containerHtml = streamTemplate.replace('[FIELD_PEER_ID]', titleTagID);
    let streamContainer = $.parseHTML(containerHtml)[0];

    let audioTracks = stream.getAudioTracks();
    if (peerID != 'Me')
      audioTracks[0].enabled = true;
    //Todo: add stub image when video track muted.
    let streamTitleContainer = $.parseHTML(streamTitleTemplate.replace('[fieldTitle]', peerIDStr))[0];
    if (stream == null) { //just use a stub test-stream
      video = $('<video />', {
        id: 'video',
        height: 'auto',
        width: '100%',
        poster: '/dApps/Meeting/img/icon.png',
        src: '/dApps/Meeting/img/test.ogg',
        controls: false
      }).prop({
        muted: false,
        autoplay: true,
        loop: true
      });
    } else { //use the actual peer's stream
      /*video = $('<video />', {
          id: 'video',
          height: 'auto',
          width: '100%',
          src:stream.stream,
          poster: '/dApps/Meeting/img/icon.png',
          controls: true
        }).prop({
          muted: false,
          autoplay: true,
          playsinline:true
        });



        video.on("onloadedmetadata", function (e) {
e.target.play();
});
*/
      video = document.createElement('video');
      video.autoplay = true;
      video.srcObject = stream;
      video.onloadedmetadata = function(e) {
        video.play();
      };

    }


    $(streamContainer).append(video);
    $(streamContainer).append(streamTitleContainer);

    $(this.mVideoFeeds).append(streamContainer);
    this.mStreams.push({
      peerID: peerID,
      stream
    });
    this.refreshStreams();
    return true;
    //  } catch (error) {
    //    return false;
    //  }
  }

  getStreamByID(id) {
    if (id == null)
      return null;
    id = this.mTools.convertToArrayBuffer(id);

    for (let i = 0; i < this.mStreams.length; i++) {
      if (this.mTools.compareByteVectors(this.mStreams[i].peerID, id))
        return this.mStreams[i];
    }
    return null;
  }

  quitMeeting() {
    this.setJoinMeetingView();
  }

  setCamMuted(isIt = true) {
    //todo: stop stream
    this.mCamMuted = isIt;
    if (isIt) {
      $(this.getBody).find('#camBtnOuter')[0].classList.remove('greenTxt');
      $(this.getBody).find('#camBtnOuter')[0].classList.add('redTxt');
      this.mCamBtn.classList.remove('fa-webcam');
      this.mCamBtn.classList.add('fa-webcam-slash');
    } else {
      $(this.getBody).find('#camBtnOuter')[0].classList.remove('redTxt');
      $(this.getBody).find('#camBtnOuter')[0].classList.add('greenTxt');
      this.mCamBtn.classList.remove('fa-webcam-slash');
      this.mCamBtn.classList.add('fa-webcam');
    }
    return true;
  }


  getSSMuted() {
    //todo: stop stream
    return this.mSSMuted;
  }

  toggleScreenSharing() {

  }

  get getCamMuted() {
    return this.mCamMuted;
  }
  setMicMuted(isIt = true) {
    //todo: stop stream
    this.mMicMuted = isIt;
    if (isIt) {
      $(this.getBody).find('#micBtnOuter')[0].classList.add('redTxt');
      $(this.getBody).find('#micBtnOuter')[0].classList.remove('greenTxt');
      this.mMicBtn.classList.remove('fa-microphone-alt');
      this.mMicBtn.classList.add('fa-microphone-alt-slash');
    } else {
      $(this.getBody).find('#micBtnOuter')[0].classList.remove('redTxt');
      $(this.getBody).find('#micBtnOuter')[0].classList.add('greenTxt');
      this.mMicBtn.classList.remove('fa-microphone-alt-slash');
      this.mMicBtn.classList.add('fa-microphone-alt');
    }
    return true;
  }

  setSSMuted(isIt = true) {
    //todo: stop stream
    this.mSSMuted = isIt;
    if (isIt) {
      let slash = $.parseHTML("<i id='slash' class='fas fa-slash fa-stack-1x'></i>");

      $(this.getBody).find('#ssBtnOuter')[0].classList.add('redTxt');
      $(this.getBody).find('#ssBtnOuter')[0].classList.remove('greenTxt');
      $(this.getBody).find('#ssBtnOuter').append(slash);
    } else {
      $(this.getBody).find('#ssBtnOuter')[0].classList.remove('redTxt');
      $(this.getBody).find('#ssBtnOuter')[0].classList.add('greenTxt');
      $(this.getBody).find('#ssBtnOuter').find('#slash').remove();
    }
    return true;
  }

  get getMicMuted() {
    return this.mMicMuted;
  }

  toggleChatSize() {
    if (this.mChatMaxed) {
      $(this.getBody).find('#chat')[0].classList.remove('chatMaxed');
      this.mChatMaxed = false;
    } else {

      $(this.getBody).find('#chat')[0].classList.add('chatMaxed');
      this.mChatMaxed = true;
    }

  }

  toggleCam() {
    let dummy = this.mSwarm.mSwarmManager.getDummyVideoStream;
    this.setLocalStreamInUI(dummy);
    this.mSwarm.replaceVideoTrack(dummy.getVideoTracks()[0]);
    this.setCamMuted(!this.mCamMuted);
    if (!this.mSSMuted)
      this.setSSMuted();
    this.updateEffectiveVideo().then(
      this.updateEffectiveOutgressCapabilities());
  }


  toggleSS() {
    let dummy = this.mSwarm.mSwarmManager.getDummyVideoStream;
    this.setLocalStreamInUI(dummy);
    this.mSwarm.replaceVideoTrack(dummy.getVideoTracks()[0]);
    if (!this.mCamMuted)
      this.setCamMuted();
    this.setSSMuted(!this.mSSMuted);
    this.updateEffectiveOutgressCapabilities();
  }

  toggleMic() {
    this.setMicMuted(!this.mMicMuted);
    this.updateEffectiveOutgressCapabilities();
  }

  async updateEffectiveVideo() {
    if (!this.mSSMuted) {
      let stream = this.mSwarm.mSwarmManager.startCapture().then(function(stream) {
        if (stream == null)
          return;
        this.mSwarm.setInitialVideoTrack = stream.getVideoTracks()[0];
        if (stream != null && stream.getVideoTracks().length > 0) {
          let track = stream.getVideoTracks()[0];
          this.setLocalStreamInUI(stream);
          this.mSwarm.replaceVideoTrack(track)
        }
      }.bind(this))
    } else {
      let localStream = this.mSwarm.mSwarmManager.getLocalStream;
      if (localStream != null && localStream.getVideoTracks().length > 0) {
        let vt = localStream.getVideoTracks();
        this.setLocalStreamInUI(localStream);
        if (vt != null && vt.length > 0) {
          this.mSwarm.replaceVideoTrack(vt[0]);
        }

      }
    }

  }
  updateEffectiveOutgressCapabilities() {
    if (this.mSSMuted)
      this.mSwarm.mSwarmManager.stopCapture();

    if (this.mSwarm != null) {

      if (!(this.mCamMuted && this.mSSMuted) && !this.mMicMuted) {
        this.updateEffectiveVideo().then(
          this.mSwarm.setEffectiveOutgressCapabilities(eConnCapabilities.audioVideo));
      } else if (!(this.mCamMuted && this.mSSMuted) && this.mMicMuted) {
        this.updateEffectiveVideo().then(
          this.mSwarm.setEffectiveOutgressCapabilities(eConnCapabilities.video));
      } else if ((this.mCamMuted && this.mSSMuted) && !this.mMicMuted)
        this.mSwarm.setEffectiveOutgressCapabilities(eConnCapabilities.audio);
      else
        this.mSwarm.setEffectiveOutgressCapabilities(eConnCapabilities.data);
    }
    //app specific member functions - End
  }
}
const eChatMsgType = {
  text: 0,
  file: 1,
  typing: 2,
  avatar: 3
}

class CChatMsg {
  constructor(type = eChatMsgType.text, from = new ArrayBuffer(), to = new ArrayBuffer(), data = new ArrayBuffer(), timestamp = 0, external = false) {
    this.mTools = CTools.getInstance();
    this.mFromID = this.mTools.convertToArrayBuffer(from);
    this.mToID = this.mTools.convertToArrayBuffer(to);
    this.mVersion = 1;
    this.mData = data;
    this.mTimestamp = timestamp;
    this.mExternal = external; //NOT serialized. optimization only
    this.mSig = null;
    this.mInitialized = false;
    this.mType = type;

    if (this.mToID == null)
      this.mToID = new ArrayBuffer();
    if (this.mTimestamp == 0)
      this.mTimestamp = this.mTools.getTime();
  }

  get getType() {
    return this.mType;
  }
  get getSourceID() {
    return this.mFromID;
  }

  get getDestinationID() {
    return this.mToID;
  }
  get getTimestamp() {
    return this.mTimestamp;
  }
  //Returns a HTML5 rendering of the encapsulated message
  getRendering() {
    if (this.mType != eChatMsgType.text)
      return null; //for now

    try {
      let rendering = msgTemplate;
      rendering = rendering.replace('[originFlagField]', this.mExternal ? 'you' : 'me');
      //todo: check agent's status
      rendering = rendering.replace('[statusField]', 'status green');

      if (!this.mExternal)
        rendering = rendering.replace('[sourceField]', 'me');
      else {
        rendering = rendering.replace('[sourceField]', this.mTools.arrayBufferToString(this.mFromID));
      }

      rendering = rendering.replace('[timeField]', this.mTools.timestampToString(this.mTimestamp));

      rendering = rendering.replace('[msgTxtField]', this.mTools.arrayBufferToString(this.mData));

      return rendering;
    } catch (err) {
      return null;
    }
  }

  initialize() {
    if (this.mSwarm == null || this.mInitialized)
      return false;

    this.mInitialized = true;
    return true;
  }

  newDataChannelMessageEventHandler(eventData) {

  }
  get getVersion() {
    return this.mVersion;
  }
  get getSig() {
    return this.mSig;
  }

  sign(privKey) {
    let concat = new CDataConcatenator();
    concat.add(this.mFromID);
    concat.add(this.mToID);
    concat.add(this.mVersion);
    concat.add(this.mData);
    concat.add(this.mTimestamp);
    concat.add(this.mType);

    let sig = gCrypto.sign(privKey, concat.getData()); //Do NOT rely on BER encoding of data. that might differ slightly across implementations.this.getPackedData(false));
    if (sig.byteLength == 64) {
      this.mSig = sig;
      return true;
    } else
      return false;
  }

  verifySignature(pubKey) {
    let concat = new CDataConcatenator();
    concat.add(this.mFromID);
    concat.add(this.mToID);
    concat.add(this.mVersion);
    concat.add(this.mData);
    concat.add(this.mTimestamp);
    concat.add(this.mType);
    if (gCrypto.verify(pubKey, concat.getData(), mSig))
      return true;
    else
      return false;
  }

  getPackedData(includeSig = true) {

    if (this.mSig == null)
      this.mSig = new ArrayBuffer();
    //we need to construct encoding iteratively due to optional fields
    let wrapperSeq = new asn1js.Sequence();
    wrapperSeq.valueBlock.value.push(new asn1js.Integer({
      value: this.mVersion
    }));

    let mainDataSeq = new asn1js.Sequence();
    mainDataSeq.valueBlock.value.push(new asn1js.Integer({
      value: this.mType
    }));

    mainDataSeq.valueBlock.value.push(new asn1js.OctetString({
      valueHex: this.mFromID
    }));
    mainDataSeq.valueBlock.value.push(new asn1js.OctetString({
      valueHex: this.mToID
    }));

    mainDataSeq.valueBlock.value.push(new asn1js.OctetString({
      valueHex: this.mTools.convertToArrayBuffer(this.mData)
    }));

    mainDataSeq.valueBlock.value.push(new asn1js.Integer({
      value: this.mTimestamp
    }));

    if (includeSig) {
      mainDataSeq.valueBlock.value.push(new asn1js.OctetString({
        valueHex: this.mSig
      }));
    }

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
      let toRet = new CChatMsg();
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
        toRet.mFromID = decoded_sequence2.valueBlock.value[1].valueBlock.valueHex;
        toRet.mToID = decoded_sequence2.valueBlock.value[2].valueBlock.valueHex;
        toRet.mData = decoded_sequence2.valueBlock.value[3].valueBlock.valueHex;
        toRet.mTimestamp = decoded_sequence2.valueBlock.value[4].valueBlock.valueDec;

        //decode the optional Variables
        if (decoded_sequence2.valueBlock.value.length > 5)
          toRet.mSig = decoded_sequence2.valueBlock.value[5].valueBlock.valueHex;
      }


      return toRet;
    } catch (error) {
      return false;
    }
  }
}
export default CMeeting;
