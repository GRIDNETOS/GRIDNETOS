import ReadOnlyService from "./js/services/ReadOnlyService.js";
import InfoService from "./js/services/InfoService.js";
import ConfigService from "./js/services/ConfigService.js";
import ThrottlingService from "./js/services/ThrottlingService.js";
import whiteboard from './js/whiteboard.js'
import {
  CWindow
} from "/lib/window.js"
import {
  CSwarmMsg
} from '/lib/swarmmsg.js'

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
  CTools,
  CDataConcatenator
} from './../../../lib/tools.js'

class CWhiteboardMsg {
  constructor(type = null, payload = null) {

    this.mType = gTools.convertToArrayBuffer(type);
    this.mPayload = gTools.convertToArrayBuffer(payload);
    this.mVersion = 1;
  }

  get getType() {
    return this.mType
  }
  get sourceID() {
    return this.mPayload;
  }

  get getVersion() {
    return this.mVersion;
  }

  getPayload(parseJSON = true) {

    if (parseJSON) {
      try {
        let str = CTools.getInstance().arrayBufferToString(this.mPayload);
        let obj = JSON.parse(str);
        return obj;
      } catch (error) {
        return null;
      }
      return obj
    } else return this.mPayload;
  }
  getPackedData() {

    let wrapperSeq = new asn1js.Sequence();
    wrapperSeq.valueBlock.value.push(new asn1js.Integer({
      value: this.mVersion
    }));

    let mainDataSeq = new asn1js.Sequence();

    mainDataSeq.valueBlock.value.push(new asn1js.OctetString({
      valueHex: this.mType //[todo:PauliX: optimize]
    }));

    mainDataSeq.valueBlock.value.push(new asn1js.OctetString({
      valueHex: this.mPayload
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
      let toRet = new CWhiteboardMsg();
      let decoded_sequence1, decoded_sequence2, decoded_asn1;
      //local variables - END

      decoded_asn1 = asn1js.fromBER(packedData);

      if (decoded_asn1.offset === (-1))
        return 0; // Error during decoding

      decoded_sequence1 = decoded_asn1.result;
      toRet.mVersion = decoded_sequence1.valueBlock.value[0].valueBlock.valueDec; //from the wrapper sequence

      if (toRet.mVersion == 1) {
        decoded_sequence2 = decoded_sequence1.valueBlock.value[1]; //get the main-data sequence
        toRet.mType = decoded_sequence2.valueBlock.value[0].valueBlock.valueHex; //[todo:PauliX: optimize]
        toRet.mPayload = decoded_sequence2.valueBlock.value[1].valueBlock.valueHex;
      }


      return toRet;
    } catch (error) {
      return false;
    }
  }
}

class WhiteboardInfo {
  static defaultScreenResolution = {
    w: 1000,
    h: 1000
  };



  //[todo:all:medium]: discuss global apps' registry containing their global properites
  //[progress]: as discussed on phone with TheOldWizard [01.05.21]: proceeding with initial implementation of the Package Manager

  //ensure all UI dApps are packed nicely as modules
  //discuss mechanics behind the UI dApps' delivery/search/install functionalities

  /**
   * @type {number}
   * @private
   */
  #nbConnectedUsers = 0;
  get nbConnectedUsers() {
    return this.#nbConnectedUsers;
  }

  /**
   * @type {Map<string, {w: number, h: number}>}
   * @private
   */
  #screenResolutionByClients = new Map();
  get screenResolutionByClients() {
    return this.#screenResolutionByClients;
  }

  /**
   * Variable to tell if these info have been sent or not
   *
   * @private
   * @type {boolean}
   */
  #hasNonSentUpdates = false;
  get hasNonSentUpdates() {
    return this.#hasNonSentUpdates;
  }

  incrementNbConnectedUsers() {
    this.#nbConnectedUsers++;
    this.#hasNonSentUpdates = true;
  }

  decrementNbConnectedUsers() {
    this.#nbConnectedUsers--;
    this.#hasNonSentUpdates = true;
  }

  hasConnectedUser() {
    return this.#nbConnectedUsers > 0;
  }

  /**
   * Store information about the client's screen resolution
   *
   * @param {string} clientId
   * @param {number} w client's width
   * @param {number} h client's hight
   */
  setScreenResolutionForClient(clientId, {
    w,
    h
  }) {
    this.#screenResolutionByClients.set(clientId, {
      w,
      h
    });
    this.#hasNonSentUpdates = true;
  }

  /**
   * Delete the stored information about the client's screen resoltion
   * @param clientId
   */
  deleteScreenResolutionOfClient(clientId) {
    this.#screenResolutionByClients.delete(clientId);
    this.#hasNonSentUpdates = true;
  }

  /**
   * Get the smallest client's screen size on a whiteboard
   * @return {{w: number, h: number}}
   */
  getSmallestScreenResolution() {
    const {
      screenResolutionByClients: resolutions
    } = this;
    return {
      w: Math.min(...Array.from(resolutions.values()).map((res) => res.w)),
      h: Math.min(...Array.from(resolutions.values()).map((res) => res.h)),
    };
  }

  infoWasSent() {
    this.#hasNonSentUpdates = false;
  }

  shouldSendInfo() {
    return this.#hasNonSentUpdates;
  }

  asObject() {
    const out = {
      nbConnectedUsers: this.#nbConnectedUsers,
    };

    if (config.frontend.showSmallestScreenIndicator) {
      out.smallestScreenResolution = this.getSmallestScreenResolution();
    }

    return out;
  }
}



/*
Raphael:
Starting Point: For the basic functions you could just give an other function as "sendFunction" in /src/js/main.js on line 159.
Replace the signaling_socket with the WebRTC broadcast. This function is call with all the things a client does on his whiteboard and the
content should be passed directly to the "whiteboard.handleEventsAndData" function on all the connected clients. (Take a look how its done at main: 68 for websockets)
*/
var whiteboardBody = "";

$.get("/dApps/whiteboard/src/body.html", function(data) {
  whiteboardBody = data;
  //  alert(whiteboardBody);
}, 'html'); // this is the change now its working


const urlParams = new URLSearchParams(window.location.search);
let whiteboardId = urlParams.get("whiteboardid");
const randomid = urlParams.get("randomid");

if (randomid) {
  whiteboardId = window.uuidv4();
  urlParams.delete("randomid");
  window.location.search = urlParams;
}

if (!whiteboardId) {
  whiteboardId = "myNewWhiteboard";
}

whiteboardId = unescape(encodeURIComponent(whiteboardId)).replace(/[^a-zA-Z0-9\-]/g, "");

//if (urlParams.get("whiteboardid") !== whiteboardId) {
//  urlParams.set("whiteboardid", whiteboardId);
//  window.location.search = urlParams;
//}

const myUsername = urlParams.get("username") || "unknown" + (Math.random() + "").substring(2, 6);
const accessToken = urlParams.get("accesstoken") || "";

// Custom Html Title
const title = urlParams.get("title");
if (title) {
  //  document.title = decodeURIComponent(title);
}

const subdir = window.wbGetSubDir;
//let signaling_socket;


class CWhiteboard extends CWindow {

  static getPackageID() {
    return "org.gridnetproject.UIdApps.whiteboard";
  }

  static getDefaultCategory() {
    return 'productivity';
  }
  swarmConnectionChangedEventHandler(event) {

    //Important: handle 'refreshUserBadges' when any other user leaves
    switch (event.status) {

      case eSwarmConnectionState.intial:
        break;
      case eSwarmConnectionState.negotiating:
        break;
      case eSwarmConnectionState.active:
        //  let peer = new CPeer(event.peerID, event.connection);
        //  this.addPeer(peer);
        break;
        break;
      case eSwarmConnectionState.closed:
        //  this.removeStream(event.peerID);
        //this.removePeer(event.peerID);
        break;
    }
  }


  newSwarmMessageEventHandler(e) {
    try {
      let data = e.data;
      let accessDenied = false;
      let content = null;
      let swarmMsg = e.message;

      //Security - BEGIN
      if (this.mSwarm.isPrivate && !e.connection.isAuthenticated) {
        return;
      }
      //Security - END
      switch (swarmMsg.protocolID) {
        case this.getProtocolID: //Whiteboard Protocol

          let msg = CWhiteboardMsg.instantiate(swarmMsg.dataBytes);
          content = msg.getPayload(); //CTools.getInstance().escapeAllContentStrings(msg.getPayload());
          let typeStr = gTools.arrayBufferToString(msg.mType);
          let a = 0;
          switch (typeStr) {
            //[todo:Paulix:medium]: do processing

            //ported from server - BEGIN
            case "joinWhiteboard":
              a++;

              const screenResolution = content["windowWidthHeight"];

              this.broadcast('whiteboardConfig', {
                common: config.frontend,
                whiteboardSpecific: {
                  correspondingReadOnlyWid: ReadOnlyBackendService.getReadOnlyId(
                    whiteboardId
                  ),
                  isReadOnly: ReadOnlyBackendService.isReadOnly(whiteboardId),
                },
              });
              /*
                if (accessToken === "" || accessToken == content["at"]) {
                      whiteboardId = content["wid"];

                      socket.emit("whiteboardConfig", {
                          common: config.frontend,
                          whiteboardSpecific: {
                              correspondingReadOnlyWid: ReadOnlyBackendService.getReadOnlyId(
                                  whiteboardId
                              ),
                              isReadOnly: ReadOnlyBackendService.isReadOnly(whiteboardId),
                          },
                      });

                      socket.join(whiteboardId); //Joins room name=wid
                      const screenResolution = content["windowWidthHeight"];
                      WhiteboardInfoBackendService.join(socket.id, whiteboardId, screenResolution);
                  } else {
                      socket.emit("wrongAccessToken", true);
                  }*/
              break;
            case "updateScreenResolution":
              a++;

              screenResolution = content["windowWidthHeight"];
              /*  WhiteboardInfoBackendService.setScreenResolution(
                    socket.id,
                    whiteboardId,
                    screenResolution
                );*/

              break;

              //ported from server - END
            case "whiteboardConfig":
              this.wbConfigService.initFromServer(content); //now it's happening from the other peer not server
              // Inti whiteboard only when we have the config from the server
              this.initWhiteboard();
              break;

            case "whiteboardInfoUpdate":
              this.wbInfoService.updateInfoFromServer(content);
              this.mWhiteboard.updateSmallestScreenResolution();
              break;

            case "drawToWhiteboard":
              this.mWhiteboard.handleEventsAndData(content, true);
              this.wbInfoService.incrementNbMessagesReceived();
              break;

            case "refreshUserBadges":
              this.mWhiteboard.refreshUserBadges();
              break;


            case "wrongAccessToken":
              if (!accessDenied) {
                accessDenied = true;
                this.showBasicAlert("Access denied! Wrong accessToken!");
              }
              break;


            default:

              //ignore otherwise
              break;
          }
      }

    } catch (error) {
      CTools.getInstance().logEvent('Invalid datagram received.', eLogEntryCategory.dApp, 1, eLogEntryType.notification, this);

    }
  }
  static getIcon() {
    return `data:image/png;base64,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`;
  }


  constructor(positionX, positionY, width, height) {

    //Important: need to call the parent's constructor first, before accessing this
    super(positionX, positionY, width, height, whiteboardBody, "Local Only - ⋮⋮⋮ Whiteboard", CWhiteboard.getIcon(), true); //use Shadow-DOM by default
    this.mMetaParser = new CVMMetaParser();
    this.mSwarmManager = CVMContext.getInstance().getSwarmsManager;
    this.setProtocolID = 256;
    this.mSwarm = null;
    this.mLastHeightRearangedAt = 0;
    this.mLastWidthRearangedAt = 0;
    this.mErrorMsg = "";

    //register for network events
    CVMContext.getInstance().addVMMetaDataListener(this.newVMMetaDataCallback.bind(this), this.mID);
    CVMContext.getInstance().addNewDFSMsgListener(this.newDFSMsgCallback.bind(this), this.mID);
    CVMContext.getInstance().addNewGridScriptResultListener(this.newGridScriptResultCallback.bind(this), this.mID);

    //[todo:PauliX:urgent:24h]:
    //1) subscribe for WebRTC Swarm events
    //2) make app logic-related signaling flow through a WebRTC swam one to which user is currently connected to
    //3) ^ take a look at how this is acomplished for eMeeting UI dApp (add a new dApp's protocol identifier alongisde eMeeting)
    //4) remove unnecessary event handlers (DFS)

    //[todo:Mike:urgent:24h]: investigate UI scalling and related stuff

    this.loadLocalData();
    this.mControllerThreadInterval = 1000;
    this.mControllerExecuting = false;
    this.mController = 0;

  }

  get getSwarm() {
    return this.mSwarm;
  }

  dataChannelStateChangeEventhandler(event) {

  }
  genWindowTitle() {
    let swarm = this.mSwarm;
    let title = '⋮⋮⋮ Whiteboard';

    if (swarm) {
      title = "Conference '" + (this.mTools.lengthOf(this.mSwarm.trueID) ? this.mTools.bytesToString(this.mSwarm.trueID) : this.mTools.bytesToString(this.mSwarm.getID)) + "'" + (swarm.isPrivate && swarm.isDedicatedPSK ? " 🔒" : "") + " (connected as " + this.mTools.bytesToString(this.mSwarm.getMyID) + ") - ⋮⋮⋮ Whiteboard";
    }
    return title;
  }

  //This gets called whenever Authorization requirements for this swarm change.
  swarmAuthChangedEventHandler(e) {
    switch (e.auth) {
      case eSwarmAuthRequirement.open:

        break;
      case eSwarmAuthRequirement.PSK_ZK:

        break;
    }
    this.setTitle(this.genWindowTitle());
  }
  //Broadcasts a JSON object, which gets serialized internally to the currently connected Swarm-memebers
  //of the currently active Swarm.
  //obj - JSON object
  broadcast(type, obj) {
    if (!this.mSwarm || !obj)
      return;

    //We have 3-levels of encapsulation here.
    // 1) app level
    // 2) Swarm level
    // 3) universal, protocol-invariant datagram.

    let serializedDatagram = null;
    let appContainerMsg = new CWhiteboardMsg(type, JSON.stringify(obj));
    serializedDatagram = appContainerMsg.getPackedData();
    let swarmContainerMsg = new CSwarmMsg(eSwarmMsgType.binary, this.mSwarm.getMyID);
    swarmContainerMsg.data = appContainerMsg.getPackedData();
    this.mSwarm.sendSwarmMessage(swarmContainerMsg, this.getProtocolID);
    //swarmContainerMsg.data = serializedDatagram;
    //  serializedDatagram = swarmContainerMsg.getPackedData();
    //let osWrapperMsg = new CNetMsg(this.getProtocolID, eNetReqType.notification, serializedDatagram);
    //serializedDatagram = osWrapperMsg.getPackedData();

    //this.mSwarm.sendData(serializedNetMsg);
  }

  set setSwarm(swarm) {
    this.mSwarm = swarm;
    //[todo:Paulix:medium]: remove event handlers when swarm connection ends

    if (swarm != null) {
      this.setTitle(this.genWindowTitle());
      this.mSwarm.addSwarmConnectionStateChangeEventListener(this.swarmConnectionChangedEventHandler.bind(this), this.mID);

      //this.mSwarm.addDataChannelMessageEventListener(this.newSwarmMessageEventHandler.bind(this), this.getID);
      //^ we could be using the above low-level data-listener. Were we to do so, we would be attempting to instantiate CNetMsg and then CSwarmMsg
      //the the received bytes ourselves. Instead, we rel on a higher-level notofication  addSwarmMessageListener().
      this.mSwarm.addSwarmMessageListener(this.newSwarmMessageEventHandler.bind(this), this.getID);


      this.mSwarm.addDataChannelStateChangeEventListener(this.dataChannelStateChangeEventhandler.bind(this), this.mID);
      //security, keep listeing for changes to AUTH requirements
      this.mSwarm.addSwarmAuthRequirementsChangedEventListener(this.swarmAuthChangedEventHandler, this.getID);

      this.broadcast('joinWhiteboard', {
        wid: whiteboardId,
        at: accessToken,
        windowWidthHeight: {
          w: $(window).width(),
          h: $(window).height()
        }
      });
    }

  }

  initialize() //called only when app needs a thread/processing queue of its own, one seperate from the Window's
  //internal processing queue
  {
    this.initWhiteboardPostDOM();

    this.mController = CVMContext.getInstance().createJSThread(this.controllerThreadF.bind(this), this.getProcessID, this.mControllerThreadInterval);
  }

  controllerThreadF() {
    if (this.mControllerExecuting)
      return false;

    this.mControllerExecuting = true; //mutex protection

    //operational logic - BEGIN
    this.refreshSwarms();
    //operational logic - END

    this.mControllerExecuting = false;

  }
  onScroll(event) {
    super.onScroll(event);
    this.mWhiteboard.updateOnScroll.bind(this.mWhiteboard)();
  }

  finishResize(isFallbackEvent) { //Overloaded window-resize Event
    //called on finish of resize-animation ie. maxWindow, minWindow
    super.finishResize(isFallbackEvent);
    /*  signaling_socket.emit("updateScreenResolution", {
          at: accessToken,
          windowWidthHeight: { w: $(window).width(), h: $(window).height() },
      });*/

    this.mWhiteboard.updateOnResize.bind(this.mWhiteboard)();

    //let the others know
    this.broadcast('updateScreenResolution', {
      at: accessToken,
      windowWidthHeight: {
        w: this.getWidth,
        h: this.getHeight
      },
    });
  }

  stopResize(handle) { //fired when mouse-Resize ends
    super.stopResize(handle);
    this.mWhiteboard.updateOnResize.bind(this.mWhiteboard)();
  }
  preInit() {

    let style = document.createElement('style');
    style.textContent = '.picker_wrapper.no_alpha .picker_alpha{display:none}.picker_wrapper.no_editor .picker_editor{position:absolute;z-index:-1;opacity:0}.picker_wrapper.no_cancel .picker_cancel{display:none}.layout_default.picker_wrapper{display:-webkit-box;display:flex;-webkit-box-orient:horizontal;-webkit-box-direction:normal;flex-flow:row wrap;-webkit-box-pack:justify;justify-content:space-between;-webkit-box-align:stretch;align-items:stretch;font-size:10px;width:25em;padding:.5em}.layout_default.picker_wrapper input,.layout_default.picker_wrapper button{font-size:1rem}.layout_default.picker_wrapper>*{margin:.5em}.layout_default.picker_wrapper::before{content:\'\';display:block;width:100%;height:0;-webkit-box-ordinal-group:2;order:1}.layout_default .picker_slider,.layout_default .picker_selector{padding:1em}.layout_default .picker_hue{width:100%}.layout_default .picker_sl{-webkit-box-flex:1;flex:1 1 auto}.layout_default .picker_sl::before{content:\'\';display:block;padding-bottom:100%}.layout_default .picker_editor{-webkit-box-ordinal-group:2;order:1;width:6.5rem}.layout_default .picker_editor input{width:100%;height:100%}.layout_default .picker_sample{-webkit-box-ordinal-group:2;order:1;-webkit-box-flex:1;flex:1 1 auto}.layout_default .picker_done,.layout_default .picker_cancel{-webkit-box-ordinal-group:2;order:1}.picker_wrapper{box-sizing:border-box;background:#f2f2f2;box-shadow:0 0 0 1px silver;cursor:default;font-family:sans-serif;color:#444;pointer-events:auto}.picker_wrapper:focus{outline:none}.picker_wrapper button,.picker_wrapper input{box-sizing:border-box;border:none;box-shadow:0 0 0 1px silver;outline:none}.picker_wrapper button:focus,.picker_wrapper button:active,.picker_wrapper input:focus,.picker_wrapper input:active{box-shadow:0 0 2px 1px dodgerblue}.picker_wrapper button{padding:.4em .6em;cursor:pointer;background-color:whitesmoke;background-image:-webkit-gradient(linear, left bottom, left top, from(gainsboro), to(transparent));background-image:linear-gradient(0deg, gainsboro, transparent)}.picker_wrapper button:active{background-image:-webkit-gradient(linear, left bottom, left top, from(transparent), to(gainsboro));background-image:linear-gradient(0deg, transparent, gainsboro)}.picker_wrapper button:hover{background-color:white}.picker_selector{position:absolute;z-index:1;display:block;-webkit-transform:translate(-50%, -50%);transform:translate(-50%, -50%);border:2px solid white;border-radius:100%;box-shadow:0 0 3px 1px #67b9ff;background:currentColor;cursor:pointer}.picker_slider .picker_selector{border-radius:2px}.picker_hue{position:relative;background-image:-webkit-gradient(linear, left top, right top, from(red), color-stop(yellow), color-stop(lime), color-stop(cyan), color-stop(blue), color-stop(magenta), to(red));background-image:linear-gradient(90deg, red, yellow, lime, cyan, blue, magenta, red);box-shadow:0 0 0 1px silver}.picker_sl{position:relative;box-shadow:0 0 0 1px silver;background-image:-webkit-gradient(linear, left top, left bottom, from(white), color-stop(50%, rgba(255,255,255,0))),-webkit-gradient(linear, left bottom, left top, from(black), color-stop(50%, rgba(0,0,0,0))),-webkit-gradient(linear, left top, right top, from(gray), to(rgba(128,128,128,0)));background-image:linear-gradient(180deg, white, rgba(255,255,255,0) 50%),linear-gradient(0deg, black, rgba(0,0,0,0) 50%),linear-gradient(90deg, gray, rgba(128,128,128,0))}.picker_alpha,.picker_sample{position:relative;background:url("data:image/svg+xml,%3Csvg xmlns=\'http://www.w3.org/2000/svg\' width=\'2\' height=\'2\'%3E%3Cpath d=\'M1,0H0V1H2V2H1\' fill=\'lightgrey\'/%3E%3C/svg%3E") left top/contain white;box-shadow:0 0 0 1px silver}.picker_alpha .picker_selector,.picker_sample .picker_selector{background:none}.picker_editor input{font-family:monospace;padding:.2em .4em}.picker_sample::before{content:\'\';position:absolute;display:block;width:100%;height:100%;background:currentColor}.picker_arrow{position:absolute;z-index:-1}.picker_wrapper.popup{position:absolute;z-index:2;margin:1.5em}.picker_wrapper.popup,.picker_wrapper.popup .picker_arrow::before,.picker_wrapper.popup .picker_arrow::after{background:#0f77a8; border-radius: 0.3em; box-shadow: 0 0 13px 5px rgb(6 25 29 / 69%);}.picker_wrapper.popup .picker_arrow{width:3em;height:3em;margin:0}.picker_wrapper.popup .picker_arrow::before,.picker_wrapper.popup .picker_arrow::after{content:"";display:block;position:absolute;top:0;left:0;z-index:-99}.picker_wrapper.popup .picker_arrow::before{width:100%;height:100%;-webkit-transform:skew(45deg);transform:skew(45deg);-webkit-transform-origin:0 100%;transform-origin:0 100%}.picker_wrapper.popup .picker_arrow::after{width:150%;height:150%;box-shadow:none}.popup.popup_top{bottom:100%;left:0}.popup.popup_top .picker_arrow{bottom:0;left:0;-webkit-transform:rotate(-90deg);transform:rotate(-90deg)}.popup.popup_bottom{top:100%;left:0}.popup.popup_bottom .picker_arrow{top:0;left:0;-webkit-transform:rotate(90deg) scale(1, -1);transform:rotate(90deg) scale(1, -1)}.popup.popup_left{top:0;right:100%}.popup.popup_left .picker_arrow{top:0;right:0;-webkit-transform:scale(-1, 1);transform:scale(-1, 1)}.popup.popup_right{top:0;left:100%}.popup.popup_right .picker_arrow{top:0;left:0}';
    this.getBody.appendChild(style);

    // the services are made window specific [meeting 29.04.21]
    this.wbReadOnlyService = new ReadOnlyService(this);
    this.wbConfigService = new ConfigService(this);
    this.wbInfoService = new InfoService(this, this.wbConfigService);
    this.wbInfoService = new InfoService(this);
    this.wbThrottlingService = new ThrottlingService(this, this.wbConfigService);

    this.mWhiteboard = new whiteboard(this.wbReadOnlyService, this.wbConfigService, this.wbInfoService, this.wbThrottlingService);
    // Set correct width height on mobile browsers
    const isChrome = /Chrome/.test(navigator.userAgent) && /Google Inc/.test(navigator.vendor);
    if (isChrome) {
      $(this.getBody).append(
        '<meta name="viewport" content="width=device-width, initial-scale=0.52, maximum-scale=1" />'
      );
    } else {
      $(this.getBody).append('<meta name="viewport" content="width=1400" />');
    }

    //this.main();
  }

  showBasicAlert(html, newOptions) {
    var options = {
      header: "INFO MESSAGE",
      okBtnText: "Ok",
      headercolor: "#d25d5d",
      hideAfter: false,
      onOkClick: false,
    };
    if (newOptions) {
      for (var i in newOptions) {
        options[i] = newOptions[i];
      }
    }
    var alertHtml = $(
      '<div class="basicalert" style="position:absolute; left:0px; width:100%; top:70px; font-family: monospace;">' +
      '<div style="width: 30%; margin: auto; background: #aaaaaa; border-radius: 5px; font-size: 1.2em; border: 1px solid gray;">' +
      '<div style="border-bottom: 1px solid #676767; background: ' +
      options["headercolor"] +
      '; padding-left: 5px; font-size: 0.8em;">' +
      options["header"] +
      '<div style="float: right; margin-right: 4px; color: #373737; cursor: pointer;" class="closeAlert">x</div></div>' +
      '<div style="padding: 10px;" class="htmlcontent"></div>' +
      '<div style="height: 20px; padding: 10px;"><button class="modalBtn okbtn" style="float: right;">' +
      options["okBtnText"] +
      "</button></div>" +
      "</div>" +
      "</div>"
    );
    alertHtml.find(".htmlcontent").append(html);
    $(this.getBody).append(alertHtml);
    alertHtml
      .find(".okbtn")
      .off("click")
      .click(function(event) {
        if (options.onOkClick) {
          options.onOkClick();
        }
        alertHtml.remove();
      }.bind(this));
    alertHtml
      .find(".closeAlert")
      .off("click")
      .click(function(event) {
        alertHtml.remove();
      }.bind(this));

    if (options.hideAfter) {
      setTimeout(function() {
        alertHtml.find(".okbtn").click();
      }.bind(this), 1000 * options.hideAfter);
    }
  }

  // verify if given url is url to an image
  isValidImageUrl(url, callback) {
    let img = new Image();
    let timer = null;
    img.onerror = img.onabort = function() {
      clearTimeout(timer);
      callback(false);
    }.bind(this);
    img.onload = function() {
      clearTimeout(timer);
      callback(true);
    }.bind(this);
    timer = setTimeout(function() {
      callback(false);
    }.bind(this), 2000);
    img.src = url;
  }

  initWhiteboard() {

    // handle pasting from clipboard
    let body = this.getBody;
    body.addEventListener("paste", function(e) {
      if ($(body).find(".basicalert").length > 0) {
        return;
      }
      if (e.clipboardData) {
        var items = e.clipboardData.items;
        var imgItemFound = false;
        if (items) {
          // Loop through all items, looking for any kind of image
          for (var i = 0; i < items.length; i++) {
            if (items[i].type.indexOf("image") !== -1) {
              imgItemFound = true;
              // We need to represent the image as a file,
              var blob = items[i].getAsFile();

              var reader = new window.FileReader();
              reader.readAsDataURL(blob);
              reader.onloadend = function() {

                CTools.getInstance().logEvent("Uploading an image!", eLogEntryCategory.dApp, 1, eLogEntryType.notification, this);
                let base64data = reader.result;
                this.uploadImgAndAddToWhiteboard(base64data);
              }.bind(this);
            }
          }
        }

        if (!imgItemFound && this.mWhiteboard.tool != "text" && this.mWhiteboard.tool != "stickynote") {
          showBasicAlert(
            "Please Drag&Drop the image or pdf into the whiteboard. (Browsers don't allow copy+paste from the filesystem directly)"
          );
        }
      }
    });
  }

  saveWhiteboardToWebdav(base64data, webdavaccess, callback) {
    let date = +new Date();
    $.ajax({
      type: "POST",
      url: document.URL.substr(0, document.URL.lastIndexOf("/")) + "/api/upload",
      data: {
        imagedata: base64data,
        whiteboardId: whiteboardId,
        date: date,
        at: accessToken,
        webdavaccess: JSON.stringify(webdavaccess),
      },
      success: function(msg) {
        showBasicAlert("Whiteboard was saved to Webdav!", {
          headercolor: "#5c9e5c",
        });
        CTools.getInstance().logEvent("Image uploaded!", eLogEntryCategory.dApp, 1, eLogEntryType.notification, this);

        callback();
      },
      error: function(err) {
        console.error(err);
        if (err.status == 403) {
          showBasicAlert(
            "Could not connect to Webdav folder! Please check the credentials and paths and try again!"
          );
        } else {
          showBasicAlert("Unknown Webdav error! ", err);
        }
        callback(err);
      },
    });
  }


  uploadImgAndAddToWhiteboard(base64data) {
    const date = +new Date();
    $.ajax({
      type: "POST",
      url: document.URL.substr(0, document.URL.lastIndexOf("/")) + "/api/upload",
      data: {
        imagedata: base64data,
        whiteboardId: whiteboardId,
        date: date,
        at: accessToken,
      },
      success: function(msg) {
        const {
          correspondingReadOnlyWid
        } = this.wbConfigService;
        const filename = `${correspondingReadOnlyWid}_${date}.png`;
        const rootUrl = document.URL.substr(0, document.URL.lastIndexOf("/"));
        this.mWhiteboard.addImgToCanvasByUrl(
          `${rootUrl}/uploads/${correspondingReadOnlyWid}/${filename}`
        ); //Add image to canvas

        CTools.getInstance().logEvent("Image uploaded!", eLogEntryCategory.dApp, 1, eLogEntryType.notification, this);
      }.bind(this),
      error: function(err) {
        showBasicAlert("Failed to upload frame: " + JSON.stringify(err));
      },
    });
  }


  // verify if filename refers to an image
  isImageFileName(filename) {
    let extension = filename.split(".")[filename.split(".").length - 1];
    let known_extensions = ["png", "jpg", "jpeg", "gif", "tiff", "bmp", "webp"];
    return known_extensions.includes(extension.toLowerCase());
  }

  // verify if filename refers to an pdf
  isPDFFileName(filename) {
    let extension = filename.split(".")[filename.split(".").length - 1];
    let known_extensions = ["pdf"];
    return known_extensions.includes(extension.toLowerCase());
  }

  refreshSwarms() {
    //[todo:Paulix:low] make it possible to choose from active swarms in UI
    //for now just use the first swarm available
    let swarmIDs = CVMContext.getInstance().getSwarmsManager.getSwarmIDs;
    let swarm = null;
    let swarmWasSet = false;
    //this.clearComboBox(this.mSwarmsSendTabCB);
    for (let i = 0; i < swarmIDs.length; i++) {
      if ((swarm = this.mSwarmManager.findSwarmByID(swarmIDs[i])) && swarm.getState == eSwarmState.active) {
        let current = this.mSwarm;
        if (current) {
          if (current.getID == swarm.getID)
            return;
        }

        this.setSwarm = swarm;
        swarmWasSet = true;
        break;
      }
      //    this.addComboBoxElement(this.mSwarmsSendTabCB, id);
    }

    if (!swarmWasSet && !this.mToldToJoinSwarm) {
      this.mToldToJoinSwarm = true;
      this.mTools.sleeper(5000).then(function() {
        this.showMessageBox('Invite your friends ? 😊', 'Use the eMeeting app to connect with others..', eNotificationType.notification, this);
      });
    }
  }
  /*
      for (let i = 0; i < swarmIDs.length; i++) {
        let id = gTools.arrayBufferToString(swarmIDs[i]);

    //    this.addComboBoxElement(this.mSwarmsSendTabCB, id);

      }
    } else {
      //this.addComboBoxElement(this.mSwarmsSendTabCB, 'no available', true);
    }
  }

  main() {
      this.wbConfigService.initFromServer(new WhiteboardInfo());
    this.initWhiteboard();
    /*  signaling_socket = io("", { path: subdir + "/ws-api" }); // Connect even if we are in a subdir behind a reverse proxy

      signaling_socket.on("connect", function () {
          console.log("Websocket connected!");

          signaling_socket.on("whiteboardConfig", (serverResponse) => {
                this.wbConfigService.initFromServer(serverResponse);
              // Inti whiteboard only when we have the config from the server
              initWhiteboard();
          });

          signaling_socket.on("whiteboardInfoUpdate", (info) => {
              this.wbInfoService.updateInfoFromServer(info);
              this.mWhiteboard .updateSmallestScreenResolution();
          });

          signaling_socket.on("drawToWhiteboard", function (content) {
              this.mWhiteboard .handleEventsAndData(content, true);
              this.wbInfoService.incrementNbMessagesReceived();
          });

          signaling_socket.on("refreshUserBadges", function () {
              this.mWhiteboard .refreshUserBadges();
          });

          let accessDenied = false;
          signaling_socket.on("wrongAccessToken", function () {
              if (!accessDenied) {
                  accessDenied = true;
                  showBasicAlert("Access denied! Wrong accessToken!");
              }
          });

          signaling_socket.emit("joinWhiteboard", {
              wid: whiteboardId,
              at: accessToken,
              windowWidthHeight: { w: $(window).width(), h: $(window).height() },
          });
      });
  }*/
  open() { //Overloaded Window-Opening Event
    this.mContentReady = false;
    super.open();
    this.preInit();
    this.initialize();
    //modify content here

  }

  initWhiteboardPostDOM() {
    // by default set in readOnly mode
    this.wbReadOnlyService.activateReadOnlyMode();

    if (urlParams.get("webdav") === "true") {
      $(this.getBody).find("#uploadWebDavBtn").show();
    }

    this.mWhiteboard.loadWhiteboard(this, {
      //Load the whiteboard
      whiteboardId: whiteboardId,
      username: btoa(myUsername),
      backgroundGridUrl: "./dApps/whiteboard/src/images/" + this.wbConfigService.backgroundGridImage,
      sendFunction: function(content) {
        if (this.wbReadOnlyService.readOnlyActive) return;
        //ADD IN LATER THROUGH CONFIG
        // if (content.t === 'cursor') {
        //     if (whiteboard.drawFlag) return;
        // }

        content["at"] = accessToken;
        this.broadcast('drawToWhiteboard', content);

        this.wbInfoService.incrementNbMessagesSent();
      }.bind(this),
    });

    // request whiteboard from server
    /*$.get(subdir + "/api/loadwhiteboard", { wid: whiteboardId, at: accessToken }).done(
        function (data) {
            this.mWhiteboard .loadData(data);
        }
    );*/
    let body = this.getBody;
    //[todo:Paulix:low]: make this rely on GRIDNET OS window events instead of raw JS
    //Update: this is currently happening within the overlaoded GRIDNET-OS windows event.
    $(body).resize(function() {
      /*  signaling_socket.emit("updateScreenResolution", {
            at: accessToken,
            windowWidthHeight: { w: $(window).width(), h: $(window).height() },
        });*/
    }.bind(this));

    /*----------------/
    Whiteboard actions
    /----------------*/

    var tempLineTool = false;
    var strgPressed = false;
    //Handle key actions
    $(body).on("keydown", function(e) {
      if (e.which == 16) {
        if (whiteboard.tool == "pen" && !strgPressed) {
          tempLineTool = true;
          this.mWhiteboard.ownCursor.hide();
          if (whiteboard.drawFlag) {
            this.mWhiteboard.mouseup({
              offsetX: this.mWhiteboard.prevPos.x,
              offsetY: this.mWhiteboard.prevPos.y,
            });
            window.wbShortcutFunctions.setTool_line();
            this.mWhiteboard.mousedown({
              offsetX: this.mWhiteboard.prevPos.x,
              offsetY: this.mWhiteboard.prevPos.y,
            });
          } else {
            window.wbShortcutFunctions.setTool_line();
          }
        }
        this.mWhiteboard.pressedKeys["shift"] = true; //Used for straight lines...
      } else if (e.which == 17) {
        strgPressed = true;
      }
      //console.log(e.which);
    }.bind(this));
    $(body).on("keyup", function(e) {
      if (e.which == 16) {
        if (tempLineTool) {
          tempLineTool = false;
          window.wbShortcutFunctions.setTool_pen();
          this.mWhiteboard.ownCursor.show();
        }
        this.mWhiteboard.pressedKeys["shift"] = false;
      } else if (e.which == 17) {
        strgPressed = false;
      }
    }.bind(this));

    //Load keybindings from keybinds.js to given functions
    Object.entries(window.wbKeybinds).forEach(([key, functionName]) => {
      const associatedShortcutFunction = window.wbShortcutFunctions[functionName];
      if (associatedShortcutFunction) {
        keymage(key, associatedShortcutFunction, {
          preventDefault: true
        });
      } else {
        console.error(
          "Function you want to keybind on key:",
          key,
          "named:",
          functionName,
          "is not available!"
        );
      }
    });

    // whiteboard clear button
    $(body).find("#whiteboardTrashBtn")
      .off("click")
      .click(function(event) {
        $(this.getBody).find("#whiteboardTrashBtnConfirm").show().focus();
        $(event.currentTarget).hide();
      }.bind(this));

    $(body).find("#whiteboardTrashBtnConfirm").mouseout(function(event) {
      $(event.currentTarget).hide();
      $(body).find("#whiteboardTrashBtn").show();
    }.bind(this));

    $(body).find("#whiteboardTrashBtnConfirm")
      .off("click")
      .click(function(event) {
        $(event.currentTarget).hide();
        $(this.getBody).find("#whiteboardTrashBtn").show();
        this.mWhiteboard.clearWhiteboard();
      }.bind(this));

    // undo button
    $(body).find("#whiteboardUndoBtn")
      .off("click")
      .click(function(event) {
        this.mWhiteboard.undoWhiteboardClick();
      }.bind(this));

    // redo button
    $(body).find("#whiteboardRedoBtn")
      .off("click")
      .click(function(event) {
        this.mWhiteboard.redoWhiteboardClick();
      }.bind(this));

    // view only
    $(body).find("#whiteboardLockBtn")
      .off("click")
      .click(function(event) {
        this.wbReadOnlyService.deactivateReadOnlyMode();
      }.bind(this));
    $(body).find("#whiteboardUnlockBtn")
      .off("click")
      .click(function(event) {
        this.wbReadOnlyService.activateReadOnlyMode();
      }.bind(this));
    $(body).find("#whiteboardUnlockBtn").hide();
    $(body).find("#whiteboardLockBtn").show();

    // switch tool
    $(body).find(".whiteboard-tool")
      .off("click")
      .click(function(event) {
        $(this.getBody).find(".whiteboard-tool").removeClass("active");
        $(event.currentTarget).addClass("active");
        var activeTool = $(event.currentTarget).attr("tool");
        this.mWhiteboard.setTool(activeTool);
        if (activeTool == "mouse" || activeTool == "recSelect") {
          $(this.getBody).find(".activeToolIcon").empty();
        } else {
          $(this.getBody).find(".activeToolIcon").html($(event.currentTarget).html()); //Set Active icon the same as the button icon
        }

        if (activeTool == "text" || activeTool == "stickynote") {
          $(this.getBody).find("#textboxBackgroundColorPickerBtn").show();
        } else {
          $(this.getBody).find("#textboxBackgroundColorPickerBtn").hide();
        }
      }.bind(this));

    // upload image button
    $(body).find("#addImgToCanvasBtn")
      .off("click")
      .click(function() {
        if (this.wbReadOnlyService.readOnlyActive) return;
        showBasicAlert("Please drag the image into the browser.");
      }.bind(this));

    // save image as imgae
    $(body).find("#saveAsImageBtn")
      .off("click")
      .click(function() {
        this.mWhiteboard.getImageDataBase64({
            imageFormat: this.wbConfigService.imageDownloadFormat,
            drawBackgroundGrid: this.wbConfigService.drawBackgroundGrid,
          },
          function(imgData) {
            var w = window.open("about:blank"); //Firefox will not allow downloads without extra window
            setTimeout(function() {
              //FireFox seems to require a setTimeout for this to work.
              var a = document.createElement("a");
              a.href = imgData;
              a.download = "whiteboard." + this.wbConfigService.imageDownloadFormat;
              $(body).appendChild(a);
              a.click();
              $(body).removeChild(a);
              setTimeout(function() {
                w.close();
              }, 100);
            }, 0);
          }
        );
      }.bind(this));

    // save image to json containing steps
    $(body).find("#saveAsJSONBtn")
      .off("click")
      .click(function() {
        var imgData = this.mWhiteboard.getImageDataJson();

        var w = window.open("about:blank"); //Firefox will not allow downloads without extra window
        setTimeout(function() {
          //FireFox seems to require a setTimeout for this to work.
          var a = document.createElement("a");
          a.href = window.URL.createObjectURL(new Blob([imgData], {
            type: "text/json"
          }));
          a.download = "whiteboard.json";
          $(body).appendChild(a);
          a.click();
          $(body).removeChild(a);
          setTimeout(function() {
            w.close();
          }, 100);
        }, 0);
      }.bind(this));

    $(body).find("#uploadWebDavBtn")
      .off("click")
      .click(function() {
        if ($(body).find(".webdavUploadBtn").length > 0) {
          return;
        }

        var webdavserver = localStorage.getItem("webdavserver") || "";
        var webdavpath = localStorage.getItem("webdavpath") || "/";
        var webdavusername = localStorage.getItem("webdavusername") || "";
        var webdavpassword = localStorage.getItem("webdavpassword") || "";
        var webDavHtml = $(
          "<div>" +
          "<table>" +
          "<tr>" +
          "<td>Server URL:</td>" +
          '<td><input class="webdavserver" type="text" value="' +
          webdavserver +
          '" placeholder="https://yourserver.com/remote.php/webdav/"></td>' +
          "<td></td>" +
          "</tr>" +
          "<tr>" +
          "<td>Path:</td>" +
          '<td><input class="webdavpath" type="text" placeholder="folder" value="' +
          webdavpath +
          '"></td>' +
          '<td style="font-size: 0.7em;"><i>path always have to start & end with "/"</i></td>' +
          "</tr>" +
          "<tr>" +
          "<td>Username:</td>" +
          '<td><input class="webdavusername" type="text" value="' +
          webdavusername +
          '" placeholder="username"></td>' +
          '<td style="font-size: 0.7em;"></td>' +
          "</tr>" +
          "<tr>" +
          "<td>Password:</td>" +
          '<td><input class="webdavpassword" type="password" value="' +
          webdavpassword +
          '" placeholder="password"></td>' +
          '<td style="font-size: 0.7em;"></td>' +
          "</tr>" +
          "<tr>" +
          '<td style="font-size: 0.7em;" colspan="3">Note: You have to generate and use app credentials if you have 2 Factor Auth activated on your dav/nextcloud server!</td>' +
          "</tr>" +
          "<tr>" +
          "<td></td>" +
          '<td colspan="2"><span class="loadingWebdavText" style="display:none;">Saving to webdav, please wait...</span><button class="modalBtn webdavUploadBtn"><i class="fas fa-upload"></i> Start Upload</button></td>' +
          "</tr>" +
          "</table>" +
          "</div>"
        );
        webDavHtml
          .find(".webdavUploadBtn")
          .off("click")
          .click(function() {
            var webdavserver = webDavHtml.find(".webdavserver").val();
            localStorage.setItem("webdavserver", webdavserver);
            var webdavpath = webDavHtml.find(".webdavpath").val();
            localStorage.setItem("webdavpath", webdavpath);
            var webdavusername = webDavHtml.find(".webdavusername").val();
            localStorage.setItem("webdavusername", webdavusername);
            var webdavpassword = webDavHtml.find(".webdavpassword").val();
            localStorage.setItem("webdavpassword", webdavpassword);
            this.mWhiteboard.getImageDataBase64({
                imageFormat: this.wbConfigService.imageDownloadFormat,
                drawBackgroundGrid: this.wbConfigService.drawBackgroundGrid,
              },
              function(base64data) {
                var webdavaccess = {
                  webdavserver: webdavserver,
                  webdavpath: webdavpath,
                  webdavusername: webdavusername,
                  webdavpassword: webdavpassword,
                };
                webDavHtml.find(".loadingWebdavText").show();
                webDavHtml.find(".webdavUploadBtn").hide();
                saveWhiteboardToWebdav(base64data, webdavaccess, function(err) {
                  if (err) {
                    webDavHtml.find(".loadingWebdavText").hide();
                    webDavHtml.find(".webdavUploadBtn").show();
                  } else {
                    webDavHtml.parents(".basicalert").remove();
                  }
                });
              }
            );
          }.bind(this));
        showBasicAlert(webDavHtml, {
          header: "Save to Webdav",
          okBtnText: "cancel",
          headercolor: "#0082c9",
        });
        // render newly added icons
        //  dom.i2svg();
      });

    // upload json containing steps
    $(body).find("#uploadJsonBtn")
      .off("click")
      .click(function() {
        $(this.getBody).find("#myFile").click();
      }.bind(this));

    $(body).find("#shareWhiteboardBtn")
      .off("click")
      .click(() => {
        function urlToClipboard(whiteboardId = null) {
          const {
            protocol,
            host,
            pathname,
            search
          } = window.location;
          const basePath = `${protocol}//${host}${pathname}`;
          const getParams = new URLSearchParams(search);

          // Clear ursername from get parameters
          getParams.delete("username");

          if (whiteboardId) {
            // override whiteboardId value in URL
            getParams.set("whiteboardid", whiteboardId);
          }

          const url = `${basePath}?${getParams.toString()}`;
          $(body).find("<textarea/>")
            .appendTo("body")
            .val(url)
            .select()
            .each(() => {
              document.execCommand("copy");
            })
            .remove();
        }

        // UI related
        // clear message
        $(body).find("#shareWhiteboardDialogMessage").toggleClass("displayNone", true);

        $(body).find("#shareWhiteboardDialog").toggleClass("displayNone", false);
        $(body).find("#shareWhiteboardDialogGoBack")
          .off("click")
          .click(() => {
            $(body).find("#shareWhiteboardDialog").toggleClass("displayNone", true);
          });

        $(body).find("#shareWhiteboardDialogCopyReadOnlyLink")
          .off("click")
          .click(function(e) {
            urlToClipboard(this.wbConfigService.correspondingReadOnlyWid);

            $(body).find("#shareWhiteboardDialogMessage")
              .toggleClass("displayNone", false)
              .text("Read-only link copied to clipboard ✓");
          }.bind(this));

        $(body).find("#shareWhiteboardDialogCopyReadWriteLink")
          .toggleClass("displayNone", this.wbConfigService.isReadOnly)
          .click(function(e) {
            $(body).find("#shareWhiteboardDialogMessage")
              .toggleClass("displayNone", false)
              .text("Read/write link copied to clipboard ✓");
            urlToClipboard();
          });
      });

    $(body).find("#displayWhiteboardInfoBtn")
      .off("click")
      .click(function(e) {
        this.wbInfoService.toggleDisplayInfo();
      }.bind(this));

    var btnsMini = false;
    $(body).find("#minMaxBtn")
      .off("click")
      .click(function(event) {
        if (!btnsMini) {
          $(this.getBody).find("#toolbar").find(".btn-group:not(.minGroup)").hide();
          $(event.currentTarget).find("#minBtn").hide();
          $(event.currentTarget).find("#maxBtn").show();
        } else {
          $(this.getBody).find("#toolbar").find(".btn-group").show();
          $(event.currentTarget).find("#minBtn").show();
          $(event.currentTarget).find("#maxBtn").hide();
        }
        btnsMini = !btnsMini;
      }.bind(this));

    // load json to whiteboard
    $(body).find("#myFile").on("change", function(e) {
      var file = $(body).getElementById("myFile").files[0];
      var reader = new FileReader();
      reader.onload = function(e) {
        try {
          var j = JSON.parse(e.target.result);
          this.mWhiteboard.loadJsonData(j);
        } catch (e) {
          showBasicAlert("File was not a valid JSON!");
        }
      };
      reader.readAsText(file);
      $(e.currentTarget).val("");
    }.bind(this));

    // On thickness slider change
    $(body).find("#whiteboardThicknessSlider").on("input", function(e) {
      if (this.wbReadOnlyService.readOnlyActive) return;
      this.mWhiteboard.setStrokeThickness($(e.currentTarget).val());
    }.bind(this));

    // handle drag&drop
    var dragCounter = 0;
    $(body).find("#whiteboardContainer").on("dragenter", function(e) {
      if (this.wbReadOnlyService.readOnlyActive) return;
      e.preventDefault();
      e.stopPropagation();
      dragCounter++;
      this.mWhiteboard.dropIndicator.show();
    }.bind(this));

    $(body).find("#whiteboardContainer").on("dragleave", function(e) {
      if (this.wbReadOnlyService.readOnlyActive) return;

      e.preventDefault();
      e.stopPropagation();
      dragCounter--;
      if (dragCounter === 0) {
        this.mWhiteboard.dropIndicator.hide();
      }
    }.bind(this));

    $(body).find("#whiteboardContainer").on("drop", function(e) {
      //Handle drop
      if (this.wbReadOnlyService.readOnlyActive) return;

      if (e.originalEvent.dataTransfer) {
        if (e.originalEvent.dataTransfer.files.length) {
          //File from harddisc
          e.preventDefault();
          e.stopPropagation();
          var filename = e.originalEvent.dataTransfer.files[0]["name"];
          if (this.isImageFileName(filename)) {
            var blob = e.originalEvent.dataTransfer.files[0];
            var reader = new window.FileReader();
            reader.readAsDataURL(blob);
            reader.onloadend = function() {
              const base64data = reader.result;
              uploadImgAndAddToWhiteboard(base64data);
            };
          } else if (this.isPDFFileName(filename)) {
            //Handle PDF Files
            var blob = e.originalEvent.dataTransfer.files[0];

            var reader = new window.FileReader();
            reader.onloadend = function() {
              var pdfData = new Uint8Array(this.result);

              var loadingTask = pdfjsLib.getDocument({
                data: pdfData
              });
              loadingTask.promise.then(
                function(pdf) {

                  CTools.getInstance().logEvent("PDF loaded", eLogEntryCategory.dApp, 1, eLogEntryType.notification, this);
                  var currentDataUrl = null;
                  var modalDiv = $(
                    "<div>" +
                    "Page: <select></select> " +
                    '<button style="margin-bottom: 3px;" class="modalBtn"><i class="fas fa-upload"></i> Upload to Whiteboard</button>' +
                    '<img style="width:100%;" src=""/>' +
                    "</div>"
                  );

                  modalDiv.find("select").change(function(e) {
                    showPDFPageAsImage(parseInt($(e.currentTarget).val()));
                  }.bind(this));

                  modalDiv
                    .find("button")
                    .off("click")
                    .click(function() {
                      if (currentDataUrl) {
                        $(body).find(".basicalert").remove();
                        this.uploadImgAndAddToWhiteboard(currentDataUrl);
                      }
                    }.bind(this));

                  for (var i = 1; i < pdf.numPages + 1; i++) {
                    modalDiv
                      .find("select")
                      .append('<option value="' + i + '">' + i + "</option>");
                  }

                  showBasicAlert(modalDiv, {
                    header: "Pdf to Image",
                    okBtnText: "cancel",
                    headercolor: "#0082c9",
                  });

                  // render newly added icons
                  //  dom.i2svg();

                  showPDFPageAsImage(1);

                  function showPDFPageAsImage(pageNumber) {
                    // Fetch the page
                    pdf.getPage(pageNumber).then(function(page) {

                      CTools.getInstance().logEvent("Page loaded", eLogEntryCategory.dApp, 1, eLogEntryType.notification, this);
                      var scale = 1.5;
                      var viewport = page.getViewport({
                        scale: scale
                      });

                      // Prepare canvas using PDF page dimensions
                      var canvas = $("<canvas></canvas>")[0];
                      var context = canvas.getContext("2d");
                      canvas.height = viewport.height;
                      canvas.width = viewport.width;

                      // Render PDF page into canvas context
                      var renderContext = {
                        canvasContext: context,
                        viewport: viewport,
                      };
                      var renderTask = page.render(renderContext);
                      renderTask.promise.then(function() {
                        var dataUrl = canvas.toDataURL("image/jpeg", 1.0);
                        currentDataUrl = dataUrl;
                        modalDiv.find("img").attr("src", dataUrl);

                        CTools.getInstance().logEvent("Page rendered", eLogEntryCategory.dApp, 1, eLogEntryType.notification, this);
                      });
                    });
                  }
                },
                function(reason) {
                  // PDF loading error

                  showBasicAlert(
                    "Error loading pdf as image! Check that this is a vaild pdf file!"
                  );
                  console.error(reason);
                }
              );
            };
            reader.readAsArrayBuffer(blob);
          } else {
            showBasicAlert("File must be an image!");
          }
        } else {
          //File from other browser

          var fileUrl = e.originalEvent.dataTransfer.getData("URL");
          var imageUrl = e.originalEvent.dataTransfer.getData("text/html");
          var rex = /src="?([^"\s]+)"?\s*/;
          var url = rex.exec(imageUrl);
          if (url && url.length > 1) {
            url = url[1];
          } else {
            url = "";
          }

          isValidImageUrl(fileUrl, function(isImage) {
            if (isImage && this.isImageFileName(url)) {
              this.mWhiteboard.addImgToCanvasByUrl(fileUrl);
            } else {
              isValidImageUrl(url, function(isImage) {
                if (isImage) {
                  if (this.isImageFileName(url) || url.startsWith("http")) {
                    this.mWhiteboard.addImgToCanvasByUrl(url);
                  } else {
                    this.uploadImgAndAddToWhiteboard(url); //Last option maybe its base64
                  }
                } else {
                  showBasicAlert("Can only upload Imagedata!");
                }
              });
            }
          }.bind(this));
        }
      }
      dragCounter = 0;
      this.mWhiteboard.dropIndicator.hide();
    }.bind(this));
    let cpParent = $(body).find("#whiteboardColorpicker")[0];
    new Picker({
      parent: cpParent,
      color: "#000000",
      onChange: function(color) {
        this.mWhiteboard.setDrawColor(color.rgbaString);
      }.bind(this),
    });

    new Picker({
      parent: $(body).find("#textboxBackgroundColorPicker")[0],
      color: "#f5f587",
      bgcolor: "#f5f587",
      onChange: function(bgcolor) {
        this.mWhiteboard.setTextBackgroundColor(bgcolor.rgbaString);
      }.bind(this),
    });

    // on startup select mouse
    window.wbShortcutFunctions.setTool_mouse();
    // fix bug cursor not showing up
    this.mWhiteboard.refreshCursorAppearance();
    let mode = 'development';

    if (mode === "production") {
      if (this.wbConfigService.readOnlyOnWhiteboardLoad) this.wbReadOnlyService.activateReadOnlyMode();
      else this.wbReadOnlyService.deactivateReadOnlyMode();

      if (this.wbConfigService.displayInfoOnWhiteboardLoad) this.wbInfoService.displayInfo();
      else this.wbInfoService.hideInfo();
    } else {
      // in dev
      this.wbReadOnlyService.deactivateReadOnlyMode();
      this.wbInfoService.displayInfo();
    }
    /*
        if (process.env.NODE_ENV === "production") {
            if ( this.wbConfigService.readOnlyOnWhiteboardLoad) this.wbReadOnlyService.activateReadOnlyMode();
            else this.wbReadOnlyService.deactivateReadOnlyMode();

            if ( this.wbConfigService.displayInfoOnWhiteboardLoad) this.wbInfoService.displayInfo();
            else this.wbInfoService.hideInfo();
        } else {
            // in dev
            this.wbReadOnlyService.deactivateReadOnlyMode();
            this.wbInfoService.displayInfo();
        }*/

    // In any case, if we are on read-only whiteboard we activate read-only mode
    if (this.wbConfigService.isReadOnly) this.wbReadOnlyService.activateReadOnlyMode();
    this.mWhiteboard.setTool("pen");
  }

  //remember to shut down any additional threads over here.
  closeWindow() {
    if (this.mController > 0)
      CVMContext.getInstance().stopJSThread(this.mController); //shut-down the thread if active
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
              if (gTools.arrayBufferToString(fileName) != "GNC")
                return;
              let data = dataFields[2];

              switch (dataType) {
                case eDataType.bytes:

                  break;

                case eDataType.signedInteger:
                  data = gTools.arrayBufferToNumber(data);
                  break;
                case eDataType.unsignedInteger:
                  data = gTools.arrayBufferToNumber(data); //here will be the GNC value
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
}

export default CWhiteboard;
