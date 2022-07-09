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
class CUIAppTemplate extends CWindow {

//position.positionX, position.positionY, width, height, data, dataType, filePath, thread
  constructor(positionX, positionY, width, height, data, dataType, filePath, thread) {
    super(positionX, positionY, width, height, apptemplateBody, "UIAppTemplate", CUIAppTemplate.getIcon(), true); //use Shadow-DOM by default

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
    this.controllerThreadInterval = 1000;
    this.mControlerExecuting = false;
    this.mControler = 0;

  }

  static getFileHandlers() {
    return [new CContentHandler(null, eDataType.bytes, 'org.gridnetproject.UIdApps.helloWorld'), new CContentHandler(null, eDataType.unsignedInteger, 'org.gridnetproject.UIdApps.helloWorld')];
  }
  /*
  static getFileHandlers() {
    return [new CContentHandler('jpg', eDataType.bytes, 'org.gridnetproject.UIdApps.imageViewer'),
      new CContentHandler('png', eDataType.bytes, 'org.gridnetproject.UIdApps.imageViewer'),
      new CContentHandler('webp', eDataType.bytes, 'org.gridnetproject.UIdApps.imageViewer')
    ];
  }
  */

  static getDefaultCategory() {
    return 'dApps'; //also 'explore' and 'productivity'
  }
  static getIcon() {
    return ``; //data:image/png;base64
  }

  static getPackageID() {
    return "org.gridnetproject.UIdApps.helloWorld";
  }

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
    this.mControler = setInterval(this.mControlerThreadF.bind(this), this.controllerThreadInterval);


    //Settings Support - BEGIN
    if (this.loadSettings()) {
      console.log('AppTemplate settings activated!');
    } else {
      CUIAppTemplate.setSettings(this.getDefaultSettings());
      console.log('Failed to activate provided settings for an AppTemplate. Assuming defaults!');
    }

    //Settings Support - END
  }


  mControlerThreadF() {
    if (this.mControlerExecuting)
      return false;

    this.mControlerExecuting = true; //mutex protection

    //operational logic - BEGIN

    //operational logic - END

    this.mControlerExecuting = false;

  }

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

  onScroll(event) {
    super.onScroll(event);
  }

  userResponseCallback(e) {
    console.log('User answered:' + e.answer)
    if (e.answer) {
      //use the provided value
    }
  }

  open() { //Overloaded Window-Opening Event
    super.open();
    this.initialize();
    this.mContentReady = false;
    this.askString('⋮⋮⋮ Sample Question 🌎 ', 'How are you today?', this.userResponseCallback, true);

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
CUIAppTemplate.sCurrentSettings = new CAppSettings(CUIAppTemplate.getPackageID);

export default CUIAppTemplate;
