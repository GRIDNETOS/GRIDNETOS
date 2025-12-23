//Wizards wish you a wonderful Journey!
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

//Imports - BEGIN
//Here, import any APIs and System Objects your application might need.
import {
  CWindow
} from "/lib/window.js"
import {
  CVMMetaSection,
  CVMMetaEntry,
  CVMMetaGenerator,
  CVMMetaParser
} from '/lib/MetaData.js'
import {
  CNetMsg
} from '/lib/NetMsg.js'
import {
  CTools,
  CDataConcatenator
} from '/lib/tools.js'
import {
  CAppSettings,
  CSettingsManager
} from "/lib/SettingsManager.js"

import {
  CContentHandler
} from "/lib/AppSelector.js"
//Imports - END

//Window Body - BEGIN
var apptemplateBody = `<link rel="stylesheet" href="/css/windowDefault.css" />
<link rel="stylesheet" href="/CSS/jquery.contextMenu.min.css" />`;
//In an unpacked version, from external file:
//IMPORTANT: remember to include:

//<! Standard ⋮⋮⋮ UI dApp Styles BEGIN ––>
//<link rel="stylesheet" href="/css/windowDefault.css" />
//<! Standard ⋮⋮⋮UI dApp Styles END ––>

//within body.html for standard styles.
/*$.get("https://localhost/dApps/whiteboard/src/body.html", function(data)
 {
   apptemplateBody = data;
  //  alert(whiteboardBody);
 },'html'); */

//Window Body - END

//Constructor - BEGIN
class CUIAppTemplate extends CWindow {

  //position.positionX, position.positionY, width, height, data, dataType, filePath, thread
  constructor(positionX, positionY, width, height, data, dataType, filePath, thread) {
    super(positionX, positionY, width, height, apptemplateBody, "UIAppTemplate", CUIAppTemplate.getIcon(), true); //use Shadow-DOM by default

    // ============================================================================
    // GRIDNET OS CURTAIN WHITELIST (Optional but recommended for interactive UIs)
    // ============================================================================
    // Prevents loading curtain from showing during normal user interactions with
    // sliders, spinners, and live-updating displays. Call this AFTER super() and
    // BEFORE UI initialization.
    //
    // Whitelist elements that cause frequent DOM mutations:
    // - Interactive controls (sliders, spinner buttons, draggable elements)
    // - Live displays (counters, real-time data, calculated values)
    // - Animation containers
    //
    // The whitelist is RECURSIVE - whitelisting a parent automatically covers
    // all child elements up to maxDepth (default: 5) levels.
    //
    // See: lib/window.js - addCurtainWhitelist() for full documentation
    // ============================================================================
    this.addCurtainWhitelist({
      classNames: [
        // Example: Interactive slider elements
        'slider-handle',
        'slider-track',
        'slider-fill',
        // Example: Spinner buttons for number inputs
        'spinner-button',
        'number-input-wrapper',
        // Example: Live-updating displays
        'live-counter',
        'real-time-value',
        'auto-update'
      ],
      ids: [
        // Example: Specific elements by ID
        'main-balance-display',
        'calculated-total',
        'progress-indicator'
      ],
      maxDepth: 5 // Check up to 5 parent levels
    });

    this.setThreadID = null; //by default there's no need for a dedicated Decentralized Processing Thread.
    //Developers should prefere usage of public 'data' thread for retreval of information instead.
    this.mTools = CTools.getInstance();
    this.mLastHeightRearangedAt = 0;
    this.mLastWidthRearangedAt = 0;
    this.mErrorMsg = "";
    this.mMetaParser = new CVMMetaParser();
    //register for network events
    CVMContext.getInstance().addVMMetaDataListener(this.newVMMetaDataCallback.bind(this), this.mID);
    CVMContext.getInstance().addNewDFSMsgListener(this.newDFSMsgCallback.bind(this), this.mID);
    CVMContext.getInstance().addNewGridScriptResultListener(this.newGridScriptResultCallback.bind(this), this.mID);
    this.loadLocalData();
    this.mControllerThreadInterval = 1000;
    this.mControlerExecuting = false;
    this.mControler = 0;

  }
  // ============================================================================
  // CRITICAL UI dApp DEVELOPMENT PATTERNS
  // ============================================================================
  //
  // 1. ACCESSING SHADOW DOM ELEMENTS
  // ────────────────────────────────────────────────────────────────────────
  // Use getControl(id) to access elements within the Shadow DOM.
  // DO NOT use document.getElementById() - it won't find shadow DOM elements!
  //
  // CORRECT:
  //   const button = this.getControl('my-button');
  //   const input = this.getControl('user-input');
  //
  // INCORRECT:
  //   const button = document.getElementById('my-button'); // Won't work!
  //
  // ────────────────────────────────────────────────────────────────────────
  //
  // 2. QUERY SELECTORS IN SHADOW DOM
  // ────────────────────────────────────────────────────────────────────────
  // Use this.getBody for shadow DOM root:
  //
  //   const elements = this.getBody.querySelectorAll('.my-class');
  //   const firstDiv = this.getBody.querySelector('div.container');
  //
  // ────────────────────────────────────────────────────────────────────────
  //
  // 3. EVENT LISTENERS
  // ────────────────────────────────────────────────────────────────────────
  // Always bind 'this' context when setting up event listeners:
  //
  //   button.addEventListener('click', this.handleClick.bind(this));
  //   // OR use arrow functions:
  //   button.addEventListener('click', (e) => this.handleClick(e));
  //
  // ────────────────────────────────────────────────────────────────────────
  //
  // 4. CURTAIN CONTROL
  // ────────────────────────────────────────────────────────────────────────
  // Pause curtain during operations that cause many DOM changes:
  //
  //   this.pauseCurtainFor(3); // Pause for 3 seconds
  //   // ... perform bulk DOM updates ...
  //
  // Show curtain manually for long operations:
  //
  //   this.showCurtain(true, false, false);
  //   // ... perform operation ...
  //   this.hideCurtain();
  //
  // ────────────────────────────────────────────────────────────────────────
  //
  // 5. NETWORK REQUESTS
  // ────────────────────────────────────────────────────────────────────────
  // Always track network request IDs:
  //
  //   const reqID = CVMContext.getInstance().requestNetworkData(...);
  //   this.registerNetworkRequest(reqID);
  //
  // Then in callbacks, check ownership:
  //
  //   if (!this.hasNetworkRequestID(msg.getReqID)) return;
  //
  // ────────────────────────────────────────────────────────────────────────
  //
  // 6. SETTINGS PERSISTENCE
  // ────────────────────────────────────────────────────────────────────────
  // Load settings in initialize():
  //
  //   if (this.loadSettings()) {
  //     // Settings loaded successfully
  //   } else {
  //     // Use defaults
  //     CMyApp.setSettings(CMyApp.getDefaultSettings());
  //   }
  //
  // Save settings when state changes:
  //
  //   this.saveSettings(); // Persists current state
  //
  // ────────────────────────────────────────────────────────────────────────
  //
  // 7. THREAD MANAGEMENT
  // ────────────────────────────────────────────────────────────────────────
  // Create dedicated thread if needed:
  //
  //   this.mControler = CVMContext.getInstance().createJSThread(
  //     this.mControllerThreadF.bind(this),
  //     this.getProcessID,
  //     1000 // Interval in ms
  //   );
  //
  // Always stop threads in closeWindow():
  //
  //   if (this.mControler > 0) {
  //     CVMContext.getInstance().stopJSThread(this.mControler);
  //   }
  //
  // ────────────────────────────────────────────────────────────────────────
  //
  // 8. WINDOW GEOMETRY
  // ────────────────────────────────────────────────────────────────────────
  // Get current dimensions:
  //
  //   const width = this.getClientWidth;
  //   const height = this.getClientHeight;
  //
  // React to resize events:
  //
  //   finishResize(isFallbackEvent) {
  //     super.finishResize(isFallbackEvent);
  //     // Update layout based on new size
  //   }
  //
  // ────────────────────────────────────────────────────────────────────────
  //
  // 9. USER DIALOGS
  // ────────────────────────────────────────────────────────────────────────
  // Ask user for input:
  //
  //   this.askString('Title', 'Question?', this.handleResponse.bind(this), true);
  //
  // Show notifications:
  //
  //   this.mTools.logEvent('Message', eLogEntryCategory.dApp, 0, eLogEntryType.notification);
  //
  // ────────────────────────────────────────────────────────────────────────
  //
  // 10. CLEANUP
  // ────────────────────────────────────────────────────────────────────────
  // Always clean up in closeWindow():
  //
  //   closeWindow() {
  //     // Stop threads
  //     if (this.mControler > 0) {
  //       CVMContext.getInstance().stopJSThread(this.mControler);
  //     }
  //     // Clear event listeners
  //     // Save state if needed
  //     super.closeWindow();
  //   }
  //
  // ============================================================================
  //Constructor - END

  //Register File Handlers - BEGIN
  static getFileHandlers() {
    return [new CContentHandler(null, eDataType.bytes, this.getPackageID()),
      new CContentHandler(null, eDataType.unsignedInteger, this.getPackageID())
    ];
  }

  /*
  static getFileHandlers() {
    return [new CContentHandler('jpg', eDataType.bytes, 'org.gridnetproject.UIdApps.imageViewer'),
      new CContentHandler('png', eDataType.bytes, 'org.gridnetproject.UIdApps.imageViewer'),
      new CContentHandler('webp', eDataType.bytes, 'org.gridnetproject.UIdApps.imageViewer')
    ];
  }
  */

  //Register File Handlers - END

  //Package Info - BEGIN
  static getPackageID() {
    return "org.gridnetproject.UIdApps.UIAppTemplate";
  }

  static getDefaultCategory() {
    return 'dApps'; //also 'explore' and 'productivity'
  }
  static getIcon() {
    return ``; //data:image/png;base64
  }
  //Package Info - END

  //Settings Manager Support - BEGIN (Note Initialize() and static member fields at the bottom)
  static getSettings() { //[required]
    return CUIAppTemplate.sCurrentSettings; //Important: static field [required]
    //take look at the bottom of this file, right after CBrowser's body.
  }

  static setSettings(sets) {
    if (!(sets instanceof CAppSettings))
      return false;

    CUIAppTemplate.sCurrentSettings = sets;
    return true;
  }

  loadSettings() {
    CVMContext.getInstance().getSettingsManager.loadSettings(CUIAppTemplate.getPackageID());
    return this.activateSetings();
  }

  activateSetings() {

    let sets = CUIAppTemplate.getSettings();
    let settings = null;
    let pack = null;
    let elem = null;

    if (sets == null || typeof sets.getVersion === 'undefined')
      return false;

    //Validate - BEGIN
    if (sets.getVersion == 1) {
      settings = sets.getData;
      if (settings == null || typeof settings === 'undefined') {
        sets = CUIAppTemplate.getDefaultSettings();
        settings = sets.getData;
      }
    } else return false;

    //Validate - END

    //Apply Settings - BEGIN

    //now take use of the app-specific settings object from above
    // ex:  this.mGrid.load(settings);

    //Apply Settings - END

    return true;
  }

  saveSettings() {
    //Two scenarios:
    //A)- either prepare the default app-specific container
    /*let settingsData = CUIAppTemplate.getDefaultSettings().getData; //getData should contain a default app-specific data-container.

    if(!settingsData)
    return;
    //Important: fill-in the defaults' container with current state's data
    // store settings ex: settingsData.layout = this.mGrid.save(false, true);

    let sets = new CAppSettings(CUIAppTemplate.getPackageID(), settingsData);

    CUIAppTemplate.setSettings = sets;
*/
    //OR - simply get a reference to current settings (set through setSettings() or with defaults)
    let sets = this.getSettings;

    CVMContext.getInstance().getSettingsManager.saveAppSettings(sets);
  }


  static getDefaultSettings() {

    let prodDef = [];

    let exploreDefs = [];

    let count = 0;
    [...prodDef, ...exploreDefs].forEach(d => d.content = String(count++));

    let obj = {
      layout: {
        cellHeight: 120,
        minRow: 2,
        children: [{
            x: 1,
            noResize: true,
            id: 'someID',
            content: 'dApps',
            subGrid: {
              children: [],
              ...CUIAppTemplate.getDefSubOption()
            }
          },
          {
            x: 5,
            id: 'explore',
            noResize: true,
            content: 'Explore',
            subGrid: {
              children: exploreDefs,
              ...CUIAppTemplate.getDefSubOption()
            }
          },
        ]
      },
      version: 1
    }
    return new CAppSettings(CUIAppTemplate.getPackageID(), obj);
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
    this.loadSettings();
    this.mControler =  CVMContext.getInstance().createJSThread(this.mControllerThreadF.bind(this), this.getProcessID, this.mControllerThreadInterval);

    //Settings Support - BEGIN
    if (this.loadSettings()) {
      this.mTools.logEvent('['+this.getPackageID + ']\'s settings activated!',
        eLogEntryCategory.dApp, 0, eLogEntryType.notification);
    } else {
      CUIAppTemplate.setSettings(this.getDefaultSettings());
      this.mTools.logEvent('Failed to activate provided settings for ['+this.getPackageID + ']. Assuming defaults!',
        eLogEntryCategory.dApp, 0, eLogEntryType.failure);
    }
    //Settings Support - END
  }

// Custom Local Thread Logic - BEGIN
  mControllerThreadF() {
    if (this.mControllerExecuting)
      return false;

    this.mControllerExecuting = true; //mutex protection

    //Operational logic - BEGIN

    //Operational logic - END

    this.mControllerExecuting = false;

  }
// Custom Local Thread Logic - END

//Callback Functions - BEGIN

//Window Events - BEGIN
  finishResize(isFallbackEvent) { //Overloaded window-resize Event
    //called on finish of resize-animation ie. maxWindow, minWindow
    super.finishResize(isFallbackEvent);

    //get current client rect with
    //this.getClientHeight
    //this.getClientWidth
  }

  stopResize(handle) { //fired when mouse-Resize ends
    super.stopResize(handle);

  }

  //Fired when window is scrolled.
  onScroll(event) {
    super.onScroll(event);
  }



  //the Window opened.
  open() { //Overloaded Window-Opening Event
    super.open();
    this.initialize();
    this.mContentReady = false;
    this.askString('⋮⋮⋮ Sample Question 🌎 ', 'How are you today?', this.userResponseCallback, true);

    //modify content here

  }

  //Fired just before Windows closes.
  //remember to shut down any additional threads over here.
  closeWindow() {
    if (this.mControler > 0)
      CVMContext.getInstance().stopJSThread(this.mController);//shut-down the thread if active
    super.closeWindow();
  }

  //Window Events - END

  //User provided response to a user-mode data query.
  userResponseCallback(e) {
    console.log('User answered:' + e.answer)
    if (e.answer) {
      //use the provided value
    }
  }
  loadLocalData() {
    return false;

  }

  //Fired when GridScript code processing issued by this very app finished.
  newGridScriptResultCallback(result) {
    if (result == null)
      return;

  }


  //Fired by the Decentralized File System API ex. when new file is fetched.
  newDFSMsgCallback(dfsMsg) {

    if (!this.hasNetworkRequestID(dfsMsg.getReqID))//check if data is a result of our own query.
      return; //if not - return

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

//Callback Functions - END
}
CUIAppTemplate.sCurrentSettings = new CAppSettings(CUIAppTemplate.getPackageID);

export default CUIAppTemplate;
