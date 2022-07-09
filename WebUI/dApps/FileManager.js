"use strict"
import {
  CWindow
} from "/lib/window.js"

import {
  CVMMetaSection,
  CVMMetaEntry,
  CVMMetaGenerator,
  CVMMetaParser
} from './../lib/MetaData.js'

import {
  CContentHandler
} from "/lib/AppSelector.js"
const fileManagerBodyHTML = `


  <style>.topBarIcon{height: 1.5em;
    transition: all 1s;
  }
  .topBarIcon:hover{
    cursor: pointer;
    filter: drop-shadow(0 0 0.75rem cyan);
    transition: all 1s;
  }
  input::-webkit-calendar-picker-indicator {
                opacity: 100;
             }
             #fileManagerToolbar {
               top: 0;
       left: 12.5em;
       /* z-index: 10; */
       background-color: #ffffff00;
       border-bottom-left-radius: 0.8em;
       border-bottom-right-radius: 0.8em;
       /* color: #fff; */
       position: absolute;
       width: calc(100% - 12.5em);
             }

             .category {
               position: relative;
    display: inline-block;
    padding-left: 0.5em;
    padding-right: 0.5em;
    padding-top: 0.2em;
    padding-bottom: 0.2em;
             }
             #locationField{
               line-height: 1.7em;
    vertical-align: text-bottom;
             }

             ::-webkit-scrollbar-track
{
	border: 2px solid #28c1c8;
      border-radius: 4px;
	// border color does not support transparent on scrollbar
	// border-color: transparent;
	background-color: #0f4f5b;
}

::-webkit-scrollbar
{
	width: 15px;
	background-color: #691486;
}

::-webkit-scrollbar-thumb
{
	background-color: #1c3385;
	border-radius: 10px;
  box-shadow: -1px 1px 4px 2px rgb(0 255 218 / 69%) !important;
  z-index:10;
}
.windowFooter {
  position: absolute;
  bottom: 0px;
  width: 100%;
  background-color: #4613497a;
  background-color: #03a4d3d4;
  border-top-left-radius: 6px;
  border-top-right-radius: 6px;
  border: 1px solid #03a4d3d4;
  border-right: 0px;
  overflow: hidden;
  max-height: 5%;
  font-size:small;
  color: #d4e4e2;
}
.navigationBar
{
  left: 0;
      right: 0;
      bottom: 0.5em;
      margin-left: auto;
      margin-right: auto;
      position: relative;
      font-size: xx-small;
      color: #8d8d8d;
}
tr div {
  height:3em;
  transition: height 0.2s ease-in-out;
  max-width: 6em;
}
tr.open div {
  height: 3em;
}

tr.close div {
  height: 0em;
}

.fmElementNameContainer{
  position: relative;
bottom: 0;
font-size: x-small;
overflow: hidden;
max-width: 100%;
line-height: normal;
max-height: 50%;

}
.fmElementNameContainer:focus-visible {
  outline: cyan outset 2px;
    caret-color: cyan;
    color: #0bf38d;
}
.folderElement {
  cursor: pointer;
  width: auto;
  max-height: 75%;
  object-fit: contain;
  transition: all 0.3s;
}

.folderElement:hover
{
  box-shadow: 0px 2px 3px cyan;
  transition: all 0.3s;
}
.fileTopMenu
{
  padding-top: 0.5em;
  border-bottom-left-radius: 9px;
  border-bottom-right-radius: 9px;
  border: 1px solid #144734;
  position: sticky;
  float: left;
  min-width: 35%;
  background-color: #141e2d;
  box-shadow: inset 0 7px 9px -7px rgb(0 255 218 / 69%);
  z-index: 2;
}
.fileImportCreator
{
  width:100%;
  height:100%;
  display:none;
}
.uploadModeSelectionView
{
  display:none;
  height: 100%;
}

.uploadRewardSelectionView
{
  display:none;
  height: 100%;
  background-color: #1e5077;

}
.uploadConfirmationView
{
  display:none;
  height: 100%;
}
.fileImportCreator.shown
{
  width:100%;
  height:100%;
  display:block;
}
.fileImportCreatorColumn
{

  width: calc(50% - 5px);
    height: 100%;
    position: sticky;
    float: left;
    background-color: #31d1c1;
}

.fileImportCreatorColumn:hover
{

    background-color: #11ff00;
}
.fileImportCreatorColumnLeft
{


  border-style: dashed;
  border-width: 1px;
  border-color: #196b0d;
}
.fileImportCreatorColumnRight
{



  border-style: dashed;
  border-width: 0.1em;
  border-color: #196b0d;

}

.eternalImportImg
{
  left: 0;
  right: 0;
  height:90%;
  position: absolute;
  top: 0;
  bottom: 0;
  margin: auto;
  cursor: pointer;
  max-height: 30em;
}
.votedImportImg
{
  left: 0;
  right: 0;
  width: 90%;
  position: absolute;
  top: 0;
  bottom: 0;
  margin: auto;
  cursor: pointer;
  max-width: 27em;
}

.customCurtain
{
  position: absolute !important;
  z-index: 1 !important;
}


</style>
<link href="/css/main.css" rel="stylesheet">
<link href="/css/fadumped.css" rel="stylesheet">
<link href="/css/faall.css" rel="stylesheet">
<link rel="stylesheet" href="/css/viewOptions.css" />
<link rel="stylesheet" href="/CSS/jquery.contextMenu.min.css" />
<link href="/css/jquery-editable-select.min.css" rel="stylesheet">
   <input type="file" style="display:none" id="file-input" name="file-input" multiple />
   <div class="curtainArea customCurtain">
   </div>
<div id="selectViewOptions" ng-model=""></div>
<div id="fileManagerToolbar">


  <div id="fileTopMenu" class ="fileTopMenu" style="
  /* text-align: left; */
">







    <div class="category">


      <img class="topBarIcon goBackBtn" alt="Go Back" title="Go Back" src="/images/back.png" >
    </div>
    <div class="category">
      <img class="topBarIcon goForthBtn" alt="Go Forth" title="Go Forth" src="/images/forth.png">
    </div>
    <select id="locationField" ng-model="">
      <option>Home</option>
      <option>Video Streams</option>
      <option>Music</option>
      <option>Global Domains</option>
        <option>Torrent-Storage</option>
    </select>
    <div class="category">
      <img class="topBarIcon" alt="Home" title="Home" src="/images/home.png" onclick=" let window = gWindowManager.getWindowByID($($(this).closest('.idContainer')).find('#windowIDField').first().val());  if(CVMContext.getInstance().getConnectionState!=eConnectionState.connected)
      {
      window.showNotConnectedError();
        return;
      }
      window.addNetworkRequestID(CVMContext.getInstance().getFileSystem.doCD('/', true, false, false, window.getThreadID).getReqID)
       window.addNetworkRequestID(CVMContext.getInstance().getFileSystem.doLS(window.getThreadID).getReqID);">
    </div>
    <div class="category">
      <img class="topBarIcon showWorld" alt="World" title="List State Domains" src="/images/world.png" >
    </div>
    <div class="category">
      <img class="topBarIcon newFileBtn" alt="New File" title="New File" src="/images/newfile.png">
    </div>
    <div class="category">
      <img class="topBarIcon newFolderBtn" alt="New Folder" title="New Folder" src="/images/newfolder.png">
    </div>
    <div class="category">
      <img class="topBarIcon uploadBtn" alt="Upload" title="Upload File" src="/images/upload.png">
    </div>
    <div class="category">
      <img class="topBarIcon commitBtn" alt="Authorize" title="Authorize" src="/images/auth.png">
    </div>
    <div class="category">
      <img class="topBarIcon" alt="Synchronize" title="Synchronize" src="/images/sync.png" onclick="CVMContext.getInstance().getFileSystem.doSync(); window.addNetworkRequestID(CVMContext.getInstance().getFileSystem.doLS(window.getThreadID).getReqID)">
    </div>
    <div id='navigationBar' class='navigationBar'>
    Navigation
    </div>
  </div>
</div>
  </div>
  <div id="windowFMBody" class="scrollY" style=" height:400px; overflow-x: hidden;     position: absolute;width: 100%; left: 0px;">
    <div id='fileImportCreator' class='fileImportCreator'>
      <div id="uploadModeSelectionView" class = "uploadModeSelectionView">


  <div  class='fileImportCreatorColumn fileImportCreatorColumnLeft'>
    <img class ='eternalImportImg' onclick='let window = gWindowManager.getWindowByID($(this).closest(&#39;.idContainer&#39;).find(&#39;#windowIDField&#39;).first().val()); window.doEternalUpload(); window.closeImportWizard();' src='/images/eternalstorage.png'/>
    </div>
    <div  class='fileImportCreatorColumn fileImportCreatorColumnRight'>
      <img class ='votedImportImg' src='/images/votedstorage.png' onclick='let window = gWindowManager.getWindowByID($(this).closest(&#39;.idContainer&#39;).find(&#39;#windowIDField&#39;).first().val()); window.doCrowdFundedUpload();  window.closeImportWizard();'/>
      </div>
    </div>
    <div id="uploadRewardSelectionView" class = "uploadRewardSelectionView">
      <label for="fname"><i class="fa fa-address-card "></i>  Token Pool:</label>
      <select id="tokenPoolSelector" ng-model="">

      </select><br><br>
  <label for="lname"><i class="fa fa-money "></i> Value:</label>
  <input type="text" class="txtInput numType" id="valueTxt"><br><br>
    <label for="ergBid"><i class="fa fa-line-chart "></i> ERG-Bid:</label>
  <input type="text" class="txtInput numType" id="ergBIDTxt" onclick="$('#ergBIDTxt')[0].value=''" value="automatic"><br><br>

    <label for="ergLimit"><i class="fa fa-adjust "></i> ERG-Limit:</label>
  <input type="text" class="txtInput numType" id="ergLimitTxt" onclick="$('#ergLimitTxt')[0].value=''" value="automatic"><br><br>

  <button id="sendBtn" class="buttonStd" onclick="let window = gWindowManager.getWindowByID($($(this).closest('.idContainer')).find('#windowIDField').first().val()); window.issueTransfer(false);"><i class="fa fa-hourglass"></i> Enqueu</button>
  <button id="sendBtnNow" class="buttonStdGreen" onclick="let window = gWindowManager.getWindowByID($($(this).closest('.idContainer')).find('#windowIDField').first().val()); window.issueTransfer(true);">Send Now <i class="fa fa-send"></i> </button>
    </div>
    <div id="uploadConfirmationView" class = "uploadConfirmationView">
    </div>
      </div>

    <div id="fileManagerTable" style="height:100%; width:100%">

    </div>

  </div>
  <div id="windowFooter" class="windowFooter">

  </div>

  `;
class CFileManager extends CWindow {


  static getPackageID() {
    return "org.gridnetproject.UIdApps.fileManager";
  }

  static getIcon() {
    return `data:image/png;base64,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`;
  }

  //HTML code BEGIN

  async newUploadRequest() {

    if (CVMContext.getInstance().getConnectionState != eConnectionState.connected) {
      this.showNotConnectedError();
      return;
    }
    this.openFileSelectAndUpload();

  }

  async createFileInPlaceA() {
    let promise = new Promise(function(resolve, reject) {
      this.createFileInPlace(resolve, reject);
    }.bind(this));
    return promise;
  }


  //Creates a stub-file in place.
  //Its name is actively editable, with cursor blinking at its end.
  //Once user types and the container looses focus, the callback fires and the capsulating Promise completes.
  createFileInPlace(resolve, reject) {
    this.mLastPopUpID++; //it's re-used
    //we also re-use the system's UIData-Requests container, through user mode funciton registerUIDataRequest().

    //Local Variables - BEGIN
    let newFileNamePrefix = 'New File';
    let newFileName = newFileNamePrefix;
    let foundElement = null;
    let i = 1;
    //Local Variables - END

    while ((foundElement = this.findElementByname(newFileName)) != null) {
      //keeping inventing new file-name until found to be unique
      i++;
      newFileName = newFileNamePrefix + ' ' + i;
    }

    this.pushFileToView(newFileName);
    this.refreshFMWindow();

    if (!this.makeElementNameEditable(newFileName))
      return;

    this.registerUIDataRequest(this.mLastPopUpID, resolve, reject);
    this.mEditingItemUIRequestID = this.mLastPopUpID;
  }


  async createFolderInPlaceA() {
    let promise = new Promise(function(resolve, reject) {
      this.createFolderInPlace(resolve, reject);
    }.bind(this));
    return promise;
  }

  newConsensusActionCallback(event) {
    let task = event.task;
    if (event.isLocal == false && this.mTools.compareByteVectors(task.getThreadID, this.getThreadID)) {
      if (task.getType == eConsensusTaskType.readyForCommit) {
        this.mHasCommittableOperation = true;
      } else {
        this.mHasCommittableOperation = false;
      }
    }
  }

  VMStateChangedCallback(event) {

    if (event.extended) {

      //we're now interested only in the new VM status signaling mechanics and signaling targeting this very UI application.
      //the full-node set the process ID once the DTI got initialized through CVMContext.initDTI()
      //no need for keeping track of requests' IDs etc. full node is now fully DTI/UI dApp process-aware.
      if (event.processID != this.getProcessID && !gTools.compareByteVectors(event.vmID, this.getThreadID)) {
        return; //not for this instance
      }
    } else {
      return;
    }


    switch (event.state) {

      case eVMState.synced:
        this.addNetworkRequestID(CVMContext.getInstance().getFileSystem.doCD(this.getCurrentPath, true, false, false, this.getThreadID).getReqID);
        break;

      case eVMState.limitReached:
        this.showMessageBox("Oooops...", "Limit of ⋮⋮⋮ Threads for this session has been reached.");

        break;
      case eVMState.initializing:
        if (!this.mHasActiveThread)
          this.showCurtain(true, true, true);

        break;
      case eVMState.ready:
        this.mHasActiveThread = true;

        // this.hideCurtain(); curtain is shown in hidden on per file-system traversal basis.
        //once the app is launched it requests root of the file-system. Only once the response arrives, the curtain disolves.

        break;
      case eVMState.aboutToShutDown:
        break;
      case eVMState.disabled:

        break;
      default:

    }
  }
  //Creates a stub-file in place.
  //Its name is actively editable, with cursor blinking at its end.
  //Once user types and the container looses focus, the callback fires and the capsulating Promise completes.
  createFolderInPlace(resolve, reject) {
    this.mLastPopUpID++; //it's re-used
    //we also re-use the system's UIData-Requests container, through user mode funciton registerUIDataRequest().

    //Local Variables - BEGIN
    let newEntityNamePrefix = 'New Folder';
    let newFolderName = newEntityNamePrefix;
    let foundElement = null;
    let i = 1;
    //Local Variables - END

    while ((foundElement = this.findElementByname(newFolderName)) != null) {
      //keeping inventing new file-name until found to be unique
      i++;
      newFolderName = newEntityNamePrefix + ' ' + i;
    }

    this.pushDirectoryToView(newFolderName);
    this.refreshFMWindow();

    if (!this.makeElementNameEditable(newFolderName))
      return;

    this.registerUIDataRequest(this.mLastPopUpID, resolve, reject);
    this.mEditingItemUIRequestID = this.mLastPopUpID;
  }

  async newInPlaceFolderRequest() {
    if (this.mCurrentPath.length <= 1) {
      this.showMessageBox('Oooopsy..⚠️', 'You need to be within a State-Domain🏠 that you own.');
      return;
    }
    let fileName = (await this.createFolderInPlaceA()).answer; // a stub file-entry is created, with editable file-name waiting to be filled-in.
    //as soon as file-name is filled-in, the Promise is finalized and blocking call returns.

    if (fileName == null || fileName.length == 0)
      this.showMessageBox('New Folder📁', 'Parameters not provided.');
    else {
      if (CVMContext.getInstance().getConnectionState != eConnectionState.connected) {
        this.showNotConnectedError();
        return;
      }
      this.addNetworkRequestID(CVMContext.getInstance().getFileSystem.doNewDir(fileName, false, this.getThreadID).getReqID);
      this.addNetworkRequestID(CVMContext.getInstance().getFileSystem.doLS(this.getThreadID).getReqID);
    }
  }


  async newInPlaceFileRequest() {
    if (this.mCurrentPath.length <= 1) {
      this.showMessageBox('Oooopsy..⚠️', 'You need to be within a State-Domain🏠 that you own.');
      return;
    }
    let fileName = (await this.createFileInPlaceA()).answer; // a stub file-entry is created, with editable file-name waiting to be filled-in.

    fileName = this.mCurrentPath + fileName;
    //as soon as file-name is filled-in, the Promise is finalized and blocking call returns.

    if (fileName == null || fileName.length == 0)
      this.showMessageBox('New File📄', 'Parameters not provided.');
    else {
      if (CVMContext.getInstance().getConnectionState != eConnectionState.connected) {
        this.showNotConnectedError();
        return;
      }
      this.addNetworkRequestID(CVMContext.getInstance().getFileSystem.doNewFile(fileName, '', false, this.getThreadID).getReqID);
      this.addNetworkRequestID(CVMContext.getInstance().getFileSystem.doLS(this.getThreadID).getReqID);
    }
  }

  async newFileRequest() {
    if (this.mCurrentPath.length <= 1) {
      this.showMessageBox('Oooopsy..⚠️', 'You need to be within a State-Domain🏠 that you own.');
      return;
    }

    let fileName = (await this.askStringA('New File📄', 'Enter new filename:')).answer;
    fileName = this.mCurrentPath + fileName;
    let content = (await this.askStringA('New File📄', 'Enter content:')).answer;

    if (fileName == null || content == null || fileName.length == 0 || content.length == 0)
      this.showMessageBox('New File📄', 'Parameters not provided.');
    else {
      if (CVMContext.getInstance().getConnectionState != eConnectionState.connected) {
        this.showNotConnectedError();
        return;
      }
      this.addNetworkRequestID(CVMContext.getInstance().getFileSystem.doNewFile(fileName, content, false, this.getThreadID).getReqID);
      this.addNetworkRequestID(CVMContext.getInstance().getFileSystem.doLS(this.getThreadID).getReqID);
    }
  }

  async newDirRequest() {

    let name = (await this.askStringA('New Directory📁', 'Enter new filename:')).answer;

    if (name == null || name.length == 0)
      this.showMessageBox('New Directory📁', 'Parameters not provided.');
    else {
      if (CVMContext.getInstance().getConnectionState != eConnectionState.connected) {
        this.showNotConnectedError();
        return;
      }
      this.addNetworkRequestID(CVMContext.getInstance().getFileSystem.doNewDir(name, false, this.getThreadID).getReqID);
      this.addNetworkRequestID(CVMContext.getInstance().getFileSystem.doLS(this.getThreadID).getReqID);
    }
  }

  destroyContextMenu() { //hack to hide away the context menu.
    //a work around a bog in the library.
    try {

      $(this.getBody).find('.fmFile').contextMenu('hide');
      $(this.getBody).find('.fmFile').contextMenu('destroy');
      $(document).find('.context-menu-list').last().removeAttr("style"); //just in case

    } catch (err) {

    }
  }
  showContextMenu()
  {
    //hack ensures the context menu is visible.
    //  $(document).find('.context-menu-list').show();
    //  $(document).find('.context-menu-list').last().css("display", "inline-block");
     //setTimeout(this.showContextMenu, 50);
    // setTimeout(this.showContextMenu, 150);
    //  setTimeout(this.showContextMenu, 350);
  }

  hideContextMenu() { //hack to hide away the context menu.
    //a work around a bog in the library.
  //  try {

      $(this.getBody).find('.fmFile').contextMenu('hide');
    //  $("#mydiv").css({top: 200, left: 200});
        $(document).find('.context-menu-list').css({top: 200, left: -100000});
    //  $(document).find('.context-menu-list').removeAttr("style");

  //  } catch (err) {

  //  }
  }
  closeWindow() {
    document.removeEventListener('click', this.mGlobalClickListener);
    this.destroyContextMenu();
    super.closeWindow();
  }
  //HTML code END
  constructor(positionX, positionY, width, height) {

    super(positionX, positionY, width, height, fileManagerBodyHTML, "File Manager", CFileManager.getIcon(), true);
    this.mGlobalClickListener = this.globalOnClickHandler.bind(this);
    document.addEventListener("click", this.mGlobalClickListener);

    this.mHasCommittableOperation = false;
    this.mFileInfoCache = {};
    this.mHasActiveThread = false;
    this.mFileSelector = document.createElement('input');
    this.mFileSelector.type = 'file';
    this.mCurrentPath = '/';
    this.mGoingForth = false;
    this.mGoingBack = false;
    this.mHistory = [];
    this.mHistoryIndex = 0;
    this.mEditingItemUIRequestID = null;
    this.setThreadID = 'FS_THREAD_' + this.getProcessID;
    this.mMetaParser = new CVMMetaParser();
    this.mHtmlEntries = [];
    this.setWindowOverflowMode = eWindowOverflowMode.hidden;
    this.mLastHeightRearangedAt = 0;
    this.mLastWidthRearangedAt = 0;
    this.mVisibleFilesCount = 0;
    this.mVisibleFoldersCount = 0;
    this.mVisibleStateDomainsCount = 0;
    CVMContext.getInstance().addNewDFSMsgListener(this.newDFSMsgCallback.bind(this), this.mID);
    CVMContext.getInstance().addVMStateChangedListener(this.VMStateChangedCallback.bind(this), this.mID);
    CVMContext.getInstance().addNewConsensusActionListener(this.newConsensusActionCallback.bind(this), this.mID);
    this.mOptions = {
      // CSS Class to add to the drop element when a drag is active
      dragClass: "drag",

      // A string to match MIME types, for instance
      accept: false,

      // An object specifying how to read certain MIME types
      // For example: {
      //  'image/*': 'DataURL',
      //  'data/*': 'ArrayBuffer',
      //  'text/*' : 'Text'
      // }
      readAsMap: {},

      // How to read any files not specified by readAsMap
      readAsDefault: 'DataURL',
      on: {
        beforestart: function(e, file) {
          // return false if you want to skip this file
        },
        loadstart: function(e, file) {
          /* Native ProgressEvent */
        },
        progress: function(e, file) {
          /* Native ProgressEvent */
        },
        load: this.fileReceived.bind(this),
        error: function(e, file) {
          /* Native ProgressEvent */
        },
        loadend: function(e, file) {
          /* Native ProgressEvent */
        },
        abort: function(e, file) {
          /* Native ProgressEvent */
        },
        skip: function(e, file) {
          // Called when a file is skipped.  This happens when:
          //  1) A file doesn't match the accept option
          //  2) false is returned in the beforestart callback
        },
        groupstart: function(group) {
          // Called when a 'group' (a single drop / copy / select that may
          // contain multiple files) is receieved.
          // You can ignore this event if you don't care about groups
        },
        groupend: function(group) {
          // Called when a 'group' is finished.
          // You can ignore this event if you don't care about groups
        }
      }
    };
  }

  openFileSelectAndUpload() {
    this.mFileSelector.click();

    this.mFileSelector.onchange = e => {
      let file = e.target.files[0];
      this.fileReceived.bind(this, e, file)();
      mFileSelector = document.createElement('input');
    }



  }
  addToHistory(url, updateIndex = true) {

    if (this.mHistory.length) {
      if (this.mHistory[this.mHistory.length - 1] == url) {
        return false;

      }
    }
    console.log('Adding File-Path to History (' + url + ")");
    this.mHistory.push(url);

    if (updateIndex) {
      this.mHistoryIndex++;
      //this.setCurrentURLVisible(url);
    }
    return true;
  }
  userResponseCallback(e) {
    console.log('User answered:' + e.answer)
    if (e.answer) {
      //use the provided value
    }
  }
  showNotConnectedError() {
    Swal.fire({
      title: 'Oppps...',
      text: 'You are not connected to network :-(',
      footer: 'Wait for the connection to be re-established..',
      confirmButtonText: 'Ok..'
    });
  }
  fileReceived(e, file) {
    //  if (CVMContext.getInstance().getConnectionState != eConnectionState.connected) {
    //  gWindowManager.showNotConnectedError();
    //  return;
    //  }
    let reader = new FileReader();
    reader.readAsArrayBuffer(file);
    reader.onloadend = function(file, event) {

      if (reader.result.byteLength > Math.pow(2, 2 * 8)) {
        this.showMessageBox("Oppps...🥴", "The file is too big for ⋮⋮⋮ Eternal Storage 😵");
        return;
      }
      this.mDraggedFileContents = reader.result;
      this.mDraggedFileName = file.name;
      this.showFileImportWizard();


      /*

        // The contents of the BLOB are in reader.result:
        let byteArray = reader.result;

        //ask for confirmation

        Swal.fire({
          title: 'Confirmation',
          text: 'You are abot to upload file ' + file.name + ' Size: ' + file.extra.prettySize + ' Do you confirm?',

          showCancelButton: true,
          confirmButtonColor: '#3085d6',
          cancelButtonColor: '#d33',
          confirmButtonText: 'Yes!'
        }).then((result) => {
          if (result.value) {
            this.addNetworkRequestID(CVMContext.getInstance().getFileSystem.doNewFile(file.name, byteArray).getReqID);
            this.addNetworkRequestID(CVMContext.getInstance().getFileSystem.doLS(window.getThreadID).getReqID);
            Swal.fire(
              'Confirmed!',
              'Operation completed..',
              'success'
            )
          }
        })
*/
    }.bind(this, file, reader);

  }



  finishResize(isFallbackEvent) { //make unitary selection?

    let fileManagerH = this.getBody.querySelector('#windowFMBody');
    if (fileManagerH != null)
      fileManagerH.classList.add('scrollY');


    super.finishResize(isFallbackEvent);

  }
  showFileImportWizard(view = eFMUploadView.modeSelector) {
    let body = this.getBody;

    $('#fileManagerTable', body).css('display', 'none');
    $('#fileImportCreator', body).css('display', 'block');

    switch (view) {
      case eFMUploadView.modeSelector:
        $('#uploadModeSelectionView', body).css('display', 'block');
        $('#uploadRewardSelectionView', body).css('display', 'none');
        $('#uploadConfirmationView', body).css('display', 'none');
        break;

      case eFMUploadView.rewardSelector:
        $('#uploadModeSelectionView', body).css('display', 'none');
        $('#uploadRewardSelectionView', body).css('display', 'block');
        $('#uploadConfirmationView', body).css('display', 'none');
        let tokenPools = CVMContext.getUserTokenPools;
        gTools.fillSelectWithOptions($('#tokenPoolSelector', body)[0], tokenPools);
        $(body).find('#tokenPoolSelector').editableSelect({
          effects: 'fade'
        });

        break;
      case eFMUploadView.confirmation:
        $('#uploadModeSelectionView', body).css('display', 'none');
        $('#uploadRewardSelectionView', body).css('display', 'none');
        $('#uploadConfirmationView', body).css('display', 'block');
        break;
      default:
    }
  }

  doCrowdFundedUpload() {
    this.showMessageBox('Sub-system not available', 'Crowd-Funded storage is currently disabled on this node.');

    //this.showFileImportWizard(1);
  }

  //Uploads file to ⋮⋮⋮ Eternal Storage.
  doEternalUpload() {

    if (this.mCurrentPath.length <= 1) {
      this.showMessageBox('Oooopsy..⚠️', 'You need to be within a State-Domain🏠 that you own.');
      return;
    }

    //as soon as file-name is filled-in, the Promise is fin

    if (this.mTools.isNull(this.mDraggedFileContents) || this.mTools.isNull(this.mDraggedFileName) ||
      this.mDraggedFileContents.byteLength == 0 || this.mDraggedFileName.length == 0) {

      this.showMessageBox('Oooopsy..⚠️', 'Unable to read the provided file.'); //file data not fetched.
    }

    let fileName = this.mCurrentPath + this.mDraggedFileName; // use non-relative, explicit file path, based on the current directory.

    let reqid = CVMContext.getInstance().getFileSystem.doNewFile(fileName, this.mDraggedFileContents, false, this.getThreadID).getReqID;

    if (reqid) {
      this.addNetworkRequestID(reqid)
      this.pushFileToView(this.mDraggedFileName, true); //render new file within the View
    } else {
      this.showMessageBox('Oooopsy..⚠️', 'There was trouble uploading file to ⋮⋮⋮ Node.'); //upload went wrong; notify the user
    }
  }

  closeImportWizard() {
    let body = this.getBody;
    $('#fileImportCreator', body).css('display', 'none');
    $('#fileManagerTable', body).css('display', 'block');
  }

  initCustomSysSettings() {
    if (this.mCustomInitied)
      return;

    this.setCurtainAnimSpeed = 200;
    this.setObserverTimeWindow = 300;
    this.setMinCurtainDuration = 0;
    this.setQueueInterval = 100;
    this.mCustomInitied = true;
  }
  showWorld() {

    this.clearCurrentPath();
    if (CVMContext.getInstance().getConnectionState != eConnectionState.connected) {
      this.showMessageBox("Oppps...", "You are not connected to the ⋮⋮⋮ Network.");
      return;
    }
    this.showCurtain(true, true, true);
    this.addNetworkRequestID(CVMContext.getInstance().getFileSystem.doCD('//', true, false, false, this.getThreadID).getReqID);
    //  this.addNetworkRequestID(CVMContext.getInstance().getFileSystem.doLS(this.getThreadID).getReqID);

  }
  gForthRequest() {
    if (CVMContext.getInstance().getConnectionState != eConnectionState.connected) {
      this.showNotConnectedError();
      return;
    }
    this.mGoingForth = true;
    this.addNetworkRequestID(CVMContext.getInstance().getFileSystem.doCD('..', true, false, false, this.getThreadID).getReqID);
  }

  commitActions(e) {

    let ctx = CVMContext.getInstance();

    ctx.getMagicButton.commitActions();
  }

  goCommitRequest() {
    if (!this.mHasCommittableOperation) {
      this.showMessageBox("Oppps...", "There are no pending file system edits.");
      return;
    }
    this.commitActions();
  }
  goBackRequest() {
    if (CVMContext.getInstance().getConnectionState != eConnectionState.connected) {
      this.showNotConnectedError();
      return;
    }
    this.showCurtain(true, true, true);
    if (this.mHistoryIndex) {
      this.mHistoryIndex--;
    }
    this.mGoingBack = true;

    this.addNetworkRequestID(CVMContext.getInstance().getFileSystem.doCD('..', true, false, false, this.getThreadID).getReqID);
  }

  goForthRequest() {

    if (this.mHistory.length && this.mHistoryIndex < (this.mHistory.length - 1)) {
      this.mHistoryIndex++;

    }
    this.showCurtain(true, true, true);
    this.mGoingForth = true;
    this.addNetworkRequestID(CVMContext.getInstance().getFileSystem.doCD(this.mHistory[this.mHistoryIndex], true, false, false, this.getThreadID).getReqID);
    return true;

  }


  open() {
    console.log("File Manager warming up..");
    this.mContentReady = false;
    //Note: custom Curtain setting initialized AFTER the initial initialization of the UI
    setTimeout(this.initCustomSysSettings.bind(this), 3000);

    //this.setCurtainOpacity=0.1;
    this.mCurtainOverrideObserver = true;
    super.open();

    this.setTitle("File Manager");
    let body = this.getBody;



    //
    let fmt = $('#fileManagerTable', body)[0];
    if (fmt != null) {
      let fmtTable = document.createElement('table');
      fmtTable.style.width = '100%';
      fmtTable.style.height = '100%';
      fmtTable.setAttribute("id", "fileManagerTableT")
      fmt.appendChild(fmtTable);
    }
    this.mContentReady = true;
    this.redraw();
    //
    // Accept files from the specified input[type=file]
    let fileInput = $('#file-input', body)[0];
    FileReaderJS.setupInput(fileInput, this.mOptions);

    // Accept dropped files on the specified file
    FileReaderJS.setupDrop(fmt, this.mOptions);
    // Accept paste events if available

    this.mElementsContainer = fmt;

    FileReaderJS.setupClipboard(body, this.mOptions);

    $(body).find('#locationField').on('keyup', function(e) {

      if (e.keyCode == 13) {
        let url = this.getValue('locationField');
        this.addNetworkRequestID(CVMContext.getInstance().getFileSystem.doCD(url, true, false, false, this.getThreadID).getReqID);
      }
    }.bind(this)).editableSelect({
      effects: 'fade'
    });



    attachViewSelectorToShadow(body);

    //assign events to UI actors - BEGIN
    $(this.getBody).find('.newFileBtn')[0].onclick = this.newInPlaceFileRequest.bind(this); //newFileRequest
    $(this.getBody).find('.newFolderBtn')[0].onclick = this.newInPlaceFolderRequest.bind(this); //newDirRequest.bind(this);
    $(this.getBody).find('.uploadBtn')[0].onclick = this.newUploadRequest.bind(this);
    $(this.getBody).find('.showWorld')[0].onclick = this.showWorld.bind(this);
    $(this.getBody).find('.goBackBtn')[0].onclick = this.goBackRequest.bind(this);
    $(this.getBody).find('.goForthBtn')[0].onclick = this.goForthRequest.bind(this);
    $(this.getBody).find('.commitBtn')[0].onclick = this.goCommitRequest.bind(this);
    this.getBody.addEventListener('click', this.finalizePendingUIActions.bind(this), true);
    //^capturing. i.e. whatever is clicked (if not the currently edited element), then we finalize pending actions

    this.setCustomCurtain($(this.getBody).find('.customCurtain')[0]);

    $(this.getBody).find('#locationField')[0].addEventListener('keypress', function(e) {
      if (e.key === 'Enter') {
        let url = this.getValue('locationField');
        this.addNetworkRequestID(CVMContext.getInstance().getFileSystem.doCD(url, true, false, false, this.getThreadID).getReqID);
      }
    }.bind(this));

    //assign events to UI actors - END

    //var script = document.createElement("script");
    //script.type= "text/javascript";
    //script.src ='/lib/metro.min.js';
    //  shadow.appendChild(script);
    //  innerBody.
    //script.src = "/lib/bootstrap.min.js";
    //shadow.appendChild(script);
    this.showCurtain(true, true, true);

    if (CVMContext.getInstance().isLoggedIn)
      this.takeUserToHomeDir(); //ony once user is ready take him to his home directory
    else this.showWorld();

  }
  refreshPathInUI() {
    this.setValue('locationField', this.mCurrentPath);
  }

  finalizePendingUIActions(event) {
    //hasParentClass(event.target,'fmElement')
    if (this.mElementBeingEdited && ((this.mElementBeingEdited == event.target) || this.mElementBeingEdited.contains(event.target))) { //do not finalize if clicked item is the one currently edited
      return false;
    }

    this.finalizeFileNameEditing(); //might be agnostic when no pending file edits
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

  findElementByname(name) {
    if (gTools.isNull(name))
      return null;

    //get all elements
    let allElements = $(this.mElementsContainer).find('.fmElementNameContainer');

    if (gTools.isNull(allElements) || allElements.length == 0)
      return null; //none available


    //try to find the element which we're after
    for (let i = 0; i < allElements.length; i++) {
      if ($(allElements[i]).text() == name) {
        return allElements[i];

      }
    }
    return null;
  }

  //Make UI-element, one representing a particular item - editable.
  makeElementNameEditable(name) {

    this.mElementBeingEdited = this.findElementByname(name);
    //found it?
    if (!this.mElementBeingEdited)
      return false;

    //update UI
    $(this.mElementBeingEdited).attr('contenteditable', 'true');

    //move carret to the end en the edtiable field
    gTools.setEndOfContenteditable(this.mElementBeingEdited);

    return true;
  }


  //This method is invoked once the edited element gets out of scope/looses active focus.
  //The active element might loose scope either explicitly or implicitly (i.e another window becomes active).
  finalizeFileNameEditing() {

    if (!this.mElementBeingEdited)
      return false; // we even shouldn't be here

    //notify cosumer, i.e awaiting callback function or Promise.
    this.notifyAnswer(this.mEditingItemUIRequestID, $(this.mElementBeingEdited).text());
    //^which is when the callback fires and Promise completes

    //update UI - the element's name is editable no more.
    $(this.mElementBeingEdited).attr('contenteditable', 'false');

    this.mElementBeingEdited = null;
    this.mEditingItemUIRequestID = null;
    return true;
  }

  cdEdInto(name) {
    this.mCurrentPath += ('/' + name);
    this.refreshPathInUI();

  }
  clearCurrentPath() {
    this.mCurrentPath = '/';
    this.refreshPathInUI();
  }

  set setCurrentPath(path) {
    this.mCurrentPath = path;
    this.refreshPathInUI();
  }

  get getCurrentPath() {
    return this.mCurrentPath;
  }

  takeUserToHomeDir() {
    let context = CVMContext.getInstance();
    if (context.isLoggedIn) { //Teleport user to his home-directory - BEGIN
      //hide output
      let userID = context.getUserID;
      //  context.sendLine('cd ' + userID + "\r\n", this.getProcessID);

      let cmd = 'cd ' + userID;

      this.setCurrentPath = ('/' + userID);

      if (CVMContext.getInstance().getConnectionState != eConnectionState.connected) {
        this.showMessageBox("Oppps...", "You are not connected to the ⋮⋮⋮ Network.");
        return false;
      }
      this.addNetworkRequestID(CVMContext.getInstance().getFileSystem.doCD(this.mCurrentPath, true, false, false, this.getThreadID).getReqID);
      return true;
    } //Teleport user to his home-directory - END
    return false;
  }

  pushFileToView(name, refresh = false) {
    this.mHtmlEntries.push('<div class="fmElement fmFile"><img class="folderElement context-menu-one btn btn-neutral" src="/images/file.png"  title=&#39;' + name +
      '&#39; onclick="let window = gWindowManager.getWindowByID($(this).closest(&#39;.idContainer&#39;).find(&#39;#windowIDField&#39;).first().val()); window.addNetworkRequestID(CVMContext.getInstance().getFileSystem.doGetFile(&#39;' + name +
      '&#39;,false,window.getThreadID ).getReqID);"> <div class="fmElementNameContainer">' + name + "</div>");

    if (refresh) {
      this.refreshFMWindow();
    }
  }

  pushDirectoryToView(name, refresh = false) {
    this.mHtmlEntries.push('<div class="fmElement fmDirectory"><img class="folderElement context-menu-one btn btn-neutral" src="/images/folder.png"   title=&#39;' + name +
      '&#39;  onclick="let window = gWindowManager.getWindowByID($(this).closest(&#39;.idContainer&#39;).find(&#39;#windowIDField&#39;).first().val()); window.cdEdInto(&#39;' + name + '&#39;); window.addNetworkRequestID(CVMContext.getInstance().getFileSystem.doCD(&#39;' + name +
      '&#39;, true, false, false, window.getThreadID).getReqID); window.addNetworkRequestID(CVMContext.getInstance().getFileSystem.doLS(window.getThreadID).getReqID);"> <div class="fmElementNameContainer">' + name + "</div>");

    if (refresh) {
      this.refreshFMWindow();
    }
  }
  pushStateDomainToView(name, refresh = false) {
    this.mHtmlEntries.push('<div class="fmElement fmDomain"><img class="folderElement" src="/images/domain.png"   title=&#39;' + name +
      '&#39;  onclick="let window = gWindowManager.getWindowByID($(this).closest(&#39;.idContainer&#39;).find(&#39;#windowIDField&#39;).first().val()); window.cdEdInto(&#39;' + name + '&#39;); window.addNetworkRequestID(CVMContext.getInstance().getFileSystem.doCD(&#39;' + name +
      '&#39;, true, false, false, window.getThreadID).getReqID); window.addNetworkRequestID(CVMContext.getInstance().getFileSystem.doLS(window.getThreadID).getReqID);"><div class="fmElementNameContainer">' + name + "</div>");

    if (refresh) {
      this.refreshFMWindow();
    }
  }

  globalOnClickHandler() {
    this.hideContextMenu();
  }

  elemOnContext(e) {
    let fileName = $(e.target.parentElement).find('.fmElementNameContainer')[0].innerHTML;
    this.setSelectedElementPath = this.getCurrentPath + fileName;
   this.showContextMenu();
    //fmElementNameContainer
  }
  set setTargetPackageID(id) {
    this.mTargetPackageID = id;
  }

  get getTargetPackageID() {
    return this.mTargetPackageID;
  }
  refreshContextMenuBindings() {


    //Local Variables - BEGIN
    let handlingItems = {};
    let fileHandlers = CVMContext.getInstance().getPackageManager.getAppSelector.getAllHandlers();

    let usableUniquePackages = [];
    //Local Variables - END

    //construct the handlers' sub-menu
    /*each element is supposed to look like so:
    "sortyByName": {
      "name": "By Name"
    },
    */
    for (let i = 0; i < fileHandlers.length; i++) {
      if (usableUniquePackages.indexOf(fileHandlers[i].getPackageID) === -1) {
        usableUniquePackages.push(fileHandlers[i].getPackageID);
      }
    }

    for (let i = 0; i < usableUniquePackages.length; i++) {
      handlingItems[usableUniquePackages[i]] = {
        'name': usableUniquePackages[i]
      };
    }
    let table = this.getBody.querySelector('#fileManagerTableT')
    let elements = $(table).find('.fmFile');


    $(elements).off('contextmenu'); //reset handlers
    $(elements).on('contextmenu', this.elemOnContext.bind(this));
    let menuContainer = this.getBodyRoot;
    $(table).contextMenu({
      selector: '.fmFile',
    //  appendTo: menuContainer,
    autoHide: true,
    //animation: `{duration: 500, show: 'fadeIn', hide: 'fadeOut'}`,
      callback: function(key, options) {
        //var m = "clicked: " + key + " on " + $(this).text();
        //window.console && console.log(m) || alert(m);


        switch (key) {
          //handle specific hard-coded actions
          case 'open':
            this.setTargetPackageID = null;
            this.addNetworkRequestID(CVMContext.getInstance().getFileSystem.doGetFile(this.getSelectedElementPath, false, this.getThreadID).getReqID);
            break;

          default:
            //Check if a valid namespace.
            if (this.mTools.isNamespaceValid(key)) {
              //If so, open the file through that very package.
              //let thread = this.mVMContext.getThreadByID(this.getThreadID);
              //update, do not launch app directly. let us fetch the file contents first to make the target dApp's life easier.
              //  this.mVMContext.getWindowManager.launchApp(key, true, null, eDataType.bytes, this.getSelectedElementPath, thread);
              this.setTargetPackageID = key;
              this.addNetworkRequestID(CVMContext.getInstance().getFileSystem.doGetFile(this.getSelectedElementPath, false, this.getThreadID).getReqID);
              //once the content is fetched, the target application would be used to open the file.
            }
        }
      }.bind(this),
      items: {
        "open": {
          "name": "Open",
          "icon": "fa-play"
        },
        "fold2": {
          "name": "Open With",
          "icon": "fa-gear",
          "items": handlingItems
        }
      }
    });
  }
  set setSelectedElementPath(val) {
    this.mSelectedElementPath = val;
  }
  get getSelectedElementPath() {
    return this.mSelectedElementPath;
  }
  newDFSMsgCallback(dfsMsg) {
    if (!this.hasNetworkRequestID(dfsMsg.getReqID))
      return;

    //  this.mFileInfoCache.clear();

    if (dfsMsg.getData1.byteLength > 0) {
      //this.writeToLog('<span style="color: blue;">DFS-data-field-1 (meta-data)contains data..</span>');

      let metaData = this.mMetaParser.parse(dfsMsg.getData1);

      if (metaData != 0) {

        let sections = this.mMetaParser.getSections;

        for (let i = 0; i < sections.length; i++) {
          let sType = sections[i].getType;

          if (sType == eVMMetaSectionType.directoryListing) {
            this.destroyContextMenu();
            this.mCurtainOverrideObserver = false; //override DOM observer no more. Data available.
            this.setCurrentPath = gTools.arrayBufferToString(sections[i].getMetaData);

            if (!(this.mGoingBack || this.mGoingForth)) {

              if (this.mHistoryIndex != this.mHistory.length - 1 && ((this.mHistoryIndex + 1) < this.mHistory.length)) {
                this.mHistory.length = (this.mHistoryIndex + 1); //avoid splice; this will be faster  .splice has to create a new array containing all the removed items, whereas .length creates nothing and "returns" a number instead of a new array.
              }
              this.addToHistory(this.getCurrentPath)

            } else {
              this.mGoingBack = false;
              this.mGoingForth = false;
            }
          }

          let entries = sections[i].getEntries;
          let entriesCount = entries.length;
          //if (sType != eVMMetaSectionType.directoryListing)
          //    break;
          this.mVisibleFilesCount = 0;
          this.mVisibleFoldersCount = 0;
          this.mVisibleStateDomainsCount = 0;
          this.mHtmlEntries = [];
          for (let a = 0; a < entriesCount; a++) {

            let dataFields = entries[a].getFields;


            let name = gTools.uintToString(new Uint8Array(dataFields[0]));
            if (entries[a].getType == eDFSElementType.stateDomainEntry) {
              //
              this.mVisibleStateDomainsCount++;
              //
              this.pushStateDomainToView(name);
            } else
            if (entries[a].getType == eDFSElementType.directoryEntry) {
              this.mVisibleFoldersCount++;
              this.pushDirectoryToView(name);

            } else if (entries[a].getType == eDFSElementType.fileEntry) {
              this.mVisibleFilesCount++;
              this.pushFileToView(name);

              if (!this.mFileInfoCache.hasOwnProperty(this.getCurrentPath + name)) {
                //  let k = this.getCurrentPath + name;
                //  this.mFileInfoCache[k] = dataType;//{
                //  dataType: dataType,
                //  data: null
                //  };
              } else {
                //  this.mFileInfoCache[this.getCurrentPath + name].dataType = dataType;
              }

            } else if (entries[a].getType == eDFSElementType.fileContent) {

              let dataType = dataFields[0];
              let path = dataFields[1];
              let fileName = gTools.parsePath(path).fileName;
              let data = dataFields[2];

              //Update Cache - Begin
              let k = (this.getCurrentPath + name);
              //  if (!this.mFileInfoCache.hasOwnProperty(k)) {
              this.mFileInfoCache[k] = {
                dataType: dataType,
                data: data
              };
              //  } else {
              //    this.mFileInfoCache[k].dataType = dataType;
              //    this.mFileInfoCache[k].dataType = dataType;
              //  }
              //Update Cache - END
              console.log("opening file..");
              let handler = CVMContext.getInstance().getPackageManager.
              getAppSelector.getHandlerForContent(gTools.parsePath(path).extension, dataType);

              //fetch the user-selected UI dApp, one which is to handle the file-opening operation.
              let customTargetPackage = this.getTargetPackageID;
              if (customTargetPackage) {
                //clear it.One time-only.
                this.setTargetPackageID = null;
              }

              if (handler || customTargetPackage) {
                //path is passed on to the app, so it could autonomously formulate instructions to be commited to the DVM.
                //once the Magic Button is pressed.
                let thread = this.mVMContext.getThreadByID(this.getThreadID);
                //use the user-selected UI dApp if available, otherwise proceed with the default handler.
                this.mVMContext.getWindowManager.launchApp((customTargetPackage ? customTargetPackage : handler.getPackageID), true, data, dataType, gTools.arrayBufferToString(path), thread);
              } else {
                switch (dataType) {
                  case eDataType.bytes:
                    Swal.fire({
                      title: "File Content",
                      text: gTools.uintToString(new Uint8Array(data)),
                      confirmButtonText: 'Go It!'
                    });
                    break;

                  case eDataType.signedInteger:
                    data = gTools.arrayBufferToNumber(data);
                    break;
                  case eDataType.unsignedInteger:
                    data = gTools.arrayBufferToNumber(data);
                    Swal.fire({
                      title: "File Content",
                      text: data,
                      confirmButtonText: 'Go It!'
                    });
                    break;
                  case eDataType.BigInt:
                    data = gTools.arrayBufferToBigInt(data);
                    Swal.fire({
                      title: "File Content",
                      text: data,
                      confirmButtonText: 'Go It!'
                    });
                    break;
                  default:
                    Swal.fire({
                      title: "Ooooops...",
                      icon: 'error',
                      text: 'Data type currently unsupported in GUI:-(',
                      confirmButtonText: 'Go It!'
                    });
                    return;
                }
              }



              return;
            }
          }
          this.refreshFMWindow();
        }

      } else {

        return;

      }
    } else {

    }
    this.refreshContextMenuBindings();
  }


  clearFMTable() {


    let fileManagerTableH = this.getBody.querySelector('#fileManagerTableT');

    while (fileManagerTableH.firstChild) {
      fileManagerTableH.removeChild(fileManagerTableH.firstChild);
    }
  }
  refreshFMWindow() {

    if (!this.mID)
      return;
    let windowHandle = document.querySelector('#' + this.mID);
    if (windowHandle == null)
      return;

    let fileManagerH = this.getBody.querySelector('#windowFMBody');
    let footer = this.getBody.querySelector('#windowFooter');
    footer.innerHTML = ((this.mVisibleStateDomainsCount > 0) ? ("Users: " + this.mVisibleStateDomainsCount + " ") : "") + "Folders: " + this.mVisibleFoldersCount + " Files: " + this.mVisibleFilesCount;

    ///print within test-UI - BEGIN
    this.clearFMTable();
    let parsedEntries = 0;
    var tableBody = document.createElement('TBODY');
    //
    let table = this.getBody.querySelector('#fileManagerTableT')
    //
    table.appendChild(tableBody);
    let windowWidth = windowHandle.offsetWidth;

    let elementsInRow = 8;
    elementsInRow = Math.max(Math.floor(windowWidth / 60) - 1, 0);
    if (elementsInRow > 0)
      while (parsedEntries < this.mHtmlEntries.length) {
        for (var i = 0; i <= elementsInRow - 1; i++) {
          var tr = document.createElement('TR');
          tableBody.appendChild(tr);

          for (var j = 0; j < elementsInRow && parsedEntries < this.mHtmlEntries.length; j++) {
            var td = document.createElement('TD');
            td.width = '60';
            td.height = '60';
            td.style = "text-align: center;"
            td.innerHTML = this.mHtmlEntries[parsedEntries];
            parsedEntries++;
            //td.appendChild(document.createTextNode("Cell " + i + "," + j));
            tr.appendChild(td);
          }
        }
      }
    ///  print within test-UI - END
  }
  redraw() {
    //
    if (!this.mID)
      return;
    let windowHandle = document.querySelector('#' + this.mID);
    if (windowHandle == null)
      return;

    let fileManagerH = this.getBody.querySelector('#windowFMBody');
    if (!fileManagerH)
      return;
    let windowToolbarHeight = this.getBody.querySelector('#fileManagerToolbar').offsetHeight;
    let fileManagerWindowFooterHeight = this.getBody.querySelector('#windowFooter').offsetHeight;

    var windowHeaderTitleHeight = windowHandle.querySelector('#windowTitle').offsetHeight;
    var windowHeight = windowHandle.offsetHeight;
    //var cssheight = document.getElementById(this.mID).style.height;
    let newHNr = Math.max((windowHeight - windowToolbarHeight - windowHeaderTitleHeight - fileManagerWindowFooterHeight - 9), 0);
    let newHeight = newHNr + "px";
    fileManagerH.style.height = newHeight;
    fileManagerH.style.top = windowToolbarHeight + 'px';

    var windowWidth = windowHandle.offsetWidth;
    if (this.mContentReady)
      if (Math.abs(this.mLastWidthRearangedAt - windowWidth) > 60) {
        this.mLastWidthRearangedAt = windowWidth;
        this.refreshFMWindow();
      } else {

      }
  }
}

export default CFileManager;
