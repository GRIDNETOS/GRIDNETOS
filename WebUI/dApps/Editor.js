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
  CAppSelector,
  CContentHandler
} from "./../lib/AppSelector.js"

import { CGLink, CGLinkHandler } from "/lib/GLink.js"

const editorBodyHTML = `

<style>

.box {
  display: flex;
  flex-flow: column;
  height: 100%;
}

.box .row {
  border: 1px dotted #0313fc;
}

.box .row.header {
  flex: 0 1 auto;
}

.box .row.content {
  flex: 1 1 auto;
}

.box .row.footer {
  flex: 0 1 40px;
}


.ck.ck-editor
{
	display: flex !important;
	flex-flow: column !important;
	height: 100% !important;
}
	.ck.ck-editor__top
	{
		  flex: 0 1 auto !important;
	}
	.ck.ck-editor__main
	{
		  flex: 1 1 auto !important;
	}

	.ck-source-editing-area
	{
	    overflow: auto !important;
          min-height: 100%;
	}
	.ck.ck-editor__main>.ck-editor__editable:not(.ck-focused)
	{
				border-top-color: #ccce !important;
				border-right-color: transparent !important;
				border-bottom-color: transparent !important;
				border-left-color: transparent !important;
	}
	.ck.ck-editor__main>.ck-editor__editable.ck-focused:not(.ck-editor__nested-editable)
	{				border-top-color: #ccce !important;
					border-right-color: transparent !important;
					border-bottom-color: transparent !important;
					border-left-color: transparent !important;

	}
	.ck.ck-editor__editable > .ck-placeholder::before {
	    color: #a2a4a9;
	    font-family: Georgia;
	}

	:root {
	    /* Overrides the border radius setting in the theme. */
	    --ck-border-radius: 4px;

	    /* Overrides the default font size in the theme. */
	    --ck-font-size-base: 14px;

	    /* Helper variables to avoid duplication in the colors. */
	    --ck-custom-background: hsl(206deg 82% 16%);
	    --ck-custom-foreground: hsl(255, 3%, 18%);
	    --ck-custom-border: hsl(291deg 28% 50%);
	    --ck-custom-white: hsl(0, 0%, 100%);
      --ck-color-base-background: #1c353f;

	    /* -- Overrides generic colors. ------------------------------------------------------------- */

	    --ck-color-base-foreground: var(--ck-custom-background);
	    --ck-color-focus-border: hsl(208, 90%, 62%);
	    --ck-color-text: #11d9d2;
	    --ck-color-shadow-drop: hsla(0, 0%, 0%, 0.2);
	    --ck-color-shadow-inner: hsla(0, 0%, 0%, 0.1);

	    /* -- Overrides the default .ck-button class colors. ---------------------------------------- */

	    --ck-color-button-default-background: var(--ck-custom-background);
	    --ck-color-button-default-hover-background: hsl(270, 1%, 22%);
	    --ck-color-button-default-active-background: hsl(270, 2%, 20%);
	    --ck-color-button-default-active-shadow: hsl(270, 2%, 23%);
	    --ck-color-button-default-disabled-background: var(--ck-custom-background);

	    --ck-color-button-on-background: var(--ck-custom-foreground);
	    --ck-color-button-on-hover-background: hsl(255, 4%, 16%);
	    --ck-color-button-on-active-background: hsl(255, 4%, 14%);
	    --ck-color-button-on-active-shadow: hsl(240, 3%, 19%);
	    --ck-color-button-on-disabled-background: var(--ck-custom-foreground);

	    --ck-color-button-action-background: hsl(168, 76%, 42%);
	    --ck-color-button-action-hover-background: hsl(168, 76%, 38%);
	    --ck-color-button-action-active-background: hsl(168, 76%, 36%);
	    --ck-color-button-action-active-shadow: hsl(168, 75%, 34%);
	    --ck-color-button-action-disabled-background: hsl(168, 76%, 42%);
	    --ck-color-button-action-text: var(--ck-custom-white);

	    --ck-color-button-save: hsl(120, 100%, 46%);
	    --ck-color-button-cancel: hsl(15, 100%, 56%);

	    /* -- Overrides the default .ck-dropdown class colors. -------------------------------------- */

	    --ck-color-dropdown-panel-background: var(--ck-custom-background);
	    --ck-color-dropdown-panel-border: var(--ck-custom-foreground);

	    /* -- Overrides the default .ck-splitbutton class colors. ----------------------------------- */

	    --ck-color-split-button-hover-background: var(--ck-color-button-default-hover-background);
	    --ck-color-split-button-hover-border: var(--ck-custom-foreground);

	    /* -- Overrides the default .ck-input class colors. ----------------------------------------- */

	    --ck-color-input-background: var(--ck-custom-background);
	    --ck-color-input-border: hsl(257, 3%, 43%);
	    --ck-color-input-text: hsl(0, 0%, 98%);
	    --ck-color-input-disabled-background: hsl(255, 4%, 21%);
	    --ck-color-input-disabled-border: hsl(250, 3%, 38%);
	    --ck-color-input-disabled-text: hsl(0, 0%, 78%);

	    /* -- Overrides the default .ck-labeled-field-view class colors. ---------------------------- */

	    --ck-color-labeled-field-label-background: var(--ck-custom-background);

	    /* -- Overrides the default .ck-list class colors. ------------------------------------------ */
    	--ck-color-switch-button-inner-background:#57135a;
	    --ck-color-list-background: var(--ck-custom-background);
	    --ck-color-list-button-hover-background: var(--ck-color-base-foreground);
	    --ck-color-list-button-on-background: var(--ck-color-base-active);
	    --ck-color-list-button-on-background-focus: var(--ck-color-base-active-focus);
	    --ck-color-list-button-on-text: var(--ck-color-base-background);

	    /* -- Overrides the default .ck-balloon-panel class colors. --------------------------------- */

	    --ck-color-panel-background: var(--ck-custom-background);
	    --ck-color-panel-border: var(--ck-custom-border);

	    /* -- Overrides the default .ck-toolbar class colors. --------------------------------------- */

	    --ck-color-toolbar-background: var(--ck-custom-background);
	    --ck-color-toolbar-border: var(--ck-custom-border);

	    /* -- Overrides the default .ck-tooltip class colors. --------------------------------------- */

	    --ck-color-tooltip-background: hsl(252, 7%, 14%);
	    --ck-color-tooltip-text: hsl(0, 0%, 93%);

	    /* -- Overrides the default colors used by the ckeditor5-image package. --------------------- */

	    --ck-color-image-caption-background: hsl(0, 0%, 97%);
	    --ck-color-image-caption-text: hsl(0, 0%, 20%);

	    /* -- Overrides the default colors used by the ckeditor5-widget package. -------------------- */

	    --ck-color-widget-blurred-border: hsl(0, 0%, 87%);
	    --ck-color-widget-hover-border: hsl(43, 100%, 68%);
	    --ck-color-widget-editable-focus-background: var(--ck-custom-white);

	    /* -- Overrides the default colors used by the ckeditor5-link package. ---------------------- */

	    --ck-color-link-default: hsl(190, 100%, 75%);
	}

	.ck-source-editing-area textarea {
    background: #011022 !important;
    color: #6fcbcb !important;
	}
  .ck.ck-balloon-panel {
    position: relative  !important;
  }
  .ck.ck-dropdown .ck-dropdown__panel.ck-dropdown__panel_nw, .ck.ck-dropdown .ck-dropdown__panel.ck-dropdown__panel_sw
  {
    max-width: 500px;
  }
  .ck.ck-balloon-panel.ck-balloon-panel_visible
  {
    top: 0px;
    left: 0px;
    right: 0px;
    position: relative;
  }
  .ck.ck-dropdown .ck-dropdown__panel.ck-dropdown__panel_ne, .ck.ck-dropdown .ck-dropdown__panel.ck-dropdown__panel_se
  {
    right:0px;
  }
  .ck-source-editing-area textarea {
    left:0 !important;
  }
  .ck-toopltip{
    display:none !important;
  }
  .ck-dropdown__panel.ck-dropdown__panel_se
  {
    left: unset !important;
  }
  .ck.ck-button .ck.ck-tooltip {
	display: none !important;
}

.ck.ck-balloon-panel.ck-balloon-panel_visible
{
  display:none;
}
.ck.ck-dropdown .ck-dropdown__panel.ck-dropdown__panel-visible {
    display: inline-table  !important;
}

.ck.ck-editor__main
{
    overflow: auto  !important;
      background: #1c353f !important;
}
.ck-editor__editable
{
    overflow: visible  !important;
    height: unset !important;
}
textarea
{
  cursor: auto !important;
}

</style>

<div class="editor" style="height:100%; width:100%"></div>`;




class CEditor extends CWindow {
  constructor(positionX, positionY, width, height, data, dataType, filePath, thread) {
    //if external thread is provided, say by File Manager, we let the super-class process it
    //shoud the super class claim it to be usable, we SHOULD NOT attempt to spawn a new one, and re-use the provided one instead.
    super(positionX, positionY, width, height, editorBodyHTML, "Editor", CEditor.getIcon(), false, data, dataType, filePath, thread);

    let fileName = 'Untitled';

    if (!this.mTools.isNull(filePath)) {
      fileName = gTools.parsePath(filePath).fileName;
      if (this.mTools.isString(fileName) && fileName.length) {

      }
    }

    this.setTitle(fileName + ' - Editor');

    this.mInitialData = data;
    this.setExtData = data;
    this.mMetaGenerator = new CVMMetaGenerator();
    this.setExtDataType = dataType;
    this.mMetaParser = new CVMMetaParser();
    this.disableVerticalScroll();
    this.disableHorizontalScroll();
    this.mLastHeightRearangedAt = 0;
    this.mFieldValidator = new CFieldValidator();
    this.mLastWidthRearangedAt = 0;
    this.mDomainID = "";
    this.mERGBig = 0;
    this.mERGLimit = 0;
    this.mRecipientID = "";
    this.mControllerThreadInterval = 20000;
    this.mBalance = '0';
    this.mErrorMsg = "";
    //register for network events
    CVMContext.getInstance().addVMMetaDataListener(this.newVMMetaDataCallback.bind(this), this.mID);
    CVMContext.getInstance().addNewDFSMsgListener(this.newDFSMsgCallback.bind(this), this.mID);
    CVMContext.getInstance().addNewGridScriptResultListener(this.newGridScriptResultCallback.bind(this), this.mID);
    this.loadLocalData();

    this.mParentThread = null;

    //Decentralized Threads - BEGIN

    //1) Editor always spawns a thread of its own, that is not to mess up state and/or scope of the parent thread
    //BUT
    //2) it also stores a reference to parent thread and updates the parent thread solely with data-writes ON EXIT (to prevent multiple entries of write instructions).

    if (this.getThreadID && this.getThreadID.byteLength) { //if not empty, then an external ⋮⋮⋮ Thread must had been injected  (inherited) into the App.

      this.mParentThread = this.mVMContext.getThreadByID(this.getThreadID);

    }

    this.setThreadID = 'editor_data_sync_' + this.getProcessID; //spawn a seperate 'data-commit' Thread.
    //Decentralized Threads - END

  }
  initialize() {
    this.mControler = setInterval(this.mControlerThreadF.bind(this), this.mControllerThreadInterval);
    //CVMContext.getInstance().addUserLogonListener(this.userLoggedInCallback.bind(this));

    // Check if launched via GLink
    if (this.wasLaunchedViaGLink) {
      console.log('[Editor] Launched via GLink, processing...');
      this.processGLink(this.getGLinkData);
    }
  }

  static getDefaultCategory() {
    return 'productivity';
  }
  mControlerThreadF() {
    if (this.mControlerExecuting)
      return false;

    this.mControlerExecuting = true; //mutex protection

    //operational logic - BEGIN
    this.updateData();
    //operational logic - END

    this.mControlerExecuting = false;

  }

  contentChanged() {

  }

  //Updates current edits with the decentralized State-Machine.
  //Both Direct Decentralized Threads' and Decentralized File-Sytem APIs are being employed.
  //Calls to these APIs target the very same Decentralized Thread, spawned for this very instance of Editor UI dApp.
  //Note: the call makes sure the target thread is in a proper state and at the lowest level results in an apropriat CAT
  //instruction being formulated.
  //Thus, it can be then commited, ex through the Magic Button, along with other threads.
  updateData(finalUpdate = false) {
    //Operational Logic - BEGIN
    let data = this.mEditor.getData(); //fetch data
    let ids = this.mTools.bytesToString(this.mInitialData);

    if (this.mTools.compareByteVectors(data, this.mInitialData)) {

      this.mVMContext.logAppEvent("No changes to the file were made.", this);
      //user did not introduce changes
      return false;
    }

    this.mVMContext.logAppEvent("Changes to file were detected. Issuing an update to ⋮⋮⋮ File System. ", this);

    let thread = this.getThreadByID(this.getThreadID);

    if (thread) {
      thread.setHasDataCommitPending = true;
    }

    if (data && this.mFilePath) //if data present and target file-name known
    {
      //Direct Decentralized Threads' API - BEGIN
      let cmd = "at bt"; //clear #GridScript instructions formulated so far
      //and reset code-formulation.
      //addRAWGridScriptCmd(cmd, mode, reqID = 0, windowID = 0, processID = 0, vmID = new ArrayBuffer())
      this.mMetaGenerator.addRAWGridScriptCmd(cmd, eVMMetaCodeExecutionMode.RAW, this.mID, this.getWinID, this.getProcessID, this.getThreadID);

      //let cmd2 = "cat '"+gTools.encodeBase64(data)+"' > "+ this.mFilePath;
      //  this.mMetaGenerator.addRAWGridScriptCmd(cmd2, eVMMetaCodeExecutionMode.RAW, this.mID,this.getWinID,this.getProcessID, this.getThreadID);
      if (!CVMContext.getInstance().processVMMetaData(this.mMetaGenerator, this)) //NOT processVMMetaDataKF which is a Kernel-Mode function
        return false;
      //Direct Decentralized Threads' API - END

      //Decentralized File-Sytem API - BEGIN

      //we could have used
      this.addDFSRequestID(CVMContext.getInstance().getFileSystem.updateFile(this.mFilePath, data, false, this.getThreadID).getReqID);

      if (finalUpdate && this.mParentThread) {
        //the app is about to exit this update the parent thread as well.
        this.addDFSRequestID(CVMContext.getInstance().getFileSystem.updateFile(this.mFilePath, data, false, this.mParentThread.getID).getReqID);
      }

      cmd = "rt"; //mark the thread as Ready/Committable
      this.mMetaGenerator.addRAWGridScriptCmd(cmd, eVMMetaCodeExecutionMode.RAW, this.mID, this.getWinID, this.getProcessID, this.getThreadID);

      if (!CVMContext.getInstance().processVMMetaData(this.mMetaGenerator, this)) //NOT processVMMetaDataKF which is a Kernel-Mode function
        return false;
      //Decentralized File-Sytem API - BEGIN
    }
    return true;
    //Operational Logic - END
  }

  static getPackageID() {
    return "org.gridnetproject.UIdApps.editor";
  }
  static getFileHandlers() {
    return [new CContentHandler(null, eDataType.bytes, 'org.gridnetproject.UIdApps.editor'), new CContentHandler(null, eDataType.unsignedInteger, 'org.gridnetproject.UIdApps.editor'),
    new CContentHandler(null, eDataType.BigInt, 'org.gridnetproject.UIdApps.editor'),
    new CContentHandler(null, eDataType.BigSInt, 'org.gridnetproject.UIdApps.editor')];
  }
  static getIcon() {
    return `data:image/png;base64,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`;
  }
  stopResize(handle) {
    super.stopResize(handle);
  //  $(this.mDiv).find('#toolbar-container')[0].style.mWidth = this.getWidth;
    let m = $(this.getBody).find('.editor');

    /*
    .ck.ck-dropdown .ck-dropdown__panel.ck-dropdown__panel_nw, .ck.ck-dropdown .ck-dropdown__panel.ck-dropdown__panel_sw
    {
      max-width: 500px;
    }
    */
    $('<style type="text/css" scoped> .ck.ck-dropdown .ck-dropdown__panel.ck-dropdown__panel_nw, .ck.ck-dropdown .ck-dropdown__panel.ck-dropdown__panel_sw {max-width:' + (parseInt(this.getHeight, 10) - 80) + 'px' + '; max-height:' + (parseInt(this.getHeight, 10) - 80) + 'px' + '; !important;}</style>').insertBefore(m);
  //  $(this.mDiv).find('#editorContainer')[0].style.height = ((this.getHeight - 60) + 'px');
  }

  finishResize(isFallbackEvent) { //Overloaded window-resize Event
    super.finishResize(isFallbackEvent);
  //  $(this.mDiv).find('#toolbar-container')[0].style.mWidth = this.getWidth;
    let m = $(this.getBody).find('.editor');
        $('<style type="text/css" scoped> .ck.ck-dropdown .ck-dropdown__panel.ck-dropdown__panel_nw, .ck.ck-dropdown .ck-dropdown__panel.ck-dropdown__panel_sw {max-width:' + (parseInt(this.getHeight, 10) - 80) + 'px' + '; max-height:' + (parseInt(this.getHeight, 10) - 80) + 'px' + '; !important;}</style>').insertBefore(m);
    //$('<style type="text/css" scoped> .ck-editor__editable_inline {min-height:' + (parseInt(this.getHeight, 10) - 80) + 'px' + '; max-height:' + (parseInt(this.getHeight, 10) - 80) + 'px' + '; !important;}</style>').insertBefore(m);
  //  $(this.mDiv).find('#editorContainer')[0].style.height = ((this.getHeight - 60) + 'px');
  }

  initCustomSysSettings() {
    if (this.mCustomInitied)
      return;

    this.setIsCurtainEnabled = false;
  }

  closeWindow() {
    clearInterval(this.mControler); //kill the main processing loop.

    this.updateData(true); //the app is about to quit; update both the private 'commit-thread' and the parent-thread with data-writes only.
    //that's yet to be processes by code-optimizer at full-node.
    super.closeWindow();
  }

  open() {
    setTimeout(this.initCustomSysSettings.bind(this), 3000);
    //Overloaded Window-Opening Event
    this.mContentReady = false;
    this.initialize();
    let contentContainer = $(this.getBody).find(".editor")[0];

    let extData = this.getExtData;
    let extDataType = this.getExtDataType;
    let extStringData = '';

    if (extDataType && extData) {
      switch (extDataType) {
        case eDataType.bytes:
          extStringData = gTools.bytesToString(extData);
          break;
        case eDataType.unsignedInteger:
          extStringData = gTools.arrayBufferToUNumber(extData).toString();
          break;
        case eDataType.BigInt:
          extStringData =   gTools.numberToString(gTools.arrayBufferToBigInt(extData));
          break;
        case eDataType.data:
          extStringData =  gTools.numberToString(gTools.arrayBufferToBigInt(extData));
          break;
        default:

      }
    }

  /*  window.createdEditor = this;
    DecoupledEditor
      .create(contentContainer)
      .then(editor => {
        window.createdEditor.mEditor = editor;
        const toolbarContainer = this.getBody.querySelector('#toolbar-container');
        editor.setData(extStringData);
        toolbarContainer.appendChild(editor.ui.view.toolbar.element);
      })
      .catch(error => {
        console.error(error);
      });
      */


      //initialized the instance specific watchdog service
      console.log('Initializing watchdog service for Editor..');
      this.mWatchdog = new CKSource.EditorWatchdog();
      window.createdEditor = this;
      this.mWatchdog.setCreator( ( element, config) => {
        return CKSource.Editor
          .create( element, config )
          .then( editor => {
            const body = editor.ui.view.body._bodyCollectionContainer;
           body.remove();
          //  config.inst.mDiv.appendChild(body);
            //config.inst.mEditor = editor;
            window.createdEditor.getBody.prepend(body);
            window.createdEditor.mEditor = editor;
            editor.setData(extStringData);
            return editor;
          } )
      } );

      this.mWatchdog.setDestructor( editor => {
        return editor.destroy();
      } );

      this.mWatchdog.on( 'error', handleError );

      console.log('Bringing the Editor operational..');
      this.mWatchdog.create( contentContainer, {
          licenseKey: '',
          inst:this
        } ).catch( handleError );

      function handleError( error ) {
        console.error( 'Oops, something went wrong initializing the Editor!' );

      }


    super.open();
  //  $(this.mDiv).find('#toolbar-container')[0].style.mWidth = this.getWidth;
    setTimeout(this.revalidateWindow.bind(this), 1000);

  }


  loadLocalData() {
    return false;
    let addr = gLocalDataStore.loadValue('');
    if (addr != null && gTools.isDomainIDValid(addr)) {
      this.getControl('').value = addr;
      this.mDomainID = addr;
    }
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

  // ============ GLink Support - BEGIN ============
  // Editor Action IDs:
  // 1 = Open file for editing

  /**
   * @brief Process GLink data when launched via deep link
   * @param {Object} glinkData - Parsed GLink data { view, action, data }
   * @returns {boolean} True if processed successfully
   */
  processGLink(glinkData) {
    const glinkHandler = CGLinkHandler.getInstance();

    if (!glinkData) {
      console.warn('[Editor] No GLink data to process');
      glinkHandler.rejectGLink('No GLink data provided');
      return false;
    }

    console.log('[Editor] Processing GLink:', glinkData);

    // Accept the GLink - acknowledge receipt
    glinkHandler.acceptGLink('Editor processing...');

    const { action, data } = glinkData;

    // Handle specific actions (numeric action IDs)
    if (action !== undefined && action !== null) {
      switch (action) {
        case 1: // Open file for editing
          if (data?.filePath) {
            console.log('[Editor] GLink Action 1: Opening file:', data.filePath);
            this.openFileFromGLink(data.filePath, glinkHandler);
            return true;
          }
          break;

        default:
          console.warn('[Editor] Unknown GLink action ID:', action);
          glinkHandler.rejectGLink(`Unknown action ID: ${action}`);
          return false;
      }
    }

    // Fallback: Handle file path from GLink (legacy support)
    if (data?.filePath) {
      console.log('[Editor] GLink: Opening file:', data.filePath);
      this.openFileFromGLink(data.filePath, glinkHandler);
      return true;
    }

    console.log('[Editor] GLink: No filePath specified');
    glinkHandler.rejectGLink('No file path specified');
    return false;
  }

  /**
   * @brief Open a file from GLink with connection retry logic
   * @param {string} filePath - Path to file to open
   * @param {CGLinkHandler} [glinkHandler] - Optional GLink handler for confirmation
   */
  openFileFromGLink(filePath, glinkHandler = null) {
    // Store the file path for later operations
    this.mFilePath = filePath;
    const fileName = gTools.parsePath(filePath).fileName || 'Untitled';
    this.setTitle(fileName + ' - Editor');

    let retryCount = 0;
    const maxRetries = 20; // 10 seconds max

    // Request file from DFS when connected
    const fetchFile = () => {
      const ctx = CVMContext.getInstance();
      if (ctx.getConnectionState === eConnectionState.connected) {
        console.log('[Editor] Fetching file from DFS:', filePath);
        this.addNetworkRequestID(ctx.getFileSystem.doGetFile(filePath, false, this.getThreadID).getReqID);
        // Confirm GLink processing after file request initiated
        if (glinkHandler) {
          glinkHandler.confirmGLinkProcessed('Editor: File loading...');
        }
      } else {
        retryCount++;
        if (retryCount < maxRetries) {
          // Retry after a short delay if not connected
          setTimeout(fetchFile, 500);
        } else {
          // Timeout - reject GLink
          if (glinkHandler) {
            glinkHandler.rejectGLink('Connection timeout');
          }
        }
      }
    };

    setTimeout(fetchFile, 100);
  }

  // --- Editor GLink Action IDs ---
  // 1 = Open file for editing

  /**
   * @brief Create a GLink for a specific file path
   * @param {string} filePath - Path to file
   * @param {number} [action=1] - Action ID (default: 1 = Open file)
   * @returns {string} GLink URL
   * @static
   */
  static createGLink(filePath, action = 1) {
    return CGLink.create(
      CEditor.getPackageID(),
      null,     // view
      action,   // action: 1 = Open file
      { filePath: filePath }
    );
  }

  /**
   * @brief Create a GLink for the current file
   * @returns {string|null} GLink URL for current file, or null if no file
   */
  createCurrentFileGLink() {
    if (!this.mFilePath) {
      return null;
    }
    return CEditor.createGLink(this.mFilePath);
  }

  /**
   * @brief Copy current file GLink to clipboard
   * @returns {Promise<boolean>} True if successful
   */
  async copyCurrentFileGLinkToClipboard() {
    const glink = this.createCurrentFileGLink();

    if (!glink) {
      this.showMessageBox('No File', 'Save the file first to create a shareable link.');
      return false;
    }

    const success = await CGLink.copyToClipboard(glink);

    if (success) {
      this.showMessageBox('GLink Copied! 📋', 'File link copied to clipboard.\nShare it to let others open this file directly.');
    } else {
      this.showMessageBox('Copy Failed', 'Unable to copy GLink to clipboard.');
    }

    return success;
  }

  // ============ GLink Support - END ============
}

export default CEditor;
