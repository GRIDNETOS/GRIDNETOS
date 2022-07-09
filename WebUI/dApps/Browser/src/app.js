'use strict';

import
{
  CDNS
} from '/lib/DNS.js'
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
import {
  CAppSettings,
  CSettingsManager
} from "/lib/SettingsManager.js"

import {
  CWindow
} from "/lib/window.js"


let browserBody = ``;

$.get("/dApps/Browser/src/body.html", function(data) {
  browserBody = data;
  //  alert(whiteboardBody);
}, 'html'); // this is the change now its working
//In an unpacked version, from external file:
/*$.get("/dApps/whiteboard/src/body.html", function(data)
 {
   whiteboardBody = data;
  //  alert(whiteboardBody);
 },'html'); */


/*
Checklist:
1) change class name
2) change title in constructor
3) change default export at the end of the file
4) change base64 encoded icon in getIcon static method
5) change window's body in apptemplateBody or uncomment the load-from file mechanics
6) change package name in the getProcessID static method
7) if system app then add to PackageManager
*/
class CBrowser extends CWindow {

  static getDefaultCategory() {
    return 'explore';
  }
  constructor(positionX, positionY, width, height) {

    if(CTools.getInstance().isNull(CVMContext.getInstance().getCurrentNodeURI) || CVMContext.getInstance().getCurrentNodeURI.length==0)
    return;
    let proxyURI = new URL(CVMContext.getInstance().getCurrentNodeURI);

    if(CTools.getInstance().isIPaddress(proxyURI.hostname))
    {
      proxyURI = new URL(CVMContext.getInstance().getDNS.ipAddressToDomain(proxyURI.hostname));
    }

    let URIStr = 'https://'+proxyURI.hostname+":444/";
    browserBody = browserBody.replace('[PROXY_URI]', URIStr);
    super(positionX, positionY, width, height, browserBody, "Browser", CBrowser.getIcon(), true); //use Shadow-DOM by default
    this.mElements = {
      top: $(this.getBody).find('.selector-top')[0],
      left: $(this.getBody).find('.selector-left')[0],
      right: $(this.getBody).find('.selector-right')[0],
      bottom: $(this.getBody).find('.selector-bottom')[0]
    };
    this.mVMContext = CVMContext.getInstance();
    this.mLastWobbledFor = null;
    this.mMouseTargetWidth = 0;
    this.mInAppMouseX = 0;
    this.mInAppMouseY = 0;
    this.mKnowThatMouseOut = false;
    this.mLasersHidden = false;
    this.mFrame = $(this.getBody).find('.viewerFrame')[0];
    this.mFrame.setAttribute('winID', this.getWinID);
    this.mDNS = this.mVMContext.getDNS;




    this.mFrame.setAttribute('proxyURI', URIStr);

    this.mProxy = this.mFrame.mProxy;
    this.mEditingActive = false;
    this.mLastTarget = null;
    this.mPersistentTarget = null;
    this.mPreviousPersistentTarget = null;
    this.mPicker = new Picker();

    this.mPicker.onChange = function(color) {
      if (!this.mPickerOpen)
        return;
      this.updateTarget(color, this.mLastMode);

    }.bind(this);

    this.mPicker.onOpen = this.pickerOpened.bind(this);
    this.mPicker.onClose = this.pickerClosed.bind(this);

    this.mPicker.onDone = function(color) {
      this.mHoldingItemSince = 0;

      this.mEditing = false;

      let resultingCSS = null;
      let selector = null;
      let cssBody = null;

      if (this.mPersistentTarget) {
        this.updateTarget(color, this.mPersistentTarget);
        if (this.mPersistentTarget.tagName != "BODY") {
          //style according to ID
          if (this.mPersistentTarget.id.length) {
            selector = "#" + this.mPersistentTarget.id;
          }

          //style according to class
          if (!resultingCSS) {
            if (this.mPersistentTarget.className.length) {
              selector = "." + this.mPersistentTarget.className;
            }
          }
        }

        //style according to type (body/href)
        if (!resultingCSS) {
          if (this.mPersistentTarget.tagName) {
            selector = this.mPersistentTarget.tagName;

          }
        }

        cssBody = `background-color:` + this.mPersistentTarget.style.backgroundColor + ` !important; color: ` + window.getComputedStyle(this.mPersistentTarget).getPropertyValue('color') + ` !important;`;

        resultingCSS = selector + ` {` + cssBody + `}`; //show it to the user?
        //  window.alert(resultingCSS);
        if (!resultingCSS) {
          window.alert('Persistent styling not supported');
        } else {
          //persistent customization not supported
          //save settings
          let domain = this.getDomain;
          if (!domain)
            return;
          this.setDomainCSSElement(domain, selector, cssBody);
        }
      }

      this.mPersistentTargetCadidate = null;
      this.mPersistentTargetCadidate = null;
      this.pickerClosed();
    }.bind(this);

    this.mPickerOpen = false;
    this.mEditingMode = 1;
    this.mEditing = false;


    this.mIsGoBackEnabled = false;
    this.mIsGoForthEnabled = false;
    this.mHistory = [];
    this.mHistoryIndex = 0;
    this.mWebPagesBuffer = [];
    this.mGoingForth = false;
    this.mGoingBack = false;
    this.setThreadID = null; //by default there's no need for a dedicated Decentralized Processing Thread.
    //Developers should prefere usage of public 'data' thread for retreval of information instead.
    this.mLastHeightRearangedAt = 0;
    this.mLastWidthRearangedAt = 0;
    this.mErrorMsg = "";
    this.mMetaParser = new CVMMetaParser();
    //register for network events
    CVMContext.getInstance().addVMMetaDataListener(this.newVMMetaDataCallback.bind(this), this.mID);
    CVMContext.getInstance().addNewDFSMsgListener(this.newDFSMsgCallback.bind(this), this.mID);
    CVMContext.getInstance().addNewGridScriptResultListener(this.newGridScriptResultCallback.bind(this), this.mID);
    this.loadLocalData();
    this.mEditAfterHolding = 1; //activate editing after 3 seconds
    this.mPersistentTargetCadidate = null;
    this.mPersistentTarget = null;
    this.mHoldingItemSince = 0;
    this.controllerThreadInterval = 1000;
    this.mControlerExecuting = false;
    this.mControler = 0;
    this.mDarkModeEnabled = true;
    this.mAnonymousModeActive = false;

    //Init app items - BEGIN
    this.mBody = this.getBody;
    this.mToggleButton = $(this.getBody).find('.themeToggle')[0];
    this.mLinkBox = $(this.getBody).find('.linkBox')[0];
    this.mFrameButton = null;
    this.mSidebarButton = null;
    this.mGoForthBtn = $(this.getBody).find('.goForthBtn')[0];
    this.mGoBackBtn = $(this.getBody).find('.goBackBtn')[0];
    this.mRefreshBtn = $(this.getBody).find('.refreshBtn')[0];
    this.mRefreshBtn.addEventListener('click', this.refresh.bind(this), false);
    this.mAnonymousBtn = $(this.getBody).find('.anonymousBtn')[0];
    this.mWandBtn = $(this.getBody).find('.wandBtn')[0];

    //  this.mAddBar = $(this.getBody).find('#addBar')[0];
    //  this.mClickLink = $(this.getBody).find('#clicky')[0];
    //  this.mHeaderBox = $(this.getBody).find('#headerBox')[0];

    if (this.mDarkModeEnabled) {
      this.darkMode();
    }



    this.mHomepage = 'https://wikileaks.org/';
    this.addToHistory(this.mHomepage, false);

    this.mURLField = $(this.getBody).find('#locationField')[0];
    this.mURLField.value = this.mHomepage;

    this.addMouseMoveListener(this.mouseMovedInApp.bind(this));
    this.addMouseClickListener(this.mouseClickedInApp.bind(this));
    //Init app items - END
  }

  //Gets notified about in-app mouse movements.
  //Notice it's seperate from in-Wizardous-iFrame notifications
  mouseMovedInApp(event) {

    if (this.hasParentClass(event.target, "picker_wrapper")) {
      return; //do not trigger when moving over the colur picker
    }
    this.mInAppMouseX = event.x;
    this.mInAppMouseY = event.y;
    let objRect = event.target.getBoundingClientRect();
    this.mMouseTargetWidth = objRect.right - objRect.left;
  }


  mouseClickedInApp(event) {

    if (this.hasParentClass(event.target, "picker_wrapper")) {
      return; //do not trigger when moving over the colur picker
    }

    if (this.mEditingActive && event.isIFrame) {

      if (this.mLastWobbledFor != event.target) {
        this.mLastWobbledFor = event.target;
        if (this.mPickerOpen) {
          this.mPicker.hide();
          this.pickerClosed();
        }
        CTools.getInstance().animateByElement(event.target, "pulse", this.openPickerFor.bind(this, event.target));
      } else {
        this.openPickerFor.bind(this, event.target)();
      }
    }

  }

  refresh() {
    console.log("Refreshing..");
    //this.mFrame.src = this.mURLField.value;
    this.goToLink(this.mHistory[this.mHistoryIndex], false);
  }

  pickerClosed() {
    //window.alert('opening picker')
    this.mPickerOpen = false;
    if (this.mEditing) {

      this.mEditingMode = 2;
      //  this.mPicker.show();
    }
  }
  pickerOpened() {
    //window.alert('opening this.mPicker')

    this.mPickerOpen = true;
    this.hideLasers();

    if (this.mLastTarget) {
      let lmStr = this.mLastTarget.getAttribute('gnEMode');
      if (lmStr) {
        this.mLastMode = parseInt(lmStr, 10);
      }
    }

    if (this.mLastMode == 2) { //last-mode retrieved from a control
      this.mEditingMode = 1;
    } else {
      this.mEditingMode = 2;
    }

    if (this.mLastTarget) {
      this.mLastTarget.setAttribute('gnEMode', this.mEditingMode.toString());

    }
    this.mLastMode = this.mEditingMode; //just for accountability; has no effect on consecutive invocations
    //which is when the value is always assumed as 2 (text-editing) and attempted to be retrieved from a control in any case
    //WARNING: it IS used by onChange()

    if (this.mEditingMode == 2) {
      console.log("font editing");
    } else if (this.mEditingMode == 1) {
      console.log("Background editing");
    }

    this.mEditing = true;
  }
  toggleWand() {
    //update state
    this.mEditingActive = !this.mEditingActive;
    this.setWandEnabled = this.mEditingActive;
    //make state effective

  }

  set setWandEnabled(isIt = true) {
    if (isIt) {
      this.mFrame.setAttribute('editing', 'true');
      //  this.mFrame.setAttribute('haltNav', 'true');
      this.mWandBtn.src = "/dApps/Browser/images/activeWand.cur";
      this.mWandBtn.style.opacity = 1;
    } else {
      this.mFrame.setAttribute('editing', 'false');
      //this.mFrame.setAttribute('haltNav', 'false');
      this.mWandBtn.src = "/dApps/Browser/images/wand.cur";
      this.mWandBtn.style.opacity = 0.6;
      this.hideLasers();
    }
  }

  toggleAnonymity() {
    //update state
    this.mAnonymousModeActive = !this.mAnonymousModeActive;

    //make state effective
    if (this.mAnonymousModeActive) {
      this.mFrame.setAttribute('mode', 'anonymous');
      this.mAnonymousBtn.src = "/dApps/Browser/images/maskon.png";
      this.mAnonymousBtn.style.opacity = 1;
    } else {
      this.mFrame.setAttribute('mode', 'AI');
      this.mAnonymousBtn.src = "/dApps/Browser/images/maskoff.png";
      this.mAnonymousBtn.style.opacity = 0.6;
    }
  }

  fillLink() {
    let clickLink = $(this.getBody).find('#clicky')[0];
    let input = $(this.getBody).find('#addBar')[0];
    clickLink.innerHTML = input.value;
  }

  //Buffer's payload of the currently visted web-page.
  //This is to speed up back-and-forth history toggling.
  //Thanks to fetched source-code of the main payload, no involvement of the Wizardous proxy is required.
  //Then under the assumption of existance of a client-side cache within the native web-browser, rest of the referenced assets
  //would be retrieved from a local cache as well.
  bufferPayload(payload) {
    //initial validation - BEGIN
    if (!this.mHistory.length || !this.mWebPagesBuffer.length || this.mHistoryIndex > this.mWebPagesBuffer.length - 1) {
      return;
    }
    //initial validation - END
    console.log('Buffering response for ' + this.mHistory[this.mHistoryIndex]);
    this.mWebPagesBuffer[this.mHistoryIndex] = payload;
  }

  get getIsHistoryToggling() {
    if (this.mGoingBack || this.mGoingForth)
      return true;

    return false;
  }

  set setIsHistoryToggling(isIt) {
    if (!isIt) {
      this.mGoingBack = false;
      this.mGoingForth = false;
    }

    return false;
  }

  set setGoBackEnabled(isIt) {
    this.mIsGoBackEnabled = isIt;
    this.mGoBackBtn.style.opacity = isIt ? 1 : 0.3;
  }

  get getGoBackEnabled() {
    return this.mIsGoBackEnabled;
  }

  set setGoForthEnabled(isIt) {
    this.mIsGoForthEnabled = isIt;
    this.mGoForthBtn.style.opacity = isIt ? 1 : 0.3;
  }

  get getGoForthEnabled() {
    return this.mIsGoForthEnabled;
  }


  //Retrieves buffered payload for the current depth of the visited web-pages backlog.
  getBufferedPayload() {
    //initial validation - BEGIN
    if (!this.mHistory.length || !this.mWebPagesBuffer.length || this.mHistoryIndex > this.mWebPagesBuffer.length - 1) {
      return null;
    }
    //initial validation - END
    console.log('Payload for ' + this.mHistory[this.mHistoryIndex] + ' retrieved from cache.');
    return this.mWebPagesBuffer[this.mHistoryIndex];
  }

  setCurrentURLVisible(url) {

    if (url.indexOf('http://') == -1 && url.indexOf('https://') == -1) {

      url = "https://" + url;
    }

    url = url.replace(/(https?:)\/*/igm, `$1//`);

    this.mURLField.value = url;
  }

  getStyleForCurrentDomain() {
    return this.getStyleForDomain(this.getDomain);
  }

  get getDomain() {
    let regex = /^(https?:\/\/)([^\/]*)\/?/igm;
    let match = regex.exec(this.getStringFromControl(this.mURLField));

    if (match) {
      if (match[2]) {
        return match[2];
      }
      return null;
    }
    return null;
  }

  addToHistory(url, updateIndex = true) {

    if (this.mHistory.length) {
      if (this.mHistory[this.mHistory.length - 1] == url) {
        return false;

      }
    }
    console.log('adding URL to history (' + url + ")");
    this.mHistory.push(url);
    this.mWebPagesBuffer.push(null); //would be filled-in once ready i.e. made available.

    if (updateIndex) {
      this.mHistoryIndex++;
      //this.setCurrentURLVisible(url);
    }
    return true;
  }

  loadingContent(url) {
    if (this.getIsHistoryToggling) {
      this.setIsHistoryToggling = false;
      return;
    } else {
      if (this.mHistory.length && this.mHistoryIndex < this.mHistory.length - 1) {
        this.mHistory.splice(this.mHistoryIndex + 1, this.mHistory.length);
      }
    }
    this.addToHistory(url);
    this.setCurrentURLVisible(url);
    this.updateNavButtons();
  }

  goToLink(clickLink, addToHistory = true) {
    this.setWandEnabled = false;
    this.mFrame.src = (this.mProxy + clickLink);
    $(this.mFrame).contents().on('mousemove', this.mouseMoveCallback.bind(this));
    //this.mFrame.contentDocument.addEventListener('mousemove', this.mouseMoveCallback.bind(this));
    if (!this.getIsHistoryToggling) {
      //clear any consecutive history events

      if (addToHistory) {
        this.addToHistory(clickLink);
      }
    }

    this.setCurrentURLVisible(clickLink);
    this.mDoNotUpdateFlags = false;
    //this.mURLField.value = clickLink;
    this.updateNavButtons();
  }

  goBack() {
    //  this.mFrame.contentWindow.history.back();
    if (this.mHistoryIndex) {
      this.mHistoryIndex--;
    }
    this.mGoingBack = true;
    this.goToLink(this.mHistory[this.mHistoryIndex], false);
  }


  goHome() {
    this.setWandEnabled = false;
    this.mURLField.value = 'https://wikileaks.org/';
    this.frameLoadNewSite();
  }

  goForth() {
    //this.mFrame.contentWindow.history.forward();

    if (this.mHistory.length && this.mHistoryIndex < (this.mHistory.length - 1)) {
      this.mHistoryIndex++;

    }

    this.mGoingForth = true;
    this.goToLink(this.mHistory[this.mHistoryIndex], false);
    return true;

  }

  frameLoadNewSite() {
    /* USED TO BE CALLED THE frameChange() FUNCTION */
    let clickLink = this.mURLField;
    this.setIsHistoryToggling = false;
    /*  if (clickLink.innerHTML.indexOf('http') != 0) {
        let x = clickLink.innerHTML;
        clickLink.innerHTML = "http://" + x;
      }*/

    if (clickLink.value.indexOf('http://') == -1 && clickLink.value.indexOf('https://') == -1) {

      clickLink.value = "https://" + clickLink.value;
    }
    this.goToLink(clickLink.value, false);
  }

  decideBehaviourOfAddBar(url) {
    /** USED TO BE CALLED a() FUNCTION CHANGE THE NEXT FUNCTION NAME **/
    /* check if toggle is active or not, decides behaviour of linkIcons */
    let viewerFrame = $(this.getBody).find('#viewerFrame')[0];
    console.log(url);
    /* if the frame is open, make buttons go to frame. Otherwise, send the user directly to the link*/
    //if (viewerFrame.style.opacity == 1) {
    /* frame is open, change links to frame-safe */

    this.goToLink(url, false);
    //viewerFrame.src = this.swapURL(url, true)
    //} else {
    //  window.location.assign(this.swapURL(url, false));
    //}



  }
  updateNavButtons() {
    if (!this.mHistoryIndex) {
      this.setGoBackEnabled = false;
    } else {
      this.setGoBackEnabled = true;
    }

    if (this.mHistory.length && this.mHistoryIndex < (this.mHistory.length - 1)) {
      this.setGoForthEnabled = true;
    } else {
      this.setGoForthEnabled = false;
    }
  }
  swapURL(url, frameActive) {
    /* (string, boolean) Change URL depending on if the frame is open or not */
    let returnVal = url;
    if (frameActive) {
      switch (url) {
        case "https://www.google.pl":
          returnVal = "https://www.google.pl:443";
          break;
        case "https://www.google.com/maps":
          returnVal = "https://www.mapquest.com";
          break;
        case "https://translate.google.com":
          returnVal = "https://www.bing.com/translator";
          break;
      }
    } else {
      switch (url) {
        case "https://www.bing.ca":
          returnVal = "https://www.google.com:443";
          break;
        case "https://www.mapquest.com":
          returnVal = "https://www.google.com/maps";
          break;
        case "https://www.bing.com/translator":
          returnVal = "https://translate.google.com";
          break;
      }
    }
    return returnVal;
  }

  runClock() {
    let dateObj = new Date();
    let h = dateObj.getHours();
    let m = dateObj.getMinutes();
    let s = dateObj.getSeconds();

    $(this.getBody).find('#clock')[0].innerHTML = this.getFormattedTime(h,
      m, s);
    let t = setTimeout(this.runClock.bind(this), 100);
  }
  checkClockZeros(num) {
    if (num < 10) {
      num = "0" + num;
    }
    return num;
  }
  getFormattedTime(h, m, s) {
    return '';
  }
  sidebarToggle() {
    let btn = $(this.getBody).find('#sidebarToggle')[0];
    let linkBox = $(this.getBody).find('.linkBox')[0];

    /* determine initial state, expand or collapse sidebar based on state */

    if (btn.innerHTML === "collapse") {
      btn.innerHTML = "expand";
      linkBox.style.width = "25px";

    } else if (btn.innerHTML === "expand") {
      btn.innerHTML = "collapse";
      linkBox.style.width = "150px";
    }
  }
  changeLinksToFrameSafe(frameActive) {
    /* For extra clarity, 'frameActive' indicates the viewerFrame is open, so links should be ones that can open in iFrame */
    let google = $(this.getBody).find('#google')[0];
    let googleMaps = $(this.getBody).find('#google-maps')[0];
    let googleTranslate = $(this.getBody).find('#google-translate')[0];

    if (frameActive) {
      google.src = "images/bing.png";
      google.alt = "Bing";
      googleMaps.src = "images/mapquest.png";
      googleMaps.alt = "Mapquest";
      googleTranslate.src = "images/bing-translator.png";
      googleTranslate.alt = "Bing Translator";
    } else {
      google.src = "images/google.png";
      google.alt = "Google";
      googleMaps.src = "images/google-maps.png";
      googleMaps.alt = "Google Maps";
      googleTranslate.src = "images/google-translate.png";
      googleTranslate.alt = "Google Translate";
    }

  }
  darkMode() {
    /* dark mode */
    if (this.mToggleButton.innerHTML === "Dark mode") {
      this.mToggleButton.innerHTML = "Light mode";
      this.mToggleButton.style.backgroundColor = "#363636";
      this.mToggleButton.style.color = "#eeeeee";
      /*this.mFrameButton.style.backgroundColor = "#363636";
      this.mFrameButton.style.color = "#eeeeee";
      this.mSidebarButton.style.backgroundColor = "#363636";
      this.mSidebarButton.style.color = "#eeeeee";
      this.mBody.style.backgroundColor = "#121212";
      this.mLinkBox.style.backgroundColor = "rgba(40, 40, 40, 0.5)";*/
    } else {
      /* light mode */
      this.mToggleButton.innerHTML = "Dark mode";
      this.mToggleButton.style.backgroundColor = "#dddddd";
      this.mToggleButton.style.color = "#000000";
      /*  this.mFrameButton.style.backgroundColor = "#dddddd";
        this.mFrameButton.style.color = "#000000";
        this.mSidebarButton.style.backgroundColor = "#dddddd";
        this.mSidebarButton.style.color = "#000000";
        this.mBody.style.backgroundColor = "#ffffff";*/
      //  this.mLinkBox.style.backgroundColor = "rgba(200, 200, 200, 0.5)";
    }
  }

  frameToggle() {
    let viewerFrame = $(this.getBody).find('#viewerFrame')[0];
    let clickLink = $(this.getBody).find('#clicky')[0];
    let addBar = $(this.getBody).find('#addBar')[0];
    let linkBox = $(this.getBody).find('.linkBox')[0];

    /* visible --> invisible */
    if (viewerFrame.style.opacity == 1) {
      viewerFrame.style.opacity = 0;
      viewerFrame.style.height = "0px";
      clickLink.style.opacity = 0;
      addBar.style.opacity = 0;
      this.changeLinksToFrameSafe(false);
    } else {
      viewerFrame.style.opacity = 1;
      /* viewerFrame.style.height = "600px"; */
      clickLink.style.opacity = 1;
      addBar.style.opacity = 1;
      this.changeLinksToFrameSafe(true);

      /* resize frame to window size - linkbox */
      viewerFrame.style.width = (window.innerWidth - linkBox.style.width.slice(0, -2)) * .95 + "px";

      viewerFrame.style.height = (window.innerHeight - linkBox.style
        .height.slice(0, -2)) * .80 + "px";
    }
  }

  // Shortcut to get elements


  static getIcon() {
    return `data:image/png;base64,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OGViMTctZjllYi00ZDM2LTg0OWItODg2NmU1MzgwM2U1IgogICB4bXBNTTpJbnN0YW5jZUlEPSJ4bXAuaWlkOmZmMDJjNzA5LWIzMTMtNGYzYi05YWJiLWFiYzhhODVmNDM5MCIKICAgeG1wTU06T3JpZ2luYWxEb2N1bWVudElEPSJ4bXAuZGlkOjIyZjJlNjZjLTI4NDctNDdkNi1iOWJhLTVhYWM1MGM0NzdlZSIKICAgZGM6Rm9ybWF0PSJpbWFnZS9wbmciCiAgIEdJTVA6QVBJPSIyLjAiCiAgIEdJTVA6UGxhdGZvcm09IldpbmRvd3MiCiAgIEdJTVA6VGltZVN0YW1wPSIxNjM1MTUyMzk3ODAzNjAzIgogICBHSU1QOlZlcnNpb249IjIuMTAuMjQiCiAgIHRpZmY6T3JpZW50YXRpb249IjEiCiAgIHhtcDpDcmVhdG9yVG9vbD0iR0lNUCAyLjEwIj4KICAgPHhtcE1NOkhpc3Rvcnk+CiAgICA8cmRmOlNlcT4KICAgICA8cmRmOmxpCiAgICAgIHN0RXZ0OmFjdGlvbj0ic2F2ZWQiCiAgICAgIHN0RXZ0OmNoYW5nZWQ9Ii8iCiAgICAgIHN0RXZ0Omluc3RhbmNlSUQ9InhtcC5paWQ6NDQ2NjlmYzUtMjdiZS00MmYxLWFjZGItOTU3MDliZGUxOTYzIgogICAgICBzdEV2dDpzb2Z0d2FyZUFnZW50PSJHaW1wIDIuMTAgKFdpbmRvd3MpIgogICAgICBzdEV2dDp3aGVuPSIyMDIxLTEwLTI1VDEwOjU5OjU3Ii8+CiAgICA8L3JkZjpTZXE+CiAgIDwveG1wTU06SGlzdG9yeT4KICA8L3JkZjpEZXNjcmlwdGlvbj4KIDwvcmRmOlJERj4KPC94OnhtcG1ldGE+CiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgIAogICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgCiAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAKICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAgICAg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`; //data:image/png;base64
  }

  static getPackageID() {
    return "org.gridnetproject.UIdApps.browser";
  }

  //Settings Manager Support - BEGIN
  static getSettings() { //[required]
    return CBrowser.sCurrentSettings; //Important: static field [required]
    //take look at the bottom of this file, right after CBrowser's body.
  }

  static setSettings(sets) {
    if (!(sets instanceof CAppSettings))
      return false;

    CBrowser.sCurrentSettings = sets;
    return true;
  }

  loadSettings() {
    CVMContext.getInstance().getSettingsManager.loadSettings(this.getPackageID);
    return this.activateSetings();
  }

  activateSetings() {

    let sets = CBrowser.getSettings();
    let settings = null;
    let pack = null;
    let elem = null;

    if (sets == null || typeof sets.getVersion === 'undefined')
      return false;

    //Validate - BEGIN
    if (sets.getVersion == 1) {
      settings = sets.getData;
      if (settings == null || typeof settings === 'undefined') {
        sets = CBrowser.getDefaultSettings();
        this.saveSettings();
      }
    } else return false;

    //Validate - END
    //convert arrays to dictionaries
    //let domainsDict = {};
    settings.domainSettings.domains = this.objectToMap(settings.domainSettings.domains);

    //for (let i = 0; i < settings.domainSettings.domains.length; i++) {

    let entries = Object.entries(settings.domainSettings.domains);

    for (let i = 0; i < entries.length; i++) {
      entries[i] = this.objectToMap(entries[i]);
    }

    //settings.domainSettings.domains.forEach((el, index) => domainsDicts[el] = el[0]);
    //Apply Settings - BEGIN

    //now take use of the app-specific settings object from above
    // ex:  this.mGrid.load(settings);

    //Apply Settings - END
    return true;

  }

  objectToMap(obj) {
    let toRet = {} // new Map();
    let ar =
      (Object.keys(obj).map(key => ({
        name: key,
        value: obj[key]
      })));

    for (let i = 0; i < ar.length; i++) {
      //toRet.set([ar[i].name], ar[i].value);
      toRet[ar[i].name] = ar[i].value;

    }

    return toRet;
  }

  //Sets CSS element for a given domain's CSS selector
  //ex. domain: localhost OR google.pl (no http(s) prefix)
  //selector: "#div1"
  //value: "color: black"
  setDomainCSSElement(domain, selector, value) {
    let settings = this.getSettings.getData;


    /*  if (!settings.domainSettings.domains.has(domain)) {
      settings.domainSettings.domains.set(domain, null); //add stub
    }

    settings.domainSettings.domains.get(domain).set(selector, value);
*/

    if (!settings.domainSettings.domains[domain]) {
      settings.domainSettings.domains[domain] = {}; //add stub
    }

    //if (!settings.domainSettings.domains[domain][selector]) {
    settings.domainSettings.domains[domain][selector] = value;
    //  }

    this.setSettings(settings);
    this.saveSettings();
  }

  //Returns an entire CSS object string, given a CSS selector.
  //selector: "body"
  //ex. output:  "body { color: black; }"
  getDomainCSSObj(domain, selector, valueOnly = false) {
    let settings = this.getSettings;
    let definedElement = settings.domainSettings.domains[domain][selector];

    if (!definedElement)
      return null;

    if (valueOnly) {
      return definedElement;
    }
    return selector + " { " + definedElement + " } ";
  }

  //Clear all CSS settings for domain provided by its URL
  //ex domain: "localhost" OR 'wp.pl'
  clearAllCSSForDomain(domain) {
    let settings = this.getSettings;
    settings.domainSettings.domains[domain] = {};

    this.saveSettings();
  }

  //Constructs and returns entire custom CSS style string for domain.
  //One which  is usually injected into Head section of each site within domain upon consecutive visits.
  getStyleForDomain(domain, wrapInHTML = true) {
    console.log('Fetching customized style for ' + domain);
    let settings = this.getSettings;
    let styleTxt = (wrapInHTML ? "<style>" : "");
    let elements = settings.mData.domainSettings.domains[domain];

    if (!elements) {
      return ""; //no customized styles defines as of yet
    }

    for (let selector in elements) {
      styleTxt += (selector + " { " + elements[selector] + " } ");
    }

    styleTxt += (wrapInHTML ? "</style>" : "");
    return styleTxt;
  }

  saveSettings() {
    let sets = this.getSettings;

    CVMContext.getInstance().getSettingsManager.saveAppSettings(sets);
  }

  static getDefaultSettings() {

    let obj = {

      //note: here we employ two dictionaries.
      //1) domain's URL => dictorinary of [CSS Selector=>value pairs]
      //2) CSSSelector => CSS Values
      domainSettings: { //customized domains' settings
        domains: { //a dict of domains
          "https://localhost": { //key is the domain's name
            "body": "background-color: black; color:white",
            "#div1": "background-color: white; color:blue" //value is a dictionary of CSSSelector => CSS Value

            //note that under the hood JavaScript employs a hash-table with detection of colisions.
            //thus no need to reinvent the wheel.
          },
          "http://localhost": { //key is the domain's name
            "body": "background-color: black; color:white",
            "#div1": "background-color: white; color:blue"
          }
        }
      },
      version: 1
    };

    return new CAppSettings(CBrowser.getPackageID(), obj);
  }

  static getDefSubOption() {
    return {
      option1: 1,
      option2: 2,
    };
  }

  //Settings Manager Support - END


  initialize() //called only when app needs a thread/processing queue of its own, one seperate from the Window's
  //internal processing queue
  {
    //this.loadSettings();
    this.mControler = setInterval(this.mControlerThreadF.bind(this), this.controllerThreadInterval);
    this.disableVerticalScroll();
    this.disableHorizontalScroll();

    //Settings Support - BEGIN
    if (this.loadSettings()) {
      console.log('Browser settings activated!');
    } else {
      CBrowser.setSettings(this.getDefaultSettings());
      console.log('Failed to activate provided settings for Browser. Assuming defaults!');
    }
    //Settings Support - END
  }

  //Mouse movements from the Wizardous iFrame
  mouseMoveCallback(event) {
    //console.log('**** mouse moved ***');
    //return;
    if (!this.mEditingActive)
      return;


    if (this.hasParentClass(event.target, "picker_wrapper")) {
      this.hideLasers();
      this.mLastTarget = null;
      this.mPersistentTargetCadidate = null;
      this.mHoldingItemSince = 0;
      return; //do not trigger when moving over the colur picker
    }

    if (event.target.id.indexOf('selector') !== -1 || event.target.tagName === 'HTML')
      return;


    //  if (this.mPickerOpen)
    //  return;
    if (this.hasParentClass(event.target, "picker_wrapper"))
      return;

    if (!this.mLastTarget || (this.mLastTarget && this.mLastTarget != event.target)) {
      //this.mPicker.hide();

      //this.mEditingMode = 1;

    }

    this.mLastTarget = event.target;
    let $target = $(event.target);
    let targetOffset = $target[0].getBoundingClientRect();
    let targetHeight = targetOffset.height;
    let targetWidth = targetOffset.width;

    //console.log(targetOffset);
    let xCorrection = 3;
    let yCorrection = 3;
    this.showLasers();

    $(this.mElements.top).css({
      left: (targetOffset.left - xCorrection),
      top: (targetOffset.top - yCorrection),
      width: (targetWidth + (2 * xCorrection))
    });
    $(this.mElements.bottom).css({
      top: (targetOffset.top + targetHeight + yCorrection),
      left: (targetOffset.left - xCorrection),
      width: (targetWidth + (2 * xCorrection))
    });
    $(this.mElements.left).css({
      left: (targetOffset.left - xCorrection),
      top: (targetOffset.top - yCorrection),
      height: (targetHeight + (2 * yCorrection))
    });
    $(this.mElements.right).css({
      left: (targetOffset.left + targetWidth + xCorrection),
      top: (targetOffset.top - yCorrection),
      height: (targetHeight + (2 * yCorrection))
    });


  }

  hasParentClass(child, classname) {
    if (child.className.split(' ').indexOf(classname) >= 0) return true;
    try {
      //Throws TypeError if child doesn't have parent any more
      return child.parentNode && this.hasParentClass(child.parentNode, classname);
    } catch (TypeError) {
      return false;
    }
  }

  updateTarget(color, mode) {
    if (!this.mPersistentTarget)
      return;

    if (mode == 1) {
      //  this.mLastTarget.style.backgroundColor = color.rgbaString;
      this.mPersistentTarget.style.setProperty("background-color", color.rgbaString, "important");
    } else if (mode == 2) {
      // window.alert('up font color');
      //this.mLastTarget.style.color = color.rgbaString;
      this.mPersistentTarget.style.setProperty("color", color.rgbaString, "important");
    }
  }

  mControlerThreadF() {
    if (this.mControlerExecuting)
      return false;

    this.mControlerExecuting = true; //mutex protection

    //operational logic - BEGIN
    let now = CTools.getInstance().getTime(true);

    if (this.getIsMouseWithin) {
      this.mKnowThatMouseOut = false;
    }

    //Edit Mode - BEGIN
    if (this.mEditingActive) {
      if (this.mPicker) {
        if (this.mLastTarget && this.getIsMouseWithin) {
          //Rationale auto-edit element if mouse hovers for a sufficient period of time
          if (this.mPersistentTargetCadidate != this.mLastTarget)
            this.mHoldingItemSince = now; //item changed

          this.mPersistentTargetCadidate = this.mLastTarget;

          if (((this.mPersistentTarget != this.mPersistentTargetCadidate) || !this.mPreviousPersistentTarget || !this.mPickerOpen) &&
            (now >= (this.mHoldingItemSince + (this.mEditAfterHolding * 1000)))) {
            //      this.openPickerFor(this.mPersistentTargetCadidate);
            if (this.mLastWobbledFor != this.mPersistentTargetCadidate) {
              this.mLastWobbledFor = this.mPersistentTargetCadidate;
              if (this.mPickerOpen) {
                this.mPicker.hide();
                this.pickerClosed();
              }
              CTools.getInstance().animateByElement(this.mPersistentTargetCadidate, "pulse", this.openPickerFor.bind(this, this.mPersistentTargetCadidate));
            } else {
              this.openPickerFor.bind(this, this.mPersistentTargetCadidate)();
            }
          }
        }

      }

      if (!this.getIsMouseWithin && !this.mKnowThatMouseOut) {
        //auto hide all editing elements if mouse leaves window
        //if (this.mPickerOpen)
        this.mKnowThatMouseOut = true;
        this.mPersistentTargetCadidate = null;
        this.mHoldingItemSince = 0;
        this.mPicker.hide();
        this.pickerClosed();

        this.hideLasers();
      }
    } else {
      this.mHoldingItemSince = 0;
      this.mPersistentTarget = null;
      this.mPersistentTargetCadidate = null;
    }
    //Edit Mode - END

    //operational logic - END

    this.mControlerExecuting = false;

  }

  openPickerFor(target) {

    if (!target || this.hasParentClass(target, "picker_wrapper"))
      return;


    this.mPicker.hide()
    this.pickerClosed();

    this.mPreviousPersistentTarget = this.mPersistentTarget;
    this.mPersistentTarget = target;

    let currentColor = this.mPersistentTarget.style.backgroundColor;

    if (!currentColor) {
      currentColor = "#67787dff";
    }

    let windowWidth = this.getWidth;
    let windowHeight = this.getHeight;
    let popUpPosition = 'right';

    if (windowWidth < 900) {
      popUpPosition = 'left'; //there's not enough space anyway
    } else
    if (windowWidth > 900 && this.mInAppMouseX > (windowWidth * 0.5)) {
      popUpPosition = 'left';
    }

    if (this.mMouseTargetWidth > (0.5 * windowWidth)) {
      popUpPosition = 'rightInner';
    }

    if ((windowHeight - this.mInAppMouseY) < 300) {
      popUpPosition = 'top';
    }

    this.mPicker.setOptions({
      parent: target,
      color: currentColor,
      popup: popUpPosition
    });

    this.mPicker.show();
    this.pickerOpened();
  }
  showLasers() {
    if (!this.mLasersHidden) {
      return;
    }
    this.mLasersHidden = false;
    $(this.mElements.bottom).show();
    $(this.mElements.top).show();
    $(this.mElements.left).show();
    $(this.mElements.right).show();
  }
  hideLasers() {
    if (this.mLasersHidden) {
      return;
    }
    this.mLasersHidden = true;
    $(this.mElements.bottom).hide();
    $(this.mElements.top).hide();
    $(this.mElements.left).hide();
    $(this.mElements.right).hide();

  }

  finishResize(isFallbackEvent) { //Overloaded window-resize Event
    //called on finish of resize-animation ie. maxWindow, minWindow
    super.finishResize(isFallbackEvent);
  }

  stopResize(handle) { //fired when mouse-Resize ends
    super.stopResize(handle);
  }

  onScroll(event) {
    super.onScroll(event);
  }

  userResponseCallback(e) {
    console.log('User answered:' + e.answer)
    if (e.answer) {
      //use the provided value
    }

  }

  async open() { //Overloaded Window-Opening Event
    this.mContentReady = false;
    super.open();
    this.initialize();
  //  let promiseResult = await this.askIntA('⋮⋮⋮ Browser🌎 ', 'Provide home-page URL? (default: Wikileaks) ', true);
  //  console.log("Response: " + promiseResult.response);
    //modify content here

  }

  //remember to shut down any additional threads over here.
  closeWindow() {
    if (this.mControler > 0)
      clearInterval(this.mControler); //shut-down the thread if active
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

        for (let i = 0; i < sections.length; i++) {
          let sType = sections[i].getType;
          if (sType != eVMMetaSectionType.fileContents)
            break;

          let entries = sections[i].getEntries;
          let entriesCount = entries.length;

          for (let a = 0; a < entriesCount; a++) {
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

export default CBrowser;
