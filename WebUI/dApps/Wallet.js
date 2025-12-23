"use strict"

import {
  CConsensusTask
} from "/lib/VMContext.js"

import {
  CVMMetaSection,
  CVMMetaEntry,
  CVMMetaGenerator,
  CVMMetaParser
} from './../lib/MetaData.js'
import {
  CStateLessChannelsManager,
  CTokenPoolBank,
  CTokenPool,
  CTransmissionToken,
  CStateLessChannel
} from "./../lib/StateLessChannels.js"
import {
  CWindow
} from "/lib/window.js"

var walletBodyHtml = `
  <style>
  .nav-tabs .nav-item.show .nav-link, .nav-tabs .nav-link.active {
    color: #edf2f8;
    background-color: #a407b3;
  border-color: #c666b9 #c300e7 #cd4de5;
  }
  .nav-tabs .nav-link:focus, .nav-tabs .nav-link:hover {
    border-color: transparent;
}
     .buttonC {
    background-color: #4CAF50; /* Green */
    border: none;
    color: white;
    padding: 5px 32px;
    text-align: center;
    text-decoration: none;
    display: inline-block;
    font-size: 16px;
  }
  #sendTPSelectorContainer
  {
    display:none;
  }

  #sendSwarmSelectorContainer
  {margin-top: 0.4rem;
    display:none;
  }
  #tpExtendedInfoHolderOuter
  {

  }
  #tpExtendedInfoHolder{
   overflow-y: auto;
   max-height: 10em;
   margin-bottom: 1.4em;
   }
  .walletSendTabContainer
  {
  position: relative;


  }
  #tokenPoolInfoContainer
  {
    display:none;
  }
  #tokenPoolInfoContainer.shown
  {
    display:block;
  }

  #sendTPSelectorContainer.shown
  {
    display:block;
  }

  #sendSwarmSelectorContainer.shown
  {
    display:block;
  }
  fieldset.groupbox-border {
      border: 1px groove #ddd !important;
      padding: 0 1.4em !important;
      margin: 1.2em 1.2em 1.2em 1.2em !important;
      -webkit-box-shadow: 0px 0px 0px 0px #000;
      box-shadow: 0px 0px 0px 0px #000;

      }

  .walletSendTab
  {

  position:relative;

  width:100%; height:100%;

  }
  .nav-tabs {
      border-bottom: 1px solid #cf01f8d6;
      background-color: #290838;
      /* border-radius: 5em; */
      border-bottom-left-radius: 2.4em;
      border-bottom-right-radius: 2.4em;
      box-shadow: 0px 1px 6px 0px #005bf4;
    }
.nav-tabs .dropdown-menu {
    border-radius: 1em;
    margin-top: -1px;
    border-top-left-radius: 0;
    border-top-right-radius: 0;
}
      </style>

      <!-- Tabs Above -->
      <div class='tabs-x tabs-above tabs-krajee' style='height: 100%;'>
          <ul id="myTab-tabs-above" class="nav nav-tabs" role="tablist" style="">
            <li class="nav-item dropdown">
              <a class="nav-link dropdown-toggle" data-toggle="dropdown" href="#" role="button" aria-haspopup="true" aria-expanded="false">Menu</a>
              <div class="dropdown-menu">
                <a class="dropdown-item" href="#">Import Wallet</a>
                <a class="dropdown-item" href="#">Synchronize</a>
                <div class="dropdown-divider"></div>
                <a class="dropdown-item" style="    color: #ffeb00;" onClick=" event.stopPropagation(); gWindowManager.getWindowByID($(this).closest('.idContainer').find('#windowIDField').first().val()).closeWindow();return false; ">Exit</a>
              </div>
            </li>

              <li class="nav-item"><a class="nav-link active" id="home-tab-tabs-above" href="#home-tabs-above" role="tab" data-toggle="tab" aria-controls="home" aria-expanded="true"><i
                      class="fa fa-home"></i> Home</a></li>
                      <li class="nav-item"><a class="nav-link" id="send-tab-tabs-above" href="#send-tabs-above" role="tab" data-toggle="tab" aria-controls="home" aria-expanded="false"><i
                              class="fa fa-home"></i> Send</a></li>
              <li class="nav-item"><a class="nav-link" id="profile-tab-tabs-above" href="#profile-tabs-above" role="tab-kv" data-toggle="tab" aria-controls="profile"><i class="fa fa-cog"></i> Settings</a></li>

               <li class="nav-item"><a class="nav-link" id="tokenpools-tab-tabs-above" href="#tokenpools-tabs-above" role="tab-kv" data-toggle="tab" aria-controls="tokenpools"><i class="fa fa-coins"></i> Token Pools</a></li>

        <li class="nav-item"><a class="nav-link" id="pendingtokens-tab-tabs-above" href="#pendingtokens-tabs-above" role="tab-kv" data-toggle="tab" aria-controls="pendingtokens"><i class="fa fa-hand-receiving"></i> Pending Tokens</a></li>



          </ul>
          <div id="myTabContent-tabs-above" class="tab-content" style="padding:0px; height: calc(100% - 6em);padding-top: 1em;">
            <div class="tab-pane fade show active" id="home-tabs-above" role="tabpanel" aria-labelledby="home-tab-tabs-above"><p> <div id="walletHomeTab" class="walletSendTab">

            <div class="walletHomeTabContainer">
            <div id='valueContainer' style='font-size:xxx-large;overflow: hidden;
    white-space: nowrap;'>
    <span style='font-size:xxx-large;     color: #07a9a9;'> </span>
      <span  id='currentBalanceTxt' onclick="event.stopPropagation(); gWindowManager.getWindowByID($(this).closest('.idContainer').find('#windowIDField').first().val()).retrieveBalance(true);"  id="balanceTxt" style='cursor: pointer; font-size:xxx-large;     color: aqua;
    text-shadow: 0 0 4px #8fffff;'> UNKNOWN</span>
            </div>
            <form>
            <label for="fname"><i class="fa fa-address-card "></i>   Address:</label>
        <input type="text" autocomplete="off" class="txtRO noselect addressType" id="addressTxt" value="unknown"readonly>
        <button type="button" onclick="gTools.copyControlValueToClipboard('#addressTxt');" id="copyAddrBtn" class="buttonStd"><i class="fa fa-copy"></i>
              </button><br>

          <label for="pendingTotalTTValueTxt"><i class="fa fa-money "></i>Pending Tokens Value:</label>
        <input type="text" autocomplete="off" class="txtRO" style="color:#0cff00" id="pendingTotalTTValueTxt" value="0" onclick="gTools.copyControlValueToClipboard('#balanceTxt');"  readonly><br>
</form>
            </div>

            <div id="transfersHistory" style="width:100%;padding-top:1em; ">
            <p style="background-color: #2b1e1e;
          color: #4de43d;">
            <b>Transfers History</b>
            </p>
          </div>
      </div>
            </p>
            </div>
              <div class="tab-pane fade  " id="send-tabs-above" role="tabpanel" aria-labelledby="send-tab-tabs-above"><p>
              <div id="walletSendTab" class="walletSendTab">
            <h3 style="margin-top: 0.5rem;
    color: #00f3ff;
    text-shadow: 0 0 5px #0cb6b9;">Transfer Details</h3>

            <div class="walletSendTabContainer">

            <label for="fname"><i class="fa fa-address-card "></i>  Recipient:</label>
        <input type="text"  autocomplete="off" class ="txtInput alphaNumType" style="width:23em;" id="recipientIDTxt" ><br/>
        <div style='    right: 0;
        left: 0;
        margin-right: auto;
        margin-left: auto;
        width: 33.33%; text-align: right;margin-top: 0.5rem;'>
   <div  style="white-space:nowrap; text-align: right;" >
       <label for="lname"><i class="fa fa-money "></i> Value:</label>
       <input type="text"  autocomplete="off" style="width: 45%;min-width: 10rem;" class ="txtInput numType"id="valueTxt" >
       <div class="GNCPick" style="display: inline-block; vertical-align: middle; margin-bottom: 7px; ">
         <input type="checkbox" checked value="None" id="transferUnit" name="check" onclick="let window = gWindowManager.getWindowByID($($(this).closest('.idContainer')).find('#windowIDField').first().val()); window.doGNC(this.checked)" />
         <label for="transferUnit"></label>
       </div>
       </div>
         <div style="white-space:nowrap; text-align: right;">
         <label for="ergBid"><i class="fa fa-line-chart "></i> ERG-Bid:</label>
       <input type="text"  autocomplete="off" style="width: 45%; min-width: 10rem;"class ="txtInput numType"id="ergBIDTxt"  onclick="$('#ergBIDTxt')[0].value=''" value="auto">
</div>
<div  style="white-space:nowrap;">
         <label for="ergLimit"><i class="fa fa-adjust "></i> ERG-Limit:</label>
       <input type="text"  autocomplete="off"  style="width: 45%;min-width: 10rem;" class ="txtInput numType"  id="ergLimitTxt" onclick="$('#ergLimitTxt')[0].value=''" value="auto">
</div>
</div>
        <span style="    padding-bottom: 0.5rem;
    display: inline-block;">
        <span class="squaredThree" style="    margin-right: 0.7rem;">
              <input type="checkbox" value="None" id="useTPChk" name="check" onclick="let window = gWindowManager.getWindowByID($($(this).closest('.idContainer')).find('#windowIDField').first().val()); window.useTP(this.checked);" >
              <label for="useTPChk"></label>

            </span>
            </span><span style='color: #b06cb8;'> Off-The Chain</span>
         <div id='sendTPSelectorContainer' class='animate__animated' style="">

         <span class="squaredThree" style="    margin-right: 0.7rem;">
               <input type="checkbox" value="None" id="useSwarmChk" name="check" onclick="let window = gWindowManager.getWindowByID($($(this).closest('.idContainer')).find('#windowIDField').first().val()); window.doUseSwarm(this.checked);" >
               <label for="useSwarmChk"></label>

             </span>
             </span>
             <span style='color: #b06cb8;'> Deliver through Swarm</span>
             <div id='sendSwarmSelectorContainer' class='animate__animated' style="">

             <label for="fSwarmsAvailableDrop" style=""> Swarm:</label>


             <select id="fSwarmsAvailableDrop"  style="" onclick = "let window = gWindowManager.getWindowByID($($(this).closest('.idContainer')).find('#windowIDField').first().val()); window.refreshSwarms();" onchange="let window = gWindowManager.getWindowByID($($(this).closest('.idContainer')).find('#windowIDField').first().val()); window.useSwarm(this.value)" ng-model="">

             </select>
             </div>
           <div id='availableTokenPoolsContainerTPTab' style='margin-top: 0.4rem;'>
           <label for="fPoolsAvailable" style=""> Token-Pool:</label>


           <select id="availableTokenPoolsDrop1"  style=""
           onclick = "let window = gWindowManager.getWindowByID($($(this).closest('.idContainer')).find('#windowIDField').first().val()); window.refreshChannels();"
           oninput="let window = gWindowManager.getWindowByID($($(this).closest('.idContainer')).find('#windowIDField').first().val()); window.useChannel(this.value)" ng-model="">
           </select>


      </div>
         </div>
    <br/><br/>
        <button type="button" id="sendBtn" class="buttonStd" onclick="let window = gWindowManager.getWindowByID($($(this).closest('.idContainer')).find('#windowIDField').first().val()); if(!window.getCheckSAOk(2)){ window.animateCSS(this.id,'shakeX'); return}; window.issueTransfer(false);"><i class="fa fa-hourglass"></i> Enqueue</button>
        <button type="button" id="sendBtnNow" class="buttonStdGreen"  onclick="let window = gWindowManager.getWindowByID($($(this).closest('.idContainer')).find('#windowIDField').first().val());if(!window.getCheckSAOk(2)){ window.animateCSS(this.id,'shakeX'); return};  window.issueTransfer(true)">Send Now <i class="fa fa-send"></i> </button>
            </div>
      </div>
              </p></div>

                      <div class="tab-pane fade" id="tokenpools-tabs-above" role="tabpanel" aria-labelledby="tokenpools-tabs-above"  style="height: 100%;"><p><div id="walletSettingsTab" class="walletSendTab">

            <div class="tokenPoolsTabContainer " style="height: 100%;">
            <h2 style="margin-top: 0.3em; color: #ff4fff; font-family: Arial Narrow, Charcoal, sans-serif;" > Available Token-Pools <i class="fal fa-coins"></i></h2>
            <div id="outgressGrid" style="height: 45%; width: 100%; overflow-y: auto;   padding-right: 0.5rem;  padding-left: 0.5rem;" class="ag-theme-alpine-dark"></div>

      <div id='tokenPoolInfoContainer' class='animate__animated'>
      <fieldset class="groupbox-border">




          <legend class="groupbox-border">Information</legend>
          <label for="fPoolsAvailable">Total GBUs available:</label>
        <input type="text"  autocomplete="off" id ='fPoolsAvailable' class ="txtInput" value=""  readonly>
       <label for="alreadyspent:">Already spent:</label>
        <input type="text"  autocomplete="off" id ='alreadyspent' style = 'max-width:3rem;'class ="txtInput"  readonly>
         <label for="alreadyspent:">Banks:</label>
        <input type="text"  autocomplete="off" id ='banks' class ="txtInput"  readonly>
        <div id='tpExtendedInfoHolderOuter'>
         <label for="extendedTPInfo">Extended meta-data:</label>
        <div id='tpExtendedInfoHolder'>
    Extended Info
        </div>
        <div>

        </div>
        </div>

      </fieldset>
      </div>
      <fieldset class="groupbox-border">


          <legend class="groupbox-border">New Token Pool</legend>

          <label for="pendingTPTotalValueTxt"> Total Value:</label>
        <input type="text"  autocomplete="off" style="color:#0cff00" id="pendingTPTotalValueTxt" class ='txtInput numType'  onclick="gTools.copyControlValueToClipboard('#balanceTxt');"  ><br>


           <label for="pendingTPBanksCountTxt">Banks Count:</label>
        <input type="text"  autocomplete="off"  class ='txtInput numType' style="color:#0cff00" id="pendingTPBanksCountTxt"  onclick="gTools.copyControlValueToClipboard('#balanceTxt');"  ><br>
         <label for="singleTokenValueTxt">Single Token Value:</label>
        <input type="text"   autocomplete="off" class ='txtInput numType' style="color:#0cff00" id="singleTokenValueTxt" value="1" onclick="gTools.copyControlValueToClipboard('#balanceTxt');"  ><br>
            <button type="button" id="createTP" class="buttonStd" style='margin-bottom:1em'onclick="let window = gWindowManager.getWindowByID($($(this).closest('.idContainer')).find('#windowIDField').first().val()); window.createTP(false);"><i class="fa fa-hourglass"></i> Enqueue</button>
        <button type="button" id="createTPNow" class="buttonStdGreen"  onclick="let window = gWindowManager.getWindowByID($($(this).closest('.idContainer')).find('#windowIDField').first().val()); window.createTP(true)">Create Now <i class="fa fa-send"></i> </button>
          </fieldset>
            </div>
            </div></p> </div>

               <div class="tab-pane fade" style='height:80%' id="pendingtokens-tabs-above" role="tabpanel" aria-labelledby="pendingtokens-tabs-above"><p><div id="walletSettingsTab" class="walletSendTab">

            <div class="tokenPoolsTabContainer" style="height: 100%;">
              <h2 style="margin-top: 0.3em; color: #ff4fff; font-family: Arial Narrow, Charcoal, sans-serif;" > Pending Inbound Transit Pools <i class="fal fa-hand-receiving"></i></h2>
        <div id="ingressGrid" style="height: 90%; width: 100%; overflow-y: auto;   padding-right: 0.5rem;  padding-left: 0.5rem;" class="ag-theme-alpine-dark"></div><br/>

       <button type="button" class="buttonC" onClick ="let window = gWindowManager.getWindowByID($($(this).closest('.idContainer')).find('#windowIDField').first().val()); if(!window.isSFOk) return; window.cashOutSelected()"style ="margin-top:1em;"type="button">Cash-Out <b>Selected </b></button>

        <button type="button" class="buttonC" onClick ="let window = gWindowManager.getWindowByID($($(this).closest('.idContainer')).find('#windowIDField').first().val()); if(!window.isSFOk) return;  window.cashOutEverything()"style ="margin-top:1em;background-color: #116F50;"type="button">Cash-Out <b>Everything </b></button><br><br>
            </div>
            </div></p> </div>


              <div class="tab-pane fade" id="profile-tabs-above" role="tabpanel" aria-labelledby="profile-tabs-above"><p><div id="walletSettingsTab" class="walletSendTab">

            <div class="walletSettingsTabContainer" >



            <label for="fname"><i class="fa fa-id-badge "></i>  Address:</label>
        <input type="text"  autocomplete="off" class="txtInput alphaNumType" style="margin-top:1em; width:23em;"   id="walletAddressTxt" >

      <br/>
      <button type="button" id="saveSettingsBtn" onclick="let window = gWindowManager.getWindowByID($($(this).closest('.idContainer')).find('#windowIDField').first().val()); window.saveWalletID();  " class="buttonStd"><i class="fa fa-send"></i> Save Settings </button><br/>


      <fieldset class="groupbox-border">
          <legend class="groupbox-border">Information</legend>


               <i>Please enter your Friendly ID or wallet's address above. If you do not own one you may generate it easily with a  GRIDNEToken mobile app (iOS/Android).</i>The address can be safely given to others and is used for holding and receiving cryptocurrency value transfers.<br/><br/>

      </fieldset>
            </div>
      </div></p></div>
              <div class="tab-pane fade" id="dropdown-tabs-above-1" role="tabpanel" aria-labelledby="dropdown-tab-tabs-above-1"><p>...</p></div>
              <div class="tab-pane fade" id="dropdown-tabs-above-2" role="tabpanel" aria-labelledby="dropdown-tab-tabs-above-2"><p>...</p></div>
              <div class="tab-pane fade" id="disabled-tabs-above" role="tabpanel" aria-labelledby="disabled-tab-tabs-above">Disabled Content</div>
          </div>
      </div>`;

var ingressColumnDefs = [{
    headerName: 'Action',
    field: 'id',
    cellRenderer: BtnCellRenderer,
    cellRendererParams: {
      clicked: function(field) {
        alert(`Cashing out Token ${field}!`);
      }
    },
    minWidth: 100
  },
  {
    headerName: "Name",
    field: "friendlyID"
  },
  {
    headerName: "Full-ID",
    field: "TPID"
  },
  {
    headerName: "Value",
    field: "value"
  },
  {
    headerName: "Peer",
    field: "peer"
  },
  {
    headerName: "Last Update (sec ago)",
    field: "lastUpdate",
    cellRenderer: TSCellRenderer
  },
  {
    headerName: "Value Left",
    field: "valueLeft"
  },
  {
    headerName: "Total Tokens", //at the end since low priority for user (ingress channel)
    field: "totalTokens"
  },
  {
    headerName: "Tokens Left",
    field: "tokensLeft"
  },
  {
    headerName: "Token Value",
    field: "tokenValue"
  },
  {
    headerName: "Used (sec ago)",
    field: "usSA",
    cellRenderer: TSCellRenderer,
  },
  {
    headerName: "Updated (sec ago)",
    field: "uSA",
    cellRenderer: TSCellRenderer,
  },
  {
    headerName: "Last Cash-Out",
    field: "cSA",
    cellRenderer: TCellRenderer,
  }
];


//Detailed meta-data shown once clicked (per-bank/dimension info)
var outgressColumnDefs = [{
    headerName: "Name",
    field: "friendlyID"
  },
  {
    headerName: "Token Value",
    field: "tokenValue"
  },
  {
    headerName: "Remaining",
    field: "valueLeft"
  },
  {
    headerName: "Full-ID",
    field: "TPID"
  },
  {
    headerName: "Banks",
    field: "banksCount"
  },
  {
    headerName: "Total Tokens",
    field: "totalTokens"
  },
  {
    headerName: "Tokens Left",
    field: "tokensLeft"
  },
  {
    headerName: "Total Value",
    field: "value"
  },
  {
    headerName: "Used (sec ago)",
    field: "usSA",
    cellRenderer: TSCellRenderer,
  },
  {
    headerName: "Updated (sec ago)",
    field: "uSA",
    cellRenderer: TSCellRenderer
  },
  {
    headerName: "Last Cash-Out",
    field: "cSA",
    cellRenderer: TCellRenderer,
  },
  {
    headerName: "Info", //hidden. displayed on demand
    field: "info",
    maxWidth: 0
  }
];

const gridElemUpdateMode = {
  add: 0,
  update: 1,
  remove: 2
}

// specify the data
var ingressRowData = [];
var outgressRowData = [];
/*var rowData = [{
    id: 'OPy7FgdB12dCBN6FGsG3VcDhY78OPy7FgdB12dC',
    value: 364,
    peer: "F4F5jKd3644DFGHfF4F5jKd3644DFGHfF4F5jKd3644DFGHf",
    lastUpdate: 30,
    TPID: '451VdaSd23F9VP39NvHJLASgYTfEWI89PKLVBsDfS',
    vLeft: 349893
  },
  {
    id: 'G3VcDhY78OPy7FgdB12dCOPy7FgdB12dCBN6FGs',
    value: 17843,
    peer: "4F5jKd3644DFGHfF5jKd3644DFGHfF4F5jKd3644DFGHf",
    lastUpdate: 3203,
    TPID: '9VP39NvHJLAS451VdaSd23FgYTfEWI89PKLVBsDfS',
    vLeft: 12743223566
  },
  {
    id: 'dCBN6FGsG3VcDhY78OPy7FgdB12dCOPy7FgdB12',
    value: 232,
    peer: "Kd3644DFGHf4F5jKd3644DFGHfF5jKd3644DFGHfF4F5j",
    lastUpdate: 722,
    TPID: '3FgYTfEWI89PKLVBsDfS9VP39NvHJLAS451VdaSd2',
    vLeft: 46777422344
  }
];*/

// let the grid know which columns and what data to use
var ingressChannelsGridOptions = {
  columnDefs: ingressColumnDefs,
  rowData: ingressRowData,
  rowSelection: 'multiple'
};
var outgressChannelsGridOptions = {
  columnDefs: outgressColumnDefs,
  rowData: outgressRowData,
  rowSelection: 'multiple'
};


//Button Cell Rendered - BEGIN
function BtnCellRenderer() {}

BtnCellRenderer.prototype.init = function(params) {
  this.params = params;

  this.eGui = document.createElement('button');
  this.eGui.innerHTML = 'Cash-Out!';
  this.eGui.classList.add('buttonC');

  this.btnClickedHandler = this.btnClickedHandler.bind(this);
  this.eGui.addEventListener('click', this.btnClickedHandler);
}

BtnCellRenderer.prototype.getGui = function() {
  return this.eGui;
}

BtnCellRenderer.prototype.destroy = function() {
  this.eGui.removeEventListener('click', this.btnClickedHandler);
}

BtnCellRenderer.prototype.btnClickedHandler = function(event) {
  this.params.clicked(this.params.value);
}
//Button Cell Rendered - END

//Time-Since Cell Rendered - BEGIN
function TSCellRenderer() {}

TSCellRenderer.prototype.init = function(params) {
  this.params = params;
  this.eGui = document.createElement('span');
  this.eGui.innerHTML = '<i class="fal fa-infinity"></i>'; //will change on next clock-tick
  this.eGui.classList.add('tbTS');
  this.refreshThread = setInterval(this.refreshClockHandler.bind(this), 1000); //lower?
}

TSCellRenderer.prototype.getGui = function() {
  return this.eGui;
}

TSCellRenderer.prototype.destroy = function() {
  clearInterval(this.refreshThread);
}

TSCellRenderer.prototype.refreshClockHandler = function(event) {

  if (this.params.value === 0) {
    this.eGui.innerHTML = '<i class="fal fa-infinity"></i>';
  } else {
    this.eGui.innerHTML = gTools.getTime(false) - this.params.value;
  }

}
//Time-Since Cell Rendered - END

//Time Cell Rendered - BEGIN
function TCellRenderer() {}

TCellRenderer.prototype.init = function(params) {
  this.params = params;
  this.eGui = document.createElement('span');
  this.eGui.innerHTML = '<i class="fal fa-infinity"></i>'; //will change on next clock-tick
  this.eGui.classList.add('tbT');
  this.refreshThread = setInterval(this.refreshClockHandler.bind(this), 1000); //lower?
}

TCellRenderer.prototype.getGui = function() {
  return this.eGui;
}

TCellRenderer.prototype.destroy = function() {
  clearInterval(this.refreshThread);
}

TCellRenderer.prototype.refreshClockHandler = function(event) {
  let time = gTools.getTime(false);
  if (this.params.value === 0) {
    this.eGui.innerHTML = '<i class="fal fa-infinity"></i>';
  } else {
    this.eGui.innerHTML = ((time - this.params.value) >= 86400) ? gTools.timeToStr(this.params.value) : gTools.timeToStr(this.params.value, true); //show full date or only clock
    //if happened within less than 24 hours
  }

}
//TimeSince Cell Rendered - END

class CWallet extends CWindow {
  constructor(positionX, positionY, width, height) {
    super(positionX, positionY, width, height, walletBodyHtml, "⋮⋮⋮ Wallet", CWallet.getIcon(), false);

    this.mMetaParser = new CVMMetaParser();
    this.mDoGNC = true;
    this.mImmediateActionPending = false;
    this.mTPTemplate = null;
    this.mLastHeightRearangedAt = 0;
    this.mRecentTransactionID = -1;
    this.mFieldValidator = new CFieldValidator();
    this.mLastWidthRearangedAt = 0;
    this.mDomainID = "";
    this.sBalanceUpdateThreadID = 'wallet_data_retrieval'; //the thread is created solely for the purposes of retrieving data from the decentralized state machine
    //the thread is shared across all instances of the app.
    //Rationale:
    /*
    1) to allow for data retrieval even during pending data-commits, invoked by other applications.
    2) not to disturb other commits while these are in progress - note - this could have been achieved through the tryLockCommit()
      mechanics as well.
      IMPORTANT:  the tryLockCommit() mechanics ensures synchronization through blocking any other app from executing DFS commands or
      initiating the commit-procedure. It is way less costly than spawning of a seperate Decentralized Processing Thread, which involes full-node's resources
      and is subject to limits imposed by the full-node's Operator.
    3)
    */

    this.mPendingTotalValueTransfer = BigInt(0);
    this.mERGBig = 0;
    this.mIngressChannelsGrid = null;
    this.mOutgressChannelsGrid = null;
    this.mERGLimit = 0;
    this.mChannelIDToUse = '';
    this.mSwarmIDToUse = '';
    this.mRecipientID = "";
    this.mBalance = 0n;
    this.mErrorMsg = "";

    //register for commit-state changes
    this.mVMContext.addNewConsensusActionListener(this.newConsensusActionCallback.bind(this), this.mID);
    this.mVMContext.addVMCommitStateChangedListener(this.commitStateChangedCallback.bind(this));
    //register for network events
    this.mVMContext.addVMMetaDataListener(this.newVMMetaDataCallback.bind(this), this.mID);
    this.mVMContext.addNewDFSMsgListener(this.newDFSMsgCallback.bind(this), this.mID); //register for decentralized file-system's events (used to retrieve token-pools as files from VM)
    this.mVMContext.addNewGridScriptResultListener(this.newGridScriptResultCallback.bind(this), this.mID);
    this.mVMContext.addVMStateChangedListener(this.VMStateChangedCallback.bind(this), this.mID);
    //register for state-less channel events - BEGIN
    //we're not really interested in low-level token-pool/transmission tokens but for the sake of sample code these are here:
    this.mVMContext.getChannelsManager.addNewTTListener(this.newTTCallback.bind(this));
    this.mVMContext.getChannelsManager.addNewTransitPoolListener(this.newTransitPoolCallback.bind(this));
    this.mVMContext.getChannelsManager.addNewTokenPoolListener(this.newTokenPoolCallback.bind(this));

    //we're much more interested in higher-level State-Channel-related notifications:
    this.mVMContext.getChannelsManager.addStateChannelNewStateListener(this.newStateChannelStateCallback.bind(this));
    this.mVMContext.getChannelsManager.addNewStateChannelListener(this.newStateChannelCallback.bind(this));
    //register for state-less channel events - END

    this.loadLocalIdentityData();
    this.retrieveBalance();
    this.updateStateChannels();
    this.mBalanceRefreshInterval = 5000;
    this.mChannelsRefreshInterval = 5000;
    this.mLastBalanceRefreshAttempt = 0;
    this.mLastChannelsRefreshAttempt = 0;
    this.mBalanceRefreshedAtLeastOnce = false;
    //setInterval(this.retrieveBalance.bind(this), this.mBalanceRefreshInterval);
    this.mControllerThreadInterval = 1000;
    this.mControlerExecuting = false;
    this.setThreadID = 'wallet_Xs_' + this.getProcessID; //spawn a seperate 'data-commit' Thread.
  }

  doGNC(doIt = true, convert = true) {
    let currentValue = this.getNumberFromControl('valueTxt');
    let targetValue = 0;

    if (convert) {
      if (!doIt && this.mDoGNC) {
        targetValue = this.mTools.GNCToAtto(currentValue);
      } else if (doIt && !this.mDoGNC) {
        targetValue = this.mTools.attoToGNC(currentValue);
      }

      this.setValue('valueTxt', this.mTools.numberToString(targetValue));
    }

    this.mDoGNC = doIt;
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

      case eVMState.errored:

        this.showMessageBox("Oooops...", "Looks like you've provided invalid data..");
        break;

      case eVMState.synced:
        break;

      case eVMState.limitReached:
        this.showMessageBox("Oooops...", "Limit of ⋮⋮⋮ Threads for this session has been reached.");

        break;
      case eVMState.initializing:

        break;
      case eVMState.ready:
        this.mHasActiveThread = true;

        break;
      case eVMState.aboutToShutDown:
        break;
      case eVMState.disabled:

        break;
      default:

    }
  }

  newConsensusActionCallback(event) {
    let task = event.task;
    //is this notification of concern to us?
    if (event.isLocal == false && this.mTools.compareByteVectors(task.getThreadID, this.getThreadID)) {
      switch (task.getType) {
        case eConsensusTaskType.readyForCommit:

          break;

        case eConsensusTaskType.commitAborted:
          break;
        case eConsensusTaskType.threadCommitted:
          //update the global world-view
          this.mVMContext.decTotalPendingOutgressTransfer(this.getTotalPendingOutgressTransfer);

          //update the local worldView
          this.decTotalPendingOutgressTransfer(this.getTotalPendingOutgressTransfer);

          break;
        case eConsensusTaskType.threadCommitPending:
          break;
        default:

      }
    }
  }

  static getPackageID() {
    return "org.gridnetproject.UIdApps.wallet";
  }
  static getIcon() {
    return `data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAcAAAAGrCAYAAABXFkfqAAA09XpUWHRSYXcgcHJvZmlsZSB0eXBlIGV4aWYAAHjarZxplmWrjqT/M4ocAiDa4dCuVTOo4ddnbI/bv1eZr+pG3AgP93POBiSZzITAnf/9v677r//6rxBDby7l2kovxfNf6qnHwRfNf/99fwef3p/vv/Xrq/Dn77sxf76M/G38bd8Pyvn+DoPv59/fUNPP9+efv+/q+r6I7eeDfn7w6wNNT47fUN4gfz7I4vf98PNv13/eN9IfpvPz/13x/TjP70d//XeqLMbOfJ5FF48F8/zZ9BT7/h/83/gzWOJF4X0nWOfPbO2f1875f7F499cI/7J2fvy8wv68FM6XnxeUv6zRz/dD/ue1eyv0xxEF/5vV/vSDduOP3f5h7e5u955vdiMVVqq4n0n5n494X/FCljPZe1vhV+X/zNf1/er8akxxYbGNNSe/lgsdxzR/Qwo7jHDDeX+vsBhiiidW/o5xRXvfa1Zjj8tkgqRf4caKGbbDOtEWVjO+HX8bS3jP7e95KzSevAOvjIEPC7zjb7/cP33zP/n12wfdK9cN4S1memvFuKJ8mmHIcvqTV2GQcH/WNL/1fb/cb2b9/T8Z1rBgfsvcmODw8/uImcPvvmXPzsbrsk/Of44X6v75AJaIZ2cGEwwL+BIshxJ8jbGGwDo27DMYecTtJxYIOccd3MU2ZgXjtKhn854a3mtjjt+3gRYMka1YxTQECsZKKeM/NTV8aGTLyeWcS6655Z5HsZJKLqXUIowa1WqquZZaa6u9jmYttdxKq6213kaP3YCw3Euvrrfe+xg8dPDRg3cPXjHGjNNmmnmWWWebfY6F+6y08iqrrrb6Gjtu24T/Lru63Xbf44SDK5108imnnnb6GRdfu3bTzbfcetvtd/xmtR+r/tlq4S+W+/dWCz9Wk8XSe1393Wp8u9ZfHxEEJ1k2w2IxBSxeZQEcOspmvoWUoiwnm/keCYocsVrIMs4OshgWTCfEfMNvtvvdcv/Wbi6n/5Hd4r+ynJPp/n9Yzsl0P5b7u93+wWp7vIxiz0CKQq2ptwuw8YLTRmxDOek//tv1SNTk2Tt5hC/7yX3k4ntYNYyuxYr1QXWPqZ+d5x2sV2CCzD+v2lnv3ct2hNzOoG7EfI3kU+zW6fc1Bt9iOtunOyt276xWjQWzrZBZggUwku9H2pN8l125MfLSUuYMIy+CPISyx25rhgn+V2O0YLT1dQNr7k9l8HXseRh0K2MPjGre5b23nofL7rVXwdUW69pCA1N5bzuV3yMngyXkjZcNLFwAiVsGi1vbymUucyfgfT6duHe3xBLoA88GisYtR8PyZRVse3B3nh4ZErOrls+5fP+WiZvG7WrACSvOl81iwA8OH7PJHGskrMBaKItHXwMrHQmZvvqZGZ+6G4sE3rbyvdGdEy6zxun69YSBL3XWMvWudUIbibnH1rytGbFU6Lw3hsznHR+WHRsdc9Tt+iyX6Bi2ajlnEg84qC1MEWdkqjFmP1t4b2exrB/Cpco51q2sRSlnYv/lCEUreP1lfpdAtGk73NQ31KOFuGbqEyhZ/p6Scmw8qmAZRn18SoO8d1Jgzd379JGr2dVDy9zXn2QV/+OBlyW8Nc47T1xrHwUZoc/0C5mzYeQLUzt3XVcI/svorWx8dPAW387u8eBZg4UroeHbewIGGXdq6+IkBNktWhs8GCOkYQb1W30auf3EMeouABp2mbuO2xnxYgVywIOxRUnhfS2+97e/3b/6AU4GnwAazmHJJ5FlWNn7zlpMgCTO5T8vO23P1ZznRSzXOiDi0Wxmuo2ZH5b/7HoHPpNwzzbnyFlWxeUaMYi7xZHiANdwtcbUaiYzHnhV6rjyKqNEIibexjfCqVORiTvUFecGtRvZ55R7QAFG1385CbH2y0smPlb73rexPnLQYQd4rpChRFzCzHDZ270sSNwz+DSY1azemIrzkZ9VXr6XJtfl/DskUkxax+BFt5VzwUZCIQCwqW3ANwHwrAHOds7BQkuZlngM2PKDGvl9gd/daHkzIrxqzXwmPs8QawXEIf0AkiXghcXooC7v8g4aOFM7BCyTCcDysU2EdbCuz5pmNAY/JtG6yV+N1dq17vATrn1eYIkFBfzrXe0AFQaglA7BSxXXJFRGJz7wcVzhhrFY0QRBbLXsOwHYaOdgMpzEwObr8ILj10asZOCCVbUA9ABtYZ67Q8FYfVU8i3Xdc2B22CffCax2STDYzFBsFIexWqs3VNbZHxD4ohLGKITGGHhzv4wr5tauoGpHrQfLgCk6eGUsiOVFYIKQWEgpAmKMyxC9VdYDarelDcjGStjh1gWPzqTrOePOAFLbe+EocLu0yOQuktXiBbumxySlNJhFC2AzWdYvwTWLfUg5ekwDfotsogX4mdPPlByrADKOICTyDH/E0+cCYYcS8bxAv0Tmrh78gJcYjDunoQfvFCJAtw1CvJka5tfKsrA97gNJiQpOgn/DXxBYh3EyHqIIODCW7kZcilGRCrqiK9bGYotR8EGrEZI9KFpgtUfPnrnOzWDmVf6FBSwQeS2Yhu1L1F7y1iLMC345p8Ocg4xBFspxTEjKnGeBdkKUPDFAe19Cx/g36UOj9cT1Jke0Hov+vUL1Tl8cwx6ZeAt7Q8uY0yYMAyBP2kxKTeZJrcpw5OvVLcKi8GuY4Vb6bOEUYg3AKhs7XS9vOZl8cojzZXEvUq3mmm7EhUABjB7SNJvhSVLehTAaEzdxqWhIbY/4CD1xF8FEWNS+oyV4g0+/uXb/g2sTYaB8RhekUFpnROWaQANnroAy7vnLV//kqqDLAgn6JrlE0B5/zUyBHHOgfGdU13Awok30E/8lMecAyqTn5NgxJmE3jNMz4Yv34ebixDA8ONSR4W6CAYixeTBq9W4koXGYjJF/MBTU0eRgP9SmDXkjjAbacCZrd3oRxVqAKfHYXDYPoEZGX4VGAhwQuQFhiQyCICI8jcwAhpPZ5jnkA7GZCK5CTC8cfixg0jFa3AfGHIcwDOLaCM1wpVpb0hDIMKwLZu7tjCjHyDhCT1vMaBcAm+TdHM6I2cFvMGup8BGUh0m+MPcLv2P6wIeRNUgGdxJQl8yCEGe1Bv5TNXh8x631OFhiPplPIyDIsrd35B4DETTwzNkU5fgrOYRvjFVvsQm2svyX7A/tQB0RNbgSSF0vHDQbVDq0zHPvWgxlZ8b1AoFMdslU1yL+hhEXHB8WeBAQd7nc7vBgzv0+8dfnpb8/8w+fXy9s/tgLryqbTE1t8Qhh3Q+AiU6jQfLI96+j/fOne8wGFoE/Kz4/4mvI9cHptaY/D+Dz14jkwD+uL3rtlPYPJiAEHO/hm4034G/vOR35cbAbPlyADhbmvzEyVPaG4v1M3X592r/6rH+zgu6fPuc/GZX7Mcj/86jcv/qc/+mo3K/F+r+PCqGJ0m7knKNy09gb4oP2AcBmSG4GFHUVl68d4Qt/W5JixNCqG47Gs//qADxlJYNFk1kDGgZSsJa7JEJUcIGUJz/AzALVB39LIehXmyRmJBLSlZ8goHPciMF68iY8DcaWi7x4JGhNV4ye8Lhiz5DvMNDcnfmRXBC3pbcJ9Y+HRy50PzSs7wUISY9IKFWArjpAC10CZgFYhtSaaLKyozhkKwjzHiAP+moNEi2pxV+JJbx/8pDImKq4VndmO48OrBHCDLSN+hH0RFIBWMia0Pq7EUvg+SsklBZhhiS9JfpqoA62OHi2l40gebz1wFZsoPNuh3aNLXV3ckRiHYZbxhBrgdDlAGTOQbQasgI8LNGh/j2Bzoie74BvPw6RSJp8IGSJhSWzKdShGjgSUuoyCzIA1JKEelZOfBDaMDGSWkLPfDLOllVzJJc+NDFYpJ00CuvXpdfWhgDBYKD9W3/ZNwj3p1EYeXaQW8k8okEDoY4La40K9HySYXsMdZH3N+ua13zEShooO99GW6poDPxie56GeTBEhfm+MLh/Xds076PwMLTB42AypHtkFhNv8E4cY3ysDLHHuKepoKqSgJcsQAmuBVNvaKKaRRQqr9+kJLlrHds1IHaqXsM6MQbU7ENikI9cV9IqshrLFce6MJt7Opll4iE871QCuJD5zk0OlUQSQ53i8pe0EFk0KIuhGv8yT69Cx2OJq4uDwUig1KV+lnTPlHhFZIL6S5KJGRDaRVTQ3wzzmpkIZDj2aiQWxCfHOJ5f0GDCf5hrk7QLgP9liaX4NTYkIvDDEtbf54Uu4HVdyyRHDbwfCYE7IadVcciMjGXHM5B3ONTpacnovAlc05vaexOKaYQ3eHi1Qk9u1N15dPKLiD+uZmZSrBiKwrcJFLTuTwjwyBcQLx6w7RIRDwRlgNYsGJUEfaggTVDWllYDkkLwC/4zAj4MA92tQ2ZGBQ7PRBvDogMjvpBdaJ8De5ZEJjEte+BWRszGorll+J+HRPEncgIufivjKArdiPcjxkKGnEZpUkcoMwIiGWJIeKO3I8O5vXz1M5SWJBeOxOLwvaygayp8sVoi5uUxgLgd2KslASSJwc4DLhIUvoqcJ3bmBKFIzjh759MJvPreCCQiMfAF+EhZYvQuZYaNDCgIyo6DrXGgbRfRWpAWSJxWGBdEdC0WhhhSWUWpBVMOiQWARAjkVoJjQWzOYslQhU3pDjY74s+TA26het4q8WKjiizfJfk4O/hgy7PQCKrkWFdYMR5GKD7ul7qfFyKlHQTSkt2RRa2RNkQD6a+vorKGR9Uj2oWUYPLB/BfGmSWH0PThLT2GQHSqFCVdAUmGQZ0RYtvxFiEB68A4+CZAAJbjFgnC7slXShyITGIJDXhArnkC9FrplwgisyO3DIl4YsCbL9kOGmTISSjwAzzfHZnFpM0yckVvJzEVVqJ2lBjCus+GfGzGSuZFdoPZ1cRsK/JItY9WEDIqkzpIvapDMyxZFy9DcJPsySDgGaPMO2o8HcaAnFwZxgn+GCGfRZHPRFJAxom1mHGqCi2QbSefz+ci/RWSUgDg2TJSKJSADCyHAl7BQhAv9BmDRokzVHdfVVUVBcIoENN4fLyk/TIXbDeVNgcypXSDl8P5GTzm2behkCWMe6yI2ztcGIjKcYh5TQVygHzafAY+jG3AS+LGUL83oVXJ7ghbCOvIvsDi4SkARGVVG7QG1ApV/GTh6Qt+3FLFHchX/hGuARhVBglMe/lbQ2+eBSq2L6WCJqRgh+Tk+7mRnBhiugBZVhZBq+2tuhiyUxZfoOYZJLI98d/lyZ5oPGYOS47ahYC5ALFd+iqSka0uPXw3oUXdBT9NIn2qYKHc0HokIeDoMH1gLHSCp5C6s2N1AMKGlyMZveEh27YxDyKS1EhY1riggBVtk/mUgJ+zlIekNwcMAm6TmAsJEn94CvFmnKk37YYe1ZPgClCTPibq3kMMtFt2sKVUeas8HYI4vEZHvMzqmA9Br30MfLWts9EqNpBspPCRAciIo4OtE52qQqN11TLQhowI0ghJGHOpxqaiw8KEEnDx4gl5Izf2R5DJXwV+G6FmOKr26RBdUWh0sFkyMBKyRyiG4gaxGHBbMHnAmjxsJWc8LMCt68sHoB7Otf1X5YjA4ENh/CKnvBemBFkSIdJjJ77Ebh+5aRlDic2rfPZqKdPIDYTnOCznXZNsystzJnCaKVYW8s/tSXruKlm8B6ISw43EaVKJHa9UCYvAP4o/ZlKH6hunFg/FGaKPNRI42dyZ0FxkXoM/M86lMks17VAlbXORJlUezLD6X1OC2sBCoDtrM3e8nSUdx6WN/CZoibXMMEMXvxwmFIfjksyX7DBQrDCnYNoHgaMhFUEbw2RDFRPQxBHCROVmqQ+idWwkN5xl90pyJZkFSCR+hxLY6fSjHIX8tbVwvHiZnlIxAdEc9gHq2unaeohnjkJ6qD4zYyD2lbXaPpkng+LwsQZN6MpKNh568XIxbRIkJoQtZOFlAs4n3gLHUU0Y5hPOMxBC9+LE4vmkw8lvberhFbXL0iBqQvg1ohY0w+ABwQzy+5P5fQBgHrFxbNDMluHXWewu8Cz8rSSCAN5Hmj6wX9fHYs17wE8YNo9cYCSJCHCFxdV5kUoVhrM8yQilBwCJlfAJOBvJME7M21MiZQObrBLTmIO0/J5gMC8SAU8imTUVVVXhJftAeflQdGPQ3hdIh+lIuBHMVk2ahQGVGD7GS2QyM+IA9tDgJiWIpC7W5AjnpqAfRoz0I/BS3Ae5Bs8pLl7TTlUiUvKC8avcfsBM4hW+vIykCHip7hiidjaBnoBpstmEPnfjN7/OdYyUUTPjqDXoKqPthTApB+qI/uldck7aHrnXhIb+aCu5v5psIGjguCRlcj+5ibwKhhLu6QS/oVW4BnQPCTZ9RzKTfmPWVohWmgkUuDVMnkRRF/6tErPo8YEKiP5bTNPgcWBrhXVCXcB7yG2DJQ4V/FWYy4pJwg2E6WGtDK1pBZ7miCeytRxuoJ9RcpBQNDoLQxRqywGsGCac3dOrBKVPk8TWPuVUpTNNv3cAj4KxLkMYhPaBb45HuBHKxIM2QEaHlBGYFxoIJ8N5K6kWtEXmHlYb0J9tOWQU8F4Jy4MkBl7BFhQEwJq0vrAcPZE1E43WQk8Gj3pDEGtAVxtNkK/pArkIVqI9e3KSFF6H7V6fK2wColAOi3viYwKVL3B9xNIlh09F7RDNImMEB+DwuLwbmIDt0RAdCDGCeAjcVChFySmhlAaUexYGM3oCKuKCYOmFXvUKrcFcT0JhOUhQmTElgcTDxqycvCOCB9ND4Vl8qIUFb1D3Op5CPZP8lsxpb8mATIQQWqNqBBvQeo1UYoAT9UVQQp9YOB4Ad254nPjtGwDZeLfqj0NyIbyIcWi7VwnECG14OhJkvRd6iBPORVzB+5CGxBs8FnENLMPL9xCPq9Fd/Ft4j2mkYbCs0hnpK62j2cGGEBxooavaRoQLkHtxEpV4B7rixC/9uNU+CfDtGI4B4pLpt2orsWfi1siM5GZ8GAINzFjaWE7/EAFH90w/E5gNvWIO+l5Sgkqx4GKl6S1fkYQQAUVfnQQDf6mcNN0ETHAk2yDn1Brx6dLzADM5BrZKyKNrcjDNhNz0dyGzoBKpLeD/bZZJNq3sIqLpa0goMQR1ahzigyRAVBTttDeJZOvMVVyFNTyQGeLCK5aWekGUTaM7pCgCSq02cAGA/W1shcbSEcUo0YBrRslTBnuH8A7fBF9gnDg8+bRJpMFqJ6uLgljQDWhF12YlVIgEAdqqW0k2g4FF8L8Qw1Mk+So5ekICLxomdCJB5sL6bFyilxM9mgx+qcrFaQwF8ni1uYq+JT8wYcZRxIpxdhF9r4oJwXbjKx/C93CIpm3dpW3GCJJpB2DCPPKBiwysvyrrt8XgVDtXz0xLSufw4LlaF/hLY57UcPAucn0fOGA57Y8QiWpxYD3IWV9QDC/63qXpZsGt+X7wGXHckMwFJGfBT9UGTgtw8rYhzTABU3uibynCBmBpRMP2JyAz8ARgnKjTlsM87g5tKCtldhUkBlFdN4DLgqIxvazIRE+DYSjjaw0VQwGzq1rEk1XEs+HAoooLNrA6qnYQkEMQ4mq2tcvX/QER0q/parNwFVBpweVZJbJ9gY+UlJ0aRnJEmhBecF6DOMOEGFGEa/XV9I0OHWzwGUCg4GyQBdQJWVOkjJcbKWKpawyBS9rVUzuShBwN0QByEmnTZwzZhJdI1xrUc4M2CmrtSNqf5tVqBMTzHWxcG2q8VFoxD4WuMjjAzToibmCPcEsehIbp5B6VUQjypQ18cl3s8jCkqDT48wyQy8g5TBRlPUrTTrY2n0kNqcG01DED0VAvQkbTwA/RPBBZOD5Lut2OajMg5wF5cB9UD1RpzuQlcEyNSeAhyurjtN3737YkDQ2gwiZCwMOzVZUoTwQiM0Zo/Syw9Fymry4obJUe7OSViFFYucqdXfSdYAQcIRlASQ4OyFPvkh6HWFVzFXqmqwzMMBJQTjDHv0PaXWtoV1j7KItlm9oUT2+vMzBwXAmyWz8p8ar7A5+HNOdfM1rviaE+EAE54OtZ3VvmwF+IWoxvZ710jLpIUz2VCN/q4GFUpdIKamAXVqqSGAmiCeHv6I0FISl4A0G7GbyCLM2hHL8ZCmwqYznYAGlVzQtAOavb5K58OgmbUUNWmZHHmzFaHKSjoyLbq4fKUbNSMp+He6tMmqE8eB8RSkJlBZk8dGo1/Ak9iwKHvfE1Djkvk3v7n6gWdLoHuNR8gE9DvfB3dYwYhAmDQSSIlqB83l8X9JykOXAUVeOgKCTD0R6rRDDgksAqohC0QzaonroQWNfHM0wNWEmaFNiEWAJ8hUVTy5ThkNLxRCOEBdzEJ+LRJknJ2mMu0GE1BEgPPghbSm/3lWAhvAcZhE4kdxa3+GF4+9eMi6Dg0eiIiy3aK5drXa82JNXghf4qAK+U/O5aSf64mAAd6womj00MRmRTjSnaOOA1pHaiCdpvSByVqhBp2rUxlZ6CKjtMIxA9TVqhOVafHKW2ABg4BGFhiIu+BHfLAOTO82hPyK5Y4usgYUjL1GyJCLno28gQt7todSRXBOqTKleAusc/VJDr2msPoqHq9w3ntTHjfMhGADfik6iWA55E8aMhvoJuh0BhNejUVWsH9CsnMCOi07u2JLsQBEIQX4EQwK3EHYgBGcPJqr8vr6lrC/a/VbEiDUwlj/C5KJCSoWHSayUL9MXbh/aoevTJmK22hWCu74PKDGQ1cojar4kqjF6+Xo0I2DZtxfH+ByGqltzXTXQgm0wEHQ3dIq9Bo7WRAQGqqimncpV3tzQzCzEjgs+HiizFFyGJoIGIfDVEhjxZ+1AdM003+wY30xKPgI2pYwkBHh+dg+iArL0hSghIBHUgD/NmNR6p8NEx+5hDu8Teof7BWAAi9dfGBN3uTAdRxgwJ4q88xiJcYIvcM1Vz9dp7ICxBY8AOm57hirTJq8GXClNRm3dPlXh+i71QNyjPhn4j+SV/3iYLDMmHoJ0IdTGShHFQB3k+4gNV2gAwLrs06ZsdeXFXlVjMDbcjb0Y+sivOEJs+qUpbSloecNnotfcaeAiLy8rFUFRfV/q4ogTHg3RAIBwLZITzFe2oQp9Ek1W+8qidI3qszsam/jC45amDVL9U9Q3Hw9Fxe38DBBeCvVBMwlX0eet4fPfITWQwGuuA5Q5w8+onsWbt4sJ21CZIUoQaoJ7QYvOa+h4q+p9FF1mH+gBfERwEmCADKp47lMUlNEn02o2B19h6hQ9QGiwQ+xYDFu8N46fwE16JSJkd4gbgLUGeg7nzymbECQFWX/0AuGqwcIg78napH4ZESMJuSuIVk2KBCq3KJJKD45CeolPXTFEtFi/moybwn1/qi0QGctOUr0tWJ+xNGIgkF1l7VQFqC68JHHm4ixud/MKwu1rxDoRlLrWwmUrNGAA8A5K1EwoQeRU6gE5r0NUhE29gBgeat7h+jBTPcDIPRGAjqT/qyhMCGjWqdIZs4MfPRaHeIB6jA12MHAV1JwXV5VTLH3t8EV8OMhQ972tS0amSxYZ26952eLwi3igHiHZ5G3e8K6qHxCPqnWjcgi/mqX3uqZaXsdHZUA+Vhcg1DGWryUp1VpApf+ArHTnuPdKVk5TlEmoNQh0keLRlmlUbRUyMfdTaqnYcjAw2q6/lC+DzMROk1KkCgKqfu3/5gqv60pNX2g9JrJYKFChJTQFS0s9rTkOLaW+nO49U04biPKJECG2U0/ex3+KOqBewPq3Wb5tafeinY44kOA3S1LNOJ90bL1xByWMPFakjqfC14ioVM338zyOXwO6Ax8EV5ysChU0QbfUIteKDa9p+GyqyRFEnhTdJqG/YE6k8kAc26Ni0TSxqjsHgq7Cs8JV7NjlZMrdo53hV7e/sk3g6oFfTSWLthbCDHRbS5tid9LIW0osVuwLnoFZxHAJHAV3GcmrtTToUBKsEB1T9qZNsVdTKBWIc9Vup4K8+8yXJDWqgWvB2WAfArR5OfMuV3tRESE5GP8K58VIGp92SqCryXADjARG0b7VbUbsnvM5LQ3gwV7BbtUfn5C4kLNICBKqrmZblSR2aWMFddfGpmJSgUeCpDngspA256PWoSeuqHR+Y1+ZBVRMvEwqgxFZ0N/nhEJItmFuMS22SU03Q+9Yn61XpBk8JJzC+divJAYawGiDaEJWmnsd5RYehmZe1QvnspjNoqmtfkgtBP1HGtR4QDvWhnTwALDgIPWvGiqtDfoKkXu3E4OhCH0OA0GxqSVhkj3PU9NF2INePQTxVHFLt2HWN7XBKUiEBmosiQRvvpr7azCJs9YfiA1BQq9pXWmhqnV4JaviGbr+6HQu8lfuzdsgLphlEFYkKSO7hNVfomIeyheouC9irTfWbBZpKtakTv/cjIUzkoiAR/hganAKY3hEKfV3fEQqUNiw0zqW+bEIKoFYy2joWkoaK3lJLkIuCXuMzm1dnLzZJ0p56cEusrvqGMR5sA78ywgs/tkUWs3BNXRu2tb1JCGUkgiM4RYAAIeWg64mbVZIOTCx8ZKrTGe4AKyPo22tA1bvhinyY+agRqjl1q/k0SiBu9TS3eaE/pkMz4VUWmo4i9ACzQZjBfJKOlHWR5kmw89uIBDlBgPlDATP0tfNPZA3xyRqpqs4ClK24UOpqmTi8hMsER/FDUgrhLQ/c6tnG/Paihh9qvwxC1kUSvM7b1KCjGr0jeMMh2JTStYVGQgbk+RnPvC8tkCeTg9MITe+MqlMom6l6HCLxo81ciGuCkqNAVAcu0e+nXLo6rbStGiEIka+yQz+o2XwSX1Jg2hRLRxtVV43HecNUuhy8k3+r368QQ5asap3NbzMO4FJPhA4uLPFp027cJtoIHskMlA+iGi49BTNwFDQVJEDnB3VOxKu0DhU2pdWYu8OdYH5ZvEzFc8QW3KdpcwuGPJljFQEjjbOWrJQP+CTJPxJ+31mexnpCYeFHebEIQ83bVeU4AjAP4AIzvv5pZP9QfxkRU0sCE0wnYFkHhD7yKCB61ureYUSpb2g3qKSTArC9kouqhsrLIEmfXzfEl6b1jY66LFG7P1CLxOC1UWfmcdeK36rzCQwiF2+SXtKpicC8l6QoMLTy28RX7+jMhHVfKtOzwOqtMT4o4L2YpBW1chyd8opHXa6E72NUEXllEHilAvWm1Kb2kqiqUxRxP2RFzOpw8UZQgPFJxIx4hYWFApvlk1QkOThuY5TEXEE5T+20qoMksQwLzIiQDnX7gOaIB/TT1s5K99p686oYAyCwG4ys3o8C9cAlCH5kSigwTgIo6/DUWULNrLYhbap3FboHCuuiaqs6w7R/G08eXeWedsGirvoGFIEghWiijXiN1yHjjXMepwNqh6hFdTTEHE4N48I7ckBgDjVVl6uknVdkkkPE/sod9lVzH85REB0lbNfq0T4N+Afvgxg28BdhjM7iN/kZe2lv4hUKYHOqwmOVps0WrzYR6ZGFV6voK543teVJugpHXN/fNjA3hio8lwmzgIAkcQw7w2vreNFxdQ4DL+aL5sLXmhf606esjQ75qhAdVHIiMCG0LcZXTUjauK3aBYJDodv5EyfuZLWA8NMxN+0pT/UbqViAwMgJXO7a4iLyIWCwGR2ckz8hbnE3cEEnulS4YiRonP40rZQe4ykwvSB11qPOFcCtFpxsHK8QzhIy21SqZPygN95TGT0vB7AuJGJsgAB0TyoGyKT4DA/a78zRDhi0+jnESlQSWiNreY5aCI6ORWufxaM0HWgPhednDEaHoZIY91iwWjHm2XGDpB3lRVzPBBEmJciTVXrW0puR/pFJ7r0Gxtfr23idIpSV9bhFOzmR/E8iqy0DABCJI8eJ5CTkK3F5tBXT8Lq5VNBEze6o3j5sZCSB2lU4rkNr0rFDUBls6kSVmBogWLKO2PASJo/+AbQymnZU3AWdpE6upAY/hDAWtIAKilOVroYVTEcbfFI76IQCq4aoBgidZWr4Y14OV2fUIpRK95ZVD0dEq+uLJAD4dLILVtGJGACtHRX8IlGDgoOzDYRtUaesgwMeaIc68lUeZqlq7iqn3ZilFkVvvU5n6ZQUuquocW141gGKZK/qd3CQqdbzAvIQkBkPw+cIN5J30b44C6tdad8nE2tCk/lqItqhvLaySP/b6uoQdqkXnczUySw4NTKGIajnkKCBeRwTKSK7Qr5J4HjllAvupR67i2AZ+CrQXKVpkZI3vm45eB6EmxSPecj28NJBWELa7asuqbhdttpbF7x4iEaTtHXArLu3R7QBSLW1VbWE8u6BKIU7dh1Fu1FH/AlVyI6XW4FHieeQSF/994KnUA0XtbnAf9oN3tChqg6sorLFAAAIep0aJbV6lYayGldUi0ChqOb0DvjCPUOF+cMaAcesxppZnoaADuugS3nb3cBIVB9MGDqtByCHx90C8qn6qoQj6WeTTBuJTxIhyEGMsWAweYQ4uVY7yUkncmoUIXk95qQuFF9TTX0Y0T2Rujo/QqzBelttyHJt26v1COhRh2RUjzbpaZXXJY33jQaEknMA6YH9WRoG5mdHCZzoXpegmmfU6NrKq+UcKT7eXnVqQ4cZdPNBjlmhQ4ioX0btcWNoCxxY1Mlhl56gQ4MBwwlXWCL9rP2C/uEfQCJm8MgHEE67YpCTouqv3yKGIRQPtqC8yLSQ6an+X5DW49qgA2tC6Gcg4bx+FNXvho57w8nbJruBOt3IyYSAqeno7uvI0xM1QQ7ko0wcvdrTGkctK0ldTakKVY6q9ToTC+jxvaMdJVWpoOMSHzoHCRgw5sYoUdL8qQ6TsmD8TW19r4a8K+ktqYR3IZ1LHZj+fCVqbSxBjF2fohEoHWxEPsW0hL4UBBRNPVFha7mA5ai+p6r2XQ9pIxak1Q5OptbK2V1HXYPNS00JkET1m6DW4EqmwMAxEZBqJR46UhglPpkOH33u90mDWUpPu/AVeMX4igVYVlSPTEo6Ubbf0R+ev9TQCFyLGKow/s7b4evIGOUmrwOVMWVTanoFnsgCiBZVXScwIevk0a8ZRmm4RyA2C4sku7Z297SNOMj+bbgI6lXVjdWzpwN1lydiq11fxVaH6LpX6tLJ9vNKJmomCPgxBE6Cms+dOKReHHH/IZGhTd3vaBDkHy6hszakT5W4X55fIhjwUh9hQ0WbFDBGFsrX7mAGOh5ZVPQiQ+kcH88Rb6vKYqkp+W8dMi3EvvaAiBoWrbxavDpBB0iWukOHXrVsycJoNjmVqBJC3VSeT4STCmlXSiNUMb8GbHX1t2mrMOnggEfqwiErbKLD1tTYM3A5gm7ZkgBZKuNCsyKsZ+lIM/lBHWPjbcefbX/osnO/vjhiKPhr+c7dLvEM/PserN/USaSWE2j6qy1pmkNNbN9PddjFQScyPCQD+EFM4aCHSTQ6ko2LXeEqvANwUyENRy7a8EWyHegNH4Qk8bqqJLk2QKvRWk3kxiKdWAHsMnU2ohhkgajDrcKryG7Tdm1QtZEgSXctiUm075gO2a1qXviUoH1t27gqhvcq0+pYiWmzFMw5/TtFZSqh8GZIj+kITYUtu6SOsZRUrYF0QPY24qSqPpEy1rygJ9oBcsjLia001PxLWuQtUdt3UQIMme+CDsRpD/0VfpqugVATZ1WjjpdeaJO5AAI4lg7Ad2SnDgCQwPejO20zuTwd7k7q9dJl6hC0r+c4EquQn1Zmy2rSUvtSngESre2OoT1IRtH5ZJ35B4+YWkVVxFC0eZm1rS7EOlB2bbeMq615+NZBAWvfcKtMCsQK4CR3dSICEMarnQF+WC3ipLqeIe3+TtADTaSnwBdXDaDaH1TPs+7gIOwNYsPYIRLztaHyBDQtNHB1MRUWC12l7u0KoRxTTcazqyylxuXNstgklZqaEaPOiRRJH9xPLd7uHXxhfneRPiZgm3X2KPmlWxxgbcDCO4aiw6Pqpo8lNF1XQbTKf6c2bNEJ3nkWQEmIJKaGRqg6owsxLtCOlSQOV1fvgJfSWnDI+VxTTQ0wkg2IqFJq3bUFyU06LrjxFYAeSgENLzoasnUUQjkDmVCn7hAaquurW49sP2dFBYsqjqq7NOASallVF9xl8ipF4qRJZ5gJlWjyARHWCP0PN4sDqoBvqgCL3+L1A/hwyw/1JpMSgq4UiGob1VlY1BtJW+WapcZd9L0ah8ZEnqunbqtl2LbamQkrSBK5fwGjvCPppgpCeuIeRWIokruwMZocYNC+uPrliDNAFqdWmzTT6koFUDs4JAxax0wKiV5Cm1AnjNUFsPBN5kTaA1xUq9FOhc7SZzXgWVHzW1UXjgmHdJWKVxvG2+NX5yqiRzisA1di3Gllg8duXcbVwVidB6+6kyvrvqFXKBe3qO6qqqfLG44UyHkFIhwQMaBTGVk3akBtX1FVHUHn5Y4Cw1XFUsdrmTuPHE5bOfH1q8XYeeM8OhSkawhUujwwBPVmqZSuXv2h9jQdvr7w+psiuUQHDor6s2FqVQ3FQePsoKAO3RaJ0oaCLrqkQZT9vGMuF+33Si1hIDv71pU/LA1O4C7vJRwEGxBwuMVW735SF40uZ/ILvpt0G05X0V/n5FXwfziKgFGXTH5NiM7U5au71MhND24RQGfiZkt7ITo3X1SxLgL64uc3nvvtVDTpTGYgT3HwOOC1i4EUgUfUFQEkAtB5qE1pa9tIzbrpqH+TLAIo3KqdfKxrTYbt6tBc6gREmJlCkoXUSdv2xqbhoGwQKqCQigWQr7E+/kZ6kzyWWHotwHO7nwcq5ltBFKoQgcBa2jlrquzB8Fk1spKpqxkOOlSDgZZ8nSVdBQxbIloAG5klvJiU+c57epSN5G5wudfQqSMiKqJfKACkCIRBlYPkrZ9TMmxk6yA9mJqqlJCFpYjRuZeiu9Jg0t/JNT4ZiNQGadFZYZ390Ml/Vg1gnPu4rkblrB4P3YcEiqgmp1gvxPaVqFH/qA0d6Uci6tSW7F1gfWTk0HRtDAQzu4nv6zSrTmAjR0lcJxwk7YIpsyobs2E6RgfoiKhfNR3rxqWThh9KhOoZTxkOCcZ4JPxReUkb0MwIvqH+w6FLXF6Pj0pcZCsx4NsRmcurQ0CNVPIHeBXC7+qwJgwRCVlUew1+w/S68LR3D/S0qjO0gPDSNrFKYsRj0dGZKwfV1UEmURNe46osDrmCXLM4vKSyEHVf7UKovRkuzKtFCXSVx9MHqh1kyYl3qMyDkOrNfmcYXh99y6bmyAQJ5INg8Rem8gqq6qxKGQ9CNjcxmfCH8woFq70i1X49m7oHB1KeCGHTMP6YEl6N1UhHk4QwxzCd6XoXHGwReIdZdT7x98NpX0fn09RZRxfBGvTwkm/J7OjlErXTwOOnyvdjP7GIQ+o+JCJmKA0W9AswoKLmaNptGOpm0cZOJV4PKW/qCN1Y2p8FwK8aBXlUzW4tnqDLyYDDd3Q6iV159TRn3sRSFd6XWMDNh9xKMlioYXJ8N5XKGjCDjYFaXHurVrfRrra0D6ZGSdhE12kpkrO6IjaiG2am1KCaHQQ2kvq0ebXE/7ZiTcdBD8RqwFlhOlsdeOLDcKRNEptykgW0TkzutSsI5M5xCVrdIdESrGyl7J1OoAa5KY4UdRAalw/KdchvHWhNKthU5ZcyVciRctGZLHhviuplPoYe5oMQ79vUtJxEmyUOQbUA5quaTGjpzoxbPY8dJXkYTgCydbXS2xHRSeEbT+vNgQ2qPjLwrX6KEJFcPLaIVkAJ3s7MAbV1VEg9y1t9gWOolzPqnoOpK010LB+3GAXBCWoENTa3FTTTrYoX8g+aOkxdBqgMXS4C49JBGUgNgjjobjEPuer5OG3BaN9oI3VMf7IguvKqSViHV5tDyT+nWaCM9rZVEoJba+eYhAC266pAR5YnEtO3sahDpWqBhWggZkkDW6WaRBzW9CZQwYjXecyQcQR8VCckdFDIgVuMXmd8BX4Ny5sONd8D9KNCr27jCMo4BmXTNXFN523BEiD2vruVAJ3pG8KP76kcfNWgV5/OBqFhPXiPOtIlTJvUygPaokZp3Th3AfroC7mRhAKncESjiq0Y7hM0U63ixxAIeYhYAZSEKhiyARg1Nw3peywKuQhTG+WEN67lCByMNtXEopoU1OHmoobrBKMuKr8QqTij9iTVSBiRdl0kuOnCQB2NI7RAG5cJWYYoVT+IcB2uIq/qcj4eATHTqowz9uuDJmKPminVkJIwEowA8SOniKoej6jtHelZnVfTnWQ64aKD5V3lEVST9i3A4KVdptrV3Q4St6Y74DxUdZASHVqJYeimlThReyHpnMbXBt98HdrrEU2MUWUH5I8wUIehygAxYOVVkhuCTqwFXYTFcsDoTdRuqtX0fGnoNbJ+baxeDWzwiimdsUhK5ecssb2zxO68iwWZKTSvSLnianDRBqUBv5cV09Vwt+tSwfK25NTvPXp41TxUGXA6SDauM/aH36/xWr2mSQCkC2J0/UtXM9rxAB2kZPqo8xCAsDKpdskHQEfGhS+h+x/XgMCqdzC/lh8L8ivoyJGptBNGvjDwXHe8XXkr8yrqLpjSG2J/00GtdLeUTqYe1CoK+GvJWcQMiKAjqhLW5DZLGolKcTpgmeZ3S0ETCEEUXW0RDqjbdoKWTf96HZqvCifkqD7fNaTUtP8E9g6TtOPtaniF+Lw9QxDy6vKwy/KQkZvkNKgNMJDMdNA9aTUZTd7nG2eoHwFb9z1pastAV5qp7HNY6yT9o4sldELjqPkyB+31CJpuDUtHuazrNDkOt6E44NznbLoC6+eGuOh/L7qQAEJN6epAse7uMLVyvVbwoHviXvh0nQTTBalHKmZkxVVyzGuKUaHmCPclZknGMO3VT2JVh0K7n6poqpytFYa+tm9yE7zq/rO6uzqlvSIfklTgxqAtvV2YIY52G3iWtKega3UvkxeWqWDMy1RTGhi6qmrqAGVdLaFKLwOStY4Oz+jew6tDNGLl73tdx3dgO7+NQc1hrDhPejcO6FAbttWBWh15eyLCtErvVhb/+w+zSK0OFZPKxFiyjguhGlhwvpyO3ExOe1e9zSNBVdGf2g0eKp60gz5NJshHKKKJ1X47dI3eOz0LdxVMgvnaOhzt5VeDrq+mAq3utsQ6aibT5Qjqt9o434ctXflZ9Rr/itVDmKQk57THCIfwOkW61GK91BhUdNYrr6Smqq29eh0xIBF5Il0HeJBDuaY/nuBxfznC84cDPADY0VVjNtXR8GGJShlKmEcHSF4DDKJcRVVGpLvf1HOH3024tmgfuvRg7H1VnqnoD10AAIHD09UZ2TzOp+NfpBqdlUNPxukaRE1QPkjl5JqumyP9VQWRKdYz5KoZrREXTFU3MMNEcMXtdR0NS6V4riRqJxDVHuwTM/BzuAOsc+qEubqRQhmqB+gmFJI/0oYMEtWJAbAGPfbE/JiGU5vSxn4sWGOZtLl1dBtS0MVMb6I6A/RO6HroR1Q9VjcfelWZoxp69MKi3pqtW1/O60jXUdqpjvCuMx7WjspPRzWP8TZUdIdFTGVqazZAV7Kue/lW0P1awnnQJ3wEKGcgii4sUeq1rVqY1G+w+W7JxPAbHG+6IqUbcalb4Yi1lznUs4zW71pXJEn94yB4qa7L1RaSLvx4/ADx9LX/Q9DUuQQndaG+AzA6o8ioRlpZh0x1mrtDM4pmqM6XqwsREbiqoAXWve0JmVpBDQBAXr3u8w/cEgH/HaCd4+0C7uDflXFLBCjHs4JOq1USTNeRLAUf0aGrONW1O9wi+vM7Sq6SN2xrIjT6H1Y47Q4H0qlU8X+vYqP9yUfDOTpPm1+jWdMe5ttk1uG7rj1+iRDmk3/m89ZFhyTBELuiBPlnEtgb+7iBPRl5QsR59cqB7F67Xjourdt6dK+hGttQFrozUdubSD9tkDadN8KGEURuXbdWddNJ4IasSmpgAkOu+nUlkxehIF5bkThjo2RhMUA+q6QTWVWdzOpoS96cLuNRZwvEoJGBsghWV8OA+lSKKsqwlqae7h/ygY/3+ELkOz5jOoQNPX4/Tj9XlZr/+2Ge/+bf7p9/sH5uIk7wDzX+kqQ2zGN4cV2VaIV6RadQWSfs6YNTEbWJdUAiDBIPZ8NER61fkI8spyPDsUba6QIA+AoZjtXU12RVvXysm26sfGOou5RkcJZpVe39GwGoi3wIQtD9tNfCKzmNhuooAZIc03f/B1I0NMapJrOwAAAABmJLR0QA/wD/AP+gvaeTAAAACXBIWXMAAC4jAAAuIwF4pT92AAAAB3RJTUUH5AgOCBM6OF4GXgAAIABJREFUeNrsvUusZFl2nvettfeJiHszK7Oym5SbokSJkiVLIm0REie2CVGAAQPyRAQE2IZlakYI8MAGNBAggzMK8oiw5YFhkH7IggAZtmAOZAgwIMGU9WB3V1fXg931rszK9zvvMx7nnL3X8mCfE4+bWdVNZmZlduX+iebNihsRd8eJc/Z/1lr/Wr+4O9s4ODjgaSAijO8pIly8eBERAcDd1//+/aKur66vrq+u70WtL0jEPZPdMIZ1DFuoIoj3mCgaAqITzl84v3lxhhQgAoGEk3nIKe8urvHNg8v+/skd/sNzf5r//A//BTEiYAQDd0Uc2phYHJzU7/cZri9SUVFRUfFDoUtLVBWRQJCISEBEyA7mjjYN1ifEQXJCco+oYiLkYFzr7/Dh8Z2/+q8PPv0Hb59e4273CJNEJBOAxWrJL//Bf58QIi1KsEQflEmGUA//M0clwIqKioofNkKZNqiDZEGzo7nHpZBTxrEcCMkIwVnFjpvdA95aXD/51vFn5z84uMkDf4QN0Q2ARjAz5uL0GCf5Pp0vmNgUFBDBBVAILvULqARYUVFR8WLQpkyDMnVQzxCMFDKdZjoS3+rv/v3L8zu//P7pLT6c3+EoLDA69mJA+o5MIgfBNWIuYBBcCSiNTEjS8a3VTX7h3CUm7hCECLiCUAmwEmBFRUXFC8LXbI+kxlw6bvkhH6zuXHlndf2Pfm9+m5urh2g8IeUMkwn9NCMasOx0Cjk2TMI51Ay3THRQDFEhYfTacT4531ne8n/v/M9KyALBCQatwtTr8X/mBHi2KHjp0qWnesPtQqW7c3R0tFPUPCu6+b2irq+ur67v1V0f3oEIILgLh8cH5DE1qIG9FOg8k9xwFSYqKEJ2I+XMXgz0biTK54wG4o6huApfu3Rx87dSDxpA4dg7bs0P+efdv/QrB/d47/AG1/sj5k1GYyAaqBoZhaCQnUj5GVDIg6wlt+VYAi6Qy4cCylrmTeR3b7zL4exnmPSvcyqZ162jJ7PSc/X8e8brqxFgRUXFjw58MmyOhTZISuOFUNSNYzmhaSZMNBCy452RcaQJNHuR1WpZiI+Au5JDRILiGVJKmCvJnWM/5aP+Nt8++sS/efgBn7T3OKHl9XZG55lee7yBqUY0OZoMTUY/fTqpiiTjusxpU4up0biSg9CbVxHM84gA6yGoqKj4UUGrRRsylsMsUkQlWiKDH7NzdBhLy3Si6CygDp7B2kQjF/HguCdC7mjSgkW/4kY65Prq4Fe+0x78xvsHN/jo5C5t6JlFpck9nhN7ajyatKhBcC/v2ycyYFEJ0wmk/FSfTx1uyJzbq0N+cv9r7OdIngh9dkKoNcBKgBUVFa8sptkGdSSlD8+6khbLpR/vdNXgUdEYUVXwjDqYGKrKtF1ymnuu5ke8tbrhbyyv8lF/l0f5mM579nyOI9gsoBJpTWk10oR9okYu5AWYI1bWk1XJAkmgIzN9ys/XaOA09LyzuuU/vfdHRJLSNRAcBKsnQCXAioqKVxWdFhWmeEANzvuMJEJS8KD05xtk1RG7non3pMa5LSe8v7z7a5cXD371O90H3Fwdc7s9IeOcDxOm0iAmJf2p+wRRQjI0ZyAjwcl0rLq8acQWQUQJUlSaoTfEnNToU32+TOaCK99qb/CfimACoEykwXMlwEqAFRUVrywmNECpAZrAKSUS9OyQjdnqPjfynO/1d3/7rcWtX/xgcZvb3SPmdJgYJ7OWaXbOizKTCdlaVrbEVNibNOhppJsKaRoJ7kxSJqSMBcf3IzEXsUwWobeM50xjMFFF9OlTlJ0lLljkg+4ec1rO6QRFisAmVQKsBFhRUfHK4lAyM5SZOIFMG4/5ZPWA753e8evLR/zL7gNWllh6RybTCMQADYa78wfbCyw1sYiJlRuNGSoZFccxlq9PiW1mOs8ElBQCi6iYRsQElktCE4mhQSXi4ng0OsDdiPZ0JCgiZEscpFM+y0f8jJ/H3clBkFxrgJUAKyoqXt0NKx3z/ZM7/M7hFX9zdYP35leZhxYnodajMsXEyeKA4i70gEnEcKSbI1GZqpIlYEFAy/PonPPLJWkaaPcbWjNizpy3gGYjuWHTCeKKdBm1UlckQFIne2lneBpojKz6JVjiSjr87T9l3/iL2aFX2AtaT4BKgBUVFc8LWaQILpIjnpGccA2Ig2VHNJIUsgKeaczKiC4tUUrqW0JoUFXUFTPDMrgGEKHDCAxzLU3HFjig9MV1AhOK4KOn51Z+xNunN//Wdw6v/J1PTu/z/dXHZ0ImUBv+waT0kQFN6bQb/gfBy99Nk82WF9yGX2eGUhutFllptHFhQi9DnwUC7jh52DmFPLw/DsrTE1Q2ZU+FLJlvHn76i3/xJ/4ks0Ui7Td4qp3wlQArKiqeIwM6qEAUICLNBNxwV0JUjBP2shJTQCSyQuldyAlijlzSGfPUs/QOlY7zGpiokKWns8xk9VpRcYbCf1kMF1tTx4fzj7k8v/9r3z66/qtvz29wIx+y9Jao0PhXnwAEIytol7m6fEhjhk8VSYZp7QSsBFhRUfH8NoRRyCElOGpzIucMKDFGZrbH3BOdJGYCr3lkT5xWjZUmlp0RVbkYGnqJnIqRDaI3TGSKz4Ahunvkp3zSP+St42v+rUeX+ej4Dl08xc3wnFCcKM7rOJLBybQ6/Wp/AdnwSUDc+LR7yCItONecw1cJnU7qCVoJsKKi4nnC3Iq1jyoTDaU+hhIkkCQwC3tMgtNb4sBa1DNThHME2hhZiJHpmRG44BPEnSPruOsr/u+Dt/zayX3eO7zJZ6v7HLFCNBM9E5KjzEji5BDoBBAjeknLBm8YU5pf3QgQhACTyEle8vH8Nj938Y+TDKZaU6CVACsqKp4b3BMuxe9uSIKClf40x0h5TpSIdjBxIeiE0DT0OXO6SsSJsmdGl+dc6x7yzuq6v9He4P10lwd5wV63wqSIOvpY0p8ixefAotBaRhyiBaIU4s04nRdboImkr/YXIALZ6QJMcd5Y3fCfv/BvSh8b3HM9QSsBVlRUPL8IJKBSZo4I4F3CzMgNWBNowoysRjYjWKIRo9OWm37CdTn+c28ffvzm5dVD3l/e4UE+RUNiqpCtpU8teXIBpUR0M2twd9xLw7cBF8UG6UrGYD37ZBwD9lUvA2oA743OjHNR+E66hQloE7HU1RO0EmBFRcWXEw2Cx9L4vWwyJ9ryE4uApcTddMSH3d2//93l1V/+3dVNbuQjTq0l6grNjrgSUcQjnSvIDG32mDDM5bQh+tMIKiQvQ6uzGAxzNsXHoEjwIfX5KnjimYbSn2g9H+aHHKUVr+kUk9oIXwmwoqLiucaAQiEfd3AR5tZx9fAB1xcPf/vX++/94s3VATfSMctgaIAmZYJlzgPkfXII5FhaBGJ2Yl8sblyF7C0SFI2Ou2HWQ3KmoqgIyTZCDxFfW+C4OWbGV90SIeeMxhnnOid5x8M859FqzoXpa0hT+wCfOQGuZ9ut7/qeLscwvp+Zrf+tOvQDjY2jT3VXWtdX1/cVXl8cPc8UESk+de744FfX+Kr8fpg4IlIYwQV8yCO6QKmUGUEcLRU2vLSL40ips6Hroc5Gec0k9Dy0E95b3OLNk8v+5sGnfDS/xbG2MIk03fD5FWbO+IcwHUc1l8SlZtatDRa2P7+WNQ5yD6HYO5Spm8BWjc/X/w8QkFegC6CRQOyXPGwCr+WGH7cF/7L/1P/guT8kk9DX6+MZry+efeAsIf5+oarrP1Zk1M/G0LCur67vq7y+nB13L07hCNEy5ATuiMOqmRK0vF7cMM9kN7IaHgSXMrC5cWXNLA6uSlZoHNxArIeYSaHjqh/wzvG1//HT43t//Z89+pTTds5JmpNjIWSNsGeRsCp2RBXPMQJ0I6gQXHA3UOHK/D7yeiZ1CTlfr99nub5nngLNORNCWLPt+AdFhBDC+sO8yBRDXV9d38u6vpgY9JaZpEBwfCIoigjs51jqZVbEIiqRqRuxp9Te9hSCjHr6dRgmUrKHvc+5kg/59vxq/ubRp/r+0TUetgf0tIQoxGZGH3uSGiEqSMCz0edMzgKTOo/yecIFkEAjCmRMlPdObtGzYmJer99nvL5nToDj4scwdgxf3Z2U0jO7Q6jrq+v7Kq6vEd0kB31gL9dBEGKYL8EFEUAasig5RtqB6aSRYdRYItNzz454b3WL75xc8w/n9/jw4WV6cTp1sqdhczGiBkDIi3mx+FFFhvmXyQzVAJMA1ZLnuUIEkjlxVL1a4CaHHNoRf3j64/X6fcbri5cuXdp54ODg4OnuYIYQVUSIMbK3t0eMz45n6/rq+r7K68tphWpENK7LX8ZmquWQ2FwrJBEju2EiOML3Hr3Pp4sH/+Q78+t/6f3Vbe7ZEck7NHVIzrTT8vxsEEOgCRPwQG/O0hLnZnt4LhGmuiBSGuJdSuN7pOZAn2sEqELuywQcF8XMSXvGJ+3NX/mTF3/6N+v1+2zX98wjwBgjOWfc/bGi5XZ4+6JQ11fX9zKvz2MsFOcgubBfWLMedA64EzyxlBU30gPeOr322998+Mkvfnx8h5v+ABfI7uUlQ0+fN0I/DYThkg9eevy61RIXkGnDbD/gC0FMCSiujooiahiG1EbsLyECdDDHpKRDxZTYOG8trv7GfxR+8Tfr9fts1/dcaoAji2/ncM+Gty8KdX11fS/z+hb0TJDRbABiAs10dPRk3ljd5pPD2/6dB5/yweIOD/UUC4mIEyTjnjARLAYyiuSiGNUEE42QMlkNVNFJpGkaNBvWZ6xdcbLXEFTLOC4DyRmxTHAhilIt6Z73CWJEndCLYebMaLBuxVt2g1Svj2dPuM9aBTrKV7dVPNtM/qJVRnV9dX0v8/r2UwMKS83ctAM+WNz6q28ef/YP3j66zNX5PbIcQ1BcAzaFbCViyCI4EdEphkPKBB/ucFXI4iTp2A9OQuhJ9FnoKU7mOUxwmXCxX2KiRYbjDGPRYmk/d6A2Yz9XmBlRlJUalo0gEVuuuDZ9SJv6ev0+axXos+4DHBd+to/jSYz++0FdX13fc12fj/fZXhSQ44U51L4aD/RudJT3j0MDt1kiu7HvgU6cXhwLgjpEg2DD88jF7aDvmYSIdD0yVU5sxb2TQ/6Zve1XDu/w3uENrqcD5tGQEIi5jANzacYFDfVAKcqJ4UHHiohmUIGm8bM4BBfa8ZiUDrzhd1688YBOm63dAmSoPrpQo78f5vzdOkZy5lQW/8GvCaFhpT3TlRFChLRkNbvILE94a/42Pyt/orTJWGmLCcPouvH8XkhLmM7ouo5ZFi7mhqv7iR/LTREwhbq/PNcUaEXFj/YOFtebPw6eHbIR1EGEIzsmxshUG8QM79vSgN4E4jRy0i6JCBMLmCneNNhUsQTeJzxMUTGWOudKus137l31N1fX+Li9y2E+5VK3T+eJXns8ChMNaAJNhvbQzyoLvcw4S3K6PbpNSjTz+Dm3FQHhBAkYTtZyY+JirOj53ulN/9m9n5KgisRCCq219BhIGRb+9XaP1TzRTBtUhdwoM3NaSeTozLyKmCoBVlR8DpLu7FfFGDaOkaDx45PzLC0VnzxR4qQhErCcycsOaS4iCpITs74jtHNO+pbP0gE3usO/9a3u1t/5+OQuHy/vc6Its0lgD0O7xIXc83A/oGZEHHWH3snuWAyESQO5r1/Sy4wzvXp2hm8k6OdGJGOE04RIa0ZqlCDCNAgrcd44vcZ/tvcfYGmYl6owi1NmWhrEsyekEazrCW60uee4bfnJ2essJbOwUVFVUQmwouJJF4SlYgbrVqbwr+tfJRKcn4BpQ2gmRAEVQd2AgIZA0y1Z0XM5PeK7yxv+xuoqH+Z7PLBj+tyhOkcNmmnkkkzocE4SaJwwmZ3noq1Kn585IGQVskJSoRejWqK+5BFgOFNSkl2ysyekCLejxpwzeCblkga0nBArk2E+TLfhGxlRx8TIIqxYsOhaTuYnnC4WvNvf9Dtpzq32kLurE5bzFX/73/nL8lMXfoILYZ9kVclbCbCi4nPQSWYiAfFIyMPd8tCCkGOg//oe2iZi1zHB6INxS054f3nv164sH/3qm+173FoecbM7oQ+wHyZMPaIZggUm4VyxF3KnE8NUaNSJlomrllbXFghFDCBCRAm9Qe5JdSDyS418luB8W3ghbJewRMpMVtEtpSOBmAOKM7HAdEjBR4GVt/y9z/6xt6nnUTvn1vwRtxdHPMxzFmqk4JxbGX0U1J3enZCmrPKKRpRVn9YN9hWVACsqHsOE6agZWXedu1MUkQbT49vc9DnfT/f+0VuLW3/l/ZOb3OoOWGiHq7NoVkyysx+UqA29rVhmIyCcCw0nvSLNBAmCWmZqiQB0E+c0ONMUcRGyOskyno3GjIkoGgNfdUf0H3XoGbum9Siv9X1NKKlO87XYw4d/A3RdR0rC0ku0n/oWMyOY0obIf3v/n6EiNCjBS6odc6ILjQneZBKZRrUMN5/tcW31yP88f0rMA06NACsBVlR8Do6lOHFPxYHMgpaPu/u8e3Tdr5w+5J8ev83KEivrMDIqTtMIEwxz5w/0F1jS02qmxVE3GjIiThcMplOaviO0ZczYPCg2aUoksDL6viU0kRgD4orjeICWkpZtvN7Bv8xoiIXgfKjp+UBuQ22wW7VrAhwdDnz86U4UMIugoK3h4nRRmLYOIbPfTLGUS7+cCCFM0EbJOMmNaZwQrGXaJmLOzCaZy0d3SN/oSdOGJlcCrARYUfE5CP0RHyzu8zvHl/1bp5/xSXuLIzvFvEVIRN0jYeuWAFUlE8hW/O86mxNUmZmCB7JGPCjJBU/O3mpBDoF2Wvrrphmk64leUp79bA/MkdYI7sUTLwaSQvIEVAJ8mbE6ncMY3aWM5xLFrwnOw05ECKDD1J7xcfVMnE5okmNNgCYQkiEaIC0JQDP0hDsJMwUpm3k+bQj7e3Q+x/YmZIw70hK6RAyJHOp3VAmw4oXBt/7huURQaDkN3UCS0kchCwRxohkRwcToxcm5J4QGVUVdh82luGgjgsUea3tmsSGYFHvXDIxznMNw0pvhmriZH/HO6c1f+tbh5d/6+OQe328/+ZzUVgQi2bzYFI2/MMdIZWgLgBbFaBpnmTlALj6uAn1T+uw0l4PglPmP3XhQLJ25MouXX+n7e/nrf3noJ9zu19pOC26rHs/2dI01sbPPg42YRM6ISh5/zzPHSHf/Rj+sb3tNuvM9h026chgyoNvfdbsht5wznoa+zmxP7FMr7Zj6hd/djlCmWDqiqaedDlFlm+kVIIE8ecse1xynic6MGCcknH3g46MbTCYTWotAVRFXAqx4cRtkl4kxgILEsFvRCqB7K/aTELLgBDpg5YJIJEjkguwzTz0reiI9e6pMVDB6WsvsdZdY9Ssg0EsmiWMho0DvHR/Pr/Lp6b1f//bR9b/xzuImN/MhSzqCl9aDFz0N/0d+Q9HHQ4z1gGOHoBurm/WkDzYTP7aPvwxCoMJj5Wef087vRqHQ+JgPbhVrUtyqrwHMtCmN5OsU5fb6nL5fFaIzW5McWynKMOhwx/dgfC28FOeOu5cBBlIaWc2MZdey9A7xaT1BKwFWvNATLoZ1Fq9U2capD6F41q0mzEl0wZipct4Cew4relbasUowFWUaGnqJLDxzghA9EEVYSUtqeqTpeWinvL+4w3dOrvpbJzf46PAWJ/G4NCNbQoeay2sy1moyfZjVL+lpNuAn2CXJ0KTtsktgYyQWxqhNwIPsEKS57dbUmo0fxVhbM7edCK68VZnC81iUNO/AnWxGzrn0b+Zczolsj4tYxvO2GPVhg4hE2JUjyUuWmhYpnz97YtmuOMotr+v5eoJWAqx4KTZKL5NVBKHRsJ41eURDbPaYRCF55pAVkhONwD6BFAPHYmR6ZgQuMsWzcegdD73jX/Xv+PXTB3yyuM8ny7ssYiJ7x7mgRIcmRZJACpFOoFcnWOl3Dw5VZfmU0MeJwNlKYW41YouPNk9bUVraiv5gcKPYRHipy+s86EiFI2kpkPpCmJYzXc7kPpFSgqEON4qIdia2DDW40u2iOyTyOLE8Htm+XMy3bSZb0qldarmfTrk4vVQryJUAK17sBmn4uAlRXMpxtiZoLJAUkQQhlbFQoTmPC6z6TIzCOTPaPOd695C32+v+5uoG7+e73MsLzlui8+Km3k0TMlHIwsJz8c4bmopjDoxTL82dLEon0NQaydPd2MgTbnQAH77fca6jSHGtH4lo/fjWKNbt1KR7qZdOCev0ZMoZGwjOBheBnJzAJm0qIjSFShEJpFxMVZ1NGjVupVq3R5Vt1xhN1qfvTlR7lihfNCmKCO5GyYQaBLBs3GmPfuVPzP7Qb9YztBJgxYvcINFhyDLrWouIjDeuNJMZWY0uZ6JkGjFSTNzMJ9zi9OvfPfroweXVQz5c3OF+PkFDolEw7+hTy4NwDgUaV4IFdCW4h/WsxIuDp51LLsQ3BC0uRiNCrgHgU8EyO3U72Sa7rRodDCKTkTRG4sis++PGGlzOeR3B0ftOhDgiDHKTaQw7NT63oYFz+Dsh7tYgywgx3zFi3SaTsvZNnZEnpHhftkhwTBcHoNOiGL12+vA3uGi/STU0rgRY8QI3yCHVVDKNgsZAorgrtPSEfkXyxIN8wofd3V/77urar767vMH1/oATWzGNfVHmmRBdcY8kV4wpIe4xxUG0RJQyEG4JAjGBIykEGHyo43hRdSKCqwzqzIrfL5otkYvaGOH52nnCc4myPNua3HxUVLqjWXZIZRxDV5wPQPWLdfy5K5ZBeYvMZIguVYTuTA1ytN45G8Fui2O2BTsa1h4a6/Nn+znb7/VCrq/xhnKLnF2F26cP12nRikqAFS8Iut7U1jf8LMncOLnLzYO7e7+1fHNxbfGQ690hi2iEIDQ5E824iOL9Hn0I5EbWtjChd8Jwt56lAzJGXsvqowvRSvNxr7PNBjjks8x948BO9bt7KvSFyHyrBrfuh/PBnNd3oxVg/dhuenyMJHWtRjHbpCOfGP3EsJVtGAhgXfB7MkFtE9w2AW5HgeP60o7gZjdaVH/xFeQ8ULNISfVnTzRMOFrOBy/H2gi4Q4AHBwc7D1y6dOnpUlxbvTDuztHR0U564WnTBXV9T7m+ixdBHBPFSpUAHS4LcchDEUbHy3uwT3EpHUST4dY3Dy+8f3CfSYhY7hGHXsvrgkEkFGdxlOxF7j7RFQ/yCR+n+1//Xnfzwbun1/h0cYcDllhUQt7cms1GhqS4NKR1DGk7kZrtXNPjBhrXI83SZkdlyyFvvVs9y5vibc+87a12JOPsVqa+fF6fnHxxWm3dJ/c5z/s8Kf74/O1ZlWFLibk5/+RMk/ZZotL19JLcJzxlUkrraSbaP/mm54dNvW0v39Y//ezX+4XXz+cdsy/+u7L79mf+zroG+AULcH0JwqughF5YNT2zLLyWzrNoOn7Xb3Dp/I/joe7PNQJ8heFjOsiHLckAz+saTLAIDWQd0jxigylqsYQ10Y103WCWG5yIMsGjMhOg69DcoU3Hoklc92PeO7l5cvX0wfl/vrrFaTtnnuakkNEoaHBiVqTftSP60Uyp7Kbw1j+HpvegOrhNbKT94wZ/tkl8O003/uyHyHb9HGTnublPn/PFjynKuCFR8000ZqPgpKgoc85YKlGcpbwe2yW2Zt7HCFJ+D0RX8Zyub99EgNvnVNu2g8KoRoCVAF9h2NktKhiO0w932rNhr1QfMiaMTFgm1Jc3GKaTiJDUEHpyzkiCNqy4mY95u73j3zm4yofLmzzoD3HvieqkyYReeywaqkLQiDloEMS1kPGP9AH2nQBCzqTJLG8mpbhs6lNnI7ed2tKWMlGasCHKLSHHWvgQmp2JKmylEgHofB2tWc6ktOmDc3ck21Y6srzvdpuAZ3+MlLdThLm2kbxQCOX6LN9L+T7UjflqzoPuhK/PXq8HqRLgq4uxBudjGd+Fsf1XxaDvkaBkcSzEQVs3BhGCBWgoU+hdMgfNIR+le7/w5vL6v/hs9YjvHXyGuZOGKSwuhqgQJRJEyF3LTBVtytDglDLJMlkDoRE0fRXuwMeDvUt+5T5Ct5+MjT1w4+AS26RINyKOLZl+n3aayEWUsNUn52kUmQwKyrFNIJUaXBhS2mJnjFhHUVAu9zjbpCZbEYUGfczElaFFwd2f2AdY8SUS4FbtEsr5pQhd33K3EmAlwFf+AkkthAgSyIANFFcqdVrOCGGrMXhIiUl57rV0m8vzBz/1xuGVq+8sbvDZ4g69tUjf0mSjfU3J2fFsqERCiIgE+pSY58wkBrI5ITtRlaCDMEEFVH7k4wd/gt/ajsz/sV/uRn9NmDz22hKFlf/el8laRZlSIvWJNm9EJn2X160H6pufpd5XGsnP9sEJso4Sx3pqEY/w2DSVdPZzbPfAiSA1AnzhOZ7surFakkBUSGLcWB38qT9z4Q9/UI9RJcBX+BsP68RccIgbgRw4rCI0BsETyVfc7B/w3fm1X/8Xh5/8je/P73A/PdhJ9RmOqcBeJIfASbdkIoFpbFBzrO9JdEgMzKaRSR/LeKtBSIGDqVH+L9OEr8a8ws8TuWxLGNfP8e1a2mbz2m4RGIUni7aoTJ6kpBRgRrOOKLf74UYGnTWTx6T7pRdu6JU7G7Xqbh3SnzDsehPwyjpFWvFiEEToZex7FAhSov0AV0/vv88fqMNgKgG+wljiNAjRh41TEkimp6Mn8+78Du8d3fLfObrM9+e3OZBTgmYad6KVtCeqWFDS2GOcS89XyPB62C/1JXeyOBoDUwNNGbqWg2mPRiHEWGbkZ0eAiUSCQGc/2m0IG8X9kFbcGsoM0Mh00+id02bg8uAmkLq8Jqc1eW1FXXGYm/lYGnJAzmlXPDP0uY2PtanfITARQXRD0bqltNsmyPXfUNmNTHeD2TpM/CU4/wzf1KCD4jkjQbl+eK8eoEqArzb2vAwAS+Lc6Q94f3F7hDmhAAAgAElEQVTr62+cfPbgjdPLfLa8j6UDPGqxF4oQk+PJ6VXoNAITggtYJpgTo2LqZM9kTeigQnRRsgJSyJKguAsXrcVdio8ZAhqAQHYnJVtPhPlRhSJrI1RLeWObM0wymbcna4KEUouTrVFwcUulN25iYUu2lLbqcrblSbi1gA15MfY4bkWET+iDs61ZnGKPk9j2a3YcG4YU6fbzqwjmJcg8nFETmxkE5dH8qB6gSoAvGJ62djcdGgw2szKil7FRKTsSBHMhaCCljhACe1noMPpAST0Od31qUkQRYUh/DNPtQwh4IzxKSx4tTvhH8k2/cniH7x9c51r/iHk0NEZipkxWCU1JyeX1FbSVqjN2GsUVkuX1hiy5DJkeJ1FExokatt5gszTrnV3dy+ysrfezMxHI2WgFO+MTd7YPbstNwHi8R85Fn9hLtN7iXR8joO1IJ/qgwswbJaWljAy2O/28e2xt/oTeu22d6PaPJ8W/P4hUvqg3apvLPi86KwKY3XV83vuffQ/bnmpQ8cLROwiOZkWmEc89+JR9przbfoS40UmZgysIZgkVJxuE0FQCrHjeBBjXXiqjnVjIvn5s3qyYEYgqw5DoFf3QXR00cDLpiQYxCzkLFiMaA+Jaoo0egjotHZfTXd44+czfWF3lSv+AY19wvpvReaLXHm+UqSqaQPpMSIbNni6FtTM2aksSsU7Bed6IL/QJnWP2+ZttGev02Pa94zeXbVOjEtHH+ulS12/SfezWt3SIgMYUJet2AVt/rvmq30lNuvvOTMtGmieSxkvpHFDxlYwA1cuNorO58exyQlZLeu/XllGbc//VPTcrAX7J6HScIVjKKSKQ18ID41yKEGEhqUwzCZGpNGQ3WnO6cA6PMJXMft8Tc8uprbiWDrm+OvilN9Kd3/ro5A4fze9xElpmk8DMMtonzqeeB+cguBMpw3K9txJ1NIpP4saR/ClSgE8kxmEYctxisLG/LZ05IZ/kFD5eyEl2pd7KbssAaatR3Ep04j5MKQH2m72Snhz94LZSlMkd7zdkN0Z1o9s3sGkjWK9tM1hgTCd+bmqqouI5Q32TRRinqogqeTDGfdCd8o29yTrTIa6YlmHwqPCqTYOvBPglY2KJsbmu1F5k3YMlDr1HmgT7HkHh1Bt6zzQu7HsgdgtaSVzJh7zT3vI3Vlf5IN3lQX9Im1pktgIz4iywH6Z0bjwEpGmY7J3jYl6BbTbsrEoWSCr04kyeUoNydgLF2QhuOqQgy/82F+moNuzNioBkjBL9TApxJzc40I2VepcCU53gowv4EMGRDXLGzDlczocB2KxTmaOxaiHwsPW3Np50MAhEOBPZcSajWdOBFS8yweTFDioPdV0RgaBrE+Cbi0f8xN7XSivSUC4wecL5XAmw4nmgJTGVCKao6WasfCg3ZRagcwjmhJzYo+Uup7xnD3/lk9Wj33h3+R63lkdcXx2xEmMWGqbSYNkRCzTd3lqF2ZthQWjEiZZpup5uuD8cFXtBlIgSekMs0cenT4E+6d9jv1vK+Uydaxjn5YM3WxzvTMeX+Q5BTYcU43aKcozQ3JyTxfyx9OSOo7iEdZ9cGDziFJDhc+ecPzdqK80EsjOJZXsDGZ/1ubW2mgqteM7I+DCBt5CgmW28FoNydf7g7//81//YXxs3HWUz+elVRCXALxlTZqX3zQrxSdzspTnBNM652x/z4fIO3zq95r9zeJlr/SP63ELXsdjrmPTOHsp5bWilY5lXBITZpCGvImESkSA0ORG7DhWhjcYiGDMmGJDFSFYaqCduTETRGHiWbgjrC2/r4up0E92FrWgxm+HmTHJcE95YixtTlgAnp6c7BLdNcgBRm3ULQjGd17VSUbQoJ31LgQm+MyB6uw/O/XFzVFXdEYqMaVgYBk7bD3dzUFHxPFCEXkNNOzuZMtdVtQjurs0f/DL4XwshDhmPck2EYch5JcCK54q5QsSZRgcySzou9w956/BqvnJ0X//f+e9y2C5Y0SPiiGWiCjEIXUy83p5jJYllKCrR4M5UipePYaRZwyR1hLaQynISyU2DOjTJ6fsVGgNNDKgHXIqwpAV8SLU+VQR4Zmr+OvIayGoyqEx9sCfyXBq8ZUh9tsvVDrmxVX8rpLglakHK9JqtFKTbVg+UbKd4inXStrLUNk983A1g6/UuW+Pg8hcxXD2/K14wVLDhBlBFCM46HYoIN04eMlaqBcpwcw0EMcxevRO4EuCXjf6Ij1YP+fbxFf/m6RU+WN7gMJ/gdATJqO7R0dHEBlTps7N0mGiDSsNSEuowMcV6gIDRIBKw7ExYkkRZ7Q/pvQxN7gkjh0ym5eLovDTDCxCUXry0NDxlI1724s1nDGlLK95wJWXpyGq5SVumvI70xrSkUMh6KBBu9cKVwmkZybbNOV/QMjGQ10imJtAM9TwblUjjc/DHotWzxDYKeXb2mzPPt6p1qXiBWM9k9eL8ElDEDJNyw/hwccLGm0R2FNvD4MNXiwA/bwr904TgsNswq6prO5WndUz+QeuzISXlBp4TQQEtPG/Z0SAkfHCMdpL0RBHcjZQzHicEiYQhTeloSYv50OAcunVzVXAlWlFSQfGCa4EJIGY4PbfzAe8ubv25bx9cfvPjk3u80338+DED8IB5AJwYm3W0ERGiADnhQDQvfYODgrREJxnIxbhBGtSdJpX03ugInYavOaxHXnm5UxQBNwJC1IYVttMWMEZe47GPHtbilDE1OTZ8uzvSllpczrmoPIdIb+wN3PbLk0Fwsq0f8eG7GRhxfTnaD3nePXaejAKj4a+se+qeMErsB57b/qTzrW66FS9RCtS8nOvDjV/CN9kRdy4vbpAJkAQVo50ar+UpK00ICXnKG+AfNf6IP6yh5u85EtdNw/EoLHgWIoAftD5PhquiW8S37kWLgjP00fUCGom2R9JAJDAThXZBisXNQNyIbjQmmGQymbC8QPay2ZsmUgCL0GqmVedkdYtP5/d/7Y2Da7/6u/Ob3MiHrOhQKa0Hj42P2lJgicjOsXriZ45D4fqMEGOc/JHOREI7jexAa3ldDxPR9Xu5l7mcEwu7XnVDDW6suy2Xy02f3JieHFsEhrHK47rWaksd7Hle8Z6jiooXjUW7YOEdr4UGvMwJzZZxvNxQP+f9+WXjj2eeAs05E0JYb8rbKakQwo7K7rmEtFsihcyWinAQuKs1ZIE8ZfjvTNO2eFDaSWAiHY050ZxOhGUTWEYlpIbQGz4zsjq9Jk5YcqM94J2TG/7dw6t8Or/P4d5pqRNZQt0JONMtb2tj+qTs2hrjsfv8FONuLKRnUn7j77eJdt3P5s6kmWxaAJyhBleiNDOjm29qcMUA1ddeceX4hnInNvxet6x7RCCf8fMbIy8fnR4q/1VUvDAsuwV3Th/x2vlz630jD8rlsj+/2Av0y+aPZ06A2xv4GLKOG2hK6fk3BA+1nWw2eK8VteFICDkMo8OA1hP9OccNJjnR9C15ssfKjd4yjSl7HXhrLMgcknjz6HffuLp69PPvrO7ySXefI18iktkTmE4CMSkmQh+bISWaS83Oh1FlZwhsW3UxRnFPCtnlTKP16Bm3fZIEERqZbN5r3eNmazJLhydrk9V1j9yWOCRsDVv2rTrcepmDIzwIUcLu+s137OC2v+uqgKyoePGYzBrudMf8CYHUJ4IoPpSA+r5H9cXKQr5s/oiXLl3aeeDg4OCp3nBbjBBjZG9vjxif3UH9Qetzz7hvJuCLSFH/DU7XS1kxCxNmsaFxpWlmhNmspEwNFr1xQRU0cSM94l/Mr/za/3f0ya++fXqV++0xoUnkMfpRZ+olV53FOCaTcySKMvFiJmteCsuJ4nmn3vKkEFBEODuIsUjsd2c0TrTZnVKylaI0N5bzxc6sSrYitXFUl24pM2UwVR0juW4dMQ/JTNmdtNL3/c5/21Z9wd2JY830CeeEyw9fb6uoqHj2OFnO+f6ty/5nmz8i4sbe3oxmMiMDgfTUMdGPGn88c7qPMRYBhPtjRcvt8PZ5wTQQtUQpljLZMiaCBMdFmMWvsepXrNrMVI0L58+R1bjJnCvzB3wvX/EPH93irUdXuNsfIhNnFhW3DveWvp+tBR3qpZUgi5IUchQudh0uhtOR8XWKskShQhq+3G0iUGRDhCa7EZjt2tHMl4v142MENxKduzPVTYpTRjk0GzucfrDLKVY5G/eCUjS3nRRl3pyV61SmNnGHu2UktuEZeYgon3SnVsmvouLFYu/8HnfaY8wSmNNoWF/yOSVCfLER4JfNH8+lBrg9BX97I3ze5Df+fTEZDmDJdVoDvRqr3PFjvRI9cciCb89v/tI7x7d+643Tq9zoHnGaW0yOaIhMUPankdad+ZAqaOIFgmWSKL0WghArvTbRIHbOfCrr1GFA1qlKAHMjEtFh4DJWSIqxAdWcftVvZPuDCeoY0RWyDGuCCyUnurbLKa85kyM/Q0Tb34FI6RnydfrV135z4+93JrsIm/ohjw983h6ldLYtYT0rs6omKypeGJL13F0cbFSWGtYKhRdNfi+CP565CnQ8sNsqnm0mf94qoxgnRAQfrIAswkoTt08fcffw/i/94/Dub91aHHC7PeLIV+ydm+FtR3TjNRyT1+jdWIkjGgkGM3ckO5qNFAzcaMwQL67qIkKS0mcWUjE8Lb1lNpAIWCqmp103H0iv+MX5MIh5VFTGrXsSXUdfj0dP8sRjUfrknnSMRnugyeA4XkivDIj2QREqWlSoT5rnuRbNiO1wa1iTZPm9ath5rdaor6LipUGXW47b5Y5JcspGCF/O/vyy8ccz7wNsvFidJnNMhV4EHVKG4pClbOjK0D9nYyJts+s7vtOcrJQ6F14m+CuCug7DibXMvBPBgD3teGCnfNLf3/v+8sbi3dNrXFne5cDnWJTNtAOFiSi5Lf5tnSidQJa8Tg1imQyoFp85wxGPpUXABsfuYfJI8NJYnpZtkRYPLt9pILgdMtkmsXEQKI/f3RibQ/N7GdDyed+hIjt9QZwRqZy943ryibP7+yIa21Hy7H6GGvFVVLw0iKp8cHIDC5mY9nBVpt4P+1LkaTUmT8sfI/Gd7QN8UkT4LNb3zGPePhRSU4OpC5M0WL5aSfWFyWSzjSrkIJQhXkaPsUcgjCOuCgsWktOhiXrfoe2Q1KEBVk3HTTvmw+Ob/+Ta6cO/9E/bGyz6Jad5QRcSpo7HjPg4e9J3xl6dnRwSbbTS2agtN9MSoOtW6/rbxkonr5VKgcnGL46Ny8MXuSRUVFRUfGlpRk+ctEsu6mS4pw3re9dXzbXrmROgSixpMs9Fsh8MF6cHDGGWCyuUEVVlGssEBVdmNoaG60lYyKAcDFbSjd3iPtfzMe+0t/zN5XU+WN7iUX+ESU9QRwlk78lihUY9ID7WoRQNcacvbiM0GQis8/Vw5pwzlgvJjW0ETxJylJl7w4its8OEhgbwZ3H3UlFRUfF0EVDpFb63POLC/vnd3zFuwJUAn+IIZ8Ykn6GYl5HFUSCTQfoynaQkMnEfhjsOXQAdMDEQN0QzJzLnnfYG3350xT86ucOHJ5/Ru9HS01Jm3LmWdzMX+iYhCsFKGwIGlssX6yr0x8uNyGSoy22rKCVtBCkjxiTl0FW4eXwktG1Fp+5GetudfTX6q6ioeOEkqM7t5cP/6o+f+4m/awwmz2Kv5LF45gQoqcVEIUQsKEl1DOoIHklayKTMvyxCkkHtgQPH9pAPjm/zrx597N8+uca19JCVrFAyE3eW0bCU8AwTadgLEwKB1Bt9nwkq5D7jXYJkqDmSS/+amcEk7hiijilOHUpjJpuevLVtD7Izo+4soY2DlgFC3qRXt40mKwlWVFS8aAihuEKsHv13ovnvrsNCFJH8yh2PZz8JZjpBhgEj9Jkmj2Q3BFbrACqzkBU3/IC3Tq6+8a8fffzzHx/f4Uq6g1JaC5QyszK4kx0WQFoqgYaJgbc9p+2cPpfmdKIyXTaFcAdRjJGxIOSpggQmnQ09fMUrjmGo9HqUmJwhNrO1X9zZ0Tzrn4PQpwSOT3ZCr6ioqHgZkNW4NX+wVe97dUszz5wA5ykRXGlcaBBEEmimp6Mn8/bqPp+e3PPvPrzM905vcdcO6bUjCiVNmnoykAxyb8iqR1qj6Qsp4oqRWQm4OCJOgzLxjPTGyjtiCASNZQRYynhyJiKEKORtV4AzjuU7d0pbBLc2dhVZ2+usHcu3pLrAMH5tVzyz/bxaB6yoqHhRcBcyiXvLw9IMv/O7p7dDe+UJ8BwzTKGzjhv5gNsnN/feOPls8cbpZa4u75MWj8pcNzfI4H3GeyP3hrkzy9Oh5ywDVgweVcgKvTvqee3LJghuQwnRy0eZxCXumZx7hEDTxMFfLmPZ8LGZclRlno1geTxduT2JZe3e4LtkNjaQ92Y7JCpOJb6KioqXiASdk3aJb8zBNvvdK6cCXcJqLxEpSssf5LxmQdazKAtjKA6scs+qa/mH6V/7jaN7fProNrfbAzpJaHZCmwid03+OyGj8qzksPudbG0qFsptSPdue5q5bjxvm3aafLkAZRrYV6T3+Z574S5EzD5154Zg9f+zjVd6rqKh4SZA8EKZ7PDg64LjJfD33tCGQyez306JA/KLX0xGYFc2MlcCiGJYqGEy6Dt/bp/eePi2QMKFhQshFZ9G9ZAGmPPhf/pgHv80knyfFLRXk52CcfDKqJEUED0IyI1nmUj5P7z0rSXgUPBQRTOkNdIihnoUVFRUVLwBBjmlDoJv3/PjkIrnLHIeMThqa3jD94jv2SfNvkJsGXvsDxB//szTf+IX/mh/7uf+mP/8NehoenRyy3ytBX0caIB+Tc0+nU0Sgyf0XE9LW+EUR4eLFizs91E+bSTs7DFsO/gf89XAROM+RnCI/gKB0GHQcKCRoQ2O5aVFKLlIiCkw00Az1smylXQGVHRVlRUVFRcWXhyZF1Jy4v8+DfoGpsJ+dJkJHBvviPsCQzyEaISaSPuI0LyBc4NLrv8D+1/5dlj/1H0s//Uksz7F4TD5/Cesj2q7Qc/tom14uAjz6Xy96SOfpOMZjoun3vvANsg7CEGfjVDDU5FyluK6bIX1Gt8wVLZRByUG1noUVFRUVLwDLyZR42jLVSD8NmGdec6Vtlyz3YJaaL3y9h1NyahGDaXydwAVydrIfgy5YzgKTb/wn6L/1X4q8/jOE1RGJc3STKU2+TtLXXy4CnP9vP+mpW5Ikc0EvkOz4C98g6UbhGM6M9zIgWFiTokmZDKOqa2ue7KmehRUVFRUvAKuQ2CegXUY0sFqt2JvOyFFog9P0X5yh6wJE9sCm4Bm3BUhP0EgM+yzjnLjqIE1of/ov0//c35Rm9qeZzo/h3JTWVi8VAcbcH9LElo49Tu0R6l8cAca0sfthNJ1lY8+j1uNBy4xPkaKKzJngVtoYNNazsKKiouJFIDl9I/ikIUpENbLIxShblh2p+eIIcLpKuBjGqvieRvAY6SzTdkdc7PYgTkgziJ/9X0wfveOrf/uvc/xH/4rE1gkvWQIwNovARF9Hmj1mtBzF9osZ2ocUp4AFfYyxU7JinwDF4scMFUdDcT4wrzXAioqKiheBrzEhLTp6cdI0QVTcM+YZi/kxlfxZ5KkA3bo1LBJQi0wtMPXEslkxcWfaBggXaFdXkTf/JrN773rzc78uHe3LRYD+WmAuiYU8IrdTmh9QBPWgGJsZmMamnzyoYsFQL955jQlTQEOxIcqeCNLUs7CioqLiBeCRBGbN3uBhmulXK15v9rCUSTESf0B84pYRaRBXTIzeOrBE0Bk62efrSzicHfNwDy70zoUFdNqzvPX/kI6vOL/4D1+qxjA5/p++7mqOhYxYfGWHolZUVFRUPF9Mf/LPo3/h/5Qwe53sc4LOgID3p9hkH3XZqfkdHR3tZBifdqzkpUuXdv67SjIrKioqKr4UPLj7Hvlf/ReeeYSGc+AB2h6bnEdfAB1VAqyoqKio+FJwbnWIXP0/OP7urxc1SAJmDYEX0x1QCbCioqKi4kvBBTlHJ5ewt3+D7ur//n2aloxhtI8PZq4EWFFRUVHxVcHKhVU+x76ckr/5t/8M7W0k9yTO8SLsCCsBVlRUVFR8KTidrmjiEtcZ+vAD2m/+z646JQDol58GrQRYUVFRUfGlYF8icdbR9j2huUT7wd8jP/wuoae4lFcCrKioqKj4KmJ2MuV01RKmF5B+SfP6IUdv/fdOAKP/0tfzzOeS7Zly1KxoLDDtp/TRIXRo5yjnMfniWXC9OLOwR5uX9LFF+f/Ze7Mnya7kzO/nfs69EbnVvqBQBaAKSwPdjV4JNtnT3IYzQ01LpjHjSPMwZnrXgx71pD9FJr3IxkiT2QxlEm2kGZsRySY53NndQ3Rj3wsooPaqrIzMiLj3HHc9nBtLZi2RQGa2iEYes7SszMq4ceLcc/07/vnn7uU6WY1hGLCUVxdAuoEHci61RzWUg4W7oFKBNwe6oK04fe+jQdjyAYbQtyWyJXJvhOb6ka8f6phlAjlnqGraNrEiEZXIlid6trc80hYjhFCqP+CoCiaGmeEClT16fq5jxFcJjUNsGAen3/boeeBuNaKfDzbPNeZICkbS0mFEXdEsiGZUIS04RA50gyOyDC2kOtDmlj6B4EJrStxjJL4JhmTohwrzlpamlAZ0EAtEHt1tJYoyymO0CqgL5EAlkWRjTIzo4YANgtC6Qh3JucW8QRFUI452jaof8fkxNHb5WiZUVqEuZBwJTsh7C/Q03lBVNW0yqqh4bnB1WgevKup2wZleAmYJ4QFNqtUJ6WD3b1BwC7gpRKHxMU6iHyu8NdKCZgGWwYLjYqykwJ06sJIUF8fUqdOj1zdH6+5N5/+4IjqpsWlItm3r4iiG4gLZhbjHQJ0tbRJ8GWmdZg2W7q2S7v17uPEe6ewFftZlUvYdAFtXkkZcwJPRuAOlBpxgLHj+iblCsiI5omKlmWLbo+q6UPgCA+XZQbwrwA0iTnYjeyaokA+4HaFZIlkieOmOoSgyaSO/C4f7iB+lcmhTi4WI2QizACqIBMZxb4UKqrZCTTFTlEywSOw2fCAwXnBAwco9EBFcBJFS4HxsRpLEOB7sFjZaMhkQAhEVRYKToSuz9+g1Xs1H6XtFGrcQKjyXVwiBgDKOezuFKhW4Yx5I7jhC8ECwQPRIK8NdXGOuyLw7mVQARGF8wHU02tRgKCFP2p4pASG7l/u+iOLygCTByF3BfCulotzwbIz32A80tjV4jVsim4IL0QEPhBzJjPZ0/fEBlyq2PCISu90bu+RuIVkHSgtMRAgB7dY0mtFHqC0jbrg4m3FBO6MEIopKJEhJOs/umCfMMqIRnQdA964ZuaCyd6Fma4GgGfEKT4kGQ9RJn/y+h7P/w8+8Ssy+3+5qE1aqmqAQTIjimGeqAn+LDpBsxS2SJ0wSXhvRjDxKeA6ICS6Lb0E5wfisS7wYVGB9o9482AUdrwp5MyOmnWV1rOt3vxuKuxm1ZDdaWjQKWY3UVUHPAfqjve2RptfOKiqYl7J2rSOTIuf9xWvrDq7loDF5SLImdFWp1w92fdOq40kglbfX0pGyGBcvD+kjwcWVnI3GM4RAjsbYMlhDUmU57e2RsLrF3bFQ9v3UgIydmJV2edEFvJtnMYzujomAOlIr9eiA9+8RkFawJpU6jw7SWb5SjX/B/kjgahgZU/DQkmImWUvGWM1re5qfV6k4LpawDgxUIgxbogt5wf51fzCIO8Vc1AesRByseikVOQLMCth46aiexRcekdWKPTHLJHNSMJq6gFfG6fvRh35uoBSvdidbIpmgFBBUIGhFojQxmK6Vg5ARKyCY9xg0SwQixQZJVprgBBruvf+7nPjmf4/zs22Yvu8AuPHYFn6+QisnNoEcBdOMG7QmRH20B7PkNTFVZAKETGwizccJazJNHFMt6FcVCBiUE7M5KoIrhBUhrlW0Tx5sMdZ6s4+3GR+DmGPazQVQjSzqB2zLqeBmD+pzFUEFtTThx0iyN6VUbHVKcQQLSFLSRsIGhlmCBQYgSqQxxzqD6E7pClJB71hNevxgKeZqUJM2Ej4wSAX8vLMhrgEWFFtv6qZ4sUtGdS6ifQje4i7kEEi6t/mrCeYKIeKmeHbSyOGO4+PF6zvf8kUo9XVdBGIgLkWaSwe8vuMeOorkmw3agpt0Bx1BdnH4bKOAgoWIrgaqozWhzlSd75DC3p6/kCpcA+IBD0LORm6V5mYiNwk+w/K4O3TeUPrywZaC7DU9qnEFdzM+dCSX8IwDEpSOm3w0O6ARYkCCUZ3vozFDKPul8dF9oDf/c//uUlmzseNNJueyZyOBAEQrf2faHXTnuBfvLOyeDogh0ysNZLHcI/dHVI0iN9+Aex/C2rOfbwC0LSF4xKUtjXIJCAFV6R6iRx8hh9WQIAlRxzTRc2dMi7rSWku1gCVWQudxGd45KUEFakFWZM/9pBYawCUhRWBcqAb3MpfSPmPx67NnXDJZM1o1pCrhqRiNHAK9vLdb1tQN0u3h4BVhrDTaFNo2BWpZEAO0jqlR74x1KGuqRljujPUBDllRZCzdenZ9J9W7O7/4vXPOjN3ImvAYsTojNJjtz/qKNoX2DhmCE1SQoZOaBsY1xLiA4u3qHjrY5NQuhd4iHvz+ZVWJIrTSAnXnMYUp7c2CWoyNt3QMNeB45VjPMHfEleB7m79ULdASAY8KaqhVSDtGhgkPvUe/3mHixmoJU86YDeHA19dWgKDYRgJzxKXTKEx80AXPrzWYaRcHTDR1RQptYRwE+k2YY2u2g6AIpJMNYkLVBmgDjBVGhg2N3Cb6FkpowxVUCvsw632wZw7UtYVUIxjuS0i8gw97rPgS6eaPfxDWnv2NzzcFKhCiMdaGLIJIBGtRFTQELC844aQKzSUKYmYlRpWhTx+TXhf/WXSaM0rcMaCmZHKJYWiD+cEqjVoVsiWC9YgSinEuOxyzxST/si3jjBlbi67+S/sAACAASURBVGQFVYJVhTJw2fP8IxHrllAlIK5Eq4gWqKwmhUd7mEUsk9GgmAlKQFEaGyEofsDrm0QxyWAlJioBXHJ5ME0W2pBlWUU9M7IhmgM5G0JEsqEe96xESzo5oBXhRxUi0gl2JCzBguu7CkKAjsFAy5onT1TEA9+/2YxK+uTc0bcaKLom64zfo/fvSozFm5YiqoKEu2E5ozliYW8cYw4VZoZ2YhFDCaEiaU0MPRa62FMq/H76s8TDDnj/uhCJZHcEp9LSQLxYrMUnZOlHooWObUjUOJobPDlkhZ7sYBN22KeQyjNbl7i0tIKMBO1FcivYvVQ8UstFi6GhsCsUO7qbQ+ajDUjGyGg2TLrAmAW0WqG5+te/vnTpX3y+PUCxZZyabBlBUe9jaYwHBQ3ogiNEVENDQDSTUyqiltbJybBUYnmPBkCZdqQvJz4FKyKYjFGxcrAA6ENwQUwQ1W2VzG0XMRRSx8nnEi/1YARCAUMRoN4jRZe7Ry2XuJIJaiCmVFSkXdTkEykemLujrogJZqCeUTnY9TUfFc+eUBR9kjEpYBEdFolQbZiRLibsucQEK6EIYrTGbG/rG9rUGdVMBCoRJCnaCrUpwwUMkk/3SumfWbyucj3E0IPev2kDpIeZIdJVaBQDc4ILi0TIbduWw0lQxEBwVMphtKKiXRRkXoggmdgaVXCyFio+JqFOkV4KjOLeAPag15c8mITOO3VlwCUXICjxhEd7gE1D9IogRWikIghCECVKZLiAwRC2cCCRyQISM2E5oD1BrRMwthkfOdJkQqdeNrFdSKB28XzkJSyMkFyRY4O0FQFhGO9R3X6Ln/XYdwB0Rog4HobgEQmK2xi6WMYiD3qcrCjmomKhReoe9J3QFuhc1A1DEfIc/61e9H0iGVHBOVgVgagiWt7TLJcJB0qcbBf0T9YGDUauHOqMVY4VDT2tQdxrsmioikJPnKwGOdFKIgjoLppVTsCveIPlkSjrKxAU9wNeX8Bdu8NOAYyJ2Zjc+0dTaCAKWgtSl5hEQ0YStJao9xiDz7XOmAgvBsqiFXXpLrwL65Z3YiRFpChdRVBVkhzs+qpC9IBSVL7mjnS6VBVZ6KN4r6hAXQTXIs3PJNwUyY7Hvc2/jYpKUcS6JloHNDGK4y6G9um8v/l1D4Af8PpGUYIoCcUs49qpa610WfUFH2EpBMQCoCRXWoXcxb2VQFygMu7nJUwyWYwsjqlhaoVJcac+eQTbyjhNEb5Y0UiLa7cp92Z/gvcx3SrsRmgh14QgrMsNTo5u/8wBUCdUwnzwfU8fUAy1QPIAltA20YbIsAqEdrEEvOc9ag3QQrRlJEfa8RYwxnYhQXKVcnotH46sTiIj6gu9z/0YdYpkTbRVQiUQJJIlkWnQXXgXitCI07pRWSCMKTSUwZLUewcQEzR38UjTIggRSDh5FyKHVC/RbxpCaskxIhoo2V8t5j8bBZeKE5GipjMneDlPm+0iyOqZsTVkN3Sc6HfqYpWKZe/tPQTQ1GjuaK1OJCRWmADfBT0XcyfGkHJwCi5ImxETWjl4lbhZQl2IXnddvjOBomxNvjjFZUqdAuJC8Ar1iug1kb2vb20lVy/4Mm2qcSI5Z8Qj0RZ7l9LJ0F2LNzuxe+rl2TjoEXJFQLsUkyI+iV5hAu0u6OExhnXJYMMIK01Js4lU9L2eHQUlTL8mvwNlFJVGFRMlWKDKgZiUkJXoEfeWcMSQs5n2+JhRbKad36t90AdZGCK2TKoTMRVV61idtXyWNHh3ikHzz/KU7raSo7iXr0kn+2lHezNDuh+yZXSvJ2BXQo5Er1E3xCtUIHqRp+QFD1ETW+IkZYIAroQQEImo666MiHSBW3fHO/rTE2gG0wNOtbQCwJMv75JMZZf0Qc8CWHmgKxeyeCfrabsg9N7m7zQghdqoPHQUZkDcqS0y0kd7KZaK3Dp2hyZzmxkRwoFXdHdvi2DFckkql2LMtMunWuyhR1QcC6mwE7EDKklkM9T3uj8aBCsJ9S5UHkqR31zTs8h4gZETkXI6vv+kWjxcP+A8S21ok9G60XTGdjKv4Is97Eoj6gGTLvatjgQwT7Q0e96/IqNCxXlLxBCMygMRp7cLAn/hIf+A17fVEepKkgZRSijChSABEV34/FRdZuaEKnctLq1JpmE83b8PY8piGpZCGMWxw7p85fJMQdUrK6gxsrxcw1hIbSKT0Up2G2Jd8Az79vvhk99vB73J3+WueMJ+NMTdef+jiBBjRBTEIuZ7k9m7jnCriJaJGOoNEUFMiZ0hf+TrxTAEN8HcyLkQXCkZbV4ooivzlxnNMaGOVITKIo0ecBqEr5C1LjdwopwTcPNpOsSjxpBMS4llNWY0UQgqpCi4GCHvLUjfhGlWIoXcclLht3Cz3W8ilS5aUSTkUStiCqRwwBSS92k148Fxkykla12cbGGINSUsFHm/ZXCzEkOkFE2QPYogmgpmOKyETp/TaFHQ7tbHEDfctcxNBPVQDnAHXC6q1qUC31HJZt2ciwALz4s/QCtkz0XBmkCzT62xS0L2WmknBtwcVyeZFdviRiMtIkP2ItMviteDXV/VokkgFFUvyTpiv/v94hgJ2XLJbQY8ZyTS5WkaSPtgkJnYQjNEi0ArueEmhBCptKLCaXWIuKLErmCGdZoKAwm7PsjvYiV2rH3GXck5E0K4j5UUEUIIUzDcN3uSUkJESZYK57tHD9CWKjAhp0gyI5uSVAgp0FgmLQjyVk2pk+AGCemk+RVBIFa7K0UwL3VGu6owCRhCWj7g8qdbdArFLnm5HLMwE2Q3eRC1otFxjUiKxFzEBzKlxva2AUI7K8YlFtBU0gc85iLaWbDBVJySR1EAUMQ7YYTiW37g6xuHjrZd3tRMTYDs8mSolXYCAi1x5SFU4gSJiMG42tv6Vm055rkKJMesSyaPhqjCglJVNmceppSNdCXIGiPlg11fHQGtEaVUUvK552k3KyyxUHJZShgijEv8yK14kKM9rm8YBkjlfjFJcWooBRCqChbs32nawxw4zH+uA1/frSIci02YnpS6bNZdhaFCKLF2UHrBCOOK2sqhVl0Y1fZAAJx8IeVgkzsKWBSwwgiVMnhOyOVZbu6O8S2IEnANJVXqIBLVxbq4/gz8JpSn6gyEC1btL00dq6oiOJ2iKOyKYnzU6F+v8EGpMSlWDGZZ/FIjMS44QYzqBnfB1Gi1nBp1lGjFSDEufD0BxEN3Iip5SzlnfCuTEvQ/PFiKY2t5SB51yexd5Rr1kuAaomD2aDMi4wYZJ3QIPnTc2lKZofMk+ntUKbY6Lsa5y000CdAYmrsk3EoX7NVSQ9Rzl2ckTpaMJ6e9M6b/4UHGUYRmZQitoKmkcJAhihT6SBczNNnGhYJqDcuKr2daa4kSkRzop7094FkT7g0SBaNUcRFTdJwQWryqFnsh7tMKHU6nCDXIo0T/jYONU42XxyUU0pSD4zT1QcB1sZfSpFK/19xhw5FRqTWbkxMl0m/2BjCNbhaVrEacjKkiHtFR52nu8vHeBhBzPx/4+i51yQ4jJfgkbzZjoeR9LsqTbHIz9Z5S28L1LdouDAGwkqtZ1ZcpoBb1uQiMTwsWFO0J9XIPi5nctoy9oXGomxoZKnk945ugORBCqZDrth/xjYekgkn5DLdv356CdoyRpaUlYtw/readO3fu9wDdKUbMde8xwNVeOQG7gISiyKR4buIyDWg+FECbSDAjK6XSgQY0dCBaBby1XSywdPQYuBSJcE4GyclysMX+ZDQum1iLCm5yvOyKsy0+4fbCNBgvWahSXfJ2vHDibdzbA1qlUMRBngqlGgTXUuw2BF0M0DJLxtaugK5roch8M5Glv6/r6VMqe+ZhT8Bh4l2XDOYu5rTggBTrSDAhpIgkx7MQshC1hyShjZ9tfpM51gmSVyhe1rhjIKi0UNm7FBJMihPnThYorjDOZOkd6P6NgxKbqmRSS1cwK/S7SV5YqivGOIvfmCCpO7x1rMhea6X3mqWiNhbDpS7zQ/HgSIhgn42Cl+5mHrR9iJvFOfDsaAg4XVF32V0IQmPocoJBrUdoAmo2Xd+ksg3gJ5SldDY4XG8KS7zseGvIkiBVJMcSS69vVKSNFtuAyotCM7vh5gSKwO/AQBAjxjgVUs3newLb6NF9ux+qSpQicVXfhxhgNYLGiCgmYBQ6xd0h7AIGpJ6UQywJtdmJrrQp0wZZmLcxyV8KnWS7VDovyeSewcLBxgAr6qmsuCRZl4rr7rnzrhdUwiHjUoTngVCuJUypAGFv82+rflkbE9yVhICUOJiKLUx0lQm95yUUb13Cc6BCG8PjmN1C/q5YS9n+vfJeF8CXrlKKFhEXJSdMROdIxPv3ckoZoZSkm5RhNNFJOUaQ8aeb79xyOZCqHskyIUSSx6ki2UVJQOTR8yu0pxEA6/6NhhI7s4zvawxb7tuPlS4XKUko1KBZudfiAc8NixJZJZXKPCWmI1TE7rPmQrexN/uSqgoJWuJX7oQuncDUaUl77yYgB2sfVHsggkZBg5eC6dlLmojJwmLYksAkd2c+xdwQ13Jb1PEuBrjzNk1EM4yVGCCllnackGUlrlWEukciY/cyvmWEHJEgZC3F/XEtIre9Lu+cEnM+vueeC4DPqYh3UsL7DX5AUYG6FsGJ5bR3D9AK0R67auPuqYgtHFxKDtojKQItohkXMKQUuvVY3G9fXCuvsDWdd+BO7gxQsNI2J4cDVoEmkCAlQduMICXmtFtiuc6Ke/FaS6K5dQGOBjzjssf5+wgXLStkRvCISChxCF9MgJcDhk4Np3XlyCqN4EbzaVpM7tIbcJ/zBK089Fm7bgMdZJeSaEboPP8JE3A/SnmpGSpdlRUtxYVTSJTS4NXc3/uu5jsPjOZNOcmrEHM5BAUP3YsN68QLIvbAi7oIWvo8oe6lmhKUZ4nQxdB3s3Cfja6axHroWmSpSUnmF5k7tfsjHr8KKKlH5RdeKD668n62Vw82oxiSW2KRahQAxDrb8tlDBKWK1MHahxzmipVqwLqY5UTZuei+BYlI1x9EpMQORSHTktUJudrBIswVwnawSohRUTfy1oiQI30rRffb0Yh2GOlRo0HJ0tJ4A1XJB/S0v3VS7wfBOUX5nAp03hPcbxWo3PtfT7qa49Egh66M2OH4Ig7f5xSG2V6beIPhMxjnknzr/mjTLp9qPp9mhAXz9en3R81R9mUNhV0VlD2g+yxy+IwcjgP2H/6bd2W+etbRo0e3AfleRTD3xQAPl/xw7DSKe8VBmbvmtDVVd3r/jMTR1PD7w4y4LAbFnfG63UVmdzPn0M3PHzhH+Yx91GS6hlP/co93xe9bN/8UAL3ztYeAeDg+7+MQAL/wQ+4z3A8HjN37bPtrG0uMetp+aZfG2ReAYUmjiOw1LjXX/uCB5fp263E9MG7zwIPE3u7zA+/1pzjY8ACAPxyH4xAAD8fnEgDnefXdAsxurGUnwEakB3uqEarbOin7A/uyyDZjvNM4P8jTmYHg/h4kyrUfDr8in2KNZb8PE4/woncByLv1Fg/H4TgEwMOxPwbLFxun/TCK7p8R7B4wt0l7M9mn2miT+ZUC2P5Qs146RRQzPd++7uFA6Pu4gpM5+nS+Ive/x2yOj/axZIfCVKS/h4OEbKe5/dPvu4et4exzBvalVtbhOByHAHg4FhupvdJiD3tP39tVfU5ttccumtP5dOBnvotY1AR0/NFAOPVQP8Octk1B+rg3U4p2fo7iDz6s+CNAVLaJUGWfDhL+kIPOoyVFIg/ej/MU84ymPRTQHY5DADwcPwPwm526K9hT/cId3smcJ/PZXcDZZR/kAT3q8z4MGJyJzMTvj7PN/VvlwUA4U5JtN+CCwy48q3nQ84c5oDu+7gPKCZ7Jg/y9+9dCdqSFCbbP+8sfCcTb//loivlwHI5DADwcBwJ6Dz+jT2Dhs456Cp7zuXZ7pUDnlYsTkHmYwXwQ8Pl9Xp5vp0D94bSw+QPSuycuCrLNgO8GnKc9B33+550ql3aaw1Q8QJ8Wc5jzpZjoWGwCkPMe34Qy7pBZuw9hc/Uj/BFinUcJfsohKW37LN7xsAvvtex4B98OhjOqe96kpMOH+XB8MQEw4KzHEUtW0x9H2srx2KKtYN4nejnWZmkJCJacEAKjPCb2K0j+hbwRDzJE/pD/327EP+t61cUosl0beF+cSHblON4HRNupszylHG2WNjd7723e1f1UHVjXdmny974NOLvmFNs9ve71KvOfYzstOofQC8HPO1Bzel2VE5tbs677wTwYzOp0TylM8+14Mv3sMuc1dwvkCjpJ6p/U1HW6wthzEC/MkbiTz57nPqNP77PMHyLoulPI7vak7/Doy7rK/XHKQ5v6uR4qNaNmSL1U48kJWQhWipybtNRtj3EcMqyF1WGPJgaiNGzaiBV6pY/lFxkAe+0SS0FLjy4PWO7KbGUQjzQyLA+iORpLuR1UkK6Pnn4BH6GH5WY9FBi3nboXbbhisJmHOpHulL7dlfIH0H2Puvw2MYdMKDuZ5ti5R4wwA8E5183Znh847yn5fU6HzX6705Ps1kG2IcsMHMwf4A3O/c3DvMBkE5CKmFspZ4eBN9sRrPvcQbZ7ezMQ9inQqcyBhMw8wnmP0ruyNvMinVlM8QF+3iSf0X2O1t3hdXarbT43J9/hbcuD69/IgxxpIHeHL+m6gux+Px6Ov9eA0CaCJ0JUcjAMLSUdQ2mYMK4ghzFtDDQjMK9wSh9Z9e1q7S8kAI5TS1bDJUMIpf+cjtFKS8uL2kotylGhjFS6B0kE/QJuuN2C38Ny8ortUooCb1ZnstBlhlvbQcjs6uL305Xmjtl2teX9nth2oygyM3zu2jV1mbUmKnNopzOVB3h85hMDujsuTnbY/okHKHNzvU/pOfUO5z7L3Gt2tlIqTljddXSfeHt5RvtNpiuLZzxpt/NQT2ubBzfzGqdUqz0YjHwiDNq5PjK/Hj71gOePHcwdh6Ytjyaz2EGLit+/1tvA0rqOFx3QBvXDuODneOR+xGKFn6mxfqEI3CkFyN2pU6SSQMhGbGDc5tIhxEpD9c+bEd93AJSjTjwTIGS0VaoQSJqJLpgqOo5II+SmqOaSe2kcrYqiGF8cJdnDwO9+4PPpyV5kVtm9FMSd+A9ph+fk94Pp1GXrGsi6b/f6uvjVvOJyUlEz6nbl5HzT4WI0bZv7IQ/JX5Md4BMekZy9fa06M607KErfQdV2iK5yP128/WfHZQI9YQf4RXJumUpu5lDjQcKWh7GpD03W8DkBzo7/m2NS77uA7wDpnXHSneA2+btc7FN32Owao8rcYaJbRKN0bDH3+w4OU0BnJv9UgUmxfp96qz679uH43A3bavHeGExwzXgo+BY1gjttL6OakFYxT4iF7rmsup5+ny/7ve8AOPIR3otY1aI9w0MkdxScqdCvK3SoyLpOY+VGRnTv7e4/r+D3IK9vp0pvOz3lMxCc/FbmMg589rN7d0rfcV3bYa3NKG1aZmGyadnloDs8rInYY86Y+46/VZ1rxTIHQrYjxqcq91+bmeFVkXIteUC6Qhdb20bZzSGvzNv3ybL4zEuaeYM2AxV6CE0RtcytcXkfma1dN4LcDzjaeX02dyJRmRPgyAxMnO1rm81LQfnudzn7Ni93cg1zLwWyZdKYeOKRTRuCTUEqGViGqpJOUOPMdxWbxFC9cwl9tqVI5gSV6fznhS/e7ZtCfXeFi/GynHKoEP08jqA1MRhaRVQzLqVIfswlBj/yMVUo8WlLGawuGyIqLgFovtgAWKUeopEmlArnohWeElGExpUUW6LWYKWSf5Y8k96b7IIK+/kEv4d5fb4zj2xqfOYqvNt2MJsYS7Ny+n9oaa4pLShd/y2/T6QizJqYTwz0g4B74mWESduiIAzbTDI40leqKLTJaRNknFFTaO+oTsrePXzFWE+8TYA6Cv2qgKAb5LnK9s6kJ+GclyKzeJeZU1cyBUFVeaB34uQOnMNcvHQOBTpvuM2+LY4nUloWWQdaZk52iA/oS6jqhA6M2lz+d9wWvqOOk3kJbS6vjQFGbbmHbfaZRz03f5UZ5Tg5RswODHQto7qDSZjbKzv2nXZtoapucSagrl3z8UnccEKT7kyIn7WimgidSixIHuCJH46/36NRZySl/1/uGopP7i0u9EOPoAmsxP7QALl4MjnnPXcT+vxToKkAn1kiZ0OlKg0xxTGBHBuixPLgEXB3GklEQqHzfs4flN2An8/RWbqDwnNmLMNOb23ys+WZNzIFg44yS7kY1F4l2+jAlH1bDEgEghbjvtU4tweGivDR7UQvwukj4b70AHMYJ6dJBRBu3isAeOuec3JNubXhbAydZJ2q0oqYYjgup8teVSxvPdeRZqkW1paE5Z6wNS6v71Xlb6MWw04HhBNPpleVD/bC+cjpI8pyT1EpLqsGmQL1xJOxjr416zINO9owdQeCJjm3N4zNcaH42uS0WRg2hmrpSL/UK/MUYHPsXL9X2nD1us/SprL248QU8EeNc2/k5Ax1LOCYDZZqOHM0MmqdqGUerRWguzsorz2+Kpxe00JDitCvhSqWw0PQ7mAwySOcqGSZAdI8pSAd0K0tlThPFZSleuZmz79mHtrv0x1tEy2Vw4Qx8+RFdx4LthcRPxz//4/aSiPjfhZyCkgIOKUZroszSokkGbWEWUbFC4OHUXUN0L/QAIhmckyk2IAHgiY8GKql8SpiXfBdKXana46o0gkG/AsMftvzwqZpAzvpRpt5iKFzuXJnPIdjIzsdbVWue29oDEbOqDW2mgI8546F4h1SDPI8gOUMg5FhLrx3o/Q0fP9GZtg6S7Vw7njE3FnfLEKme0OjSc6tDWMwLkZ73JbP9tFVJ/gcQO+o6OLOtpSA3Y5jRx+2/6BXw9E14b/7dTi11iOZs1JLJ/QpHpLNp0gw+/yCMErOxtBIBtfvJv76rZZXLmcGY7gzKF5sm2AwdO5uQV3D6hIcXS4LfmfTGXXan5QLuAxbqEMBtzZBFcv9vTWAcZ4ddvoBROHssUTTgeVgDHdHBS7OHxdW+2WTjJrika70ZHqwWV0S6tB1pw8wbJ3HjgrD1nnqlHLqiHJsuYB1L8o2j7JflcNBDHBkSVnrC/1KicGpoxBDOXSIToBVULa3qJm2rBEvIDjxrnHEBKFCpHjZ5fvh+Ps0UmW0wVB1sgquhXURySRPrMQerWfcFAsdiS9GcAju5M+ZCiZu27j7g4Akaem1NSpLpDymh9J4RfQE0iMjaNcg03F6prRBqL7AG28S+5rGWHwWQ5oXTmTzcsJX2ab7aLMzaovhvbtlXFvP3Bk4q0vKzY3EqIHWCrgBvH8jT2NNtzbKa6/eNda34Mpt2BwXQ/38+aNsNX1W+6eo6zWGvsrNq8rmaIut0QbrgzsoDYPhOik1DJtiuN2drbZ4eBVw7D7a8QE86qcYd9cfRUPAnXXnD15uefx44Piq8vSZ0os9WxHviMyHS32axydaqMzhGG4NMj/4acO/+veJnEvIOlH0oKPu3wb4GNiYfZgHiWGUknk5+X29HbOn/9Fk6CvcvOloFCTCUiWs1KVBdAiRzXFFmyuQGkOp+8eRCpwRoyQMxluIj7k3HHBvK/F3H5RDUS9komZag16AZx4Tzh+fVciZ9+ZFCqB/41LgyFLxcFdqZW1JWKqLR75cC1HLobV4nbN4oqgTOmHbtlizt9NDxzSuyCFN+vdmmNAfGzkqJi29bDQqBHMiSmstVVJybcRcDjOuS+BbNBrZqwhmZ/+/9fV15vsD7lUncvz48QP2AA/HYrDb4f3tzLubN0bzMTmAKhbgm0/wHrfOnYFxb+Tc2nBe/iBx7W6h196/2vDmO7NNaXNbtF6GrbbUHpjQYL0gxEqJcZkLj3+ZdvUSYfUCW6tn2Fo6Ab0jSKgKzzreYm10F9u4Sb1xmXzvQ/LWFXJ7nWY8YDTMjIfGRCC6UCK5jws8GsMP3zLOHR/zD1+seeZMRKTQiT4XxPIuZugd+IlAFWCrMX74TsPv/sfEoIvZ9ZnVOZkIhFomqeYz4Nt5Bs6wrc98ZF53OkchS4nRpY6CVXOiCctrgeX+KqE6g/Qfpzr2LKydR1dOEddOEepeibfljIwGyPAubN3gsc0r+OYH0H7EkaV7/OlfXyN382uA199x3l1wI968YtRV8VirAEeWhJNHhN94sebosnJ8RVnpCXWUqRdbhck+thk1PUfpbxdTTWq1zuKXh+NwfH4p0MOxcMznVLlvNwxm86fl2d+LzhkPnGzSxc+cj24lPlnP3Fw3Lt90/vf/2DJqi5FzJoWwYNwZ3n53/Rtb5ecaOLIW6PeXsHCKsPIMfuoFrp/9Evmxi3DyHBw9TlgJsKSE0CF2I+RNJ6+vw+1rpGsfkK++g197E8bvYH4Fl7vIJDKwD6CXoEizOyAPnbAj7tzMDuub8O414ze/NlOQiNrcXZit83z+nADX1zP/8++3NJ2oLcy9/8RrszlPrtd9f+Ixmcb9xi3b/v3h1fsFRA0z3Zw4LAs8/bhQBbj8iRB6JxjFJxmvPo+cfR4efxbOPgEnThGPHoclp64NFNSAkSObkO/eJl2/Cp98hF97DW6/yrHH30Paa9y7cWfX6/32e9tv2gvPws17zgfXx5w8At//do9jK8KZI4GVXuFGm+RUocQ0qyjTPauTnLL5ZM4ugCiH7t/hOATALyggTpSYD8iJ007qL1rk7k3ulITJ+fB25sqtxIc3M3/6SuLKTbhx17nbFkNddTd46ahwY90JnaGe3PST3ffq+BFi7yztypdIj32T9NQ3aJ/6Ms25c+WPjggsAStCXXlRC3ZuTzt22Fqj3Xoc7nwdriXa999A3/7PhMs/JN74CXHzA0IePLLwZAiz+pcAMUKsSjyvroV+Df26JM43GerKuXzTuT2A5b6QWicYXO5qWp/rQOfaXehXTMUeVRQmVcxmJrikFExSQwYjOYxYfgAAIABJREFU48odm4LfPDV/DTgLvPCk8P1f7LPSF+pY5lVHuHiqxOSg0MpVLGu1OTaurRvDttyHfi18chdurBsbm5nHTwbWt0p89uwxYdTC7zVPcnP1m/hT32X89C/il16Ax2r8uCJrjq8qVNDDqCalZjK0I2CwBveepLn5EnLlV5B3f0L1wY+IV/6OcOJ17Pa7n2mvvv52uYdPX4StMfzOH485d1z4p9+uOX8isFzLNF44n1c6YSx0RzqHdzkhelhk+3AcAuAXAeh2+B/baMEiHjDr8qu05GBNaCLLRdRwb8t5/3ritSstf/Ry4sbd4l3c6zyTCCwDT58XLl9xxuvOsR3vP33LE+extYs0p74Nz3yX9KVv0148A+cjfkLgiELfO/5PGKswnvB8ufsaWnEvtzLDexl94Wvo28/ir76Av/5n2Pt/RXXnNbS9gXhRjvV6cGQNnj6nnDkmPHFKOb5a8s2qAHVVAKXkFApBi9+1XCvDJnPxdKCO8PqV1OUvOv/33475X/7Mph7aGHj/RhHuTNY/iNDO8c0TAzztNOFFNGTmtAqVbfc+AX7lG8p3nw/8wrOBo8vKhRM6fY/PMjaGxtqS8uHNzPow88GNzJtXnyL90v9Iu3oM+/J3qJ5dgfM147UIS4ItK9QOURhFZzRNRhRk1N2PEbDh+DPnkEunaN55hvjGM8jbf0m9dJTmyo+3zeP4Mbhzd3dzfvf9sojPXipq1n/952P+65d6PHkqcGSpu29S0jmMEnuuYhEiTQRR8x06fEeHDPdDEDwchwD4cw2CO+nQqTEoeaVFOctMFXpzYHxyN/Py+4m/fafllfedT66WCh5jYBX49gvCuRPCNy5GzhxR3vw4gcDHt5z/5z9tV901p74Ep78GF78HX/5V+MrzcFHhlMBRoV7RskMExi4lSJWZKUi04yB7dEl6gaVkcD4i5yLjc9/GHj+H/90Z7PWThGs/Jo4/pl+POX3ceek54XsvVDx+PJTkWpVpDO7M0QIui7bthZOzf79/PcOfzRJxtVvb2KU+KPcrcYWSXpCzzykihat3Mo1t9/6G3fff/HrkN17s8+TJ/Ul6Wlsqn/OJU4Eb71V8dOc5/vTE/8SNteOEFy+gz63A+R7VkYjXARTGrrPA40SN03G1vhLpLWVwGB8HTiicDNTnL8IT5+Hc08hrTxL6p/Dbr2F3PuK3fyPw+Allc+ycPR44d0xokrO+5VRB+PC28Xs/aO8TH00o0q88B7/7JyOefkz5/rf6nDuuVMGJUTvqvlN7y4zaD3NFwN15QC3Tw3E4DgHwczseRufsbDI6XwnErMvj0lnengM37mV+/H7Dqx9m/s0PEpubs2u0wG/9gnJyVTh3QvnqE4Hf+voSAG983KICw8Y5sTriT15zLn9gNGe+wujML8JX/jH+zX8EXzkOTwucBtYEelriWwmaLegNYbwhDCbyR4GVjhaV5fKdFXCz4jWeDfSOB+TEE4yO/7eEtTPknx6Fq39F4DLnT2/xvRciLz1T8dix/QGSx0/OAHMSI9wYwVufJB4/Fohhh8cxVX12r3GnDkrV5TkeW860W7PrTaqafnjT9g385sdfvLXK77/zL/g7ucQrTzyHv7iKPrcMF2qqfoQQqLuCEr0NGG0KDGBzWD7USqR46ivASoAlo7cCHFXkhMPZHpyOcOLrpGPHSUur8MYyXz8f+ee/fJ2zxwK9SuhHoVeXHNCNUUm1+ORu5heejrx3LfMffpT4u9e3q/1efcv50jPCGx8ZdRzzG1+tuXAykPKsKlDKJcViSoF2BVV9rpbdIQgejkMA/DkCv4V0jsxX5ygUnJTyeqX0UOtsjp0/f2PE375j/Ns/yduM/Fe/JDz7mHDuRODx48LXL9Z85fzMb3n+8dm/k8H1e5mBX+Dqke/Ai/8l/OJvMv76Mbjo9E45rJaJ1iIwctgQWIfRbYUb4JvdhdBiaI+BnwBOAEcNWQ2gSi0ZeQLGvUivXzHs/xZaHyH+ZAVu/oBh+wFX747YGBrDsXPp7N624rh1jvT1ProyGVy9N0v5mD+AGJDSzBMJnavdi8KZo8JSX2i3fHrI6HVe4Hef3//HZmNU8X+88lv8h2O/xJ1v/TL+lR7+XB/ORKoqlCCpCLQKtylftwRud/cEYRBLPiLHgZMOJwL1msGSwIrQLDvUQqoF+k/R9v4LJPR4+TXhj177G377pQ16FQxTqaBTV8JRFczg0hnh/HF44kTmKxcif3ip4c9ezdy+61Ov8M13nGcuCi+/lzm63HJiTaljEcQ4xbNPSYhxvjvVpC+hdzJ3HtCz8XAcjkMA/NwDouxsT9Axh+az2p4xljw0RWitAOArH7b81VvGv9tBYf7KLygnVuEbFyu+9kTkGxfrR87h6HLgxSeP8+b4a3zyzD+G73wf/+ZS8fzOCqO6OBD1ALgF3FW4Sff1IYPb12BwB5oRRGXQX2P1yFk4+TicOALHAn7a6T/ucFRwVepzxrgXyP2M1b/GoKpY+vGY6+uJn3x4mRefNH75ub1ngMYgHFvRbfQmFAozdLEo2N49Yppy0nmBdS1dyTRourzKDuqZ9YOAk2v77/397t/8E/763G+TvvUd7KUKfy7S6wV8AnwbCteV5ka5N6ObG3DrYwb3rsFovVAF9TKD1eOsHjsDJ84Xb+9sgKMGpxRWnOa4UC8pbV8hXmAc/ivIwv/5Zksdf8Q//fomT55SsjnezmhpdUHFOHUkcHIt8OSpyC9/qeX1K4lXLmfev+a8+4HzzvvOpaeEv3ozceao8tIzNdqTafWbulOH1kG212iVQ8A7HIcA+HPn/T16CF1dHLJ5SXXoqpFkh6Z1Xv6g5Q9+0mwDv6eeFL71jPL4ceWZxyJfvRC5eGbxrdwYRt67+zQb534FvvEP8a8twXMK5wvwjUTgltFcEeoPgSu3aD56ndGN9+HuJzC4DaN7pfafBqiWGPSPsLp2HI6egZNPwoVvwJeW4BmBM13JkNPCclA2Q0D01xg0Az7+4YAnRxtEvb1v6762vN16ZuBMv8tLYzu1Nq2DLUUIo12aiVHqmPajsNoXmg2fHlnG3Wu+8VS9r/vl3/zwu/xr/jmjb/0a6UXFXiitrXxTYE1orgu8r/DWJqMPX4Ybl2H9OoONO+V+tJtgCUIN/TVYPcro2GP0Tz8DF16E8yfgAnDB4ZjS9IGnBHHB7SQ+/Ed4c5v//PE6zz72Ck+ccnqxlFxXSl6i4/QqZdyWsnVrS8KXHo9cOhP53gvGX7zR8L/dLTHC9z5w/EnhR++2PHsuIigxQNt29L6BPTC8O6FEOewyfzgOAfDn1wvc3hFh8jUv0thqnLc/yfzxKw3/1w+2g99LzykvXIg8fkx55mzYFfi9+Unmz98+y3v5JdKLv87oK2cLSJ3r8gLH0H/P4I7Au9B88Ldw+ceMPnoT7lxhcO8uDDcgjWd1xGIFdc2gtwzLJ+DkBVavvcto8D3646egFTgfcAU5Kay4MkiwMfhNfP0Or752lVeu/C0vPbP3tQ3KtjxDoeTojcYwarYXCJ8BYZd83RUBKPeoCDVOrgXOn4DbN8rv52vcDxvfk+pzfvzNu0/wOzf+GRu/9Gs0X65pnlN6fSiSXi3fX1dGP30f3vgzuPJTBjc/gq3bMN6CpoHUzt2TmsHSCqtHjjE6fgGuvQE3vgV3XoJNheMOlwo12TwlOAGGF7C7v8rWq1d5+fJVvvz4dZ46Xapgt9Z1nfCuEPlcmb5jy8qodUSUX3qu5icfZP7wrwvX/P5lJwZ48tSYX36+15VVm5WBm3TNyFbigtvisode4OE4BMCfd+9vhoAT4UtXcJ3B0PngRuKPftrye384A7/HHoNfeE55/nzg2xcrvvrE7qnDH79X8drNZxk+8T2aF74BTys8Xspu4Q5XBD4WRpcNfvLv4IMfwZVXYf0jBlv3YLgF7RByZsoLBgGtoOpBbwnWP2YwuMPqeMBo/Osw+nJxH54CjwKnnZXG2bx3BL/+6wxvvcMrVy4DN/a87uPWWd+aBfqUklReUejMSSUamcRcZVZ6Trv6ltPXdkKMcTv73bzkY7/AD+Dfvv2bvPX8r8FXj9FOREgAKzC6q/CawA9fgzf+mMF7P4brb8HgJoyH0I7Bunpz03vSeeYby6ze+ZjRrY/h5lW4dQ3a79N/UqEWuGilessFwzcj+fZLjG5f4er1N3nn2h2ePJVLK6UEEktt2dxR86JF0Ul3cIgBjq4o3/92zXKv5dXLxrsfOG+/5/ygl1lbbvnWxZomGSdWSoBbmR04ssk0T/DB4HdYLPtwHALgzxX4Tbo+mM9azyClVc4Ht1r++LXMH78yo31WVuC7Xw4891jg6bPxU4Hf6x87b984xXp8kY2L32brksA5KXlk7rBZxBSj9xP86PfhnT9hcOU1uPNJodiaYYn7ecMD28FKXVo4bN2FzQ0Go3uQx6yagX61uGJPdNbttLByEfo3noHL3+XqBy/z9rWbPHt2b0buw1uZ9S3ftrEnBUfaXAoHLHVtlfKcFxgmPQeF6b2wLg2lX8/AL8C+V7r/2/ee5c+Xfp32+edpngJOw2iSEX5d4CfAy68weP3/hTf/Ej55GzavwXgTmvbR92PUZ7C5DhvrsHmb1c1bMB4zsn9G/1iErRITrHtK85gzerZi88ovcvf2j3j9k/f5zrPXWe0xVc6Wg4JMC7Q7BQyDOst1+X7pbODUEeXS2ZZ/1dGhL7/hPH8hc/F05uhyoLVZJaJJEfJpf8FJi/pDDejhOATAzzf4+XaT9MAxbSbaeSbZhE9ut/zVW4nXPsq8817xO+oKfvtXA+eOBy6cDPzK871PNbe3Pulxe3SJu499g42nL9CcE1gtgDQSgQ+Bnyb4i99h8Nafwu234eZl2BzAeNzRnomHF7ptizqxHRXDnBtQGGhgtYqw8nypJnOMYv1OAZeAZ7/Oxo1vcmfzVWB9z+t/bzhb9dRRnhUwaoyNLWelP6GZfSrFL1+Ff8tWKNFJ095JXcpJcYH9bvX54xv/gBvP/ALNJWF0pqu2A3AXeBd47Q0Gb/4RvP2X8NGrcO86NPcK7Wm24H6MYTwqOSx5wIAxDAesjjcg/ktYruGFTvF7DNrHhHuXnmLtvW9z9fbL3Lx3m7UzCZGyJm12QihF7dRlWrlIu56OHoXVnnKkD58cz6wsC3fXC8D9wY8zp48kXnpWWeo5a5M+hT7XUX5HgfLtfRvt0MAcjgMZh6VnDwIMF/081/5Hugak5sI4Ge/fSLz9ceYHf53xYsr4ze8EnjkXOXdCefrMp1cgXrlznHvxKwwufpV7jwdGRxSi0Henf9vhMvDOnzP4+Kdw+124fRnu3S0A2GyCNQuMkBWAbEbFQA+uw8dvwXs/ZPDWX8E7DXxMqUwCpUzNGWgvPsWd09/mg9tP7nnNe3UpEzcZeW5zNwnujWfzzz4pPVdM7MTzS7lr/UK51nA8u159AETc38k/YP3iBUanSw7lIMNgEwYfw+CdrbJ27/2wrOVgAn6jBYeRyf1oyr3bHJR7eftyubefvMLorb+k+bADWhHoK81J4d6FwL0nv8Zt+TpvXT2OO9P4Xwwyy1GVrlBDd7rTUDrN93uCUcq8fe3STHl76xb89HLmk7uZti2VYGyu28l9B8i56vCz3x96hYfjEAA/d+D3ME9xIioIOjEGzuYYLt807mz61Igv9eHLF5SzR5VLZwLPPPbpUgbevmrcHj7G1omvsnXuaZqj0F9x+upF1ngD+LiFq6/CjffgzlXYuAuj0S6A7wEubTOC0SZs3IJr78HHrzO48lP4iJJO0RY0Ga0JzVn44LHn+fOPvrQtT++zjCdPRtq0PQ9QOi/u1r3SxHay9ikzVRlO+hJmc9pcev0NG2c4Lv+eAuw+75OP7jzLzdPPMzorsAaDXrc2tyhrdeUV+Pj1soYbt8qaNqPt1dIX35ByD0ejck/vXGFw/T24+gZ8PITrzMrbrAnNaWHrqacZHH+R9+88wbidbzg8KWg97znLVBCjKvRCaZN07ljgX/5qn+99a3Y//vSHxh//tOXOptHkciiBWVPmSXx2PjVi+1N1GAc8HAcAgNpxENNGlnscGSe0EY+G5iFL2sMRQh6TQyB5JLagktHckM1ooqDZCT9H7eB9R1sj3/k476B/AO5uZi5fN/7TD2dG7jtfUc4dDxxbUb58/tPL70ctJM7SnPkS49OrcBRYKeG/eh3qT6B/9TW4+RGs34bhxNB+SvCbB8HUQDMoYo1bH7J67S24mgsAjjpechlGx4TRY0/xRnuBVz/ae27dg0BUtShqhVLkOmW6xrila32by+tGrTNOsL5l3LhnXL9nbM5RqplZvGBjaHsG7Luji2ycvMTgGAxWmfUouglcGcEnb8GtD8saNoOypvZZ3nTiDY6Kinf9Gtx8n9HHb9J83DmVTqFfj0E+26M58xy3R+dZH85ab5VSckKyuWYOMqvnOVHRmsOpNeWp05FffC5SVbPff3LbuLtpjFufCV66fowyEZj67g+Th2P/R3RlpEIvQ1+ExjJ9UYJBz5XgkKRmNR0jayZmCGIkj5i0e37/yX6yub0+wSgzQ0T29FVacM2+1MymP+Sc9+UDlI7QgYSQXbC5B0VjKeArQVGJqJTKyu5O9p+PXJ/74oG+/Rw7EcCELgFbtSQHv3Mtcf2ebTO6pUi08u1Lny337MptZd3P0Zx4uqvWMmfJ1ynU5NW34O51GN6FdqtDkj1YeMslBtVswuZNuPkB/evv0r/r9Mc+pUEHazA4scZm/zTrw70lw79ztWWpvt9jM4PjK0IIwp3N/P+x92ZBkiXXmd533O+NPffMysza967qDd0NNBoAIYAAAS5jAxLgGGdkI1GSyWSSmUwaszG96EE2b5rR25jpQRqazKQxSRQ5Gg7FEReBWEiiATS23tfq7tqXrC33jMyIuIsfPfi9ETciM6urq6o5qEY4LFGZ0ZEZEX79+u/nnP/8P0nqaf1SkBtJUi/oHCXK4kbK0kbK2YWU5mb/ZpzHM6ub2t3w7/n9rp/ixljdC7jW8lwtsAJcP+ej8c3bfg7jjp/Te78g/prGHWit0Fy5CjfeoX3V+dfLGaTjwBQk08dY11muLduebReeKVs0JO3VrzNHB+PT+al6puyJ+YB6vfcuNjuwvKkeAOmdDjUXgWdnFmjxIDkcH+1IiXEmIdGERByJcbTF0ZaULUmITZvItOjoFql0cJKSqhJIQMiD6481BWPINE0LSkF6X1+DgGhEhCAIul/3O5xRUhGctaTW0BFHbJU09FrKCTEpMYlzOATjAkRNL8/ycUqHDrhs5wio9KI/53zjeztSrq+mLKxqvmVx5KBwYtZQuo/g6MpyQCeYIR2d9uDXKPzHFeDGebh1CdYWfTgQ32ukMRAFOgdxTKO94ckbt6/6nrZ8Hy9lG/8oRNVJFjfub+0tNZVK0LflAzA2Bkf2eGWTdgydRAkDLzLQjhXNtCiNhVbH0YpgedPx0/fSPr3Voolt9ADooFc7+0lzm6n8fcf4OVq+Aus3YWvNd4/n83m/1yROPKCu36a5eA6uv0N7SWi7jAzV8NfDjY8Th9M024YkLUZqZIeH7HbNQTCfI+NrhSXrI+t9U5ZHj0j3o3ViaHYcW5FmrSjZaT/n5GpBFnAIev+OUoJgrfUAgcEEIWINLjSkoSEIDBIIqdGMQp2vL+nt4/cDwIUgrJiVFBGsffAKTEGSJIgYEpcgzgt83N8fjCEyiAWJhcQkGEkJVQg0oVXOnKAld4QGi6AiH1OuV157kj6vl7z+5NXxldUtx41lx9tn/F1vA+9jNzVqqJXvfWFttixxMEo64gGnVMlMWF222d6+THtlwUcb8ZbvK7vfzTYnxSQRzXaTRnOF9vptKi36ewlKPh27Xp3g3YUy0PzwkdTNhIXllHM3E148ux2Z1jdgcc11DyAbbSVx0t2E0ywDEiXKekvZ6igLy46rC9t33/x8u9x0hBYOTluiRLtu6B9mXHDzJDVoBIVPnQCbeMJLc8X3XibRXZBe7jY1HXtmaHMFVhZoLl+hsf5475pkPlrJiBCbUVJnPPFFfC0wSYsifj1xgS6hywiBy9iizmuzPrrf8sqZhLQFk+Pw//4k4YkDIZUQGhXpydJJ182p2w7hnPh9ZIhLf5sQiMVCLJiOIRTjo7GsVysgRJ1AolhMBlAu832U+75YRZBzzmGMIc9Seqx6sKshCMPQu2qTYsSi99ntlFYMLvD5EBuEqHWk4rBOSF1INYVADB2jfmKd177UjBL5cVzsXd2XQs9flwEnvnPg9nqP/AJQrcOJeUOlJJycv7fo6OzNhE5aIQ1DqEBQKvRYqceb9tpt2LxNs7VaqDM9gKOIUw+m8RbNzTUarWXa7RjUw0gTfC2wCptBnTPXS7xyIeXpI3d/Avvhux0u3065vpLyyvmUN8/19wGiPnP47oIy8nbEWMMwUTPUyh64QHDqsmZsZStS1jaV//0vY6Idyhl5l8JP3u/w+VPeAPbmasrsuP1QIHhpcZJmMEI10O00tM6676dsb0CSH0geRCiURZGdDrRXYGMR1hdptn1HTBHl0zIkUvaHs8JBjb5znGQycv4xg2eCqgWbQpIK1RIcnTNMTQitlvLCm8qhOdjqKElWd62W8nRUng0pLtHepjoEwb+lFGjiBSKStRRp+dOIisNo6kX6Y4Naf9+YOEAQn9gy+kCKtsvLy92ILwgCqtXqA8lMdpNeKyvbI0BVSCVF1dx3BJiIkrYcNlJs6vNGqSgpgksgcKCRoIn4KFAciC+CG5G+GsPDOnYyns0xJ6eOu8waJlUhSR2LaynXlrSQ0oOphhAYoRze2+0fxVl9hewYXdhs2yI+4og2acYtr+/p9MExDxw+ZHApuA7NqEkj6tB0pb68olgHErLZEZK73Ojfv57w8oWIF95JOH/DR3eXFhSXwgHTzyJMUzh3Rbm+lHLjZsLf/2pAteQQ429dVZc5l/sN+Q/+Mu22UOx2K/yLP4lJf1NZ3XTMjHoPvUMzwV0rxKTOIkawxlExQjO/MIpPQcdtD3zE2Rw+wGviEt/X2WlBpwkpNF0vM56I4ALBGqFWdsSxYkteKKAr1Zf7+GUgKD3MwmbrNU5SFMOeMcuJfQlLK0q7BS4Vrq+mzI9btjpKpZSpwuxQNx+C3r+LvUuwWFxmqmxclj1Q9dF4BKYkqChBYhCTCRjjcCI9Z+N7jciCoFvzyyPAYnr0QadBA2MMgRgQxWiAu08iSm0FOkmEVaglQidQYulQJiRNDUkpU3+IHWmQopJ6JwTEy/I/cL2Nv+UFNJAi0oF+JpUeCEr2BFVY3XKcu9Kr/+2Z8JY8jYrcx2L2qSTBYVOwbjsaN4MKBGWfczUfwVFbTFfnrTkAwuJc5iqfUi8rI3fxWV+7FPPNVzv89N2E9y54wM5bGbQ/5O6O1TVYXfMP/uFfJt1aR6nk56idwFZmLmu4c2/QPrw82P/8xwnPPpESGOGrT4dEKRzZYxmpfHC6uh0bklQJ1LNPxDk0TzOppStOKvajWaBYr29Gz4a9mS2YOMsBGeMoBx6FUpctDVPwuNzpoJct/sBAKTPCrVcMS5s+4gsAg/L8WwkHJi0jVYs6wCpOpS/6Y/B7hm4RfysJUJvJAjq/WVksLlEvf+dSrBoclsjFiDXZ3u0jdVG5X/zrgp+/3v2dCR9JDdA5hxof0ro0ue8IMK1UiU1KGilhWiYmIZISYkNSKthWCxsYYvW5Zc1SUFJwtP54nKR2AL/MgBX1J+WcXBAlsBk50hSmZuDakm/sbra020x8T4eRkhAah3VtTEd7jq75qAOjM1CfhMqoZ2yaHALuM+wweG+noJRphTYgrHavbxf82g6SFluRY2XT3XEB/ORsxJ++GPHCWwnXb+idU84D/3nwWJdkLYu5z4CFD81h+9kbvrD4zsUOc9MR/+g3a0w2DE8dKrjU3044PNOfwnlk/jaV2xtolELiY80uCJYqUK5BWIGg4ucw7jyYKNCIlxUqVaHSgOrIdi0o5yfEpgnWFNbpwAFPCn9SexyWLML1IKhArSy0E3+2rVR8QBsnSpw6VIPsWkkRi7eBXn/qdTg+yqEa+9KASzFiULE448A4NAhI05gUhyNFjUXxuoJGDaJCbyO/19f3oOexIQsICpHg/WYIB2uIJi8yGmP6ws17zrKYhHKshAJR2EFMSllK4ARLB6wh0YQgzIubWbLJGtQ+vNHf4CbBDjdyLrjs6Akui4HEKTeWHZubsHTb9ybHqdJJIbgPZuyhGUujoti0iWnhG9+jwpscA2YOwvheaExAueLFre97HWSW9qYMQcMD7PicZ3wEGfgBtBzSTJHWKhvtlNcu7nz9O7Hy1292+L++3+H5NxIWrvc0JNOdsrYf8h4J7gH8BiPMM+eU//Kfb/Kd1xNePB9zZcl/lhz8WlHvTVnjOCQLsBFDK+2SjsQ4GKvC+AzUJiBs+OjcfFBcerfXpOQPIdUxGJmDsf00agMLtg1hRylrMwMz3b6eC8d8HVjz6iBKFJut25GMwDUxnj9XspSzjxY8G7RnSrzLcWY7IA7HRzKcGJwTrIQZUS9CTILgMIkjtRbjhLIGqApxLoQgSmru/5RW7PkbfGynNob77QMcaoF+RFHfbgfwvN4UBL06R+qUViE6KwtMNITZMUOc3t8dP1lXKlsrmLXbsDEPm9AuZ8fpUWjsOQkzx2jeeg9WL0PYLDAP73kV+427VIeRSdhzkMbccZgCQr+wN2OHrKfIagIbt9nqxLx5NeXPX+mwZ0R49niJG6sp528mnLmW8M1XEs5ddWw1e4eITgYL5bsoXeYL/T8CvgP8pwXQi/Dky3+NF2Jp3cd8/4t/2+J//Uv4x98o8diBkKmGYXLEcPl2wieP9mBWtm5jVtrIukEiQUtQLxsYdzB7DKbfobl4ETZrvl7nkvtj55osGi8XfVpqAAAgAElEQVTVYXwPTB+hsec4TPhziUdpCDehuu6YCpYIrcv6dAuHNi3osmTWUX0Hv1zRRZQ0zQQGYridcQ9KAbgMBAPjdVeNkV0PLnm0WfQIHKZCh+OBpUCHU/C3A4L5/eqKCjC5mLAIWgh+RgJoth/MabdWTgjTW5jFy6SL80RNfMNz3vg8D8yepLHwBs3FC9DcgKjjKfP3lHczfrMNK1Abh8l5GvMnYP9pKtMphNBxSqOZ0lpNMDfWiVeu0Gy1uLKY8O3XHLWysNxUSoHyrdc7/Ogd5cptT3IJC5nc6g7ZO3eHRX01+/ef7vLf/5vs36/j/WP/5B7n/FYb/tmfRLg44u9+2vLciTKbHUWIeeaob/i/vXiN9YUNoiVLYxV0qgQlKE8K7T2nYP59GrfP01y/7d0fks59MHSzaLxcgfo4TB+kse8RKgf2woz6/K/6U0CwqpSXLlNjiVKg3dbcomt7l84s/SCVr/8wEJLY9/n6n33KOTR4GyoVAisExpOUdqslwu6ANwTB4RgC4MMBid5tQHr7RpFIANBOlKSwAcQubyqmdzq+x3Fkj/LqtQVKS2exN56G1YqPxKpQGVGYBfaepH3lMI3FczQ3V73RahzfmxxaHv1VGjAxTWPmEMw+QmUvMAEl6yBKiVZT6rc6dC6+jVk8z8bmFguxsrbhe33euJhQDuGVc8pW4uesige/MMPvBCiPZxKkif9KEqg5/5x8fKcGp/LU710Etn8C/Mt7BMAAb3YRtXwk+VdvpfzZy1scnDGcvxXQjmGibri+WSa+9Brm6DO4eaBhKAkwbqjMV+DWado336OxdpNme8P3791TFJilPktlaIzC5D4aM0dh32OwD0oT2WSmEK4ptdtK+eYFAr2BNWk3bcmApN9gnVULj5lsDft0by/TUQ0907tcgtGadJnJRuhJrA2AaZ9e6ADYDsdwDAHw52wMRm1dgd+BE21R3ldQpkZ3SDUp9y25dXqf8O23FqitnqG2cJ7S4qMwpbRLmfLHLLC/TOXqI7SXLtJortNstzyTIdoCd7cCzFndr1SBygiMTsP0YZpzj9LY+xjMQ2nCRzCl9QS51SG+2CK4+Crp6nnidoe1ltI0EKnSTj1HJq8xVAvA186+IuDTk552Xy9LN7r+2dspcdwDwVNVfJST3D0I/icB/Hf3kQUuZV/tNX+d37/iWLgSsbSSMDNmeP36FVpzlpgjBLeuwqRFRgUdM5T2AouPw433aa8uQHPVAyCxVzC4axDMwK9Sg0YDxqZh9gjMnYK9c7BX/RrIwK96W6lcu0V16S3GR68w0fBRmsgAw1Z2X++BEaLUiwwYUUIrxImvDZrsUHd4xjBes12d0e12Kf7BD+oKHkaBwzEEwIcgHSp4rcOc9dmVRcsbQAw8czjkmzMpMcLymtea3OwoneT+86AzI8vUV9+mce0MozdPsbjHwAgwkkUA+4Fjn4Lmddrtpk+BOqC17I/yUXLn9JvJNtqg7MGvNgmT+2H/k3DkGTgkdKYVrKPUSeBWRHh5C3P2dYJrL+M2rqJp4o3NU1/bgx4zs5JtlMb62qkRmK3BNz4X8ImDIZEzrG0pSxvK7/1xiz17hetFJZewsMvG9Kifd7orwp2f81//TsjUiKESeqHxG6uOi7cdb15IqVSFrQ7cuqEkUQ8Ii+naH76SzaP5C9rzT1H56xbuS08RTFu0ZJCawHTgGxpvPQOrN2lsrNLc3Mx6A23Pn3FXIMxJSFndr1bz6eiJg7D3CZr7n6axz4eqkYVSC8IlKF9X6lfeYqT1GkeO3qIa9tRe8lK0SH/qM/+mm8bMADNVZaRqub6S0GqrZ5ZaD4DW9FSgQmt60aQU2isGIr/hGI4hAD7EQIj6U3B+mnaaS58qldCwfzpgYlQ4c05JQmi2lSjxlPH7HafnW1xafJflm2/QvPRp1uf20850QaMSlGaBYyG0P++bsDXxO9lyCdoW2OrVoFz/PuvN4bJWh9IIjEzD9F449jSc/iXkxBE290Jj1CFxCmsRwcIWvLsKZ36EufE6Gi8iPUVIavj0YTX72VhPoR8dEcYawlgdvvJkwOEZyxcfrdCOfVP6t17v8Pd/rcL33uk5pV+dk20geCaGS8Bm4j9CFTgGHC+CXxmujsD+W72P+7t/J2B6xDA3bqhXDGmqjNWEsapwfC5gfUsJLWxGKS+ccVy+eIdr59pw9VXa+z5B/dwKuieEhoWqgTHxh5KNQ9D8jNcEbbfgtoWtZYg2/HVKXK/w2XdNDAShr8NWGjAxC5OH4ciz8MgX4eQemgeAqveEpAncgvqVVcauvcrRxpucmNvqgl/sehxU2YGvkqf1BUicQ1VoRcpoVXn7WkouvrG2DHvGQVS7vZtdBZ38n2JfxXAMxxAAH1LQyw+10t/LlNO+i5tHtSSUAmHvNJw558tvUSJ5//h9j5GqcmzPLW5eepWNsy+zMbeXaMLQHhcqDSWaBA4B8R5wv0zDGJph1euxrVZgcylzJYj6ZblEQPK+slEY2QOz++HgKTjxBeTkEx5Z5pxnO0SRZ4hcWKO0CaWoTat5Nqs19s9JTosPAmjUYXpc2Dtl2TMmPH0k4KnDAcfn/PKthMLcuKUcehPW0arF5KgQawZ+vZDyssKfJ/DjDDtOAX8XOB5kaJiHngP5uTDwr9WoGMbrhsDCeN0y1fApv80I5sYNSZryyHzCKxcTzl9zXLulrKxuvy5jZ/+YztRJ7IHfhfeWYTwgngyIywadEjhhoPk0zU7Tz3VtAhYvwcYtL5UWtUDTfjQSASlBqeafPzEHs8dh72PwyOfh9BF4BJjzT2+LUFpTyteUkXdfZa79Kl968jKNimY+iXRFE3IZv2L9zxVqgy7LbFgDtZLh1lrKm5dj2gVj4WoJxhv+j1VC6fph9h8Ytyc/h9HgcDwUACgiSLaiJVOySHxBAGstSRIhJu/wV3C5XJdvplTzcDf6FAGvf/vMFBMGzraqUAqFWkl47mTIK+9GdFZhYUW5seruW5gA4NisJU5SFtbeZ+Pmj2m+u5/W1NMwlrVElIC9ed5xH1R+ExqzPpq7Pg4rC16fMsk23Jyxag3Ysge/0VmYPgoHTsGhZ+DoI3BK4aRDnI/+opWY8OIG+sZ5uLZA+8Xfg6S94xw2Aqg1YKwhzIzB8fmAI3ssj+wzfO7kzva0VgyCUi2Z3sKOsu05zLx7DNRVKeN9YleBI/mNEA6CXyGLGvayvaHNgVDQqjBa9Wa6mx3vMTg3HjA3bnlsf8DVpZSzN1Jeu5Byc0U5OxAVln/yPxDsOQzTv0N8/goyGaJzhk7FwgGFSEC/4Hv3JvbBwnuweMH7+m2uZ/OXy9hkBxJTzRi4e2HuOOx/HA4+DScPwKPAQf/aTaCxDOE1ZezMOcav/pSvnniZg9NJd+EmrtfTp7rzOu9Xh/GyadWScHPN66v2gX5DmBk1WOOd44t9hCKDBJidD5bD8dEN6wyqKRhFjZcBMiIYBHFChO8Z19QRBAFRkmKCgFSzfj2NH6rP+8AB0JF2t32r3gJJcagD1dT3FWkuoSoovebGj4MUWl+aaBcszyNDl6FhaGCkYtgzarpZrFThzDVHyT6YW/7UPsNS8xa33voZW+dmicf3EtfnaBt8us1CZa9CBdqNOo2RL9McnYfJA3DrAmws+4gjjXv528B60kttAib2w/yjcOQpOFpCDjjY59Ndkqa03o9oLLZJ39jwvYjf+adosrP7g7UwPgZzU8K+aR/xndoXcnI+YLJhPjjXnE2iUXoyL/QiwdT6jO2IQCvxNcaGpdcRn4Nf1gSueBJlN8tohCSFUmiwAuVQiBIlMI524mW9UgejNcOTI4YnD5V47kTCO9cSLj7ieHfBcfaasrHs32t46Xni1+fhy1+Dc2eIqyESBDRLBj0JUjHo5FM+ktvzKlw/AytXYWMFWm3ft6mpl04rlaBa8ySkmWNw4Ek49KgHvSO9yI8ONC7BaFOZemODqXd/yK/Pf5dnj95C8J/BR3xdocIu6aS4voUuZ4XEeWcMkcx5KWGbqHg58L9fLUkXWHvqMrJj5mQY/f3tjThIfRO68XqfjrzXU3GaUg0qIN7j1dlczcMh6hDMQ2di/JFEgOqlRf0mlEWCqpqlTvwGJuoywOzlU0TcL4QNdM7wTFLvTacIpUA5OBNweK+wtKpYfCvE4vqDM4n6pUdSLi+/x9bV75O8OUNa+S2wdaKgEAFOQqWktGtCY+I0zQOnYem632w7WxC3aZCCsTRNxQNgfQTG9sFcAAdAZh1M5FItDrmUYJZi5NIW7qUfYX72B+jW5V3f5+QEHJoTvv5cCWuFZ4+G7J/6kEtVssUVF1e6B8FNIySilEs9kk3ZbAc/ol7NNpch7DlaafcMYI0HxdAanHoVn3YENgODRsVQLYU0KoZnj8GbV2L+1MZcqRluXk1pvf4HBKU9MD6LzCtSrdGqhoi1EAh6WJGKQ6cbsO/zcPNZWF2ArSXobNGI427drBlWoD7mo/epOQ94sz6gZ7IwPwswuqTMXEuZfeOHfJrv8oWTb1IOHepkG/gM/tv3vRYJXz7N306Us9djlpvap2FTCaFe8RqhMsAAlYG0ySDwDdOiH/0oK6gTxFkwft/GCGqUxMZdrc5EXWZ07qUsDY4AiB+yGP2BA6Bv6PYKD+K0mxLZHg15vUmTR4kFx96P89j5ZOtPWRM14fCc4ZW3U2rARgvevpbwy489uNf/zLENWtFrpLcbuFdKWL7Cko4RR0I068t5UQMqRx3tPUKjCc32PLTnaeRSapnLR6MEzZL2UGREkbpmqtvio78Nh2wkyEJE/MJ7RNcvMbL87q7vb3QUfvcrIXsnLHNjhgNTAbPj95AHztNpyfb4e9XAuvW0m1opuwnMdvDr0lEzPzyRHhAGgWRu6P7QF2ZyXnHqXyMyeX+i9xpUlFpZMManpI/sSVndTLmZI8jNF4heHqf9md+m9GYbU62gR0N0PLNZn8cfKvYDWyVoHYb2YX9NYum/JpWsabLu2xwaNZ/ereT34ArU15Txa465n7zI6eU/51cef4FaOeqaNaeu1/FQBL+ioEN+dPWa3uqJpwbakZKkyqVFx/dfdLSzqfzK44a9U5bZMUs5Y5imhf4/sp7Bu2l9HYLfRzPauTuPM4jTrvamqO9VrtkSiXM+1jMGcZJv+j5ylIdLzPmBA2AgQXZj5JPnEyj5KTARP1k2e1xEsj5c9aD58YE6D2xsD2rzlJIxdNshjCi1imHfpKFaTWm14I2LymdPOK4uJR8+AtplHNkjJG6JevmH/OCyg7SFbX2ZlfW9hJsQz/vWiMhAZVRpjwmN4ifIvVmNXz0NfC2pq++ZmchJJ0Vu+ehP3kyp/uBH2Jf+Ne3Vc0iyuuv7+y++VuLglGV2zBBYuTfwyybZFN9zYayXYEmUdmaJZER6TymAXx5YiYDJUtHesisj6WQvJFkIZIwvI4oItbLn/WikxIn/GxMNQyf2dcKtrF6YL/jk6s8o33gXjEGvnaAafp7NOEEOW5gRNDRITT1FNht1YyBSKh3nP2MA7Yp0TXZzi6OKKiX10XB4E8LzMPp2xMwr3+LxlT/jy4ef5/D0ij+AqnQ9AE2h5UH6/Zz7sxmoVzPCy/pFifLi+YgLN3tZnpqFyRFhog57JwICI70UajavfYdEdo/+huD30Q1rSt2mTVHj221UERRxjtRFmXer9T1L1mQWN7qzCv0vGgB2bwwRHGkGaj1WV4rLfMUMRgvq3B+7RS27LgZremtFc+1EFcoBHJ0NmJ6MaV3zp+bvvun49MmIxAmHZx7M6erEnHBibpkoeYEXr7cpvbJO2Pwya5sn6TSFeE5hWnwUUQhX2yJUrJfOaneP/4oUdkSJUlhXZEmRCwn1t1qUX/4OwWvfxC69SnnzCneq89bLnlwyUvVgcTcjybwVucOmWQTBpoUt67Oj5cBv3J28NzPNvmLfDZLXuIpRiTGeBGMkF+gtHHgkFy/wjNRKaLNoSUgVbq0l/ORsxNK6bwXorRKFZJ3K6/8bbvGXSSRE219is+3QpkGmFMZSNOgV4STNf6bg9+g3okaGThVVSrEQrivV21A+q9TfXmHq3W/zXPJnfOmxnzDVWEXV1++UgjFt4aC2E/glmU1SaKVrvYUKN9ZSXr+Y8PyLzp+XBD572nBwxvL4wZB6WXop0+wAbIag9vNRntmKfTQfGrAOZxIMngRjjNI2MSIhBiHtOGzqTa5EAlJS+jQdfxEBsONaSGARY3BoFha7LtBZa9HUQZLiEIwan0vOosGPY7pzJ1JMvmmqKsaoN2h1cGpvwCdPWq5cSwmBc9eV/+U7Ed94FuZ/qXrP5rg7jU8dXqZsf8qL19aovHGb6soXWL/1SbYO1khnldakEI5BXDZEKJVCE1hFHJ1so5fYQcchGwqrSn1ZKV9JCc+8S+WdHxBe+h6ljddJ7XWisEM73p0gtNlR2rFnVSapTyF+GLf1bSnQQRAMM6szCkLLH/LcKnjmrpHdfy+3VnTZhW/HyrkbCT89F/PugmO97SPDPFKd2Wux1rKxsYBZ/SHpaxGt9VvUb36e5snj6F5lcwpkVKAmaNnQFGg4pY1QMXhCE3gWtkI5UUqbUFt11G8rlQuOQ+//FHfpR3x56jt8+Yk3max3SJ0SBuLbPF0P5GUg2sqvmcNHt3Hq2xqs8YcQESFy8PKFmOur/slbwPwY7J8yPL7fMj1isJnlodM8/d9bVw/AkGY47gcAK6EvBI5ZtKqkNiDP2eG8vVqQWmzH0l7agghckoIzpE7vW7nqoQdAWzGE5RATBqTiSLUHfoIiiaJJBnqJIk66dZRtKPExiweN9BQ1chCMEyiZ3s8zo4YvP17mlbNbvHvFExhffFeZn4j57CMlTsw/uEt2er9wev8GpZ+9yktXlqidv8bK0jmal5+kuf8RSrMlOlPG15LKAhkQRWRH/9QhkWISBxsptSUo33BUr71H/dJb1BZeobbxIiU5hxlZY6qRcPYarKa+BWGny/373435z/+OlzebG7NcX3EcusfIV3dZ7SWPgwSFaKdr5p6/VLhbDTFLiXaXq2SMr97nyXs9k1Rpx575uNFSXroQc2Yh5fqqsrrpn3/wsI/8V9dSOu2U6VFDqXQZdS3ii9fYWjvH5uXnaB9+jMreg3RmYXNSkKqvDW4aoW4MnTwwFIFEKbdT6puO+pLSuJVybOFNTq6+wNHwBfadepPjs4s0KkpakNvTAgj1tSMUpPuc+jRnkgrWaGbXJaSpEqXK+Vspb152/CCP/oCJUeFTxy0H9ljGar2Dg1JosDcDB0S2Wy0Nx0c/TNSG0GBsSloBZ6KMyR+iiRCHCeW0RJmUSDoEGmAwWCu4h/DzBr0b98HU32wnoHM8BdfGSJVEI8oqRBJiSHyBtVVCL3awlHGSYlxKHAaEieDk4wKAuuNPxmSKMPTkoJwDEX96KofCZ06G/PZnS/zzKxGdbLO+eEt580qMU/+843PhA3unv/FUQjs+z8WlVeorF1hZfoXK+0dYHT0EI/tIxmdhdAKqFY/WBohTpBMRtbaoNzeQ1SXCtRtMLF/meOcsnY0LjJrzjE/cYn68zWP7DdaE/HAs5o0LjktXfIRHf5aV6zeVP3w+4tQBw+n9NnNPuDMA7mgaPGiRVFB4GS3BuBFuO+3iWykvchnBF8yyV826etzA5twHGoWRA0SUtdKVLHRiuHQ74fVLKTfWlKUN/9yxGly/oZTLUC0Ljx0XRmvCRjtGuM1mvEVz8xbNd99j9dIJgskTrE8fQCbm2RqbQusNaFTYKhnqgKaCJI5ye4va+jp7Vq5zcP0yn7Rv88zUy8wfeY96eZXRStJtw3GpZj5p2fq0/eBXBKVcCMgruChWxIteq9JOlGbb8fL5mB+/neCAFWAkhN98NmDfpGWi5uu6eb3P5LOpPQbprinsvszJcHxkw1aR9hZxuUyqMZUkIDKCzWq9JrGESUJcs4Stsg9cTEjiNhEt3/dpZWxsrBsMqSpra2s9Ik7m53c/Y2Ji4qONAIdj1/ijGy10ewABG2T+gEa6h5BSKPzS6ZDLv5Lwb77r6ACvn1e++0ZMO/byW/DgQLAcwpMHEyYbt7i+fJsb629yYXGU5PIEUTRBJ5gkqU4TVCZ9j5kJieOEcGsD2Voh2LpNJVpmT2mZEzMrnJrfJJnscGw2ZmpEOLqnxGjV0IqVA9OWmdGI5iPKH/1VQpy1KeTC12Xg8oKiOOJUCY1QC4VjH/azakEhrAB+lGBchAmnlFNop9nzjPbCwEw2zRSupGYXTNW3PFgj3Rpe/rjP7HtyS15bXNxIeflCxO9/L6KVwq1l//y5Kbh5SxmpC8f2CnvGhF97ukS9bFjdcnRi5d2FDa6vbLHRvkal+RLVxVHqb0/StDPU63Nsjc2jo1Ns1etedCKO+VWzyKdGrjBXvspY+SajcyscmV6iHKZZulHRLGLLa4daSNnuBDR99WpHF8Q2264buTVbjitLjrevxNy8qWxmU/7VTxhO7Q0YrxnKYQFYnf8mjzDzWuPQ8HY4HuoU6HDcIQ2aAZ/PVOX04l4typie5vT0iOXzp8ucv97hlbd9OunbL6ZMjcB4zTdeiwjHZh/MJXx0f8ij++HcjYQb602mrq7yzZfOc/Os4lLFzuwntiOEQQ0rgsYpuE1qbFKzmxyd7fDYfsej+4XJuuHgtGVyJOwSJJLER2rzE5avP1vhnWsRL72fcvaiN041eP1PC2xtwZUbOUkkwVrPp82lzz7UEWQA/DDCqEIlI7u0Oj7LmZTol03LQVB6WpfezFg9hZ9cJaUX9cWpP8CEgW+Ev7qU8PvPt/j/fpay1cxkxYD9h4Qbt5TJUeEffCHk9L4AEYhi9Qa6DUNo4egeZanpeP3SFlGyyTvXbrK4HrLRKbOx2KBye4Im40jQALGcHon5j3/1Go/tu0W1FHfbjzzb2ANz4vzJOrADkmNSEGkv9PQZ8QzNRBV1GaFNIE4dncTRqFhub6SkqW/Z+f5rvu2hCjx+UvitZ0vsm7RMNQyBCGEg3dewBYPdu6l+DKO/4RgC4EMMfrkzhJLbxXivQMn7JqVngVQpCSfnAn7tmZQzFyKPDjH85U9SHj1ueOlCykpTub6S8vlT5Qf2Po/NBeyftkw3DIHAtZsRK6uQ3r7ajdT6oscxeO7JgEf3Gx7dH7B/yjJWM14RJPvciYPNTkoYCBMNQ6MCo9UK9bLln/yfWywueXBokIERoJtw/bZixJGkCXHqmZf7p3ZPh3btdXBITj4aAD+AslM0hU4HWrFng7b9r1FUjMH2amD55hsYqJdMt2VH8YSQJHWeTGKFOFFePBfxF69GfOenKe22j3hyhZPRKmgkfO605Yuny1RCYbPjmGpYamVhs+PBdHbcMDduObInoNl2PH0k5c0rCf/Ht9dYWVkDrjEx06C1GZCkltGao15yJKnmzRndtCx4Ues0JevBky745BFZ6npa2iZjKvu0pz+MGARroRMrzimhNdxeT7m2nHBt2fE//lmH1Y1MTc7Arz3lJeGmRrxuqpguUdQThNRHo0Y+GNyG4DccQwB82JKgO2iCSsar16zv0aefMjk4fESROH86nmgYvnC6TOtryr/8ZszaOqyuwQsvOY4cEV48n3Bz3bHeTjm+J8Q8oNpgORDOXItY2dz9SL53v3Bqn/DMsYBjeyyTI4ZD05ZKaLqbVZx6F3eAStmgzpN+wgDqAifmA/7Rb1X473+/TadDpgvr2/A2FOKmrztEiSNKYtJU+eTRkMcPlnbdJP38FsrxA+CHUzTyut5bMXQS/3qtwdgx0w71B5T8AhpMVvxrx14T1CVKnEKaKuVQaLYdf/N2h79+O+ZvfupIk/6EuAgs3FCePWX4xnMV9oxZWpEyU7I0qsZHawLtyAOSNdAoCyMVy55Ry/G5kKcPh/zgTMTihvL915qsr3q25U/ehn81Zdk3I/z7n61ijM8UjNcM5cD3aiXZVDin3ZS8uqweqFlLQ/YeNMmlDH2E20mhhI9u11vKcjPl5QsxL51L+OE7jusbXmzGAF//YsCpvV7QQPBpU5FBgo30nOYLh5gh+A3HEAA/JuBXjE6Kd7fQK+gWNRVzkfByKKSqzIxafuPpCnECf/FiwsXL/ncuXFDeuaA8c0qJY+X2qlLP/OiePHR/IPjS+YifnU159Zzb0cXg2U8Ynjth+cLpMnNjhlIoVEu+ETxOvGqEzewcVHtpwsSBiz3oh4EwVoXPHC/xu19NeGvB8cLLjtB5DdkWsOHAbXiihq+vJYzWDU7jHT+jr2M5gqwpV4Qd5c1aGfhtJF0zdNwOijEUCCE+DSqkTmh2PONxrGYyRiSUAri5lvLG5Zh/9ocdNps7X/9SyWuc/r3PlDk0Y4mTjPxU7jkjGPHCCL0eUc8e9tGj8NiBkEMzAStNx1OHIl6/nPKTM44zC8q/+n7KaB1urbX40mMBJ+YC3ydpfG9uimJU6STeob3YEqIKgc0iW+etiTxJK/fug/Utx1ZHefF8h/cWHH/0fMKVLR/15eD3658zfO6RkEMzNstmeCAPjGTgm7E+ydSAtXBzyLDpfTiGAPixAj8GT7eFJmDy1Cc9JYV8v65kEcVY1fArT5aZHjU8/3bE3/zUv0AFePmMEieO9RaM1oTFdeXMQsz+Kcux2YDZsbtrIXjnWsTiunJjzXHuRsyPzziuXuv/IJ941PCZk4ZPHi3xxCHf0Gyk52DfSkBTRYwHLU/uodvjaIKsVhaDFe8yMFqDb3ymwr6zMWcudlhf9wosdfUluVjhxpavWyUJ/D9xxOMHE1a3HF84Xd7WJ5jXVrsiRDvImzVjaCY9URslY3wOgmCpYNCqQuIMrQ6sbSmNMtTLYI1QssrZ6wn/9mcd/uBbya7R6fS0bwn4h18o8fSRUnedNCpCYPOUZVZ7zKJPlwFfIHmtsfc7o1XL9GiF0/tTPpu5RF8AACAASURBVH0s4YUzCRduOb7/vvJvfpzyvbdSPnnUE5G+9LhX2ElS5e2rCb/8WMXXHSMF8Wo4HoA9QSbN0sE+beqzFYsbjjNXE64tp/zoTMqPzigpvt7XAEbq8EtPWZ45Znn0QECt5PsEjZHupdA+wZAeIcwU+gGHboDDMQTAh3iI3LmgL9tAkG4vmSu0BAQWxqqGZtuxd9wy/bjhwJTl4HTEt19OuX5DqQDvn1XeP5vy6acMi+uOakl4/3rC+wsJ9aphpCpM1IRG2dBJvG2PNTBWE9ZbSrPtm7RXmsrCiuNbL2zv6Hn0uPD150K+eLrMWD1j9LkeAST1EoLYwG/gObNVCv6mBg+GTpVO7FOhYSCMVw1PHw75x1+Hb78ac3PVR7f1FCL1EeHlLV97SlKI4pS1rYgvnC73gV+c+g0830W79cqitmecORVAwbeksOEO4JfL2KROvcu5qtKKvOlrqeTzhmdvxvzB99s8/6Luuh5GRryD/Ym9AZ89We4yHkeqeVtAT1LNCljjiJOekIIb0MjULGVZDoQDU5aZUa+ycvFWwuceSfnBuwlvXlH+6Mcp1RC++0bKqf2G0/sMZ66lvHU1YWZU2DsRcHJvQBQ76hXp6pcuN1PWtpT1LWUr9o38Vxcd719zvH5Gc+U1qlnU96knDLMT8PRhy5cfL1MOPeHFdvU9d3Z4H7xPisIRwzEcQwD8mIJgnrIr+gUWbWBUfSRlrLfVaUceRT5xKGTPmOXRAxE/fjfh0i3llbc9YP301R5wnTgq7J9RKqHvMRupCJXMtifNcl7lEJpt2IoczRasbcKLb2wHv688Z/jGZ8scnLaM1vK6kRCnOcPQyySZTO4LpGtoW6yDQl4DEpKscTo0QikUpkctTxyE+XHLteWU/zuIePE9xWZ1wVHgZgwbt5X1LVjddPxP32pzYEo4Oms4fzNhs2PoJEJSaFkYBD+SXk023Sk6L4JgnAs9exbrxRsJTx0KaUXC6pZycy3hjcsR334p5tLV3dfB+Dj8zhcCvvapKpMNQ73sWZXVkpeYUjU9VnA2SVYVCbTbVqEKacbmFHy6WTIJRt+WAdXQ0qgYjs8rnz6R8NL5hG+/nvDaOeWdBeW9Gyl/9VrKSM23MDQqwu31Do1qhxOzhiN7LFGirLUcK00Pfs02/OAl1zc1me42OfXqs08bZkbh158u8+i+kHLoDz85+GkmHcmAh2AOdKYghDEEv+EYAuAvCAhqYfeVwRQp0hUaznsDg8D/0sFpy1SjzOMHQhZWUp4/GPPWZdcFQoD3zyvvn++9+Pg41Ov+NVptYWnp7hqu/rPfDPiNpyscnA5w6lmOmln+SIEimZ/wxfXXzQbnI9/wQiukzrdYRM5RCgxzo4bxmuHwTEC1LIh02OrAy+e9mPMU0HRwfQOWNxznF1pM1DwYrKz0Xu3kycIuOgB+RbC74wwUyCvOQRwpb51PeeJwyPpmzPNvxCwuqSfv7DLGx2DPlPCFxy3feK7K1IghtJ5JGXiLUVTFG0Rn76zPecFAgKDi05DeP8UflJK057QixqeandchJ7DCgemAkarhE4cDfvRuzHdeSbhyHdY7sNFRLq341xwPfXr1lTOOMEgIDaysDUTB0GdpFGY/f/IJw1gNDu0x/OqTZfZNWkrWO2VI1j6R4v0u8z5D3WEtDBOewzEEwF9koCwgRNcQNPu/nM5uCz1p4jyATI1YJuueJn91KeF7R2LevOR454Jjc7P/NVZX/dddbP0ATE7BP/xSyN97rtqt9YW2l77N2wyksGlnPprbCUA7eMgJYEVwRrFqUOcj02rJz8GpvQH/1a8bEqf8kz9sceaG57OMZX9nE8/EXFhTAultygDvvVd4AwPgd3effvtJJY5gLVJ+749bd/UrJ48KJ/cZPn86YN+UZXrUMzFtRiwhN4Y2ihHFOYsxGdSoywDD+xbaLMpOnXYfd2RWQtZfi7xWiHoAHK8L43XD/hQO7wl47GDExZuOm2uO8zd81sBlc5hm09SthQ4MO7BhfOYpw9y4UK8Ix/caPnfcp8VN7v+Zub90G+wH8C3X+yyuneEYjiEA/gJFgdvNPfMYMHNWlrxJ3ucru2ocGWO0UhbS1NffxgLDaLXEvsmAm086XrsU8b3XEzbb8PbZD7fdf+oJw9FZ4dEDAc+dKDHZMJlCjd9hXeEInxtC56yF4mfNWYWDc1GMvkR8b1lg/R8wGfCLhZkRQ6MidGL4nc+VeOVCzPde7XVN1wubdpI9tmM35AD43TMI3uU4ekjYOyl87nTAp46VmR71kVBoMwd0zQ86vihqRLOH8oSsAUl7prB5qlzAZQ3pgXiSTK73ljNEDb5/tBdZ+X8nG4bPn6rw6ePKVke5spRw/qmUpaayuOa4va5cX1aWV5XVtf4IFmByXDgyZ6iUoFGF2XHDo3sDpkYtk3V/nXJQM1ndV3Y4/PS8/mTXg9EwHhyOIQD+gqVCe3Y6rpt0EgmBJNMM9TuK11jOwEIB0+sbdOrVY6ZHLAcmLY/Mxyw1He+fTlhY9kzJKB0Ao4zBWQ1hpGYYqQiHZrxtzeyYpRIUT+g+FhWR7ubbrVdm3yj9LuEMmP/KDnOSgyB4Cr7LyDSBFSoKpRD+g3+vypcfK3Firs37C44fv+67ysu7pOge5HAfYkP+9BPCsycCHt0fMDlimBkVqmVfew2N4FwPHERc/4Yv/cejvCZojG9e74Ka9LRjTYZyzvlevqTbB6mY7NpZS1dqLLRCvSzMjJZ4+jBECbRjx2ZHWW8pi+sJixu+hSVJlNGq8T2bZb/qxmueUFUrG1Q9w9PLovn3nDOCGRAPyNfZbm4vu4HfMDocjiEAftwjwd1uco17EQGux+iX3qZnBJLUp9TKQW+THKsZTu4NaUXK6X0h15dT776tnkXpVfiFIBTKFqplGCkbKmWhEgjlko/u8o1L6Tl2SNYVripdw+OitqkMEl8G7B5lABT7yBDZxp9Pk83yqWJgatTwW89WWGymPHU05lsvp5y/pN0Z+sBVnnxglnNXALyb8dXP+HTnoZmAw3tCFGWkbHyKM2sez+tiZOC3Cxx0NVyK9bJcRN0iaE4uya6PzdgmSapddqhLlJLpHVZ2sqmxRqiWDBN1/07S+YAo9SLXOZEoX3KlLH2bp1kd/XU9KbYx7AB+g48PI7/hGALgLzgI9m8Gd9iCJUA1EwnVXspUUN9nlwFQvikivd6y0YpPgYl6V3OXOVAY76aDsdKt5+W9YEpPr7RPp1EA50kZmomaFoHZp2e1Px26w+cd3AS1UL8qoqIxSpp6Y9l62UPD9GjI8bmAA1MRP3zH971dvKK02rtMX5VBmZcHOsbH4MvPWL70RIn5ccuBaUtgJbMJAhGDqgciP0/uAw4/bps/YR41WpNHf574kh86fNO8EOBT4pmQi0+NZo73+UQXRbB7bRW5ka94CbsCCaub3i6ktXP2aX89WHb9XLpr2n938BtGf8MxBMCPKQjuHoPs7CIvJIgEaP6/TJQ5p5hTIEGIeEZg6oTQZKorRgitT6Va0zulDwo9I34DBd+fp1qoJ+WRJ3R3XhkAsDyV28uC6rYIarAmJAO1Qy8I3kNKIw6nghWhUoK1LUfJGj51vMT8pOW1ixHfIuX98z17pb6Rq6a17hwF6j1EjtUK/O5XQ549HjJet+yftJQC/7695qV/T0YsgUnp1nK5kwO6G7j2hcbwLKWZX6+cYOIyoFPxh4W8bcJrzgrGZiAoPZ/3xKm/1oULYtCuBmiRkZxTs3qrU/qNa3f5LHk6dJDhOgS/4fi5AUBjDLjUO5OLMHQj+Xc5dgdBXw+Ubc/Sru2PZv5u/j8YEcKgt1nmyh75/uo3Th8RUqjF2YKItWZN+j2Q1P5NK6Ph7876FExh079TRCwDaTIt9oiop/hbA6MVQzvx/YOjFeHwTMCJvY4Ll3cBQDsAgn4qkTvMdBf8wjsD4Nc+b/niY2XG64bJuulFT+rjOE0li9ykC37mAzd3NzChmR7pIMFICj12hVSjqo/8nfcb6h1e1Kco8wOQMZ7c5LRIWPIHFnX9KkV5hNhNS+8G0PT7XvZSqLtHh0PwG47+65+XeHpG6sYYnHM45zDm/qr9g36CgXMOyRQuUpdi7PAi/PyCYJF80vves0VzsPK/l6p6okzxNzPGIN30VdZsb4TA9J/Wi5uXklvhFBqW2b2eU8xmduOHD9CHK9aHiilRa6QblaKCtUpFhHbktThbMZzYG7BwMua1M9r1F+wPQ3QbCNrsK7kT+JV3T59+7inDrz3twW9mxGSpY82YuZlhrihWDCIpKmUMnQ+49nZ7BCi5bmx/Slp2AkLxEVzu3ODXiWZWW9JbWXlq1Ag2S5tqd02R9eBo16/PZF6V2nu473qbgTSn3EPUNwS/4ei7bbMDGkCascAehCHuIBErEBGCIPBWJS7AaTKc/Z8LEDTcPQWjCD6yC2zSrdeQiUWrXwC+2Zusib1IY8/qei7NGYnS1/KwE/Bt2+CKkm/sJBKug6W/3vOy15HMPcPLhwlivSffRkuZbBhOzlvWtpTrSzEL13cgGw2AYKUFNcAmPQDUncBvZ9MJ9szAb3+2xMHpgOkR73KeZNqnuWSa19I2qGQ1V3EgFdD2HcGvL8redjiQDIAKUfXA9VDNJed6wtrdFSX9V8c3+WvfXOcLpcdWld5xKHfGGDgo5WINfWthYKMZgt9w3M1I0xRrbTda6ykECdbaLhg+sBRokiSIGBKXeMHiYQT4cwSCd35sEDTuajPJosVBrzjNFPr7NijNUxEZ+GVwtdPflx027V6qdHuU13uO3BEEpRhJZqlAVLCh0Ikdy5uOelmYHTOM14Xrov0AGPesjXIQrCvU2h7fOr2s6I7muTtdh9/5YsgzR0qMVCUzmXW9T6GeXCQZYGi3oTO5w4Emj/x023zqHYCQgnzaTqlkI0VpvZ4PX0+4IIvycqLMjhF8oVareUZgMNqXXdsX7oblOQS/4ejeCbYHQHnK0zmXieEnu7bS3DMAhmGIVXCkGLEo6fAq/NwC4C7gp3feePoiL9kdtLa9suZyW5kzgdMB/dLtKja9H3WbDiiD4Nj9uQeCg8NlmGELbRjqPBDWK0I7NrTjlLkxYXZCeM8WyEGQCVcqvSY1pVyCmsBYC9aTrJm+GAGW2LVYZwSOTBvKoRAlvmqWi4ybTPLLmmIElbWMiI+zZRuzxnTB706CCTsCUjZregdQKf7NQdKRiBcfQGRHhq6/7obUgdO02w5jZPshRXUIfsNx/2N5ebkb8QVBQLVaJQgeHFdzZWWl/35OkoQkSUjT9IGHl8PxIMf2aKRbb9nha6eNptj8bgobWX/dpve3c2fwHMwGN7QigPZE3Hok/t0IDztvetIXnXSJN4XN1WStGoH1bQXlQBitClHsqJUNX3mqxPjYwN/v4PVAez0DqPVOG6NV7+MXFTfoYgbEbY+4/9v/sOQNeVVpR94FPsmiZ2s8ycSIF7k2BVWAnESkXSnpHPx0G/jtdB23zXch1SgFV/WdsgFdI9qCZF13HZj+mp4U5jl3S7QmJTC+6d3PPTukPHdPiw/BbzjuOiILgm6tb5D08lHgU2CMIRDjPcF0WAN8mCLC3EopVw358HHk4EYmO0aNusvv9P+e+0DAHkyv6Q7N8/nrqvYzCovgnd8gYjztv1GxbHaUaginDhl+2nR08lJbkRRjfTp00whaUkzkpb1oZf1A4QD4Rf2f4h/8qqUTK+sthzWe+BJmTejW9ICjT/9Se39Bu+0KLoN7d0fd1J3mZqeIsHiA0K6/5AenqXNLJhF2kS9L+4G04FjYr2y9c2Q/BL7h+LAjTdOCUbj2pTyL6dEHBoDOOdR42rRLk2EN8GGJB2WgJnSnuHFHBZoBtZF72JhEPhhmd1LA2QkE7/T7fdFqTvfPCDqB9RJdnTil2fFSarW6l/nSZCDbWPbvdcVA0wpSUsaiLBIscvlTugrRRvy3o6NeqDsMhK3Iey6O1TwIiun3u+tPMxbYlfnn1junLe92DndOjcoHnnxkh5qs3M8a3AGQ747oIjBsvBqOvkN3LrfYY4EWI8EHzgLNi4wmMKAfjnm40+gEm9SbYySjZTquTYhF1BCmMVoypCpoHEFV0GaKOAs2wLqU2MZYN0Tgu02G3gmwdlec6d+i5AMix3s/xeuuz803zfwFir1/xSgiT370UrXS/97Es1ZLoUHVsdp0BIHXD+174wMguAE0RakaYaKk7E09MaYPADMXidzLLiz51oZ62bNSrc18+GzP8fzOh5TCdZEPDzjbyUM7XJ87SI7d6W/Krocauatr23c9P9R6GYLfzx0AmQgNSgQ3HUFokCCgIoJ02XIWnGDjhKSUQuwQkwClB3I1u/2pA32AO0WE9wqwfQD4oCfQ1ctEgZDgiLvce0OUdUxX4jI4RcQhAagzOAF1mdSXDAHwbkHwTtDzoNJMehev92HfeTf6LMhpbSNRFFNzMlAfLKZLU69o0k6gkxqMOMQUpFR3AMGwDNMWZoBJYNbCRL7f59FfDJtJ78P71zc4J6gYkjQ7QGivBlckCO3kdF4EDGV73fXu56//6uxGNoLdI+zdIr9+8Ly7Le3uxN6Hkd9DAYAp4JQ4SVALSOxBw/lecRNYBOsBMdWMiJwBk7iH7vM+cAAcWSghKw5sjHEpFWMwqhhVXCBslpuYBILIYJ1FJWfP+Z/dcA1+pOB4dzHbRxGvFl/F0GvoH1A9KWiJ5o8MRj+aneRSB53E0WwrzbajE0PspG8z3sL3/HVBMIRHgWmg5pRxoJrCmClEfxnw/WkhH6LAViTc3oCxuqNWsr73LxBMdjLNe+1SV7A52jEy7ynGfPhoUAu3rWbzqPTXGe9vMXwQaN5bRmAY+T0MoyRlVDy5y+sPm66YgsXSiSIQL87vwa8XnT2ICO2hB8C0bnE2JVGHMYITSFESDKkoYauEUbDqshNGiuIFDo2aoST8PaTEtu90d7/B7EaweBCAuvtJ3/X9ZRkMXwbJFNIv6ZY4JU6gFTvWt5SbaymL6461TWWr07MPAvgu8LWB1f5cMckQFb7P5WECHwEuZeLTBv/v0oYSx8p4DRplKIUW5xzlUCiHSmgzH0fxzNBiRNhfH/OtEcVy3Yeb4+Su1kURDB/ENf0gws7/z96bB9l13Xd+n3POvW/vfUUDaOwkAIIUCQgURVHipl2yaI/tePeMXTOTqthOXKlKqjJJqjyVyST5w5k4XmqSSo3syLI9sS1rZO0LRUpcwQ0Asa/dQKP35b1++7v3nJM/zn1bL9glkWKfqscGu99y373n/r6/9fvdWO/+ZWhqUWKEG0CnPjKjiNVnT1tYWe60LvdTBYCBBGEMEvCsBCsivkGBNII4MaeFJgxaGseZaDXSCDCSNv6ujXULnvbK86auA4SNRN2aacUfz2oSaNkbfMf6sWljqYZQrFiyJUO2aFnIG6ZzhmsLlsklVwestIDa+dadXh9yV1GoVq9r6BUAqIAkjBXdYRoL1RqcmwjZ2i+J+xaDJV819KahMynpSgvScRvxgQqksFEk2AJGTS7qVUTituVE3C1gWSVN9WPZhxvrXQ2AwjlXJmJNaAw2GYu0AVJ6bY6caZ0bbWlcec8CoLESrEIJCdJD1xWrI2nRIKxGAqjGqVxLN2xrpauxsDGIf7di8TuK234caz3gazbDNGMjYy01DfmKZSYbcm3RMpW1TC9p5pYN88uWpYIlW7CUWvg731oJfq1D7nqN0yQFxCzLGs4GYGouYC3lYSHmhtkrgWWpaBjoVAx1STZ1W4ZCSV8HdCQkMa+p1dcmuHCTzSk328RyK0B1J3ZpA+zeO8sKixCyqbFppavtRaUHaWwUtETg1yLPthEBAh4lBBoZESQZVKQ9Z8EGmHgSZWSkK9eqFWBc+L2x3h03yu1GKyIBthbRhtl1WGXqnyEa9b5q6Lg/p7OasTnLpRnD2KxhOqtZXDaUypZyDWqF9o87ruCPPfi9leBnbFu9rwGIEkpS8Ieh5UoMvApozz09t2DJLViWBgXTWejvhM19LiINjcRYl8rPJBUxFVGPiebg+Xr1vtYoUKwCwVttGFln/vI2gXAD/N5j97V2ZA0iakZ0vLMtGSMlEcZgopnXeu3PRuT79l22X+6+HqBIYEzYIAHWCATKdecZSWg1xmhX77O4Ey2tS39utMC8a4HwpozmOuC3Np1as6kj1FCuulrflXnDhSnNhWnLlXnDxFWNvg53w4KGv8nAuA+PScH9wIixJGsO8KoBFIEcMK3gpLW85gmOK8H8ojsGFUJNOnC0wNS8ZcnA8pChEgpC7eZodcsXSiccCIoIaxU3HltpHWBvqDaIWyFGX+t57RPrQtibmr3cWO/NFVOxCNCikSMTqWdjsEKgtesKtXWqqCifF+0u3m0NTncfAG3QRqrsRUyLxgBColwmNPIUIhbGyPXdgL93AeCtBK3rGPN2o6zWB7+IziuCCZcuF5GAbEQ1li1pppY0l2YMF6Zc9Hf1SjNjoIAM0Bn9jAO+Aj/pCN7HpeAMUDaWSg2qVdCBG5ZXNDOk6RiklWWbEGztdyoPgYZKDYo1KAhYjD52acZgDFjXzYWUFl+6ppiYZ1HRcLy6SQmztlGJVSB4PcSKR3MfZsW5X6sDd2MEYWOtv0JbFwxdWTyWiEiXs5HJMLTtJf0u3FcbivAb65bBD27cVdhuuKOab/SHNcFPxNqkggQeNspP1kLIlwUzORifs1yZ01y94m7UBLATeAC4H7gHOFzf2GLFI6I3q8/5taU/wxVfJqJNw3OFPO3DmxLGqnAaOAocAXJzJnqJxPMkmSR0pyEdtw6A1a37xWulRB1EB2v57JFj0fyQJiPMWoqHN44C72YNcmNtrA0A3Fg/xnUz5vbWTLK9yYhlJRha2/7qVc9ZB/zcEzwwGmMt1dCQr2jmcprpxZCFrGnElvuBp4D/6gZRKYFtY3hZc5JgJW1ai4qEwnI4BoddKZsvhW6W8OsRCPrKko5bhjsFhR5JEFq0FfitHKat50W4gsl6QNPeHWoRQq8NfgTrdN4J1huXuJk64PVA8MYsQxtrY20A4Mb6iYDejWpG7rnXa2K5kX1cj+VkJRiuN5nYVjNYC/wi426pEmqoBZZyzVCsaHJFSyHnnjEs4fclPB5eB/zqGKBuAH5rgSA4ktA6zYuy4IEI4OejQHICOA7MT1v6Og3FqqZaEwRaOL2961LOtc8DXg+EnOhsjObgYhQh23VI0vFoH3K8jYh/HbLyjbWxNgBwY72D1sraTlRobUuBXS9auD743ShdtibMiusDqRDyOuDXfO/64HstcJ2g41ea77pFwhN+kzN0FZCtTHHeCPxuBIKq/c75BeBVC8e1O921AILQ1Qx1fZTDrhPZNVhgbBtZ9nrXRGKxIkBE85MWvWYj0a2Cn10nJyBu4foLsVFX3FgbALixfuIg2Br52ShGqZuy2g2ZPCy35unXU5x2pUGENSObOqG1Gwm4Pvg1jLN1avVrHdY/zYDUa+zi8DpAeLNrLRCsS/kl3a8ksCsEyk37r21zcNjYdm7QlSoMthEg2jUj5tbzaOpSSteVeZDRAQY3hDx7A8dnvWanu88Nu7E21gYAbqybAptW4FrL6za0pzjtXfm8GxlRa9t/isiou94T0aI3JyIj7d0Q/NpCEbGaODrjw456YNsamdVByouA6U7WShBU0ee1rNEKDHswnXff3xgItWOBqc8w1o98XRBkbVRpnGW7PuSIFhE/Fx1eB/xEAmtr3I367+pnebfpaWysjbUBgBvrtsFo5VrNprOW194OolFs0VAtv/Vja1Nvty1yRsI2ZIwQAiH8mwc/mg0kSjjpodaA7FIVtnUIikDag7iFhIQxLEc1LHDnvEI6dP2XDwGHUoB0n1e1ULQwHINkGJ3lCNBc5Gcb2LUS+Faeu/WG1Nul/ZrhZL2q2EoY7pKk4fVBkmbEfrs1vbX4S22dxHxjbax3KwDWGb/vdAkZEliIiRh+aAmFpqYs1kDK+hjrEYoQK0OstXjSBysJdAi+QOmNG+lG4Hf92TDdFh2uNQXWNKOrI7nbMY6tr6vTa9aZPttUHRC3BH4NSJcCIdvn6eICnoi7n0H0NxllfU9q+EoIl3FD7ncyX2qBwRA8Hw5pwG+yumAgIZpC8ta4729M9IicAWuj2Fy0uhzrA2H7dV7D2aBJaNN8obcu3LeBruGm85drdXuuNavoDnTjvv1pWAkVp1QpEo/H0YFBGOVYXnwIqJEIUwR+iYpvyZQTBJ7Ep0bJ1EgTp3qHXM5dXV0NWjVrLblcrtG30ErAfburp6fnRxsBitAZOo1Fho4xwAoJ2qCtBU8DBhn5jNI6xV+MRZmNysJ1gW+V7WrtCmzm6Wxk8i3XMVrr5N1uB/waIGjbjXRbb8dtXVr3Zi4CbDf6HpBSDmgVIE1kg40lq2EGuAjMRWfodkEwFYF5OcCNRhjbkHmoA13r/F0d+LRx5N3GCoy17iVWRFHxrSenV3Xetp1Wi6V6E2/iANIN7690olZX+9aq7bZKObXvpw0A/GlYga4RihARU1jfggkxGoRnCG2IVR5GBQS+oFaqonUMKQ1agObdx+V81wFQWoWMyQZZqpQS6QswCqGhKmpIKyJ2cemMdTQvpawg2LiR1jV2bf+2IES46nJadItMyTrv25Y6u3UxnrUiEmuv00HaaPgQCNF5k+8dNNKpa0VIsRjEFExbSNb1A6PnKQWdIWzBCd7eaQTYGWFfKyimBCSlG8TX0QEaayOpJkuoLaF29UCBwMMBTpM8Q9ww4rueQ2IiYNZRF6YUPrbtKOur1UEKqdeHI1+hRWjY3vAzbfQdsHVvfMXYi0jcVoS/sd5ZSyQUqltifANCIjRI6SOtQpk4RhmEAVkBbQRCeEgJ1rrZ3fc0ANZEAIkYgQqx2iClIhBOKFEY584LLbFFA7p+I5noo3FrIwAAIABJREFUp3jPt5ddF/xW1oyo19eEazmxulHLu/F727b4waUvo+YNu76y+er3tI0UX+vxSVxEJoWjE3O1PA2icBOA68h1Q22p1R+hi6oaG9eHYv19IrJpP/r8DykY8CAb3Fn010i34mqATfRxHaFJXMtJJfqAMHTcouWapVCBVNkghCTuRSlaKVCipUnoFvb6avX4ptifEBZhjRtQbInHJK2Rv0UKi6wLDrdEovIGgLsy/WkbO040/24jsBWJ6EkbQPiutD/1ruo4mKQjsTXGKUQIo9Bot48D4VqwwxBjLdYThDps0GC+dyPAtMAfjKN8gTAaT8QQ1mIVeMZgjUSWFLVaDWsERkEoDBKFMOY9DYC3NIIQvcA2AKMlFdloqxftEkMNgukmbNbfK9ROZLambaOL0djWY7LrHnP9eXVIFS1dm62jD6KN7Pr6381aN/9XDQyLeUO2aMiXWx0twVENj/igVuj6bdWw9UdxgVrlk6LA6ssaShEABpFixdyyJRXXBNqSKxkSMUFMuSYeGXGDCrHOAPw6TTKrnm9XpEFFkzu0NatZn+GX0h2D7zmOVE+CVKIh2STF6ihecIOIVLTQ/4v6i6vUZ1BFc0dEj2ADYd7p0Z9RjfKUEa6Ho6H2bixaBnjCuGY2HaBsHGXd1Zb23ZcIv+sAWK5VECpO1a+C1fhYqlojPYm2IVLE8QKB1jq6SZxSoJLSeRob4HfdKMDatVObtiUaa3jlorkjW5tbrLWNLlAbRX3V0FKqWkpVQ7nmhs6DKI1Xd/Bba3D1VF4rsNKWYRWN1GcbKAradPFsizFvBVFXR3MMMEsFy9UFB4L1VbCWtzV8aCUw1TN9P4q1xmD9GQlLERhWa7CYh8xOuDDtVOpTcUEiJvAkxDzwlMBTAilXD/DbdVwNERHyrwS6uuPRmoZsF94VSCHwFcQ9QTIGmYQgkxCk4pI4FqEcsGqzUrW+3ZFZmYloptOjHlBbz0is+AZCtO+Jxk97XcdqY/1klpTSqfegEMI4N0ZYpJBIAdpqhLHueUIhpY8wAb4SKP3uc3HuOgAqo/CkRy1qlZNCIo27EbEhoa0grXXpUe2BtFgMQloUCvOemiVqpyVb7xltSUvRDmR2hTFsT0mtzjY26j/GpTwDbanULIWKZblsyJUM2YIlX7VUaiaaZxNtBtHa1dFCa3eWEKIRobb+rtkR2g7opqVmWf8u2lqMgUrg9PaWipaX3my+31wWXkzBIeBw7TqAdb0h+Vu9M4L2IOavQjjRMppRyEEhZ4l5mod2KYSE0LiUqBAga1G68iYj4fpayEfcp6L1nK24Bms4EQiBkoKYJ0j4lq6koCcj6M1I+josnUlJMkakYt/urDjAFav2YWvfsG0M/1t3f68C4PU8O7sqvbqx3hkrtDVCW0UZBdqJHRlr8YQblfLxsNapoGBclGi0JjSAjjbOexkAY6FPzMQpmTLWaKSQKCNd/U+H4IfElIdVAt8oR+mkjQNCY9bhtfqpjfu4WY22lUao1Qgae50ZMrtWpOhOeS205MuGxaJhIW+Yyxvmlw1LRUMhEpg1jSFsGXmH9fcwWGvWy95dh6PSKSe4SMZEzRi20ZnY/r0cWIQGXju6upL3dgh/o6Gq4bH1drffssvDmwTBtV63gkbti8BXgLNr1PzPnLecOR+S6XD2wFPgee5nzHfRn6sJtkZ2omXgvSU6asxTtjg2oiX6rl/TFdFZPbKW0gHnQJegIyno7xAMdUu29ClGeqCvQyGFxYtUK2hJVbc6LO2NWM3N1QqCjXQtazCginWcug3liXfW8l3dTypQQjY8K2stxmiUTGCswQqFkRolPTABQhmkL951knZ3vwYoPZZjOaQJkSJFxVaJC0HN+C4a1D5VwOqQUCqsNcSFRxClat57CRG75v+JNcCsFVfqrfZu7sy21N9co0XdAK5Jdm1dZFKqwkLBcmVeMzanmVo0zBcsy0XL26dXXom72d2lb/v8tK75GnwLmAJexEkh1Ts/0xGGKVxXaN2I6/o7huvfEQJQHohoP9aAXOBGKy7h5JCOAW+tcmXaVyF/K9/nxjt/rX1wMyuegDFp6e8TdGdgc59Lc4NLySopSWKRSuCLJnNP62eZFqfL1CnwbPuXl8Kla5upU9vYk2sx3oh3IAgaDZ6UrrM2KhEYrAN4KfCiveu63GXblTNCEtPvblXTuIlhsCgbR1OjHJaIxzyMNijlUfFyUBV0kWTBKxMPDFL5xEKFsQZ1pwZchLjSjEAIRRiGKCWcSr0FYw0xUkgUmApaaBA+VggQIULHEaKMsClCm8TIgFBWkFiUDH/0ALixbsttYGWv4vpcjC4l4YRaXY2unkKUQqCUs0au0WK1AbXWtc5XA8tyxTKXh7E5w5lrmmuLllwRFmbfPW5IFvgB8ErMyRP14cYUPKKoy4uMcZ0eLQI2WQNRW9EB6YGOg/Hb74zQQDUGeQnzOIaZWm39VPWPzFW6zctSjRoyr5QsM3EolkEIQyJmScUNvnI1+KRqqclGoGSsbThbobaNOce6pFNdGFxK6eSMZTsznasD2zay7rX1Dt8ZK+ZJNBZtjRN4FW5W2RNOxbsagZ4EpBVO9DjqYhcGSu9yixrWKpQJkCaGtdrpvVuJNYYQS1JkMDXwiOGHcZT2HRgJgURR8e6sCqiM19gjSoCRPkKqhtvsBzFqskpVlEEZEhI8EWC0TxgkITWNqvUDNYRcQuARsx7SeoigYwMA37EAKJp5zFXectRpV28MqYVQCSzlWgSCUeZYSYvvCeKedUVp1Wy7Fy3NE1pDuWbIFQ2zOcvkouXaomV+GZZmVlvZvu40nm8QkkiXTrmZHyzWVri5wpoC4SOQSBlGaT1FNTTMzboh7kC5TGPqBgFiKg3JJGSX3HchAqRJ3CN6a+x1BBFExxp/VzjqmJtY3f3Q29k04KF2KvPT03cji7J2cHj9sXX3n2Tc/TEI3WMVGFbh2rQllbD0pA3dKUjHBfGYbWRgJK3D/M4BqIWWWmgJQtNQupBS4AlXQ/Q9Q8wTeFHOtj4G06gbN457xfjEOywKNCbECqdubiRRd3qUTjYGLx4DqzHWYLCEJsAK0+iSjK85i/nuWbVOg5XWzQBKi2clSghCnBq80Y64JLAhJmbRUhPqsLE74/rOICVWn82xrsM4YUFogdESg6SUmiWGIK0T6LCTmtHUVJEEhrQVlIJtGJFHyRLCKpRJg/UxtoqRWWB4AwDfgX4Xwk1osdZAssV54aG2lKqwXLHkioZ8OaQcOFCUwqmP+54zaKm4RzJGAwxd96F7v0BbilXBYgGms4bZZUO+4iKFWBKGeuH+HnhoCxwajdE5JEhmBDHffYa1zoxpA8Y0wbmNIcSu1VXY0spvISxbakvwxhWP586HvHkNJuaarxneKtjWBx85YPnYw7D/gSjHmYl27s3Wi1uRw9CuGrUSVeQ6/76u1QQCMFnInoU3j8LzR+E7b8DE1dVP//kH4dAobB8AkwF8RcyL4SmNtRrbiKFce2tr2tvNrIAogygBZSfQW6dhq2koayiVYTwLY8twNgtXS1CMIsFKBRaXLVNZQ3+noCdtyCQkceVGu7SxBMbpMFZqlnJgKVcNlSCSejIu/SkFxH1BMibJJCTpOKQSkPBdOaM+X18HdLFGKvcddydGdVhPqsaopROfsmgJYTWH8BR+rMhiDi5nBn+6TFEtIva7UvcI68tfZbNcrmXF09Qdfv4Xhm6Y53jfh/53wtRDgthmEhJU6KJU7Vk8U8SYFFZ4IGtoCVhNIGoEWLo3IsB36vJA6GiguanmYKxLBwTakq9Y5pcNk0uG6axhYTmgVGsyengKkr4gERd0paAnLehJQ1dKkopLEr4zWpXAUihbZpc1M9mQXNFQCyAMLKODgmd2wUc/bPE/Dv6gIR63xGIenidRSiKEiTrBmo0w1+1krafBovkhIXysNYShRdUMD5cs93/X44WvGP7iuGVsyrJpq+CBnfB7z1je9y9BpO5WHrGel1sD3FpzcrfBbSmHoHcvPPlZOHAVPvR5+Pw34cUj0d8F/LdPSu59TMBhS6zXEotbfB98D5Qno/pH/cD0jdOfxkWt1rhHGEC1BOUcdEzB5jNw7zF4aRyO5SFfiOxcCLNZw3SnYKBTkElqwNW+KjVLqWbIly25kmW5bMmXdZRxiKjUcLOEiRh0JD36O2z0UHSmJMmYxIuo6rRu7SxtP9FtpNrvgCjQer7TPTaA0Vg0WglqXgwbU8QzRZSqkNq0h2DhPB/58GM8/4Uv8fgTB/jKd8/yuccCSH6c6bFv09fbwZG38jz6AFyYgD273LWJJe4jX5bkCm+z9YlfQp/KcXXimwwOPEiqr5fJ8TJXr77MBx6A8YXDbBu8BnIezP3QOUBh5nkyw3HmLmc5fR4e3O8RhiG9Ww86IgLzEouTkE6A1hlSBw8y88IPEOoAidgknZ2jzMzk6cxMkezsZGZ8mqHdT0FtDuQ8p05OkUnCpXF47IMfY2nhTTpSIZfnUnT3baOYfwUVgo09SCY+z+AH+jj+5WPEUrD3w1sYPxJn2xOH4cLbfOf7J/nYb/8zmKyQm/42oRnFk0fpGjxEcT7L7NRFBoYg88DjzL9VpP/wPsLj32Mm309PzxypTJUTJxfZPABV+oipBbLnfp1yLWZH79mMTf8mOvGrouYNEagJ0uFmbGySwKZBd+OJHEJWUaaDtab01X/3udQfCAtWWoSVd0xqKwAzCFhn7AwaD4EWyk38CRChgqxG4tJowlqMkijjZk7em0sjRBzRwuNpcarioYFCGaazmoszmjOTmtMTIWcmNGOzlqtzlmsLLo15bdEwtWSYL0C+4jz4aug6KUNtqIaWpaJhYiHg4oxmfE6zWICZa5ZUF3xwBH75o4LkrwkS3YJUEuIxQdyX+L7A81yqVUqDkiGeMngKVCPKdA/fE3jRQ0kXPSpl3euVq1EqqfGUQcVi+Ht8dsQ8Zs8Jjk9q7tsj+OfPWB75HSBxlw2jFavwb+VQ+Z0Up6SATA/07XM52aklmJuHT+7zePijio6PefQOKjo7JJmMJZ2CZFKQSEjicefAJOImerjzn4iveNR/lxAkOgTxDkG8U5DoEsS6BIk+QWJE4A8KZAz6l2C6AHNRirdUhM5O98XjviMbrwaWbNEyu2yZWjJcmddcnoVzk5oLU4axOcPVectEtNemliyzOctyGaqRIoaUAl9JlLSOvFw0052C5iiHWPF72qJD8ZN1RK2JWJVCrA9FJRnYtsjrV2Js3T9MauunKBbfIEg8xOy58wz0hmBiPPDEv6Qw9wKx0QMsTy+TXR5g62bBW2dKJBJd9HRXuTIFvSOCifHLbL/vISglkNt8uv2zTFyL0cVRVHIvXR27SA5cQlV9jKmwuJQnXx7mtVde4N49ezH5K6j4Pex+pMzxt7ax+55PsDj1OovzHZjavYBPZmgntcoYxSvzSLGZMxfOMTrSQbVcphZeobsr4MUXC+zY1U9h4SSnzsyy6VCB4iwMj+xl+2bNsbdPsvuR+/D8TsbGLnDvtgl6H3qaxfERutVrDAzuJje+zOKyZdumPuKjPovnLlObPEHmod9CLLyInj6KDq4yNpklHZ9mdglmJqfY8vgn6ekeQFc6WLpyhEppku9/9Tj79+XpHNrO2NUYJqjQky7wzR/AppGDbBrsJFvezrZ+D1nqJZj7LidP/29/0Ncz+wdxued7MdF/xSiJUQmEUCgDKpR4RuILQSwZ3wDAd3IqFBFrwB9RHSZfscwtGy7PGc5OGk5PaM5f01ybtcwvweIyLC3DYg4WlmGxAMtlKNYExRqUKpblsiZX0sznDdcWNWOzhrFZw3weskWoFKGzAz57D+x8BpKbBfG4i0xinnVAJTVShC0PjcA02E1aH7KFhaT5/46OS0kbsZNYpIrheTF8X5DY59H7uuLZBctnHlb86ucU7BSruCrveLXM4a35U9xk6vMGN0IyA8M1OHMeTl+AX37IZ+e/8OjugUzKI5V0QBePGWI+xHzjmFo8je8ZfM9G/7/Gw1/7954Cz4JXc1GMiINRkJuCyiKcX2oeYjwhENKSL1sqgRt/mc0Zri0Zri5Yrsxbxucs47OasauW6RlYWITF6DG/AMtVKAU2AkCJpwRxX7ihf9nCekNzP7ACAFee6p9kFCiNm0vWvqWUyKD6uujaNE81uYsto5s5e+Uywwc2MTuxgGcTmOpZ+oc+RXrn+3jra3+Ep/bRqcsoFCn/DJ2DD9OVKmOCBboH7iPubSGWeoCpiWuoyjj5BUNm82auHjnFtakcS9WHWJh7le2bPaimiHtX+E/fK7F1QNDXZdm21cPoHqzuRppzhNnD5IpJhgbGuXDuCls37yCz/zLJWopzp4+xaehhkvffT1KeZXTL+xDBDjyO0jGwB+RBRnf8HGfPX8T3+qnVFlm6CPOzcHZsnt0jFfq7YfxcjYTI4/t76OwYhUVNmH+J7v5nUOIYNhykMzlOJZ9n9lyWnbv2k+ntg9w8ftBFIp3mxLFpDn1Y4AUg/E+y88kLMN1NYewS6cELHD8K9x38LTxTIdX9S3iFf+D4sRn6u7fSv+2D3P9AJ3Y5B2KOgcEUFy+dZfAjh5i8epT7D/wKduIVUtN/+Ns6nf8Dlvf9ayH6UPEFhEk5RQtVQNs0iZS/kQJ9Ry9bjYRN3eBpJYBsSTOxqLk4rTlzTXPxmiFftJQrRF2g7UakUoFq1VCuWpYKgmspQWfCkk6AJwU1bVnMW3IlB371xpe4gMEuEFua82u+Ae8NizQamdaO6tFbkU6s/1A3qMs1KEw0VmvwY6jtcdfjI2qA4JF7POIvx9k3ahDb62+vabB63jEONo9XtIvaIdZKid7m29dzez374YEd8PfAwe2KRMaQTFRJJNLEfYk3LVGhI4KQno14cVkfgW3LuS7gZkDyYOp1wCp4Rde5KEYh3G4p9gkS/Za+RPtbTU25nGM1sCwsW+K+yxKFIZQDQakChbJledE2qj4rVzEHxZwlCDRYiycVMc8SU5KYp1waVIrGpV+vRrwGFMFPYKosZjRBTLBkciQ7cphuGF8Gw3kGu2E4s5ezX/ka23bAmfFpHjy4ncryEeaPddM/qNj8+FMsv/anlAuQ7n0chiXeXIVUCoL8SV5/Gx7/5DkCUyTTKcgXx+DCFFvv+SBDI5eZK2yhOz2JIU7VjjE/Aw/cA7H0QY6de4PRYRja/hjV+a+yXHuYN469xKOHd/H1b1zk0//FrzPzwl8y5D/N9FyJLUMA17jy3BFS8W6kn6cWHmG4C1DdlBe+S3LwNe57+INcO/1NLl+Bn3kyw/xSgWQK5oswcQUOPpShWhqlJ/EDsHDlGow+/s/JvfUfkOFjBOERahVI9jzA0EA/x195ltFdj9I9ZMnXrrJpYDe9g5dZuPYRSqWjTE58k+XlDL56gUIpzbCCB+5/H0ff/CYPfuABiL/G9NuCJz9+L6Tez1/+X1/gP/vZbro6suT0JylPfJNM+iD62KvsSMHz3/hr9u6FaxUYnPo6dJ+x1H4TUfhlQUITxhfRNkkoq3TTXkvxpJTOIEVsDhvERD+KJW7JmtbnrUINpaphLmcYn9NcmAoZm7YUSpaKwwvXYGBWNM1YB4JhaMkuW2IxSMRd+tKL9LoCx2NLbr55XFI5A2hk0xMXRcHcW/AfX4XFCiT8iNIr4pSMS/dTKde2fJMBGJ6AXYOKQ7/iw64Q6VURUkCfwZcBA90eDERFIysj82uY+Tp84wVB5Ta7rW3Eu3A90yqlS9PeSiCS8OFTj1mGPrPCoG6CTX2Q6JLE+yEel8TjCWIx8K/Bsb+FsTk3mnIzYwFCgugENrs6XuGioDwLYc3tl4qFjLJ8dhQ2DYOMgUq6rli1hnMyNXlzc4k36vOdnnJEFjFlSfiKdBwySUnCF6gI7FRbrc826p1rj0T8aCxRLPSoeSGh1Agh8XQMTws0AUbUeLtWoL9vP8lEFtXbQ1ouEWRGGOg+wPkz32bP+86QSiZJ9G/iXu8SublFunoeZm7xuwwP7WXyh9/CF0kG+sscOf08D1q4tnCATX05sll4/PAWkPeztf8bvH7UcujBR7ky8QOS/svgP4LSk3z9OzPs3z3DfU98hlShSG/PDJmdGeIWzozD0I4Syr8PiieoVKGqd/D4Ix0w/y0ujcHYxPfYteM+fvAKfPJTmxndvBPu2QSXi7z80mmGH36Cq5eeI+btI5g4TVj9JrPTcN9eqIRlskXo74dqHrbv+DRLyxOY6mX6hg5z/txrdGSg/Nb/w+wkqFRIRzJBMj1EKnYctv4TFr4NyZmXKOVTeJTILys2Dw0Tjz9PX8eDFGqbkeZrvPwafPyJLYS1kMDfwv49w5A7ynw2CaFl+toZBlJnuP8AXJkRdCkoh99k9PGnufzc91hUT9CXfo6hQTA1qBXhpckxDo9qBnZ+kfzyq5bhfyV0Moksa3o8QZmApHZRYE2FeMYYR1ZtLdpopNqAqx9BWHdTHq0zBk7SSEdE0Mslw0zODaqPz1nyRUu1FgUBKsKGFnthTHNAvhq4DsFCxdXtAOLXOYTQwmIR9ALYSDfSdsPg0/D7XXDkIlxagNkcLORgqeRqP4a11Aqu7w4ozx3f+4PIqAsX9RIPwYTE47FIg6gebtaAkEsXBf/wXZjJR1p874AlJQx1wL2bBEMNwy3dIy5JJSzJhMZPBvh+DKUUUhhkoDk7Ca+MOZIkuInUqwJvGFKdrjYoNcQ1ZDzoTsPWlODBPo/ETkVls8V4IWGgqRUtxZuQDAzW+Xhv/aS9K2RYWMpZLnuQShj6uwSDnZbOFPim6VybaL6rlajhx9n8ouvOnRVI40hQA2shZijZAsPd0NObILn1EKTLlK4uEfcmOXdukp5OMNX76Dz4NMy+SWDypDL3wD29jJRB+UlGDu+ByscpnfszHjkAbD3MrtQIR149gVGgOj5A78Jxjl8a4Mnf+BBcHWfhNCSS0N97juGeRT7yMAwNH4Rcnu70D8jlIDFWInPPYcTV11gcexY/dQ+B2M49O04xkJqlpDvA9PDwoQSF7FVSySJPPQrFxSNUw/30soXnvv81HjxwDwuzJyjmIDN0ms4eIPMRYqkTvH5skYUFzdatfeSrvcwunGfvkzVyp1PEM5uoZL9FJr2FIJxgZhFUcphq8BI7dz0BiecwuY+x8OoZENCZAZUYIZ3IUMgdIfB+lo7gy3T0n2BrzyzZHDzzud/gxNEvcPjhXyO38EVmc9tJpWao5GHLjqcYv/QsWQVdHUOkkzMM7Px9xk/9H3D1e2wZ3smLbzzHE7/9OfpLX6FShe5uhS1rBkY38dKr3+HRR0NmZ/5r63f8mfD6PAqlgG58UEUI0njSwxNC4HmeM0LGw9hwA69+ZCB44wK8sU5FQGuX/qx3fk4tGuYWLeWyAzilmhRbSoqoy0444KtZymUn2RNGqb06QUVNNqmzrICkbm3DgYlFePA8BNssgS9QMYvdLdC74bCGh2dAXAMuwZFLcHwCLi9AtuzmzrShTangemfDohENdoYoPyc0xnouQlBVmjMPzjMTBmpV18Rh7E8kS7YqUycF1GLu2JrgF8PJxyuEDPFUFSkNUtYcW48SSGmwWMLQzdqJG0TN9U7Kzjjs7oRdvZbtcRjdCXSA6ZGYLg/d4VNLCALPEpQE1WlLOGeZzF3/qtRYW7G+Dn5eDPoG3fjLfBQ5tgJjtQzzi5bpTsNSQVCqGoJQEVcWKwUmSjsbW6d3s6wVZ/8oAdEoDRZ8IxFWYqzGeJrlcJkt+/ZQDs6T3CGYn9hKufBlhjbtpJSL09lzhp6+D7O4dI3+c1+kGiwwPwepzjmGL/4SBf00XXu2cvof/5x9D+6lJg6R2r6P2niO2KbvsGcnZAswM/X39O7YypNPxuGyx9RkES272XfwSZbG/wEO/DJDy0vMn3mTWmWQkS2fo28LULnC/OnX2DoEvYc+Qe3cVZ4/eorP/uqT/O1ffJ9f/IUPM32mwPOL/w3p/tPEq/FGnRUFvA488F/yOjjHcjNMACyDWLaMMMgHHvoqS5UDdMZPgewhFoOl498lndnDscpeCvwurNGNPT0PcCD6rHvhgac4BWQosqP4fQa6gPs98i/CxfMhxk4yMPIJFie/wBe/4/GnX/4SSmZALDT0QgVvAF31ok7078+TiHXxmcfgU5+4xBOPfxYKU2Rz+xgeUHj2BFue/gioCgd2wtTFOdJ9M4Tqt63mPwh6JIoqmCQIgxAhXhiGCCEJTehEDjciwJ/QakZ+1rrBY9eKbsiVXL2uWo2Gm6N0kue51KavoCsjScTrkjyCXMFQrjSZO+qRIS1guJrsTHBh0VI7AdUDEBu1hMpZLSXBehKxWSI2K9gnef8pwcMXNG9cNZyeNkxkDQsFKFSj2qS5EXuJwdoAbN0MGkAQaD8ywWY95HxnZbdX+TheBIAe1Cu6wiBliFIhUtZ5M8UNg7466PkxyKSgvxP2j8KHt0NmK9iyxFQlpgfCAdAxQagNQRBSzlnKE5LiKYG5BuOFlt0Wc2nTtvTgejszBpu3CDoSkIoLQmPpTguyxSYQNq5o1LGcL0M5cLOEJhLIxkY9zqYe9beD3Y+DGcYKgxQKqQXWGIyy2MwWbO0U5fRjTEwNsCeM0+X/kHwNYt4EQh6gq/8ZCoVv07/7UYKZLMXSAlpBZ/ID5AvQNax560t/ju8DsV46Ui8xeeQNOjr2MvGqRsqtJPyrbOmH8ckJth3+DbIn/5qOxAj7dg1DStOz5WnKR18kHkuSUXOMZed4+9RJHnrfxxjcPsblS7BzJzBpKJouPvuZX6N29Vs88yR8/s9/SCyxmV//1/+e/X/8f/Nvd3+RVCLetsfWd0YF1xhBmE/TL85QLJdIZKbo64KYgJcrHyO4jXaRHB2c9D7N47vGWD7ydxgF23Z9Fm/XPPkTkt/8nyDuGWIxi1KM5mv9AAAgAElEQVSug7jRFLXOYS+X4PNfhUq1i5978qtceRW2b+thdmkfm4Z+Brv0j4xdgs3Doxh1gvFz0LdJ4g/8jk1l/p0QNg7SYBx1CJ7vu7kXg0aKiJx6Y/3YwU8bx1JpbXNcoVx1s3+FcrPhxdomCEjhVNFH+iX9nYJ0wlGj5cuCXElSqjo1hyB0qcpqDSpVSzG39lFk5yyXJDx/Aj7xLMQ+A2LIDeF7nkIYDyV9sD4y7cFhkAdCHpoPOTgVwuWAV88bXr8CFxagWL0RCBogxFrp0pkCJB7aqPaokJDGTNy7QknHa8ZGooLFc9Ixwn1JpwjRcqOvQwsmhAO+3h7YMQzv2wEf2Al2F4Q7wCDR2kMveIQeaC+kVgkJAkmlqileMWTfUCy9YnnxmuBqsXnCwjVYcvy4S8UNDwjSCUd6EI8JMnHFph6JkoK4B9miYWxOI4UmuwJIgypUalCquetfCdqFjFtr3MLSJqf040iDWmsbfVpaaIxfo+u+T1G4egplP89Qf4JKrhfNAgMDu5mbvMDA6BzsOMb8c5qO6bfwt/46vYuX6GUP9O5leWyK/MRzPHQvFGoZKE+RX3YO68gOTcfefRAfpXRmhInJV9m+yVIYG6d782eg9hLlQo3Sha9QDl09fWbO1bT23jvMti3TJAd7Qe2lazhHKpNjeSlLTzfo8jViO0fIn5znt371n7D9qbcgrBH7s8f5H3/3h/ybXX9JMh67KRAEmGWIfv0l+tJQS/4Ml6/+Ganhp28L/JopdY/xk1VGlJuxLy5+lSMv9PP8W2kCDbHbJA/9u+/Dz/3Kv6Cn8l3GJxbJJMcQ8iVXRUnvJtZ1gbmlNJu7i+SWTzCyu0J59n+12dr/Imw6S2C7IZQuArTWbQZr5UYE+JOI/KwDP9PC8B+EUKxaChVDoerazO0K4VklBX1dkpFeyXCP03oDQTWQlGqGcs3V2So1KFUd92e2IFiOWxZm16iPBHB5Ar6mIV+CX5oE+zCED4I3CFJ4bhAeCyJw4w1x8EYlcptEPaJ45HW4503DV47B8UnIV5t0Zas9T9NSdRKN9GFo/CiN6EUgqdsBsF5OfccCYP0gNcZUMDbtKMQMaC3anIJWh6ZNMUG4OummAbh/N3zkAAx8EPSAY2LRBsJQUgs8glBQmzHUypJaoUZlGipXJMWLgokL8I9jlrOFNdQ5JCTSEIu5BhlfCTrTcP+oJBUX9KQVw92KrpQgk4hI1DVMZp3EU6gNwyMwMdbOXhRqKFehVHGUe4EWJFqSna1qJpKmzNePpRBhLcYaQKJ9S1dPGcovEmi4Ng87Dj3EhVdfZvf2HSzOjjNw31Oc+8Gz3DP4O2S6TzB+9Xm26T/ijbfh8EMFhFygI3YNlM9bpwIeev8WXnz5DO/b08+Oj3+c01/7K/btuR/kLLXiWXY9/Wk493UycY/pyVmGt2ynMn2EngOfJZWrgRjG8HdIW4LMNOPnYZv3ItVwM73J05wa+xj3bC0yeeUlOrq30ZH1WMzD0bNfYnxmBwBH397BQ3/6Yf6H33uRf7Pj/71pENQo+g99lPGXrrCt7yRdfR9nXvTf8TnPiSH6I/mk4jLsH53n81/NtMm63TKwhsDSSTw1SjL1IB3Jf+CVIzAzDR98NMELr8Ch3T75PAQqxfzZZWqVPyb1wAOW/G8I0VnCeiXXBeoJN/8n7UYN8O7ebNcP54m0tWwU+dmI7VxbqIWGYtWQK1ryZcd1aVeMO8Tj0NcpGOyGoS5BR1LhSYm2rq5UDZ0HXqo6vb9cybKUdoTXPRlnxBAuLRWEMD/lPuDilHv88ALs/i7cOwyP7BKMPGCRDxjkrgAhQjfzp2LEfInvS6zy8B6W9GYCnhGGagAnZ6Bkr9ewYhqRr5CuWchBnd8S7gXNdKjizumW7nZpd9XcYDNPq3XM8XJqN2juhbZN9mlN+JQuvd3XA4fug4O7YeBR0P1OdijQ7npVKlApQ2kioHKmSulCnNIFyZlxw8k5yXgBJquG3AoPJJ6EdIfr5h3qdkP1S3lLTwf0ZgRdScFAl2T3kCKdkG3do9XQ6QqO9CoKlZClNbhTdUS1lytbilVDoKXThWiJelu1C21LFIj40aZBPaswxmCkpRZfJuwBb+AVylehPwXBlQpdPlwbu4yMw6W3nmXbbjj5/efYukkx0A2vvg1DfSC2boFsHpFQLM0EPHR4Hy/84DQdHTvIZATFN/6KdGYEVI03jr2NAvbEf0A6DVPXrrLpwDYuvPoddm/r5uqb36SrayudmZeJx7pJHniYyVcLBLXXsdUJzlycYP82yPjfoaMbOnY/zfP/8D0SHmzaHKcaVF39yrhr/dbxHRz6kw/x3//ey/zP2//8pkHw+b/+Lo8/+X5gmZEdB5i/dncyggGwMAfJ9E42H8wQ/J+F2wa/Rjp0OcbVy89x3/33MnYZHvmZPQRnzoOqkkgJktuf4MxzX2ZkS4mJyRIP7gLUP7K8/DAiNYiVNdcFaqVw5K463IgA7zL4NYv6KztBFdbqFWKwdaPp6ifZomGhYMkVLdVaU36mzqaRSUJ3RtCbVnSnJemEwJfOghgrMUa4CDAizi5U3QhFPa1aqDqx2UrgxioSvmBy0mJCqEo4sQRvLYB/EbaegI8c1XziPsHIgxK730eOaBKJGsmkIJmIE/MV+ODtV/RU4PGcIVuD8YWoI9XezDkz2Kjjs3m+TPO8iZaHfYcA4KpCno6OX2BMjFBb14kZGvyaIQgc+Kxb81PQ0w3v2wef+QDIB8F0ROAXRmnsEuTzhvzLmuXjluIFjzfHNW/NWS4VDcXa2oaroxsySUFnBnYNS3YNKeI+TCxoetKSvk7BQIekK+3ozFzt2DbB2kJXSrB3RJAtelyebc6j1DtCayEUKoZsSZKvSKqBc4CEagJcC99xU01erCyo3v1ZQE8qagaMMgw8+GmuXTlLZuYqPd29DGzaBjvSXP1Pu9k2cIG+HXD6OLxyDHo6TmLNATp3Pkn51PeJxeHo98+TUpfYsnUbUm2GeMD7Dz5OYudWzr3wl3THIZnIsrAwyj1boGPLIebH3sAGsGnTJrCvs3v3h6hWe9g0MounjzA5CYNbfwYu/BWVkqv/Xs0+xSMPLvLWiaPs2QpkHqUytkh/J2wege5dhxl9PMb9//5Z3j65yw2EAm8c28H7/+SD/KvffYV/u/3zNwWCHzw4AJlpzNQExXAAp7Fy5+vM2EGy2Td56oPDhJdewpidqwS1rbU3na4F6Ny6m836eWbmdrD9o/t44++/zKH3A/F+HvSG+Zu/+TJPPNzLUP8oiwtXqHUuMn38DD17/p2N1f5QaNXnIkBjDNKTUU/9nW24mtAYbTFx8Coa31cEOsBqKCegoxZDBFCRBhU65nXjSWQYYo11eZ+fUkBszD4hwBpMJChqImV3cFFBoF39b6lomcsZFvPO268TEAvh0laZJHQloSMJqZgg7gl81Wyw0NbNp6W1G37vDi2VDhzBcc1SqlqKFctyBXJFw1LRkvAFhYpLmzpFAYvWMFcyfP0svHUFdhyR7B4Q3DOk2LTN0vGBkO7DhkwmhZSug1M9CPsnYWzBkC3BvL5OKtSu9PoNvFukNVs51FQrAAaAjzGCUIdoE1IJDF7NktICvV6TkICYDyOD8MF9IB6SmIxAW0moLZWqJn/BsPSsZe51wytjAefm4GoesqElV9NriiPHk04XsDMtGB0Q7N0s2TPi4QnIlgz3j3r0dTrQk8KNKmjjHBdtHYFCoB2DTzJKme4eVjx/fPVA5vISZHssi3lDriQp19yeTvgCKVYzzdWBUKxzv6ztbdzekoFPVVXIqgLdO1PEpy+S9g/Q1R+jsvwqE9+A+3c/wqWpBH2bNEMzNUb6DWOXLnPu0gm2V2D3KGTS95JJKXoG4dr4ONeuwXAORnfsInv0NToz+4knT6FEDWXGCcwDVMbeYH4etu+AazPP05mFpeIlRrcnmZ3IMDjyNJPz32Nk1zgkf4MtQ1/gG9+HZ/7zH0Li53hoS4K5468wf+wlZufh3h2jSHGF3KVjxK/mSfpwYP9FTpza2dhYrx/dwfv/5JGbBsGZbDeb/RAR30526RSkP3JXbpPRvjfZvnkLlfIsnfs+gZTn7zyqnH2BuLUsVU/CuasceuR3qS39f5B7mSMn+vjFjz7F2bEJupIFurtHOPbGIomOcwxeOkdx3z/9NZvZ+sW7jjZJnXSzNaHBCz2kEK6t3Ui80BCKACU9hJJIIxFWRMrfFiF/ugfx6xFePeVp6krLtjV95DpACxXLUsEyv2zJ5R0I1RsFPAUdadGozSRjEPMdbZYzME51XLpShxtYt6KRGg2NS6NVIrb/QsWSLwuyJctyyaWvSjUHlIWyoVjBpWILhrNVy/ms4HuXnQxOf0rwmdd8PvezGvkLFaRMIaXvuvwOGB6dggtzkC2tBsCV6gA3lQ2xvDM7QWWrrTa4oQKNEKCNwaKpBJAIaVzL9d7K92BzH2zfBbZDYqyPNpJaTVB+MyT3HwP++oWQ78xaFgvhmlFkqsMBaTIuSMYF8Rhk4pZNvZJ9WxSDXRIBxD1Bd8Yj5rkaYCvMWGvwpJPUMsbiSbfX6nptm7pdxDifsZQL7SMRuYJlMa9ZLkG5Kgi1whjr9PNWCO6uEvptSHfZu14bNGikJ9i1Hf72j/6OX/y5/bDtPsyZ50n07mFXMqRWcmTP3/gzeOwwKH8L7zv8YfKzLxBYy7bdj1JaeonUtl9g9vQi3T276OycwQQXOXPuW+x95Feg8CyMfprx732dZP9B0nKBxNZn2DtwAaon2dz/CY68/C3uO1DmyrkpYh2bQRQY6N+GrQYUCi/Tse9jPNPpkz9xjI6ecb717Kt84tPPkJz+FqObtnPyUo3779vNqbcvsHMbxJXTsty/7xKnTu1ofOdbAcGtW4qgDkDiFaaOLpPYdXfOe24Z9u0aorzcQfnstwiDe1fZRRrX/OYueqlwhskZ2PfZ/VRO7ePED/6EZM9Btg3Nsm1kgfnFZ9l/z0GKS2/S372LTTv/GefP/Dk1Bbr4F3+pkn909wGwqgpYaQmUQXgewnPEskp6hDIE6yEbrNyyHq1jhH1n66Tcqk0UK9Kgrd5uPeqzzVSQEE7uKDBQDlzzy3LZOgNSaXbSKQGpFI16TSYhSNTVBCIur7r8jFLN/nJjI45IGQGvgWRMECQsnUlLJQODNTcsXa4KClX371xZs5TXpOKuY6tUsQQBYAWBhpm85O+PS5RV/PywJvZUDd9LuLGJ7ZbuvZYd5w3n511q7EaqETdRqa9PS9ytoODugeCqCDZsavtFvK7GuDKNteufC1/B9h6wOyXGKoz1MEZQexuKX1H81Q81356DpcJqgIgnIRGH7k5BT4egKyXJJAWZuKC/A7YNuPSmHzHDeDJyloSj3jOmhZ+zZT5B0wRIEV2rdELw/7P35kFyZdeZ3+/e917uS+07agMKhX3tRgO9sFeyxUWixEUyR7Q4GitmpLE0Do/scJjyjGNibEd4JmSNYyhrJEsja6EoijvZXJrsZm/sRjd6wb4vhULtW1Zm5Z753r3+476syioUlu4G7GZHv4gMAAUgM997993vnO+c831DXYLRacymUhfgLMxqMk1mjrVQUZSrFl4IAqL2eayy/7qR7s6t6+hv7/CkhxPIQriPz/5KjLHLp9lgnUG2fBqdPstS+iIVDW0dDYQTrZTci4jqOMHQAFevaXbuaYLCK0Ri+8lf/TqVKsT0NNMzrXgqzpb9feQvfIVsroXE5HF6WraAusiVyQsE5qDvgd+mfEUS1E9z4JcOMfr6YYLhIWJWEFSUzqaLiIEB4iMOR566zPDGfsKBaWh5hPv3vQaJILHuX4WJL7Nz50fATmFbl1gqtRC055cfh61bRzh79u2D4PMvT9IUnyQQgAMHN3Ji7s5c90D8XmbGLtISTzOduTM5ffKeL+Ad+SsqR59mobCXew4Mw8B21OkSsQf2wczfkp1rIZ48xOzcG7RFRuhvhIIF2ZkCTbGx23ZUextFZhtb2wR1mIAIY+kAFiGChAgqw50oX/BaK7GyCWhZ8319v0DgqvmmteCn9IqztsbfGH3KKV+CJV+HsVA2WVvtTaSf/TUnBI0xs7E5tkQikP7mskwx+dSimbkym6UlwZGCgCMIBwTxkKAhKmlNSDobJb3NFr2tFgNtkoE2ycY2i8F2i8EOwcYu6O2AtmZNLKqwLdNRly55/PgiFF52qC6W8ZRCaQnCQm0SDLSy7EV4q6DhtjPAd+nYcMdp0Bs8yfXmtjWFnpoiyo3eyhbQEwEdN1b2Skvciqb6VomjxwXPpwS5qiAQglDENLQkG6C1DTpbBTsGBTv6BcNdFlu6LbZ3S3b2Wezqd+hqskiEDdUZsMV1FkW1WSwp/D5cCdISxgcSvVyztqQ5nx19Du1NgnBsdcMD4GuJwlJJUKyYAEjVsrplsXSzbtfOf+mb1NTfzeFaLrHOe6B9A5fOnyaScMycdcLh//ryGZKdj9Dadx+hQC/p2Ys0dPwaJXcnP3r+JXY+8bvMjaU4NbKbazOdzM5DcxwiIYuh4Xa2bLaYnjrJsTFIFeeZzgtQ57g0eYGZFPR29MDYdwkGgmDDzMgRGjo7iTQWUZwCO8PoZMqYODaHGdpwhZnpn5ItDnP0J19GuJC7co7xF46Rnu4F74fkp19j84c3EwhsxrEtgjaEbIgEDAjWH28cGyD8pYN88epvUixX1m1A2bYBdu2GiUmMjt4dOga7UoSsNGXxKM+8vK4z0ds+8idfoZKHQMuvksse49XXz7N45G/QwQ5IB5i5BvEhyfEzGdo2PQbJTTj9j5FshYHYEoHyV/QdzwArQiOUh9DG3aEqTLu7rRVCCwLaxtKSqjKprpQCbWmEL8n2PiI8V3VqXJf5aa6rernKdGbWMr9sSVCurP63tmNqf41RQSIslgfhpVxpG6jPPrVmVfuoBKO56ZesNALHB+OQ1rgORJUZpShVoSEsaIxqWuOCTBGCjkW+DD87XSEtNIW8xvUEs3nB8xcFnzqqUE+alFajEV2SzkYP5zYW/G1ToOtmXf8/32rv9sJZvbbut461uwSawiZy0Zj6sMqCe7nK67M2WU8u14NtWxCPwqN7gsSCsJh32dQusW2QSGIhQTgoiQWNJZWoQ2tRLwwu6n35/HsnVuqz0q8J1gI2z9PMZAyANcZhOnX9KZm1DJmCcZxIRMzGHMCvBS4vVL3updN34TZX7QBVr0Lq4hybtu+hvJiGvq1Mv3qcf/5re6Ghk+zFr+DITjYM/iI//fFXaWrdxEN7gcZFoknYED1OsiWPW4B8aRszUw6JiEdVD9NxaDMdiYvkFi8Q2/8wmTe+TFcrDPTeh9i6iVNP/R2Z3CRCb2R3/2Xc4gIlt4LwgJYmhrZtBZWmmtpFY0sMy1ZIu4Gtw49ydeQ5tgydYEkZScPJEejqA8aTJBKt2Lbtg5paWWNOCKql284Em5sBPcyhg0mwjwJ778h1/9pTl0kk4BcePcHHnjzAd1+br2MT3tkdLrpBOjbcD6XzDPU+gfxogtEffIPGvfupnP73jE5Ce+sI24fmyYxOkbw3yeXv/RQRg8GmHxGNdt/5DFBELJTQCO1hYYRwURqhfDWIHMiSRHjGZ1kJz79hGqHeVykga60Rao95ffRfM7OtUWSlikeu6JEpKHJFqFRXjz9YljEhjfqqHEFrtaXMdXU1v964kg2uRPiw0lRjS7AtkxmGHEEsJGmMQGsCuhot+tssBtssEmHT0fjILoftg7YBXq0puYLzM+BdkL5Bq7UcYSaC5nu/7w9v/QzwbdU4/XtYf1+1kqgqTC/CVMmMydRqZbuHbfrbBeWyiyUU92502Nhu09Vg0dMsaYkb8HMs8wVqYVmNLajRkfXNtat0XessMkruSr266hmqNhIU7B20iYSuf3ZLZWPLlclrlorGUd5VoHFYHn1Zxw7pbsbBTR1zOPv2UK2MkptPI+1Gvvoffkh741mEHcabD+F5MQLBOInOND1dMNgzx/QcMLvAhTFItu0CeysXR8BxztDWuEDTxgqWHCX1epqlxVEqbhYyKfJ5Y1KMVlTf/An5iuaBh/bT23CVaNM2bFkh0ngPlyq7yDLFybfOUsldYWHmm2DHyec11fIrTE8/x8bNn6BQ6iRfgdkliCdhYa6HXPoMZL+DZVnYto1jyeVMcN+2swYE12SCkS8d5ItX//F1meD4HCCCqPIRcB67Y9f9w/fDw/dvoZj3cEveKmZkNQt0+xjg6BJXr7zClSvHkeEsXPgG0SRw7d8TuPef0BCFcr6EHYwwPZ9h8cgz9A5spuru5NwlOPy9v7jzdkjOYpSyrqCVwnYFrmX5/XwWGokX0liuRrggbGHkv7RCaBDaukEs+PNLg67NcFZlZmvg0fWgXBVkS8L4+ZWVeXjqN9Q13nvUiQwLYdrphPA7oYVe1uUUa8JqXQeC9RSkBKRlpM+UJXC0JqgEYVcQdAzlY+aIJRMpz6dWBVqbrtOqWh5F+uB4Fwmlqmmq1hqnhCZi1xpNVhZPLKhpa7HZ1ClpS5hRGFuCp4VhVMSKELoUYhWmiVusXW36lZcH1SMB6VOfGldp4iFDbdb0Z6/Ltio+nV/U5MqKUlXhKYnCxqrZWyGuKxPA3dMDnVwA+9UX6bn/0xz+wbe4d1sXu4ZNyaFQOEo+M8Wm3QcYPZMlffoldn/4ES6/8jybDz3O/EiaPYceY/7iT/E4wfDwJubzO2lrnaA8fgRK0PRwkur5AxSAhexunB1fmHWDm9qzVh+aJoa2TpD2ysT3jZPKvaZV/GWys9/DnYJMFXbedz90tWO//C0KKYfOrRugMoi0ZpmbuUQyliQY7acxcIFcboFYU4hYww7GLr3ma8xa/vWrmtECDAi+dWbrqkzw9WMD3PulQ/zB767MCSIgEoLsokuhALGlV4BfvCPXPVOCYDRDJBGn6r6JUoPv+j2jziWSTz5M5fQouXQFtwQVb5CJqSuI6f/M5oP/iImTf4ddMJ27py+ksO0U2zaHWMz1sqVrw12oAdo2wlXoYgVR8rDLGquiEBWFLim8rIsuKYSWJnuQRhNJCIEl3m8jEOp64KNuALguC1TadGeWXSiWNfmSouTX/+o9glVN2qxqhtxrnZyVqqZa1bhK4Xr+iIW6ib6iXu3PJtZkAVLWmiQkjlWrGUI0KEhEBJ0NUHXV8oYacjQDjQKnT0IMtHaXKbbcTdRg3jdxzqoxiNVBhXg7NU5/PeRrgY8WRinIgUizpCcCll7NKoQCmp5mSWPM3CshDAjatqndWVKsCKaL23PuWC1Qo32lGEPhu54vk+aYoKg1IUnG1kmIPSiWFfmypliGatU0emlVG9WQ183L3oz1vhOZ4UwK2h7ZQ3HkGfbufJi5uVfYvGk/5SLErCLx0Ajkmujboth93314E1fZuANobSIRPAruLC3DQ2SzIAlTXvwW45fnmc7sIxyFmZmPqqW23xdqw58I1fZFoeK/3K6DQ0bPVaSQ1R6qVpxcywbKg78l7M3/IDr2nhB7Hv0f6NgOF8/FeeXL3yIQgIBa4MKbs8yPXKVUvUqueI5C6RwBdZhzYwt07jhEfukSExNx2jvwS0pyORMM2IYhClqwd+tZo3VXd7x+bIDwlw7xB34mqJQi2fwo8aYztLY/wOiVMnemy0yTSHThVqd461SCC+dM17pYk/m9XSp0sbgTLrxAoGc71fJbuEBXT47uFvBEDyw8TfdDv23SLwt2PfoIA/09XJjYR2NiG9npl+88AFasDHY1hCVjFB0XV2gkFh4eUtoEtY1WChxBVXlIJbCUhRCCqqy+j3bFwHL95vrilXmYl2sq2mwWrqcpu4pCxaNQUpTKfs2oruOxWjGCsPNLmpm0sUpaWFKk82aeL18SlCqCsqup+gPMNecHvSbaXi8zXW5R12soVUzHoNaaSMDMF0aCgmzVKLjEg4r9vcB2x9SZaug7r5nKmMzwfQ2A8s4Vq1wFC6WVSEmgEBFgg8NwEzha+eLSpjO3t0Uujw24Spt5Wr/Gt6oZ6na2Kr2ache1Zi5f4szzu0SDtsBTmvms4qcnKiysoy9bKZrMsFCCQkVTdo2Em6eqy8PvXl0n9NrA4W4ckVZgrkqgkmFi5Dk6t3yK104VaNv7SygcSlWYnfw6C1evkJl8jbOXr3L14hZe/buvIQP3Mz3rQPN9JKPAtl9AECWWnCW29VdIb10QduLjFqFdKBFB6hK2LmMpUARwRQAvkEWKMKFKK4GSg6cqFMJ9VDq+KDJdGZHc9Agb+lrJpkBbO9g8tIWRyXMoO0VbRxw86PjQg2RSkBk5TK4YIi6fwXNNV+5aEHQsCNrmtWfLOaNsvgYEI398iP/xym9AJUc19xwL8/soFisUshWa9OS7p531JJcuTpJoCrJ3Syvbt8GWPu9tU571RzgItncSbEif/z6Nnfdw6TLANkpVSM2NQ3gDqTdf5gfPw2yqBZIRstlxgtVXyBd+xLOHufMA+MEBxjZ9fekTweqMy/OVNmpzefmSIl8ys3mlymr41MJsQqmMZmxec3lacWlacWXO49q8Yjqtmc9p0gXTPVqumkH2qqdRnl7WkLxRF6LW12er+HOFtUYJV2nCQUEqp7gyo6j6GWRLSJEc9JBttQFm36vosmJ03tSM3t/3/ObZ1NvJYKoKJrNASi13zWg07IRNPYqY7S27fJSqEAtJbEtQqmhcb0XO/nYzzxoDYbpUdd0praSKqqbWgvGXTOU1R6+6/P1LZU6PGI9KWG2cq7WpA+ZKHvmiWc/lqsZTCk8boNZaXycIcLPv+26zwIY4UMzz1iUYn4Di+DcZ6j5LcWScIDZ2YD9trdA8vJVkApQL/cNJ+nscJmfLiIWjlC//La2DTzD67T+nZWgngaGnhWf/hmi03n0AH2v5rIhv/5snQ0O7+eZLp1iYnmbHxstWyv0AACAASURBVE46o9sJOlmaWrfDVIpHH/8C41OAbMOOfYjw8K/z2cdrXbzrgKAPhLu2nAfLWfWZR44OEPuTB+jVkuk5cEtvUcq+zqYtH6eceoEwpXd8PmFK9OyJ0twIdP4jSqURjhyD3/mVa4SDvCMQFAI+8xg0tmxCV4ZoGHoSmrZz756dTE88jx0YpLUZzr51jKaGk/zm7z1C26YByidP0NYJfU/+Y6J7HuCBe8D+AK3uRhGnvE6LsfCBwd9T1Eq3puvVZv8gU8BkcwWT7ak1FJHnQbEIU3OapbxmLiNoiptXY1SRiEhiIYgGDVCFHdPu7tjC1A19ayNLimUjW26mwVi3e3vKAFmupDl83uhABv0QqjOmsIeEcZUXGik0zLtwQXF1Hiq32SX5toDmvVIuvskYRL3KzU2fc7HiDFFx4fI0HLygEAeqvnaQwN6osbcr2l7SXMsZ4Lg4pshvVTTFJOmCIhmWIG9uLFSfcdWAbaVGKFZfZh8cK64mW1IoDW9eUVydcTl6wcP1MLqzxZXbU38pSmVNtgCLeY/FgqC9LClVfErWWqFx5S02wTtllbSQg/aOGLEA7D+0mZEJi5ZGwdTUWwz2Qn7yTdglWDz7Co3JAyh1hPLCa3R86HOk3zyO9Jrx3DiFyWdIDDxJJfFnIh/sIhzSuJUqyHenpVx0+lBW349F4E/FRz7yb3Q+e4zm0hTl4hThoX2QGOK1736V/Vun2H7fIRavHqaYvkZk/rfYtWUz/yJ+gW/8FDI5gdICrSyfXfKMhqwHnfdfJJ3aCL7u86Hdit//nd+C8s9Itm9DuUskDzyEPnecZAQG43/GseznyZBE3+ZdEGiSZNgWGoFxSTwBE4f/ku7+J3n4vlEI7+Qn/ynAv/njE1yZ9JvF5PpNUfVHPAKfehQe2pmBzo8h5t9k5LWn6djwa3jVk3T09DA7eYVSAdq6oFToI5TN8rNn3+TeA/1Ucr2oo98ktP8jVFXwAwC8G6nA7Qi8ClGTwjKZWraoSGUVs0uamYwmnTW1luUGl7pover6EbtnKKbFJc1cDBpjgkQEElFNLASJsFGKiQUtwkEjRWXAUGNLhW3VVGMEYq0clb/jaGo6fcZnruJqRmY9JlKGhgtYYEvNxnZFcEgb2yThK/tc1Ry+BGMZX/3kNq7Jbdfb9HsMANcFeJ8XFaajRdR13q4HkjVmwPPg8iScPwFDwwoRqyKlwLLB2aMYatG8tWDWTzoLx6+6dDXZRIIS2xbL7ILyRyXWCjJo/0NVHarUN8jU3Nprg+qlKkwseBwf9ZhJa85e84w9lwvF/Pq3yPNLoq4Li1mYW9LMpRUtMY9QUGBJiQjUGrlW16Lv5rFtywCjrz5FdzecP3eBrYfuhRyoKhDawuZf7CD7+vM0bopBcx9tc+M8c3iSg/mvkMnD4L6dMHGS+eCjhAb/ShQCCagUKOkQTqAA3rubnSuU04RljJDYidf71yJ59bc0XXMElqqMXFyio3mKof6d2Lt6ufbs92lpAhnuoZp9naWlCt1tS/yHf70J3CQMNDD5zLN0DfeSmbpGsQKR6AFi0Rm80iUqeivHT51leHgnVvAqDT02l984RU+vzZvf/AoNYbg0Bo83D2KN/S3bWoypbzwMTU3tTM7M0N8KIg65NMRaOjl6LEp/V5VqcZRStYnxuRT9vZtJzcKWX/4s6RMZGjru4cqxNxj80C/zTz8zC/Y2ovbTBB0olrppOHgPl576DtKGaMymWN6GdJo4ceJ5PvFPP0n2+Hc4egH2Bo5TqrYzn73AQPD7EH2AyasvI20YfOxxpo4cprl1GG/hDPcf2IOqzlHRnUj3Glz+OqNjH1Cgd/iQN9yV1+r8LvvAaShXFem8YjLtMb6gmPad32s1l/qNqwaIpmPUDBvnSzCX1lyeVJwbdzk96nHmmuLsuMfFKY+ROY/xBY+ZtEcqZz4rWzKzWUXfLqnimtmu+lmvmkC3Xu48FVRdsxlentEUCxB0oCcBDw1pQo0Kx65iWZ6pA16CN8chVbiZG8TbpLesusv8XgDAensmr34d1Exxg/5LmnqaXL0e9NoMEPBcmF2En50E8ZrJqC3Lw7ZcAgOK+3o1LWaIk3wB5nMwk/EIB8SyTVatf1PXUZyuNoPsroKqEqsyQe07drh1681Thk4/NlLh+29VeOGky9GLHrm88ZZcD/zWHqUCZHOayXnNWEoxlVYsZD2KFWUaYvRKnVGIG9cq79SYxHMvj9AQg8TQ77D1Q49D+wAnTr3O3GIUOlpQZ23SaVi6nOPqs1+jKTrJw4d2MTbXRiQEk8dPMiv3Exr4f0TOjhEJTBCiapr41Luf9QmEHKLlLJ4dpio01a4/EqnMAgUry0DzeWbHXmRy6iS5YxnmZiDSfh+hzXuZGDtOyD7LYB8sTl7itSNvMvHCs9iJD/HqK9dINsHUDCTsI+jyKE7zF4jGHHZsf4DGuOZHP3yG733tFBu3HaJaddm9uYXGBrhvbyMTE1fYtfs+olGwrQShEKTnZmhMgmjYT2lpO7Hdn2RmLEzYvsTlkVHyFYiHi0SjMDZ+ASXg0g++hnbDPP/UG3T39OFd+jadA+10bpXMpyGY2ElD4wS5E3Ns2gOD2/cjcGlKnCARHmHvdmB+mmDsPvZ++l9QKp5hYvIkQ70NpDM50ukGkskBpsaBaxUsYeGVx7E62/jW08dIF/sZufwGWsPZc9DZ/AEFeofBT637cNayPVg7BqF9+hMWsjCxoBmbUywuGarC9danf2p/VtpE2NpvQxdANmfAKhTWxCOChpiiISZoiAoSYYiFPKJBTShgdB3DDgQdQcgxHYS2pVfNrtWDtWUJ5pYU0xlI5SDhQNyB+/sgscfYMzm2GXKWRTg/CaOLq2cZebcM5nvNDaL++6i1AGijdU3pRGJbCslqM9zaf6kBY+06VTy4NAnnTsPQh/3xF0sQCGhaNsKuY/CzOaiU4eyox+4+SWejBdrI4im9ssZqcoOerg2/m4xeyxV1GinMWrSkUXIpVjRnxl3OjnucueYxObVsMnDbRy0LzGVhztGMzigSIY94yEi0RYLg1E+7vw2e852OSQxvBEtIUmf+hKbNH4e5q+zaCiRbyJ1xGb32DFu2fwJNjMRQPxRf4rmnX+bRX9hDdT7IYr6I0/NvP5+LdGN5VcpLjSRCIfL5KlUrjKTy7paTl8Nt6ob5HOWEJBhtxGr5z8KbeEATg77hhzl19AUyS1fZ/+BH+fY//JDNGyHpQP/2/4rp8ePYVpCtm14msevXKJy9zK7hJEuLGTYP3s9S5ThC54l7GRgY4sTXXmZr3zQ7t0NbIxTTh4nt/wLM/BVNjY/iZtNob5H0wjSh0EMM9r4EAi4sQu4aJOffpKG5mfJbp2mKgit2s2VvL7gnmB4bxJbPMZeGnUNGtejK1CSPfHIPkyeOkWi6h1juGqfffIPudsAOMDYCtvMKmXQXnneV3o4Y2m1HhDdQKY9y5q1jdLXaXPnua3S3Qa6YRmtINuwgaZ8i0FllT3SQqemXcCvw3Etn2LEZ4gmolM+wfRvQeJCW0qtY1gcAeIcOi3WnoOuACq5vMFHK9/0rKuaWFBMLisl5QzPW6CkhwBErFJmsyxxXNSiolV/Bj9CLmmxBk85rYmGz4USDEAlqYkFBLGK836IhSSzoEg6aAdqAbTZCI51maoeua1wDri14jM0bV223Ci1ReGQYwlshGBDL4xNiBq5ljDP4zTdOidbihlqQ19khvfcY73WKFjUABMtvjBB+2iotbj2D59/fTAkm0jCcAqvByMkFg4LIJs2hbjixCPMVoxW74FPmhvI0fHYtm0esHm+p1fTwTJZXcY3NUsG33CpWNBcmPX7ylku6TuGlvrpl3+RJWDcTrMDojCYeUbQlFe0NioaIJOgILFbW940CoztVAwwHIDb4eVg8w8zJ7+NJmJ2BpuZReh89xPZGKC49RcAZZurUN2lsHOL+/ZCZiqK0g9X3MZzQvV9WVYuKzhOIRFlK2YTDmoKcJ+hF390XLFkUnRSy0SG8kES1WojQRmj9XVIjX+LFt14guwSf/MQexs8/xcMHIRrfw/TMEpfO/QW5InS27mRqEeaf/SpdGx7g9IUMDTHYeDAEi31MT4yj575NQu7jwV3TzKehtxNOX4aD90D11F/h7PodUq/+JbOpPjpaDtDQrpmdTjI9YQLza1Nw/74Y3/lpjk89sYAuw/kR6OkNMTvyPWZmYee+Nq6cA68Cs4uDtNpgywpzl1vJFKC17Q0KS7CYNs1JDWqSDXt+hdS5b6H1JM0PdHP+J01Eo12omcP07obyW2Wqnk1/7wALiyMspaFsg+edItkJl48bL01pdbBheIANvRKGushlv0ZX/yJLs5tJ2BYnrsB9uw5+AIB35vBuSHeuN+dUoxZdvyi9VFakci7Ti97y7J/WZnZF+vY4tu/0kCsbgAnYZj7Q881RXb0islzrLi2Xzd8VipqU42HbhrIMBwXxiCYZESSjkIxAIgzRkCIa0IT9f2NbRi80YMPUoodCM58xDhUW5r17ktC0DZwBs0Fb0qe0pmA+e6van8QY39p1W6eHac+RrDbE9Qen34tuEDc8N9fMQwobS4rl2l/9+ljWyla+G1ld/c7TkCqDWATRYLLzgAOhfugdgPaLMFuF9CKcvKrYO2juo6dWVPWrnsYSBvg8DemcIlswNGihophJe8xmTANWoaSWzXZHxjX5/PrgtxaIguHV9TutDfW5qrnDZyYmFzTzbYpsUVNxDeVarxOr18gH3nZt+DaPK5PQVvhrkvF+2nce5Pyrr9LZ0Uoun+HIX/89Bw4OUSxAuDdK5/b/kuybP6LsbqCl+xz5yQXyDT8WXjlHMLwItsYtVZGNFhlVJeJG8d7ld220WpgrpSDokYxXUQuSclsDWvz3ohx8QX/kgctUvBZyqafo6W0AnYY2QUP+CsHYkwQbT3PstZO4KglA6vwpkEkWC5o3fvqGf401EIOrF4AIDbECbc3d7BqaAedDOC1tcPYVEsE4afs8yDaKuQFmCeI1fw4hBG3tcEnB9kfgvOs/vr1wDSC4EbFBUSwsEEvuoa0lhSWqhFslvaEoE5MX2doPF8dhdBKSiThTKcFUKg/8FEiCgJFv54Ac2fwsEGL2xRD37N1GNnWYQjWFreHSKHzm493E21u4cPw4/QMbKRUCFItnuXRyms5WiIpfZdOTv8XMy3+OlBegIcnjX/hd0i99aQUAtdZ3JMKSWi2/32pQMO+u1lFBfS/KO769FEBfd57rnV39edbqLcWKJl80bu1LeQOKSq9ke6GAgQQkhKNQAPrbBI6E+UVFpVLXyMDKbGGt7qNcs6EV/U5m2wbH1qTDsBjWxMIQD2uiIUEkqImHoDkh6UhKCmVN0NE0xSRCSlJLLqms6T7E/5y+JnC2guOw7CogssAEpLK3qv1JNDZCOP51M4U0C8+vn9XnFK75+/eqG8R1WatRZhUiiBAKS7p+d6y92gqIlaJbjdZTeqUJJVUGMQdyENMIY2mCfsbdewROZs1/H59TjC14RALG8cHygTRf0VRdM4w+nVaMzZu1BkaibDqlWMpBPnvzM7LXrVeZ9WRZK4xB7VxsB3JrZgNdD7J5M6qTLWoKZUUsKNG23zgm3l6d753QoLu2PkQ5+xKBSJJTz7/KjseGoPExcj/6Uw488dt4c9+jIoPQtAG8LGE5QSazhytLY0R2/zmOkLjBhKmVVoyEIFUIEHzX4AeQsVOERRLlehTJYUejBLMOFa+IHvx93Mt/SkNklLx+iKlrL6HtFgpjR2lyINH2NP/Nv0oytZC8YX29Zjxrxk8USim0svmlD83yhU8moMuj+Obfc3nUPHo7dsM/fD/D/9Lwh/zbwb8mEgrehK1Zfczk2+i3QjieRDqLENnMwpWn6d60k+IsPHvE4bsvuDiBKpZlIaW83olk7fE3Z+hsTvI//WYHM7k4Dx+a4cVXJ/j45++lqek4qjpPIhHBUdDetQXiW7n8g38g0QDhCERj24EAx7/1lyyW7gIFquss5fV6wz36/eb4p9fpiPDWxveroNKfkKPiavJlTbZkZqsq7oorxLJzgw2zWU1DVDA9o9k/IGmKCVrigta4scRZKtQMbA2V5bpGdaNSZRlQa0e1ajJF14VSEdK2Ihg0GWXAFoRD0BBVtCUlW3ocEJJsCTIFxbPHqzQ3CKKWJu/XrTa2g9XvZ39+xiqvwqlR03J+89stETggLIQoIaQFWNiy5F+tEpCou66sdoN4TwNgdRm8BRopFcL8bk126yKEMjRxvUi6T2fP5+DyNRjYa5xAbBuj17pbM9QBcsz883wRfvhGmYd3huhoEMuZXLaoyJYVr513mZg3dlae31zlelCoA77qmgC0fnMIR1fAzrbE8r3uaZN0NEqCDkih8JQZ5Tl6wUMnIL9UB4AVQAoyeU2mYNZ8MqoIKTO6oW9iiXSnskAnrllahFjCI1UGPX2R0z+9yI6H/hmlsf9EqBG8fA+Hv/EdEh1b6Wn+MGX3J8wtwZAzfNfjc4sAWnto4WJcJD2k0DhOIxFri1jKH9ZHz0LfwCTJCMyn5wk3DtCU1PzP/+ciUws3CNOFWOW1V1NeEUKghOA7L3r0dxdovfACjgXhOLS09LCwEOef/OsSO7Y9yB/817fvLF87rooBusPPk11wmD/2NLkUBIMWLx3t5m+fniAWvL2u+fpjagH++OvTfPqRDLHoAB9/dASKh1nMbiGc2EyhnCKy/QDFy6+hl56iveMepHwDrwQXz55myy9/nOSVPLs/9MhdAMD1wK/uz/XzPrWfCXgfKIDWzsK7YXa4XN/x55srrhlYXypqCr5cmFfnnB4MGEpIC81cVvPpAxb3DNrL7tyFiukCLblGk7FcNXdgOq2YWlDki1DwBbVrmZjwNxTXB0ohoODTVVJqggGYC5pRipE5j0jANG0UK8aZfmxOUfX5sMYwbOg2G6MQYkVm6xKcuAaLhZvfV1P9sRHCQ4gyplvSIlDzMSRSByg/T3RBzRBXLlOhShlrI1PzlIhlJqSuvqmv3/gXMnBiBAbOg9xhAo1AAMIDsKUP4schW4FCDqZTmu++WuLSpDQ+khVjcFx1NZWqqQvnczfWanXW+VkoYpqbElFBIirYuzFIOKBwLLM2lRbL8a1WmqqnsKTnA+X6tc10DhbzilzJo1SRRINg+1XSm8XIa+uA7wgUrQaau+Bb37vKjo0gEk+QjDzD0Rf/lJbGZjZs6ECVT9PdGMOJZUk25klu+CitiwFcZ9vd36e0A9LzAyOJVh5KlJEiTkDuJNDxSR7emoNOBXqIK1//M7r7N4D9IudGkzffodaAoHnmVxi5Z1/z+Fe/mWQhl6G3ex+nz73Fj17dQL5s89rRAe7744N88R2A4LXSEyTUt2hv+wiRcI4r46/wrec3rosTt3ucHTUBVakShU6YOycZGm6ABpfxUxUi8yXCTQeZuvxtOrsnYOOnuPrsN5lPQ/mN79Pfu5fDP3v+LtQA60BtvUxQ36KX4P2RCV7/81WRPdp3ftBkCh7pvKnBVN2aJJpvb6TBDkF1CTqT8PA2h2REErDMUL2npFF50YKqK6h4xrV7MuVxftKMVOSKmlLFZIiua4brXc9sgspvfV8m7PxGGgW8fk4RdBSOLZYp23JV+5mmGYDf0g52P8ubnRCG+jx7GS7MQ6F6qwyw5gOn6sDARkqrDkC8n9PVUd+4Y4TgpfBfUvj5lUDg3vRRyhfg1CXT9dm32Yh4ODZEopAYgq0NcGTWp8/S4MU1b13ysK3afVupNefXUpLrUJvSMjU92zYBWDRs6sRNMYvmuMSxPEK2wLbNoql6isaY8NegRGE0Qu8ZFhw+5WI7LAu6uxXzfQplwUJWs1hQtFU0rqvBXjOHereOwglyuUf48KEJTp27SOvoMzS1gpOAzNwCY2fbCMe209KcYK68xNmjr6Ac6Nj9b1BWI5LsXd9HNFVjUKwjaOnh6jyCMJYXQ0b2QOn/JnMsQW7xOTZ1gadmSadvj1yrgWA9zVgDwVxRkytl6N3/3/Lmj/+I/bu3843nVlL4144OcOAdgGCZMFU5zJULP6YqoLWpkdTSeutd3xCo13s2NmxopWVfmXPPgSxNofUUodSD9Dx4gNLxHzIxeZlYBAqLU2Re/CYtUWjZ/yDBngDMZ2lL3gUKVOqbw5taBxTr03L9vqJIFXo9ePRdE4oVWMxL5pdcMjlNpVKnD+pToaPzpmT9if023Y2WqeH5m5vWRo/R9cx8Xtk1MXJXk0VDFAbbha8sA7myZiYN+YIBxWIJ83nems5UbTKFcv4GwB4w36slDvcOgLPJbMg1Wx05C8cnYS63+r1vDzBMxmwJA4Rou845s/JzuwqEqEnJCSxLIETZ1D1XNf+s/5B7LkzPwbHzMHANrE2m3hoOQXIQHtoAoxmYKZugZil9q4BsfYozmjAUeDQiiEdY7hiOhY24QjIsjbt8CCJBiSU1nhJUPEXV1TTFBPmKIhGySOUF8zmBlB7BsF7laFKumtdMWrOwBLkmTVUpPC19ARt9V2nQV169xv2PdVGazTG85ReYGP0R23dDdS7Ec5dKfOZjERYyx/BUgNa+QVoHHqQ6eYTF4LY3kNW73owshEZqhdJmhlSICkrmTK2OAE5k9zdmL1c+nSuco61pmFL6PC2tmyjOzLNiRXwb6xGuo0SllKRT0HTlj9j/xMNMnHgBIQZW/d8jRwe49zac5a9nF86z9SOfI3vsK+jIHvx2mVXg93Yl0Vqa5jjxNOQLMQ79F/+c49/8d+wYnqF84juEOvbS5mqCzhUCIVCyl0z2Gt09eePRFWolHm+4CxTomuhi7c/1+xbs6unOAOCuksIWviKxZ7rPl+t/i3kzArGYM/W7mtGpJU0FrODChga4Z9Am4AifetLLFJAUZkzBtgRBx8JTmqADLXGbDc1GKzJbMo0PM2nNQk4xl3GZy8DikmZxYU2y6nGjiY6VIMeDjS3QvRWcDX5NqNYAMw0zGZbP5fay5no16VpGFFqzTf+8CaXL5ZcUGtsygYtlG6pZrhHPvtGzr33ln6lFkIumEcZxNMGAILZBc18/vDpqANBdB9hu+g0t0xgQcCAREzTEBI1xSUNEkIwIYiFNJFhzAZGEAxYBPwATwjAPZddUNm3LPOOxEEylPWxphv6lBCdgmAcwVK1taaYXFfNZSbaoKFclXlCvrKF1AO9O1QDv2wn58VfJqYeIyJfYvm8/ldSbNHRupq/zBLmFt+gYvpeZC9eYPyrY1vMzpP0RCA7eK2QWlHN3dxHpu7j4ygRCCoRtISwFooAObfpMoTivBw/+PqkLJ2nZXOaZHzzF44ceAk687cBs9X4tGd5+DyR3M3vmL0wPQmAMrEHwVp4/Y6V0kD/4vVf5X/tuDwQ726F06RvE2yBbDBiZtnd7sexd9HeVKJfijL3079i9+1EunXuODd29XHj9u+TyEGsYZPNAhEp1hu7toFIhZCAHG0IcfyF95wFQWSs5z3oXRfpM/53qOn2vUaAaAbqyTgwujNBwTcKsqsiXPBZzHqmsIpvVy7W/2uiDE4CohoNDkkTIgIzS2m+nF8sD6mijvamkwFImr1C+E0RECEIBQSKs6G6S5EuSdN6IWY/OGVHtfAnyRUNFrWQQNzj8jWyoDUJ7DE1Wa4ggB9lJWKqsL7Z9/eGtA3hVPO3za6IMxOAWmdJ7E/wCgINWAkQFS1axLYUlLaQM+aon7q2jDT8rz7sglkxgpLWfBfZBbBi2n4CX52+xVzg+tWkZvVZLGiujeNg4tRuxBJvGqMn44iEzNxp0NAFb+vZKmO5SX+xcad8Vwp8rbIiae1T1BAFLMNBlc+lalUB4BQCrFTOWk7Y0qazHUl5SrJiRCFv6Ij/i1mWSdwqIS6UWGuIdiPI1As5Gjr32Jnv2SrxcmaFNnyCgn+K1F4+wpQ+6Q9OMLzxOdu7HtPb+DY7S3G1Nd61so/yDhxZllBYIL4jw9V2V007/lof43p/8IfvvgVy1iyce3cV3fvgSkHznDIV/Ta+cf4Or82+wd4sRYxACdm29wIlzw34XUx0I/seDfPH3XuV/uw0QHBsDx64QVTuIiqeBTbe25LpFVjgxcoLu/Z/jzPNfob9/I8+98Bxd7RCMJai4YRItO2lNXMEtXmE2FScYaGcxfRgreD/Vy0fYuqn/Lkih2RJsiXCsdV+m1VWs6//085gR1jsoLEuHUS8lVjfW4JuMVjxDf2ZLZh4qVzCzfzUZKtuGaAR6mgXbugXlKkRCJuoW1Dq4WFZpsaSh1mzLzPkF7Jq6i/Hxs6UgaBull2RE0NdmsXvA4f4tNrsHbLZssBjskjQlBW3tgqaWW1APSWhogmgnOI7pCBRCIE/DyQnIlt8O7Vn1QXBlKSrlsEZa5ecw+3MAC6UkEtv36HN9sXCJEC6I6soYxC1iq3wFxlIrbu62BU5SENwNuzrXpzXr6c1Y3KyhSEjQ3ijY1C3Z0S/ZNWCzs99iW4/FUKegr9Wiu8miNSlpjPq0Z1gQDgqCtlH6MYpB0mSDtgAtsaRZZxXXUOIBG7qarOURifqjmAPXFaTzmkxRkS15lCqKimfskVgDdHLZkUS862ywsauJS9dOEen8dey+FPEEsOnXsTq7aGoQRJub2L01SSwCl69CZ+JZtj32UbRowNJ3f2y66pnrKS2NtCpIPKR2EG4I5VkoK467NM2D999LqQKxzhjFzAkevo93CX5mPy4qGN64icZthxibNtc+7MDurefX9xP8jwf54tXfvM5Z/rolLKCjG6qZU76DzLvf60Wgn9Tpr7BluBPXE/R1w+ZemJw4RXvbFhrtI4SD89gtD9DXkWUp30w48TjNiRlam7rI5qp3HgCtCqghjTdYRg0KKgMVRH+V6gB4Ay7VoTLeBvBUxZ/8Bak8PEsgtfy5A7/6/E9zvaVQCZTYuQAAIABJREFUfeSqBaZuUq11fwqyRWMZU3u+hT/6oC1BuqjZ0Cx4YLNlWug1y8amyxuBXiEQpd9EYAuzGTmWEb4OOhAKGCorYPtRttJEA5KBNsnmLsHmLovN3RYdTabu09UtaG5d/7wjNjhtZlM18mdgLYK6AGemffUXfXsAqLXJgpSqUhPPqng1wc9aVliXKb0X3SCufwp8AJRojKOxwDYUn6wipRl9ELXZxtvgFgolODcJ4qpfa/UVd5x90NYPTcHrwS8SN4FKJGS84lobBZt7JFt7JNu6YUu3xVCnzaYOi75Wi65GSUtC+LU+QTiIyf789WTVdfpKn36XvruIbYll/8v2hLUstWfbpkFqzd7pN8P4869+x6rrmQBS1XtTIpYDPnEHqNDM1AWGhqKUZr/GtdfHmZwH7/RXoKkBK15mqTBMKN7F2asR9n/qEPaO7YyemSTozFMtxu9+ISUEVFxkGarEKQQUrszjaQvlaKguYnfupPHgFhqb4cqJC4QST1Cu9L67GrV/bWMR6N5+icyFBQbbzX0O2gYE92w5d92NfOPYAOEv3RoE21vg5Ck4P4Hx4bttybsbP+S2FeT1802cOj3FxMwlBu+5HxH7NIsZCDjzNLTGOXoaaGglFNnCwvwZpH6WienLhHsiVIoTH4hh3zny83rVFzNwWvd3WqOUpljRpPOKVM7MQ3k+1VBTUrFtUNJE11UPepqtVQtGsHoB1cSr6x0FahuVJQQBywBfzcQ2FhLEw5JkVNLVYLGh0Wag1WK4y2LbBslAh9ENTUYFLW3Xn29DEDo3QyxmKFDbBjEJr12DsUUzz3hbg8x+Bqh1Bagug6C7PD1v12WJdQD4XnODWBcAbf/elNBa4qpaTVD5gF4xv4rbywALJTg/BumzK5fBskwdTw4J2qPiuvpeMFBrYhH86odsPnMoyNYei6FOSV+bxYZmi45GSVPMoiEqSYQF0SCEAxrHp0lrYCvXiFUvB3Xal7/zB/hDNQbCFkQCggNbLCwprgNArY15c64oyJcMI+Iq8PSyDYUv41Z/s/UtiNFbHxPTUEjnefGVi7Q39TC4IcpM9nFQJViaYeTKYZYmz7J5oB19/lXGD88zNXIcXSngBefv+srxvCraslHSQlAiqG0CKoZDFUkOqdPMnv4m4NK46RN0dLSQW3qG9vsH3x3w+vWUMxcAexvJvgazj0hJwPcTDDmwd+s7A8HJ2TZ27v8Ntg418rGPP2gCQHF7oHcjEGzr28zBfQ+z6wno74Ly6CtMXPkGnR2bKFXGUPaTJGJQPv9tLo7n2HHos1ybhM72AfJXTzM89IEbxJ2J2ta5gbVoeO0ISNXz57OKikzeqL9o/E3Gf9m2cWbQGroaBYmINBsDpgao9Qr9uRYE136vZSD0o3THEgQdScgRRPwuv6aEpKNR0Nsi2dpjc+8mi742wRN7LCJBv53eL8E1xWFTOzQPGZrWCfhNO7NwYcbM/nlvg7nUKJRy0bqKpoLSLl4NHJYtFqormdJ7zQ3iFnOJWkvfSsr3Z9Nm+F1rjxuJp6/HNFSqcHUaXjkG4nLNP803PR222dcuiTkrj3MwBI4FT95j8fnHHF9iT9HXKulucmhL2DRGJfHlTA9syxjrWqLmHbnCKtzo3DVgyZVRn6oH2aKhJQKOJuh3CK/d7FxlGrRyZU2+7FFxFZ7S111HsapRStziBtz66OkcIhjczYHdcO7SOGGniqOf5sS3f8jE9AQTcxCLPsaxEyNMLX2Yq5dnOHgwCvmxcCWYu/vLSSm0LfEsCy0KBFyJ44WQeKYmVrn0h7HIIzD7FhTGiTjz5CrAorwjS/nx+7tgJsf4mSlKVjsCiW3JZVPdkP3OQLC5uYviwl/T0N7L/GRq+b7dsFnSV6tZtaeueU+1dIpkd5rjL/YyNQejUxCIf5TGto/Tvq+HqWtfp6tjN2+ehq7WOLmJo2wdhFCkh2h0gbHJDwDwrmeEay1nPH+eLl+GpaKRQat1vBkRVzMiEQuZjs/+VouqJ/A8f5hd+yC4TtaJvvmcZX2GaPkUaTigiQY1iTA0xqCvxWZbj8NAm0Vno01zokZZ+VRGDJ7YB00DEA4LQ39aArEAExkouW/PNWAtrSWFMrQhFUwfbJFlHdBVJ/Ieutl6LQDWQFuiVBhPgecpqp7CddXyeIi+AZ2+/sYImSycvgTinE8NIrAth2C3w4P9Fm3B6+tn0ZCFVpKhTofWpKAp5nd4hgWhAAQsjS3EMp3p91fd8vLWTzka7VnTnFWqwFKppkmrlhO4tWtC+Uo15SqUqtrMs6rrA5v6qeL6EsM7yf4AUvMXGbl6HFfB0GCS0ZkwwdgjtHf9Eq1dH+Zjn9qO7GqkMQGR8BIPfu7TTJzPowuvF6RsuPvcgZIIC1zh+efs4eoKVemhRQM6f/JfXhs/z5kj59HpY2Sru7k6AmRH78jnT81P8tOXr4FOIa1uEMIEWT4IBnxn+RvRoZEbgGC+cAxLDHPmxHFahltBqOtmxW/1WpsNnjw/wujJHLs//otkc7B5ew/zC5fxCkc5/eNxOtrvI5U6TjQCsXiRSGwQtwyvHHkJQg/S0HzwAwC82/XBtQClfGf1ilfz4Lt+86u4pollqQiuMlli2TXAWXY1rjIbhuvp63wCbxUT12isWs1RSoHt062RoFy2RmpJWiQjkqaYJOCsANXjQ9D2AMTiwoh0W2ApmF40Qt2eeufXamVrrQFgfSb4Ho52avyzVQ+AZYyah8BTCle7lKue8Vz0rq8T38418jwjLzc3X6O5LSzLIdAA0UHY2SwJ+fIrJmARBCxJX6tFS0ISCVqEHDNG4VhmdMGMsBgas3ZC+kaBlFiR8VPKrGUhjEu8Yxn3ifmsolzVVF2PStWIaysNldLqtyoVzWeZIX3pt8Wvm0KvbIB6bbr99juDk0lobm0mFAgTCm8iaGdIdLfSFDvP0uxXoOE3GXvjGwxt3E6x+Co/+/I36N73OUT5VYKl8F1fTg6Ozwx4CB1CSU3JyuDZNugAZI/iuVNs2bYF0fNrXJtQ7Ns9BF7ijnx+30f+JfEg9Ow8gF19azlLk9JvdLLAkQYEd285B/ZqA+DXjw0Q/ePVIKi1JhpxKHphLl6G8tgLWLJSp465ok1ar1Fa/1oPEBfm4eyl10m98XW6mmFhSrB11wXsgX0kEm1cmykxMACJOLj5GFL+GO10cv+v/zPGr6a4dvXVDwDwTtKgy80pq+qAK7SlFqtV86XQyOW5H98hXpkmkhqVtJiF6YzH3JIilVcsFYzEmTGxFb5prqktasycYT0Y3oi2YpULufBnD80XK1U1jVFBxTOODhV/BKg/CQd2QeJeCIWMJJclBeIqXE2ZaP7dJGa3BQjvRTeI+jHGZRA3GayReDNpsadMt29N7/VWIFhrihI1ClGZdTGZB1E19V1bCoJBRWyzy30dAp+xplIywYinDQ1pKHCfZsdvYKEWBNW6LFc8BNe75DWBdc/z15sS2FLiKkP4u34WWCi5LBU8loqaty6aQXm3en0GaEmBkNr0Rvu8vr4Npnll/sd9+7dKw+TYApZd5B+++yaRAJTHf8jLR87T0tXGqe/+d7R3wHe/c5qGGAxu3Mm541+hIfQtgvmLd38pCQ1KgKpiETWNcwEPW0hk+RyZ6W9QLIf4+x+c49rLX2X7pnYuXr4IxN51g1A0BMW3/g/uPQA4T9LUOkgooFeBEKzMKTsW7Bg+b2iruuPI0RUQLJTKoDUTM0mWMmfYshlyBWiIucvg53ne8st13eVX/c/XAiFoHvv8b/ALn91I0+AGMgVo7hnj8HOAPEu1OkuA4xw9Cd2dG7Hb2nHzoKtTfPV//1OK7hn23/MBBfr/SVWwngoVmEmRsCMJByUhf8eqSaC5nolr51OakC24Oqe5OOVxYcplZMZjPKWYyWi/gcZQqaWqkUGregrPU7ieWrbTWcaKOq+1tZuurgPvsquMSn9IMJtWLCyp5X/T2wjx3RAOmzlFS4J0QZyHkTmT1b4dYBJ1n71uxrzeUe8GIW4TmKw78JI3+bzrDHpXsljLqiBlGePpoVcFQOI2LtDyafq/qXgwngE5ZepjltQ4AZvwriAtg5ImR6zKGMuVOpry/2XvvaPkuu47z8+9773K1dW5G53Q3WigkSPBCDHIDCJFK5qWLdmy12N55N0zOz5zds7aZ2dtz9kdjzd4d0Ye22et3bUsWbZsZUskRYqZBBiQMxqhgc45VQ7v3bt/3Fdd1Y1AEAR1Zsd85zw0gK6qV/Xq3vu9v9/v+/t+PbWydlyV6qwwLq/eZ5jUvfajV4WnDWgGbF+YQZt6XrEEM0mXiUXF+ILinfOKVEYvvweoyBlYFgQCfnuOI5CybNVrtG+r1Ymurm2XpeRuYZPl3Ee4FnL5BB9/ZBv1CcgU2nnwvm1cuTBNIAglDZu3gFWAlpp5nDog8nG85N9+8FKgsoTSEksrLM/BQyNFAKdQgsVv6Yizizue/AzbN0FD+yc5cuIFWpsTXDn3Ol0tt46AWmt2bYCJmUYGLrYze+b/4LUDg+zdoimWXIquWjbp9qo2blIA1tXfxTtHe4j92d38/pVfp1GdYENnnGymSP+mbhpanuS+7Ybt6ypwPY9iqUShWFx5FgoUCgVKpRKlUmkFKHa1KEYOfB1mIpC0yRVgcQQaEpA7/RyOMCS9XVvXMzd3CVrauTIBsabP8sgDIWpDkJr7EADf527tOrv2VWeZDCOEAb9gAGrCmtqIERgup6zKAtVSgfA0IaG5NOEZV+4RxdkxjwuTHldmPEYXFFN+ZLiQ9khmjMRZtgi5oukd9JTGU3o5uiynrqpBcXmB9Re5bEFhSUG2oDh0qUg2rysA2ADBfp/44jNW5QU4fx6uLLyb99/NpYnfbRdbTQCSotISIMuMSLnq/0W1Jsv7OG9wvXJLk/aurpRZlmdIIqISgS2DoLhBKWuVv14ZBEsuDM+AugRSeliyhGN7BNdoIls1a+qqSVea8fmSUR/SV282rqoZ+xfRfsuNt5xqN+07ngalTdZBICgpzdiCIlswkenFqRIHBjzeGVAcHFDMzGvm50zfH/6WoPxRwzGfkRwWxALa+FtWET71ag3d5fdYNhu+NWk819uPm4eCamVh8STnRlqo31hHNldgbWcPsZoA0XVfoK6phxPnLLQQHD0PhQtPMzz4v37ga4oriqBtLARSGdkoqSNYhTxO7hlS80c5+fyL2BIGzvyQ/g3rcOwl3CL82s+vIRy8tevu6IO7+pIspWbpX9fM1Mw0DzwSxnYL/NJjDjkXMiWj75srmVp/0TXf+/U8z06d6eGPsh+lJXqRXGGRqVnI5u6HpgP8ylN72bfdIu+ajFe2aHpdM0VTSsks/1uTLXjkCiXyPiAGrBKP7slwZRCIR0ilu+jtqKW2s9E4WYT3UpsIsZSCcxcuEI/WwfkT9D3ycxRS55E6z/D8bmI192BLKUEZhpEUAv0hrr2vuE9X71L9lVFULfRSCqJBSV1U05TQNMQlkxGFnynwU0yQV0arM5nW2DZEwppEVFAb19RGIOZ7+EWDmmhIEw5AJAAhxyLkmIb4oG36AG1LL7NBy4WcZXICleb9YkljCYnSmvEFxfSSZt5nfjsa+taA01gtfi1gEA5fNrY9t1L/W3HPRFUdsyzLZq3eIRudyvqwrxhSjtCq7ZHK5NHbnSq91vX87zfiXCd69cymRwjTL2k7K0HwRoBfBr7qKE0pGJmBA2fg3l6FaC8hpSAQgNA2RV+L5sCYv5i6MJfUTCx4NMWNCHcZAJU/36nquTPqTAKtyixmgdbCl2PT5F0zQOezipAjGZ03UWFng+TcuMvBiy6zvsRebpVm9GqZtkgYaqPQ4JNywgFTYyqbAQtAVmtV+m/SJG5vXRf2yrCJijf0x4F7OXnmAJmBKTJ5iPQ8QoN7hYPPfpPOOti1ay/DowfZGIfgzv+BlpM/QM/+uQ7VfkYUQz3kiiksx8bSWaxSgKibIBNeQGoHtIPWyohXSI1WNkUXZFiYVgelsJTEwTGycqKIK4o4nqIkLIqWg7JmCYt68rkhlgrf1c2FaRK7HoIgDJ+eIljnb5jtPXjOaTava+ev/t0Ofvrqs8wt3XwRu39tEulBW/cu1gibo8cP0t21jdHBJF2dQ+zY9yn2bvg2Bwe68DxR2YUuT67sVa9q25oHd46xmIETJyHecg97t73KCy9+nT07umjbcYb/8TfbOT2Y49hAEOOSsrqu6/ntEiWknEcAzXWSR+/Oc+oS7LpjPS8++zbdPUF0fhE5H6R3xy9TXBhDiDw9j3yBmVe/STSwwOzsAkuXYN2undjAzm7N+RNvYiulEH5e1VPe6nTuh8dNRIHXWviqAUZKs3BJTNQUCUjqYpqWhKat3mMuJXCVJpOuaGgq3/sVr+LikM9rUlmYCSnCQR/0goJIQBOPCGpC/o46JIzJbcAiHDBgWAYtKbTP+DPRjPYJHGUR7khIMLWoODHkMrNQ+WC7umBzf5ku75MmNExNwOCc2a29VyGf6y3+nvbFAXIYJbSqY1235skHTYQrhN8L59/XstaqUqvIRfr2gN/y9fwmdCF9vRplFvb1ndd4Xs5/HxbEgpXaqW1fDW7XKnrpVb/TCuYX4dBZuKcTZKfGthW2BaEOzd0d8NxZmMpBOglnhxXtjSV29zjUhGUFAJXRnSxHfUppP7VV2cIZlqYilVNkCqZXb1OHJLugOTxYZGIeOhsFqbzijTNmvCSXKnZL1RW66iRZogGa6wSttdCckNTFJNGgwLGNLo709XBX2B8J4dtnue/ra9z7uSdJHp5gePgQQsO9eyCvGmhuCbE0/lMidb9Me9Pf0brvs4zv/y49j/wOzJaYOPjviNckKE5/DxW/Y6PnrjvnqBA1mTyFRDMZZilaC9huBIUCUUJTwtIaoYyKU8CW6HQJadngOIgAaFx/wxHE0lEKGiJWGu0FyeORtwrYepKm9D/CmiZYSjE9dghbgRNqItHqsjBVorvzTrKZIcYnj3Pf7iWSSeMO0twCDXs+CuePQN0iI2ea6Oxo5IXXz3LnDklNYhfTU4eJ1j5JNvMCRTdMax3U9dZQl0zy9HPQNftttu/4JNs/Vc/SsQM8/8oAT326H9wQUxPH8YrQ2ARnLkAgaPYn/Vs3UshAb7QDjzW4hfOks/CxhzYzMrEISw6R4BBbe6GtdRPtuzo499JPiUUhnYaNdzwM2STzM+9QX+8wP1PihUOwfR04bZ9mV90ApXSevt6tNNYuEY3fzbETb4FQBLauJTDQx/Dz36RrZw/TJy/T3LeRcFRx5cwxuvd9Drz19Ic9bCEEtm2byaxslHY/RLX3CYLlfwsh/AJ/Jd1oS0HQ0SQiktaEItUsKbqaoG0EghdTxr3ddSvefeX2ibzv4GClNMLSRnLKgVDQCBfHIxCPmNeOhwU1YY+aiCYaFESCRvsxYEkjpmyJZQdrSdmCyUR/CxlfOHvGvO/6KDy4HiI7TPrTtvzFew4uz8Jc9r06P6ys71SnA6UwqZbZJWDhagBs+Tj8V3esortaorLCKj+E8FaFmO+HSCpXhaqWKEt9mg2r5wcliWvcgAVT9M9riIXA8cWny0B6o0KgZmUaXfpekp4HQ+Nw+BTseQhkyESXoSDU9MHuI/DSBBQ8mJ2B2ZT5ThNhQ37xtGEXq/JrlpnJrkmZF11NrqBJlyCT8yXLsppUXrGYhTPDiksTRsllfE5QLMLEpL5mxFcNfk4QmhqhtU6ytlmwtknSmoDaqCQcqPQdXrvux/sGP4DUOz+m6Jr7376+h8x0G9rdz8Qk1EQj4L5FWx8UDn2Xtm0PkD/6t0zPTzO3CGvuepDUWz8kP/V7Z2X7/yVCcgu5cBy5kCRUHyTnZXDcCAiN1h4SidY2ShttT2m52LZASY2LwtWmB1ZoF0tnjBuGE8QrWmixiBWsh+wSzoXf03SkmL+kiMhjzM+HCAfzrG2ySU0NYandCGkT66ijy7pAqO4XkYElcoXnWFqEuedeIhCEeBLcwD7Gp77PR3bDXFoxMXuYQhG29x+A/Ke4MvAt4sC5t/cTcuCunTEaG9fDuhDf/Q9/xZY+uG83HHx7gPZWM60KLrgFmJ+B9ZthPgn73zpHU3M3bW19jA4dJ5eZoa1zK05XL70dJ5g+eQUn8iB1HRLr/EvgtbC2DcINm5gaPsuJQy8wPAT33PUxxsZ/QltzH7/wWBevvPUSkdoF3NwZ0mkYXdhOOJhFUmLnI2tg4gzpgyfJFqC5tR/SOZqbYeLCOcbnTfYoc+5tlqb/nnCgFdt1jfGiq1yE4sMI8IOgw/h1KOXvbB3bAFZzwrg3SAmRoCIWhqklTSqjyRUN2Lmlikmu1n4R2m8zK1BunNekg2bgObYi7INhIiKoi0JtVJCICuJhQTSgqYlKVMBPy0mBlr5GqatJ5xUD44oFP4XlCehMwN5tENpm3MiXnd/HjPND7j32/l2LOyJlZdGbzpimb0aBzmrw8UOvtuo8pPaLb3plnlJUGxSXmTPqFtHPpuzft+J6uup6sVXXE/71RmF0BuZzZvJZZem4qibzGzWaryarlP+eL8HoItyxAFa7wLY1oZAg2qO5rxOOTsOkvwlYykK2aKKrcs9poWTqbFprv5ZjQC9b1KTymlTWGDWncrCUg1RWk85pBoZgPlURTV+cu/aupwx6dgAiMd++KSporRV0NAi6myy6Go3maCwEjiV9Nuq7NN+/zyNe10VmehPF4gAqWSLacoShEybjkM5k6eray9ilywRDawlMJ3FL03SthaZ6IDWGDt9HxN1PafT3te75piiWsqjaAPFkHbadw7WzxgRNCAQRtA74GbY8igIlpwZBHk0elMYWYSQ1WMpBeqDtWUqWBVYtTmma4JV/oUvhIZKDHnWRUX70Cjz20Q2cHzyB5+U4dwU6G49Q0/QQi0OK2lqYuPQPxMKwbu8mJs+epbVrN2SPsLiwBi/7MjkJwS0Pkj30CrW1DQQDzZALQ/0QLWvuxc0fYPIUPPjwU4xe/DaXh47SkFJ89nd+C+/0T7C2bOXK3zxDMNROw/onIDTJxdd+xL67YS63ke0bGnj78H7C1hUSG8MkWmdAfBF0FpKzzI7M0dzcyzsHX2GbtQPbbuPM66+wuRdobiM0dpbt9+9Dv/gGDXdu5u1v/oRC6SK9j+xk45TF+cHL3HPfdhZPWmzrOko83ouzpoHJowMEQhPMzcL69ZugJsHQkQEaWzYTqknTKoZp77qHNw+9SSwCtW4W23EcLA0KDyksPni98//yjtW6n/o6Va7ybt7WgC1JRBQIG9sWRAIeiYhgTUqR9A1y03lNOgfJrCadgWKhKkWqKykxVarY5ti2kVpbSsOEhJqIoL4G6uPSqPyHBXVRTVu9RCuoCVtYlqGiz6U8pLSZmDeLYfmz9TdBeCcE4r73n68DKYZhJumnT2/lvsEKVqKUQBA2Sjh6Cc6cg817q/NnZTByqsKvMh2+3DPogKj+XQmzVSjeAgCWXR0CVBwpBAjXd/xYdT1R/V6K4BY5cw6ODcI6CaEoyECFOFNVgrvhPVJVKcFykCs1pEogUhVaesCBSDe0rYX2czDpl8oujig2tRvZsqJr6r1LWU2+qCm4ilxRGRJCQZPxx9xSVrGYhlTOdwrxN2D5LCtaGkr+Xa5OcUbiRlMyGhbEQuZnTdiMxaYaQVutxZp6i6Yai0TEKBNZosLIE1WyayuiYQK8f1/ILKMjz1Hf2M7bp6KsbRllbd82Dh46SXYR2hqfI1dwaN98D5dPPUdddC/jEzO0dSvOvHWIzXd9muLkR0gXXkQPfEGHe/9CFLwIyZBHbSaOJ7Svb6vwZMmwJIVAIxE6QE6kjLyZGzY9b5ZGySwl33jaceNYQSC3gH3xv9ZJ62WC2Tom5ydZVw+feDzGkSMnWNcN3/vxIk999nEunH+WuVMv09MO6PXMpS4QDDcye/YKrS1w/OA5ulqgrqmV2sRRXt4PbQ1TLE1DW8sckegcc1NbiS4eIRTchOjqZc30IGeOfBthmXYH7U2SOZ0hlxxGvOOydyuMzkviEz8mEJpAleD8MGztP4eXh52bYXwaBl46RzgE8ZorvH3wNbb1w/Qi2KTYsfNOSpl3aLzjKaInv83IOLSr04TjHZx58w12fOwJ0oe/yq4dewiE15A98h3a1m+lrcNmdvgYoRAMTcGWuhAUGzl6yuPxR/upr09RWDxLUNWzdj0szE+TSc9y+iIUvTfZs6WdpbkxPJ00EaDW4AkPreWHEeAHFAWWTVHLfn+gCQckUipsaREJCBprlAG7gml+X8wavdCZJc18SrOQ9oGwyshWVdXw8JuOKfk1KgGFomYxDTMRRSxiBIrDAUVzrcC2BG11mrZ6QbagKZYgXVDMLClyebOm2x5saoXAFl+ppqwLeR7Gh2E2c+vpRc+USlZGOF3wuS3wF+fgzmOweSuwt7yPkKy0RnIrLJRlX/tVv0P4YKluIQosuzrIqufKqtC+nNte/TvfzumoyxvHFM+egy9vhUBv5a0IH5u9m0FAVtXD/OsnPRCz5vuwpcCyNMH1EF0PvW/D4QUfpIowMufS3yaxJCykFSNzHsmsIJ1XpPKKbB7yRZN5yOYhWzCkqEyaq3r4qo9qd7ywD3yJmAG6+rigLmpILjURQU3YCG3XRQWJqBnzoYBckfq05Mr+1GUg1NofLO/vmBzbguu+SldbKw0N08zM3QEBzbaNEldtZj55ir6Nj5AefoahS0matx1EWvcxO7KfRBzGB75PWy20RB+H+FFSU5/RVuwrQth95JubcJIKS2iUKOCSQYmM8fPzgqAdgl6OoA7hqAglVaAkkrhWEUuGECKEKk6gkyOEBv+NjnXvIjh+lmxgiNbednKzY+y/mGb37keZuPI8T33h51mYeI1cBjrXPUhk7Ru8/kKQd04mSGdLVZs3MFZJg4BpmH/2wb4+AAAgAElEQVT15CgQ54eHXNa2Kh6/5xTRUCPCquHyiSxCwtwSdLbBmtadDOXWMpuPQOC3lsldJGCi4O8v281/H6gy0m6sn6Zee2SSP6ZuTzMP22A3PU7N6LNEYg7TqVouq19GHY0i5W9BAkZy/tOb4MBZ0PqXcNMe3xz4dUp/d5bG2iT/7DMuG7sgGt1KIXeKxfkzNMszWA5cGhigpW0fodh6Mpnz2LKX2niYU+dneWwfHDgKDYkxch7Yoa2GBWoLsyWV+sMa4PuJ/m4EgdUODlqbBct4Jhp37UhQUBezKBQV2ZJJNy1lFXNpxWxSM72kmEtpklmTlsrmIZc3PVaetyqn6Nf0ylFhsWiAcCHlK7dYmvOjRvvxY3c4OCkPpQTpvGQurZlPalI+i6w+Cn3tEAj7qbuy9dElODpqtD/VLZJM8h6wVMkmCg3sgP/uCTj6Vfj+AVjbDI/VAP1lECwDYB5ErjLJy2lJof0ZmTPKGYTKSdb3biWgq4t0rn9N36VCKxDVqdU8yw4WWsJ5wXPPS75/QHG3Df/6CZA7zdOVv4DIpKnT3WykrFfVnZMFGBiF9WkQYd91ISAI7dJsfMk4R5TT5mcue4TsAvs2BUnmNZYlOX7ZZT5lGtUNG1j4rTNQKkE2deP3FYmblp5w0OiJRkN+xiEuaKwRNMaNklBNWBAPCaIhw/YMOWJZe7Qc7SnKXoPlTZa46uuq+MPdOqupdSOUCrD/wGFqGqC7YwRqP09ojQupbkoXBan5BdKFGu7ekyTUlSB//givH4ZH741gxwuQvQsv5/H//s04X/rtjRTm79Vu7b8kPfevhJAFHKcOKeN4KoxSyjezVWhPIKlHCA8lUyhLI6wIYWI4+QJWIUNW/UBPHfwRDQ2vURh4jZJnbIQGTo5RXwMPPnYH546+jh2ChaEfEY3A2jaIB17hrRcT/Ke/H7nhmlTdeqSUwvM0B0+X+PEbgj/89Vl2/OLjhEa/QWu9UXtZ23EH76S3kRTv3QljWKxlUaTYHIPj//gddtz/KC9/+1l27eggnbH50h/+lCu/foh/3/lVQgHnmt5/Gk0Jj7cKIU4+rUFb/MW3LZ79sxY29pyitR7G5iDvNfLoP5tl8lW4dPkNbAFb7toLuokr555hSw9kClvp35An610kX2qnrTFnWKBaCpRWKM/9MAK8rcB3tau1wND3lTIgaOxkzIIQDoAXtiiWIB9R1MckjXFFa0Kz2OD5ZARYzMJSxuzk55KKpZRxk/dUhc1ZboQug5MsXc0hqGvQPHuoQDAAa1ssLk+UKBQ1xVLlw21sBafXJ7/YhqYuZmD6MgxMQzr/3skv5fuXyQOT/g0pM4VimtBT8KeL8Lm/gf/z25LUouSzH5WInUCrqiTddKISiQlhFDSE8AExUAVO1wuj3i36K291yx31scr/KxdyFnjSaEMFYub+ToJ3WvODVzV/8TxYM/CVX4LQU6DjgBKVjMCkuQf6JqLA1SJgnoJkFgbGYcMgyG0mQg84EOyDnl7oPgGXUyZtaduaE5cVkwsFZpb08uYolzc/SwWjQuK+S5AVjuP7TgpqawR1MRPl1YQhHmE5yktExDIZKxIUhB2J47tEmDaQSjuOrga/VQ354prbAXnraQd3LcUCbNwQJh6tJZWdYOL43zIxDQ3x46z96BdA5Bl+5hDhZkhdgMxijgfuXk+2MEwg/ADKOs/Lr4/y+Mdg7tJLRENtjA/+RyLN/1EXuv+KCBtFwN6G9GIoHBA5BAIlFMopUiTt2z4lEDqAKpwnv/i3Olh4GzX6U6JxqLMh2LcTGlph4Dz9d8QpZTxGL8yztn0blvsO54ahu2MLJTtAcukoX/3hLRDR/A6A2aTmhWN9dHT8lERtB3PTo9gRmEzJWwK/8pEkTtq6k2LhHTKXrvDQrzwBdh1/8vuv8fQr3TxS3MNXfvst/mX7XxN07Bs7y/sfLlOAf/W/hzn4D5BMbqStrYdsegIWthOMnmJjY5jk0hCoDVA/SGMEhudgQ80chewE2oINj+8je+ptEwEqpZC29He86kN0e48gdy3ewrWAsLrvudIa4RvdaqOG4QABSxEKSH9HbdFUo8gULDIFExkuZjULGcVsUjC1JJle9JhbMqw8z5fZcj0T+RXLJBpAWCCrIo6FOQ2NAlcLzg57xhF+SS+7d69JwEObIOgvruUmcDENR8dgfNE0wt7qfVrMwuAw9F4Ges3wE1pAv6btt+AfA5L/9jsB/vnXLL76IvSu0TTX5okEbSwRwNNQUsKw0EomGWc7kqBt4UgPq1yvo+RHjg5SVyihZcrKanCxkSACaCRalK2YAnhaUNJGk7VUtCi6Ak8JbGnjWAqNJpWB4WmXc+MFHq6T/Psvhqj7ooZ+D6HNY4QGLpvPvph9D/FMFY4rBakUDAzBjnPQsQEsx3xP4RDEN8Get2AmD+mSaYlQWpPKKCwLv+8Tssl3v3okXvH1CzqCmijUxQWNNRbNNYK6mKQ2IoiHjYh7NCgIBwShgPB7UcspWj97UF3fhBUeg++mlGNIPOoWQNBEjrNvvkpjz2NYsVGS02PUR7cTqD1BbQIOHoPaw98kcefjrOtqpaR6WZo/RbxpJwFngnSqwMilQ6xth8e/+CXSA18Dr8RSYZxIBOYmYmx1/oD51Ji2Gp5ARHZDZPuzSvU94TmtKDuKLM2Cm0EWr3xJZA//pci+SZ18FncBArU7mCjAhvZNzM6cZeHMEMI9Rl1dO05hkLHJPrq3DKJnBxnN7WXj+jQOpxkaBxl8kFT26C2uc2ZDdHTA5jc/FUeJZqaTo+zuf5STc/Xvex2dEb3s3VfgyonjjD5/nv7t7Rw9a6rGPz3QwyPczR/89hH+bftXbwyCQi6z7Q6dE1D/6xx982v095wDBVMnIRKBYM/v0FQ6yukDP2HLnR/nrWNvsr4H7GgP0UI79Vu7SB55Bhm/C/tDmLs2YeWDAr+VdUFzeqpSF1Ra4FjSSEXZmlAAlCdJRAzdOFdQpAuapaygqUbRWguz9TCfUixkDHsv5bP2cpZ5U/my1uiqKhnAwqy6qqijbYMbG1ug624IdVWzPwVy1LQ/pAvXFYG4uQiwAIeuQO8JoE1AWFdUTzZI6v77AN+4MwSDArKGqag9AV4BtAvYKATK8lBlXz1dRHq2r+BSQqFwlUPBtciUbFIFKLpFHBQRB6Jh43MmpZEZyxeh6NlIGSDkaCK2xpElJOra18NF4iLxzWKlxrELEJbQHYSHLKjToEtoXTLvOy/ghPnsmcItjjVfKGF4Eg6fgc7tIDcY4YNQUBDdonlsC1yYg+PzPv0jxXtOH9Y3CqJhQW3MRHfxCNRGoD5mUxeTJJbbbkwvnwE9w3S2y0o8sgJ65Tp4mY1qUaXq46c+b2pjKrQviVbWuiu+SzRvvq/GmhoOnX2Ozg3Q2tsMoydYnO8gmxulpxMS7b/AzP7v0LThUzz/rR/w0fsg1ruBgVeOsVSADT1JrBJkTn+VxSUjAr+hp56aplqCcxMkZY7Gjse4ePFH9D2yxMT+7z/uqBO6ad+/gYH/GVof5cdff55H7q/BFdsZG36DYMtmQoEzEKknUQPF7Fka7/006sT3mUtBoGsd1H+GJvdPwdnBQvo4DdFRAmKCqcVNdPe14qVf9ut8Nw96q49MDrK5C+TmLpCosZgbfx4d/Pz7DySQkNZ0b9tJafEMqakxMrnu5d//9EAPj4jd/ME/P8K/7bgBCEq5Qm3jW3/zNT62737ePPgajz2wA08Ucb0ushf+A7bcwZZ998L8CT5y/4O8fegVOnrzpAtnCM7G+c4PU3ziky98CIDXAr9b+5Kvn/p8t8dLYTTxTAnPrAzSb/yS/vwN2MZfLRyQRIJmMaqLStIJSGYlqbxn6Os5zWJGM5tSzCxq8kWzyJ4e1mjP5PXzfio0uIq959km2AlZsKEN7tkO8XuM9udyBLgA88Mwlbm13r/qRSxXhJOj0HUY7q4Btgto0mjpK34kHPhUZfFylv+UJqpbdopQK41ldcn/ZGUSi0eMEg3LzFH/NcV1FkvtUGGXCiB8E9crL/HKv3ag8hpaorVjdgszHpyAtw6bz54r3noKGQWFgvFBk8NgbTQC04GAJrZZ0PCY5rNzMHMIxnM3fj3bgWC4MnalgGBA0NogaKuXfsO6IOGzOeMhSSRk+vfCjvDreuXaHsuqM3LVey73+JXdJ6ojP1nlTC5u4gaYLhSFWBaHXR03VvNmzZGaG2VtYwNT85LZ2WkCNqxftxPljmELzeLkIAXdy+TxH3Dvdrg8BNui++nf3MvA4CA1EZCBXVBTQzQepSPWzvlnvkoqM09tvJFo752QHGNdGywczpo+3NYvwuW3Ue5G5NLz3Lmjj2B/P8GLT5NoXkNsk3EzKi7maNnxWRaOfhfv+Ctksi04kc1MnSvQ0vYOR07Dpvw8yUWos2uYnJmg99G7ePsbX+OuX3kKeP49R36r79TIKDTXgXI6SeWumEXiNhxvHTnB3b/0IMXxIsHYNmBlgfmn+3t4WO02keD1QFCsVO58aDeMjbxG/zqQsUbefv1FWhrO0tvdwvTMcWqHHa6MH6O7LczaNaCLGtuR5Edf5je+/AkoLf1TBsDqRIz+QCK/6umob1CMXm5yrnKPqPj3GW1Gy3+7tgRHVoCwNgqFGkGuKMkVlTHb9VOk00uKoGPSUQdbXaYXIRTQ5EpweFBXlIn9w/I1LrsTsLsHWj4C8VqzEDplBZjjcHTE9Iap96mw4iqYTsOLA0atZpcE7hUQ1Ghh+cxZVSGglEkmy5pkqlLYvMpSSVWBpec/zqr67q8j4bOic174hJrITV5PXD2+dA6twyasLgg4BUffMJ95Om3uwftJ0SvPyNCduwz9SbBrIKCN4kz8I9A7C59MwffOwVT+6tewbIjGjbC17asF2RbUxqAhIVlTa9FSa1idtRHLr+lByPGFrH1msOW7iViyqk2m6nYoXWkFsqpED4xYxNWycNcq1+qbqlPceFAKT9HUsoamSJiCtw3Pm0RxnKFJza5+eOvYEXZu2cK0hkCgk23390NmnsLCJfq37GZp5jzpBZfs5Vfp67kb0aHZsPVhxi6+QLTty0we/CMspTgxCHt2LRBNTJEcHyRW9xtcGPopEwvw4Gc3kb54lqFhiIQmcAcniNd9nsLc81CMEYtCstBObfQSBXWZkZkrWNbPs3NzD7nCZVJF6I4HsB04/YOvUR+H/PELtwR8q48798DcQidOQDE5DtwmC8S7776Xb/zRKzx8L+QWTgLdVz3mhTd7eFTs5ne/fJQ/7vhLgs5KeNq8YYAzZ3qWd04e0LUGJufhO99+kbv32ERDLqnkFM1t9zE/tR+vAEr0U1tzDLtxgUS2FopxBg/+I03bfu+fMgCqWwa8m4klrzUlr3Jh0NcwgKmSUFPKEBNEeXktM0gts7jYlpFD85Q0NT9XkitqGuKa1qKis8GkDZXWtNUHSeUUkYAgHJS8cKLId99wV4JgEeoSsLMLmnZC/RaIRwXBoOkFkhdg6iycHDPite9XYkwpk7gaW4JnThlG5N1pYI+ANQotyvnQajo3VWyeG7FaqkHMZoWcS7W69HWfW04Wh7nxsvwuz9dRU9ycUHBY8dZBePGs+cxF79ZTyNWp0KUsvHUJNh4E+XOm3hYKaryooPhxzfakIcO8NuJ/b1VHNFZhccZCgpa66hYGSX3MIhGFmpDRsA06kqDjj4dyylJU7q6oEiOvvmNyVeqzusXhWhHf9WRcrzVf9U1kXKotyubOTxBoref544f57K/9EtQWaZ8QjI4Ms2X9dkLd7awZETgddfzdn7/AXXuhd9NnoPg9Et1PYl15g3zp55hacGhWp0hm20lm2mlPCBI1iqFx2LOrh9o1rWSm11JTP83kmT+lIw65Rbj06mXIX2RLz1oGh4aw+x+A0EGYzUFdLzPH4NzlUzxw/xOMnX+GbTuAeIGZ05exJPS0w9TMSfI5iEdB2M2cu3TsPaVArwWGQmgGB6GlYYR83hCq8rdrUcwe4BOPNqG9GS6P+sFctdq7fzx/oIfHrV384ZeO8Id+JFg+gvbKaLCtby1Dp4bIFeAXPv8A81cuU3fXkyQPfY2Z6ThNaz9LvO4EU2PHaG3/CKXJN3Hqeynmz1O7ZgMy8YmP/JMEwOtOomukMd/VqfuqiSZW9LVVnIyrr31tR/dqx+vq35cl0aoXFUsa6xhHCF/RAzwtCdiKiAeuJ6mNGAPdQsmofdRFDVBGgvCJO4Kk85oX3vRZMX6PcUcCmtdCxz5IJCrgZ6UF4hj8ZAAuLxipttuxd1DKqMsPL8DTJ2B0Hu65AO3rNKx1oQHDssQXz8StAjd1YxATrh+IWSbUCWjT2Ch9z7VSVXBYJhc6/vMUhp3q+QrdZbNGbiK3rZR5DReYEzDkMXbJ5c1Bj+OjJvK7HeBXHh+FIpwagR8dgCejYN1tPkYgAOGwoPbjmruuwPlFo9taPsIxk1oPObC2WfKpu8JMLro0xIRPajGgFwoIAo5vnFwFfNV1PSmuX/teKRMoqsgu0l8DpZ/G1CvTcu9K569+rLguCK5kYktCCjIjl/jsU/+a7JX/jamFDQTVMDII8b1P8MOv/DEP7IEoe/nIA4/Rsa0Fpr7OYvJ+5gd+TO/OfUwlczREXkQ2/Tzywo+oi8OJF5+lq2kT8UQMx7bRhU7y3reIdu1l6Di0Ow517SXW3mMz/g6cGB1i+x1fhNLXIf0kY4sXSOz/S9paIBSOY8WSpLIwPwL1H2kiLx+mc0MRxDjulQSRSJJEywUQ26gdffF9l2+0Biexi2h3BDm5REOTx4nk7VlzC95adClHMgX3PrQP/fVR30xXmDSGV9mJP/t6D4/ZX+Df/sY3+YP2ry5/fysAUFpkp5pYu3MXqB/gzp5hPhmmcOBlGmoKzIz/hKY0TE1Ca8+vMjHyDTo3bGXi4ima1nYQsO+gFOp+w/6nDn76Gn8X1wHB6+1IK6w142DqO/yZFbVqBdA3sHlZBrqqa+pVgshlQ9IyZdySwl94LIQIoHFwHBu0bVzIvSyFUoFCySWZVctaj7mieeEd3RanhzwmxiscgjU1YHVDYwPEoqbPy7YF8nV45iicHDE9iPo2koXLkeBkCpYuwbExaDqq6Ggs0dvg0VJTIhqwCEqQK2TNbqaL3MOyFFYIiGgIK7B9b6isfxaqSnchDFDiW3LkffKNupnr+dqISpMplphKegzOCUZnNTNpRTKvyBVN2lPdzvvnwVISXjkNYQs+GgRrh0+ICZkor/FuaDliNi/alzx0bLh3m8Wm9iDJrEuh5LJhjVFniQZNXS9YJj+xMtpjVdpy2bVBXA1gelkJqZLulEIihOMP9nKcYZmUt9ZXbRhvtPE0c1BfNadE9Wz2o04lwSpBc+d2ksP7qdn2m/TMnIb8NojXM/rSH/PJR23eOezS1pGmocGCeJiR4zuIW6/R1gSL4wW66g5SKAoKV37E6Qtwz+NPkD0ySqpYT2fHCDT1Q81unKl3ePMbB9m6Hs5eLLFh6ycYeGWY/vWPG9J95hssTcF86sf0dkAy00yxME1uKcXwqTfIZaD+vidYeOObNETg/DvQ0givHoBPf+YudOZBUnMvEgrdnoxVgKPkpzYzMnGFotsFrbdnjKbz/TTc7VA73ENx8j8BvWzrP4flr2snBjaCWymIP/dynqbwV/jK57/Mbzf9P2itiTgV8NvaP4DrdUD+TWj9ZezMERoSA9Td9fMkT32Ejs43WJw7QzgYwa6tp7N4DwQWiMc/isUl3NpfO6Rw/umkQK8X9cE1hKyXJ7S+CgWv+TplEBQCge2vqGVChVcV1enrPn814Fas0fRy1q4yWsXyrtvsqCWQR+k89nKEqHFkCMeyCdkaS0CmYLzbPNs8JuRAX4dkYtysxvEIRCPQuQsStYJQCBxLYB+BN16H/Rchlb+9i/cKEPQdz1N5mEzChRnF0ZCiLgqxAATes0akQkiFY5nPGgtBPGRSO0obtZNkzrBZPQUBCeEgBIIGYPMFA/bF96B1qoGihnRRsZBRLOaNbqd3Ew7w72cHrzxIpuDlUxC1Ya8F9iZBwNFEwoKarZr1LXBiAjJZo9pvSbg4qkiES2zvsulssKiJGGJLuSZYZnHeyLmiOqrTWqC0viq1fzX4WWaerHiYqbPqquyJvsk5vXpMVgTEq3I7WqOFkQBMTg4xG1ni4tgB2urX46oL5CQsTkNH7w6aW4/T0dbI9JxFeDpGJDhHPL6Ny1dO0tgwhwz3EmtdA4v7uWfvNgaPPENjvUXNGo+zJ3tomZ5Hyr+ipn4vm7Y2Er8jSmjyZWojWWrvfYjUhddJRAMsFr/Ilbmn6emqJ6m3UlP/PYjHCLl7aWiawol3MvDMM3R3P8jI1AKxBCTqjrNzRyd4DqPTE3Te9xj63HO3Bn5VX6aQgtrEx1laeJoNHaCdFk6mbs8YTYQFZ3/0NEJB19o7EHIBW0LAb8nZtekcBRfOnOtbthOZeeZVxlpa+d0HfhtdSBMOOiZ3ajk4FkzPjpIvwYVX/46+Lmjp6mHujR9hWTCWhrzbQF1wjpkLh2jaJEleFITsYUqZe8m37NgbdkO3HwALToFQNkyxxiPjLhEURhDW0gWUVcKTIaxSERkAlS6hpI1nOwjtokSJqwzgPuioT18HzJYLC7oCRvpaRWRRcdNeJk1cuxq4It2pb7Abq+6D0sIsCaua1VYsRsIC7S6nodRyPFrAtoJYMopluQScIkqX8LSJBvNF34jUb68RwpBq6ur9fhrf+WH2ELx80cieKfXBb1S0Nu8nowx4zGX9LKjkPaskVxPlbVlhs2rfJLbkmYisnAG1fb1t5f/eUzcXa64uPZZ8MPf8zOkH7bNZzrzOpOCFU9BbB7W+fF0oCNGooKtRL7Mvy2OnvkbQGBc0xCW1MUk0aGrM5RaGMnAhrl0iKN/ja6Ufq7MjUGF6irIO3LtUz/VN1OhXlwuqX6lcO68uNxkPohyBvEdvy07S4TxDmXo291hkFktMT1xi/5uHiUZgfPJ12tb1UZh6hWKunWmvj76NbUyMXSE5PkBTfpDBIZiYOcne3ZspujF+9MN36Kq/zLGJy7Q1Qdg6i1I7YLBALAI4L5C6MIbtneXUBeju1uxc76DtbSwlx0i5OwjmFnFLec6eOUMsdIb+/naOnHyFpvr1tPVtAWsrHYWToAeIBFJkDg1wZOC9gd61iDBaweGDT7NlC2hrDyIQvW3j095Sw6baXrBnWLhyCK3XL6fURXluSti++eLKaPDtP8YrlMh2bCRXKLFp4yWCtgFOITo5dnSER7/4AIy/ysJslKU5cJqht7eP8bMXWdPUCIEBpk/Nkp6D6Nb7qYv8zqa4GyfF7O0HQGGHEQQRrsTyBBZBpHJQSiOwcFwbqQRCeAhb4gmJR3ml0bcd/64HftcCvhXBnrg2ofpa9TlusEAqXbWo32QEsDzxq5+jqzgyukIiMGzGwornyypjWUkBRR5LQNAJ4liurx1JhaioKnWU9lqxnPq0LI10jdv7fNYvg/2MHJO1/76039Sfv032AOIG9dvqx+jbMe40P1ODaa3Bcw0IDs1Cfd6UPi0LgiGoj1UYmGBstIw2LIQCpsbiWOXIb0UC8V33HSvBTiw3V1/1nb7Lt66rCGDVNfGrZdGuBsnqOVy9UTSZFzNfvGKBgGMjNCxNXsRuirPl5xoYeuUcwdC93Lm5k/mMIhZ6jZANBNuZmLxINjfGxr7XuHh6DhmCsJMgvOuTyInv8fDD9dDZxoWXrnDvFsjqWur1IkJDMLaT2bFjsPFL9OYnOH0Gtnw8xvirsGMjXB59m6ilmEqdpTE2STI9T/uuveQOjtJUA4FgjMOHx6ipgXzxAvNDF6iNmSm/kIeGHZ+AQpo75Evw/fcGgiuiP8Mroajg+FmIBw7T3tMD9j23ZWwe+Ma3uXPPRpYyKWamK1kDyxdyt3Xl77s3n6PkwcmBjZw53892+Sd0XIDFosnkBCxz5rIjbN7Sy8LZYULx3+Tl/f83H39iH/npM8zM9VHXp6FrKzOnXiPRZJjNGedX8ALrz5XsAkJz+wHQToOecSFQwPY8pDb5I61dtK1RlgcFhVUSRgFLKIQUxjRSy9u6YNwM+K0mpaCFT4gxjthiNTnlOouaMv7ZCBGqqmmo5UmvuHmD8uXHiZVkgvJpSAi6KjYNoHXxhot+efetkQjhGVsmKVbdC0FN2CYUljiOwrI8hKvJa79upT84m5obFuj1zw54/0s4lJ+GLY8V2/ivErL9jY8/ni1ZNkoWy3XlZeIKV/fwrR5U4l3mm1g1fitzUKyaJ9fekqyog+vrbxSvRSRbdtsolwqk+buljfSjpx10SVGcX8B6eT8/fhr+my9bUDpIS2wLOJBcrMW5/CqXxqC7CbLy47R2/Jiiup9C+gcsvfMcm3atZfzSaUrnh1m/YS/kYODoIvfevweKJS5fPkZP36/xxne+yro+m741LmOvTtMUh0AtyElF2IHayBniMYhv+gX2/+Nz3PfJDg78uJN7d22io9ulZc0Sb75ymPXrIZ99gFDdZXS6CKk0w8deora+F5h73+vljm21CNFCbdcA2ans+zff8I+dGyGXT9HQ3EBQzqG1WHYxKY9JS4KlfHtPHwjLG6Fkwfze8d0pbAlrWh0mxwdZCoZpl3/PZ558grmlRYLhNFr9hMFR2BIepKllJ4gFUtF/gR3+gig4EQpiGk0Qedtnn6sh4yIXFYElib2oYVEhkxorJdALCpX2UCUPD8+XqlKGVSblzwz8tC7XGPTyjtNT4K1CR6VAKb0y+tFi+Sy7a5tToVQJrS2Ukijl+c815zVB9Br9T+XooTwQHCmqUkdi2SC3LHvmKQ9NcFWkqX29Qb1MnvGUwvOU0ZLUhgFoVd1z25KEbIFjW0hpr1Tw4GcPfh8et3aUF5ay/rcQIC1tvsuq8V9uWf/f6W4AACAASURBVFgdOelyNCaqhnp5bFb9W1X1rZbnh/m5DHPL41WtmEseSrkoQsvj2PPHcXWttNIX61+Lq393VXalKnNSTiqV36vSZrx7OkBRaWwZJFgM4y2u4RcfgnPnXieVzUBHjMsjEAwskinCjk0R1vbA8ODXiTV+mmLmBzgSroxNkZ8uEA34u4XCQcamTOp7aiTD7MwJiiUYvPDXtK//NGvadhHs2U37HfeS8bohDMWk6WO7OATDky2kz3yHXf0pLr4xSyw4wukLQ7Q0jDMzNsM9D3Sx/w0Ibc7h5YZRIgvNQYQFM/ODy73Ct1IHFALWdyimpxZZmh+AzP0Mj68hqhff93iM6kUiTespqfUMX5gjnVvD5vUXl0UQLMvCtm1sSxKwTCYiZEPYhrBjzpD/fyZLAaGAoKZmMwjY0rcePMnps89QGj9ARhQJBBNsWdvLwpAmP3GUTPFXiXX/L8INuZSsWdNOpiK3HwBVRFK0wMNCSYlnC5Qt8WwbJS2CnoNDACkrnf5CezdszvxAwA+9PKGq/ezKk9j1NK7SeFr7CvmVU/nA4imNq5Q5PW1O16Polv+NeY42P0ueOV2lKzviakagKLPtVjJBvTKIao3CXCdX1BRLHiVP4XkerlekpC2Kysb1HFwvsHx6KkDRs8kWirhKLbP6MwWNp6RfADeUdMfyI3JhrYoBPjz+cz+EWJk1oCqqk3JldaGsIV4GLK9stuyDkeuB62o8zz/9se+6evn/y3PE9TQlpSgp43LulTdd+PNEm8eZx5TnjIvrFvC0xNOW+am0qU/7r+cqvXx6qup6SlMqzyelcbU5varT9VOwy5tTHxhtS+K6Hp7SZuHFRqWTBIthepr2cHkSDrz4Ep0d8PoxaKwHz80yOQGFLFB3inhsIzMLJoUZqmsDuYnWJhgZr6G9GR54/BEa6/I4wT30b+8jFr+Xng0jHDmVRE0NcvnAAep23s0bz0H/Rhgcge3rmmmo7ycUhuMDDXSsqWFtW5zuhgHGp86Ryw+DmuO+e4DZDJkUnDibZOy1ZymwnakJeGjPrYKfETD43KMZetfdSSwOhWyOjdvrWLe1Dut96ENbKNZtqWVu5AJhXsFxoHXNBJ97JFoRTZASKaUBQdvGsYyc3vLpg2LAKkeAkif3BXCi3dTUwfjkCaRYYssTjzNbhLnxjxALdkNoI/H4AxTrf47o5j8RijCIJSxyhFQIR+vbnwLVuoTwyoQQUD5l3UKAizHdFQKFtZzeKhfTtPhZgJ++br9dWa1C+UXA8nM8pVcV/UV5j3y1eou4RnHZX2CW35AwACulwPV/USYcKK3RqsIuKO/Qq4dgxT9NUiwa9op5rjLvVVe8B8vtE56nyRaUeS8+mJq0plgVfSqktEwDvvgw8vv/G/gtjxe3kgaUfiRoS9MCWf6uy2BX9EyvaL5kKCOWrBamFitKBSvrfWJFelOu8PKrsC/LUVz5NSqkGr1caNCryWZl7drV0WlVa4XWK1P8glW5Vn8u2tLUs7UWGMaBQFgBSq6L0AqpbdyCJjd+mO1bdkO4Gzef5+EvbCF5/hWm56exvCF2bYaRN9/GCgTY1AdTc30w+xotzVsZHoF4JMlssoZQcZjYtidIWMDYOOfO/4BcCnpb4a3jsKXHYuzVb7Hvyc1ceusM9320gcP7p9nz4D2cOwh3bZtDBtYTilxmceFe2nb0w9QxpiYWqYtv5MThZ0nEYW1nD8nkZTZszRGPf4rudQPAOK8cMRuYmz1qovC7T4WpjYyRXpqioeURLp7/KXIMurvuY7M6wwV5J/n3qIsWosCu0HEu73+LNfXm+1zTAPR9iZapv+F/+q0wf/YPBXJFgZS2/z17Zi1TCinUCiawFIJgQPKxexx+7o48ly/8kKPH4PHHP0o4qjn/3LMEY9C+JoJNDjpaKLiz1Gz4e1EUYWwXcBPYwkMKG0Xy9gNgQGmEMjRnY/vjIqRGahBKUxJFvx5mKH1CVyTelBS3WOy5Nm3hZsDP7A6Ni0K5P6voqUpkqKrarfXVLApVTq8oXQH0qlxSOLCyUF9WxS+UTEqy/D48F1yt/LSlzyqVYK+a02XZtPKkr0Sv/oZDVRPJhR9J6uVdcLagaIhbTCy4yxZKJhJVaCVwVcVf8Eb9jx8e//mCn+eZ1o3qXZMRVzfjaTlb47NsUzlYyChsv12k2pT2WvW46jlQLXYtJCtqiazYXFalSMVKZZarZq0oM2cFurrwXN2OVHX95SjXKOlWrQga6Uc3AUcQsKBkaRxbYQuFFmFKOollSSwFXqaO3IUJVHARJ5YmtfQMwcRONq4L4oj1YGeJJtaxsPQaxH+VuXPfoLnj42Rnn6a549fILvw1iXgnqdRpyMPYoQHauz/J/ftgbLSNovMwjY2zJGrPUiAK2Szr+rpBrqe/7wiFsR9SHwXZt4HB196isW4tidgUb/z9AfbdE2FpPguly8Sj0JCA+ubLzE0BTgvB0g9QgXV8/nGPT9yfprXrKVRmEBkogVXHyKVX6dz3x5D8c1JXhonFYXCkjXWdMRayW6gNfJ8LY9vJeQnq5CipLMTj8P+x92bBllzZed639s48051qRFWhADTmRs9ko5vsJlsi1aJohqwQbVp22BE2ZdphO+x3yy9+0qP9YL9StixbDIfDkswQW7KCbqqbU6vneUJ3Y56BGu94hszce/lh78zceYZ7L1AFNAq4GVGoQtW5J/Pk2bn/Nfzr/3fHwunRNp/50L/i9X/jWeu9TCUPcOrRX+P6T/+Esxffz7WXf8S5D3+Y2XP7FNOv0eudo2euIR/6r+BZy/sur2MufDB4b5mMr/7B/8KpU/D4hz/M/3j5Gj15jZdeKvjEJ3+R8f4PmOlvsZX/S16/eYa7H36UJ3+6ySMPXWXn+nfY3x9x+Rc3+fM/fJnLl+B3/tO/ylPf+iLnNuDRR+/la19/kQcfe44XXvwp96x9lrX3/75MxTNwgW8CAzyz0K6S6vYDYBGHs42xVCjOGMQ48sb7JGQsimLUYDVw8L14HA57C2W3+Wb4YtlzEfycV2YVTEtlUngGuVBWUPkWNGpMM0nzXxMAVN+yLuvot1a6J/E98wqTwjegk2pp1iBc9yoCoSaRlUok0kgj3obgoq1dUXoXIpuh0RK1hh+9WPCHXyl45B7LtGh3SeeUwmV4f3tL0ifH2wB+prvWawBsyp8mlJFqACymwQ5pdxzMlvtZ+HMv8wu2RE1gpUl/T4KNl42qMNbUbiGRaboEAL22z4qS9A2ThS0J+1SlLX2ozK15EtJOck6pKzdRQtBIEOke9MJw/9rAstar6OcVg14F2QboAZm3ZF6oXtuFtX1+8NM9HvvkJgc3v8veLAQQ/+Afw9//b88zMJYnvvwHnL8Mw97XGD363zB95VXOPPK34eZrnFmH8TNrXL7n45R7/4KDCVy+/yEOXv7HnL0Az78AvcHd+L1P8MKzn+PG5Dk2107z8H05N14uGXz3Z2ysr7H5wCZc+wGPPnyeqy9f5b7LcPVqGGt56XWg92nufaCE2YSS+6F4mrKCi3d9EPR7vP7qz7h44S5e2b2bjbWLPP21z/O9b73A7/z7Gzz5/T3uu/AKWDh998PsPg+Pfuj7cPmvM/76Ezx83wd49rXXeeHFLzHK4N4y42C/YnBqg1eul5y671nEv8xLz3i2Bq/zzFduor7gnvPQf+ijfPVffJFPjf4NDJ6k2HV8/XNf50MPw9nP/Ed86t96ma/+6YjHzp6imv6QU5/+PS49/x0Orn2HZ1+FB+/7l5T2s7z8yhfp9b/GZqb89Adw+Z4PcvfdQw6ePeDXfu83efZPPg+v/wWVBoF/P9tkffM3seYn3Pexf4g7+5/JWD0Db8F6BEfVuxGSrmoNMf0WAFX1tpS6rJqYURRYIgg409buYmXC1gVS4xd+9pbLoKt6ftqNRkunTEs4mHpEghXOd58r2D7wnN+0UTmlu9nIXMLZkgG0o3NYZ2mZTQBXQ+lzWVknBec66JU5Zl762WSuJCspUB6SG3sPr2179ibw599z7I2TaN5KGNquPN57VP3CqMdJOfSdBX4mkThtDGbjGjJp/y/rAiDAZKZc2wmmzNsHjmHPN4GSpHqeDelKm2fHRCWi2ifQSmSTJuXTVHq1/rkmsIwVl8oFQla9zkwC2POBrUiSZdI+a9Z0GafpYs1MJEwMhTPrwpk1z+kNw6mhRdXT9yVZtk5pSoyZYs0IV1Y8vLXF5KkdBufBlRfYvHQ3f/9/PsPud78A9n4+8Hd+E17+J3zv69d4oPgcz7/8Eh8ZfRI2LrH/2oD1rWfZ2bnAkz/zfOJXgVMP4l7b5Svf/R6f+ff+c374//1DLtz1De7/xG+R/eiPuefBXX72fcdw4xGGwyfZvXnAN77wAy5fhLsfXYfxfXDuFL2dL2Ak3DNrrnBj5z4mB99kPN2jn1/g/Mc/zfNf+efcdxe8vHOGS/de4fK9FU//+DUunevx0H/wQf7R//5j/tavg7Mf4KVXnuDimW9QKrz+/AMcPPEFzp//MFZ/yIcefICiXKffh/3JFdbXz2DMFT746KPsvfCXGA/jvVe55573s/HBx9l58vv0L/4QxnD2FHzhCz/kU4/D2prhFz5yD5sffBR+9n/xr74Iv/pLn2D71e+BhSf/1T/ikQfg+viTfPiXhzzx1b9gc+vLfPITH2aneIybL/4z3v8Ln4QN5ftf+SZntx7guc99njNnR3D/Z3jsg5d55vP/iO/94Eec/+yvc8P+vf8nd7/+d8rrDmdvMpEM6zO8P+D82VTaZuPdowRzeMKiC0BTVAH8dsYB/G7sO77+VMlrNz3XdpXtg4rxtKX/r+qF1Q+3i4SX1ttMOplbUybVFkhkjuxSk1OgpqjLUgCsS0n1+eooPM3+mvGLBMDrXpAqFKWyswPTxCbHGmVcOCpXs1Z1AXRPjnfGUQOfmDbRT3U386gAUANgngcAtHEcwBMqUt4pBxPl5WuCEd/YGNGsnTkZM9PtbXuvAfhsC1zLyNwNIaUGUxcqDtbIAgs6ZZIi0uqMxg9pTctgTYG6UXFKzptbWB/CqTXhri3D3ac9l6aW2Zbi1LI1nNLXkjxfwzPFZD6YU6shY5PpTXjw0jmef/477L0C930UvvOD5/hF/QG71x/iY7/8Mq88+xKXzsLsYIMnvvk5fuF3/l2e/+I3OHX6Eq9d/Qk3XzmNe+7/oHLwoUfP8oX/8x+ysXWR5559lQsHGRfP3M3VVwruv3eb3qX7KV7d5Z6P/TIXX/wcP30W7r771+CnP2HnyRuowPn7YXjzcf7157/Fb//Np6lmcOH+LaZXCp77s3/O/fcDg3v5+GN3sbt7kc27J1zfht29F3hYPsTv/e5nuPrCl9jdeYJ7HvwdfvLtP+TRxz7Jt3/wDS6uwWy6wbkLH2P/xvdYH8HLV05zZusBplqSb/w1nn7m/+bs6U/xx1/6Kv/h3/ttePUGN574c8rqEvkrcPPghzzy+Ke5/56vkJ/6L2H2D7h67SXkey9RIHz0UWXrfacZP7fHwQReuwqbZ36NtcGUn373L/jAb3yW7R8+y3OvbzIw/4xzp2G6pwzOX+Ce9dPkvWfZuPsSvWqDn/3x57n4EGx94D9mc/P3/mDgH/hdm59H8xLHDkqB6Ho3OEqOdwEA5rTuAKtLn/U8ifd15qdc3/e8vq08d9Xx3JWSF68pr99Ubu4q0xlMDm5HHvp2vcetn09EG91LTec0TjLAdxb4SQsAnQAwyYzyDCRLMsDIpFMJBRnngjdvVcF03HlyyHKoysX1leXQi25UxSR9zZtfezZX+omO5Xhv+bpefk3HuFcWNrdgax2u7Thu7Bt2J8q4UEoXAPnUCMRMybMBU79Pz1isWMQZZOrYef4VLozuo+f3KV5UfvHMTZCPkk1/n71Xwz3d1Qc4N/giD77vMXa/9Rx7k5c4d/olNrfg1Npd3KhusrkG2zev85mPA/aAfPgZDC9y9bUX0ew8e9OKszf+grF7hPzKLmQP88rrT/HQ9/8FV1+/zqwKIxYX+Rusn57xt37jLl5+9Qq2Bzdf2uGei3D24gZXbuwh+iLnL73IxvABdn/0Ez7w0Cm+/6NtNi58huvP/T5m8FuIHvD1L/8hj3/kV/jLr36Z+y7Dt38KH/3gVTb3nsJkj8BmzuXePYy3P4/zcOW1GbMCTl2E3/qr8Nqf/RFW4PwvfpjZ09/k2v6nOHtqmx9/J4PiNI88+DPyi59la/RFdsZwbVv5hY/+bbj+FKPLf4XRuOKZl7/Cz3725/yVT8G1a5+C579OMd5nlE2469wj7G8/STFb5+qX/182zsL1l+Hy3fcxHcBdn/77VKN/R4x9mJ4X6A3wZkalu6h6RDcQHYBxSyOzdwEArh4vn9ffrAkvhYPdsee1m46nX/NkmeU7T3uubStFCQd7QahJpWXNHecxTxvz2ZIXe9tGwY0Dtuv+2xtpvR11PoDShPc1MTK31eHv2JByl4RLJ+D3Tql91r02aQlWc+xjGy0T6/Jn5sL/e+mW7k0MH0XAxreogaaUVnw90/D3y0CoeBPrQ+J7uhLGS95zlqztXLvnPc75Gt6Mh50dGI+VnX3YPvDsjiVo4rroLGkEayusEdQMqNyM3BgyycmyLJSADyaMVfA3p0ztiPFT/5i9KTzyfri2/3HODL7NMz+Gex79KC88/0+4//77yPR1rJtxY/enuBxGH/9t8h/+EUV5iRs7r3Iu+xI/+gk88tD9PPPsczzw8N+G3nfZMD9l7+AGmw/+Jh95/BcYnH6ae9d/ge3rX+Cp54D1a1x/4jvsV/fw0uvw6cfh0v2P88LT3+Kus3ts3P0gDO7m+otf4uyFc2z27+cnT/0pW3f1GF/9fV64DlujP+bBT/8XnPvw/ZTP/AG/9rtQPfk4M/ctTm3tcv0Avv6tJ/ntv/NXmex/npuTTQb5LtdvvAjAS8/sc+bMg5y67xn04DFe+B5s78DVq1/lwQd/hdev/CV/7dfPgD0Ps3/Kz16Cgyl89td+A0avwfgyV55/iZ39Jzh3/hwPPfyrfPObf8QnHv8ROzf2uevhj+J2nub5V17jfZc/yd5kl3N3/QqS/xJnPvIxtjc+IfTuIdMM6yt8ts80L/GVIFogajF+Eyt9MLNIgBkurpPd//WsGh8UWsRnXafrO+KwKL7DDptnfdbZn/Oh9Lk3Vl666fjpy44fvlDxo+c923tB/Hg6hom0GpLyBgGpjsoPA8C0jDUPgMLxjWaPOl8NgOn5lgGgy8Lfb63Df/3X4ZP/HVy6JGyuQ78UvvQ/wT/9euiRnhzvjPKntYFRTNQs9cla71n4Tz4Jv/Lfg++FcZdZodz4H+B3/zd46kbY/C1dgqWdW0NlUnrPDlmTlbwxlSA9Imir37N+ba5v/nz181F/NmPg4kXhfRfh/XdbHruc8dCFjIunMtb7hn4/WPSIn4WZXGNQEZRADPNGMDUzx4OqQ7Rm+IE1o5MFeguh3fDvXm2+WM+Mnd0bqASGuojgZJ3MTDGa4Rji7AyVfUzVI5+eZdbbR2RChsEyAinw7OLVoP4UZ89svAszwLle1dIcMQ6Uz0rYnXiu7npevO558hXPzkHI/PbHMDPBDcdKIKzezqqlcXOj5W75v9nbdD6APP0Mfjn4+ejc5F1wRKj7ORKziJPM7w55ElLdTNNtBTQtAE0k9epAb0URJT9mIJbp7X1OjnrP7E10BlxtJu3htdcUEaGXeTaGjtNrhrWBC16HUpFnOWr6QXQCH5ilMqM/WGc2nVBpXRmyGEK5tFaxKvz4ZCHewuFipSpMiOZU9JHo52lsjncFzg1BJhh/BVP1EN1EMmDtdWxxKbjjSBEtPDNUBk2Q9y4tgbZlj9RPrxblrW1aVEPvb3eqXNtVnr9akfeV6VTZi/2+Nd++7XC93SyaUYhEYaOz8SRqLrJsFirF6mWsEu1mrzXhYJXKfp0tpjT4lPLpNfYw50k3BopEO1uSDVF9cF0wKavOvBX+HCfH7Vj2zrMw54l2FWGa0n9kXdbrL0syPhEYroXAZ5Xo9Px6bgLL6C1ozfJ1vwygvYZgy/sV76tLhK3nyqf176nSi/rlIvezSfisLpJ/8PDa68poAOe3lIunlFNryqiv5LbAqMOqCWxuXHBKIaMcT+jVLHdRvHgqhJIqBBcCAzl5Wm6tnudA62EXQ1Z5xAqqM8QVbPiKyhgcGSobgfynSlk5qlmfXn4TtELVRAZ/L7Lyl3u63OEAaADfMa3tOBnRDsdqfFiKStmbKDf2lRt7whMvem5MYHPuZpw6A5fOCxdOt9TuW87vjx8Av6nDz2sgLjnJjT3l6g0oK2X7RizB2hYoMzO3KclJBvhOzPS8EktvyVrSQ0An2jPVGWANfv0hDAYwGgr9PIw0mDmGpc6tq6ZkmrA+05+ZX+vppdVA5SII1mo0KcjaNKBj0S+zZkXXjNLKhZJuzcaG+PcVFIWSWTjYD5+5qq+lgp0D5eYeXN9Xzo+VUyNl1DcRoB1WIDdB1EO8ok4pc8U0zFilHzV3w8C+opKdLNBbWdsCIqb1RBWLNQavFSKeKTnGeAxBxrHCoEYwmSETjy2DFrMTA2aCGh/c4pxB3dtAgqmy4PTQkz4ViqcM69iB2BwQKqnwBEHcnuRkWJwrMQa8HHcWUDo1vXSmLu1ruOY5UjxC6YSDmXJmw/DIPRnfftZREKLYuiR56gx8/P0ZuRHObWWLoPAGNqr5yHXeW3BVdPtmNsV6g6m88vpNRz8XBr2g8VlWga5+fgvedxdc2a140jt2t7XpQzabSzILdiID884tdabg4g8xWtY4dlBFkLBJKbzXg1Mbwi9/oMcwN0vMbruzpPNgdLs+z7Jn4ChCWL3WGxD03Z/3Hn70XMH2XpD+q1nd9dtOp7A7hp0DTz/PmBSejUrITSDGqEKlClqF98xCxaStMknYMuTdEyVahEp9yJqckEkvlnZLyJVeMaDKDigzWJ/2KK3BmII9LRhJP1jb3XJS01Y2TCY4bVwdEQmSdhoDEMFFgkfEH8mTd7KRXMHbVwKVOgLTClRjGU8REwZu1Sk9I3gxqOSBbajgIgPx+OtoiT8Lkji5JyqDNShqEKmelSHFLitpNDorBz1gfQsevtfyzCuev/nJPsNeaxVzawXaJRngCpPbWykGh0hdOL9hKF09yN9QenAeru057trKeca6pQCczpOdAOA7sO4hi3uumQPBef/K2u0kBZXBCIYD4Zce63NuQ9jed/Tjere136RpiWBeF9dJZ061uSZZuUJT/V3vu9dYbwBNWV/nON7SNbolKt4McmmG6evWR62r+8uP9fnqEzNmM0021vB2synsTZUzGxmz0jPLTZNJmno/0tpAtjtn2DKl320PiEGlCtolosGxRw0zZlhrUAsuK3FWKNXgNAM0KgMFY+476bjtAJj7UBfx3qM4BImitg7rHOo9BhMWshrECg6HGHMLIWUihdHZzPuIemo1T6VoVOVLL1TeUMWHrCQAoLVwdtPy+EM5r28XnN8wrA2CnFLoc8jSMs+iy3wdqegcAEqzEXT+dkn/ZOnt0MUNLp4tkasK+e7WMCjP+IR643wYeL+2Z7h4zrK7XTVlo3pD83q8CPzk+Dkdkri1Syuj103VutliXTbtbHUWRgNhfSDsTxznN0PFoFZ5SdflvIfmMpWio7NCWXgf1cWKyLJeo1nywHkNZd3SeYrKB9cKBO8NlfPMSmVrzfL4oz3+5BuzIN0We4F1yX881fgeSlkF0XrvFY0fTCPapSC4uO+8i6oLPsjnyJqiWehrGoXc55jcQCmYPAMx+JnHoYFNLPLzMQx9pwGgPcgwGzmlOIw6BEvpQ5/OqGe2PiVTg049otoQLORNgF+qSr+8ATJNHvbzeL0S+wa1Q7RpMsBa59waGOSGfh7kkzaGwtpAyK0s1UhclYr5xkxXFqC6fgP1CUHFzOeyx88swybSimFXDipvmVUer55eHt58d1yy1hdcZrm2W3HulOWFUcXB3vIS1EkJ9B2Og01GFEpDJs581g7MqotA03lWDfR7Qi8Oza8PDYNcOhmmmNXbfGedyuG530IxIVH5nw/mVvUP6wAt7SdWDsrK0MuEyvnW19MHfdIr2xWjQY6xod85HXfJP7NZvR+0/ppJMrrcjZ53b19cnUIP8o0MHSnYYG5gXLAs8l4QC94L3CzD6xUMJqj3vNcBcO/CTbJL61R5QeY9VnoUKGKFvvPY6RAzzSivzzAzwQt4dVgxaCVvknIob/g1jdXR3HcWFnywKurngSqdGWmb/cd40OfLRcsiW1VQu6z6/eYSYZ/KYaGoWHINtlSV83gVcmswUST4zIbl2t5cxV4audbb1t85Od6WhDCQVRLZPV1Rvk7/ypig3wnQy4IZaWZcbFu0Zsg1AWwhgxRuecNTv1jiRFaA5tyTXGe9amIAqIJrbLyE3Doyqw2g27ndzpjQG6/Ps0oAvr6X75VnQo1HM4EBVES6rgn7hrMZeebxzuKje416weSC+DvvBr0lGWCufZwFq56cnBLBKBitqDZCxOpQRA2p1iD61icc8xTxXOZ6Jz5G1LS+e60rgxy76NEpZa74d10W4r4JEGyi1E5aKK1YchyZCDuWxui/Feye39RkLms4wcJ34CalrfxZM1oT17YSxQEdkLPAkm7WWMJuNrG0UQtMC0v6jHJ4WnfcNasret+yAuyW9cvbakrtvmKCkbPznSpN/TmMBA7CUVWVo5gI+i4HQjFxrCbqvJZaYSULfw8gDodHsIGkogZ8FdfjSQZIhsVai4qi6kByXIwgnC+oXEmmIJXHqsVo8MHDZDEFuv0Q2HkYaKn+lQuuhFkdPft0hEDxgFPTmuUqS0HjsGj16N0gBcJ2yLBZS0ecTJPhwvlNrmXDeiDrlFmXCRaHeRkTNsY4HS3JpnMChu+Mw9ekOB9IZsgiM9RBn8mfuwAAIABJREFUY4fUqRLoinRKtOkppkPyR65vbcsX6bzpStB+szXEJf3N+TLsfG3Gq8GpNpuzLruPc+v/Pb/CTRjrEI0eieox1kTZPY9KFqTGEFRNMDjXEhC8r5ZvLO+pDDBqGHmtEHUIHmODU4FBMFqRqyJiyTAobSO1/v2tKhPVs0uZDXTvWeUCyAGjuiTqoKh88AecKsM89BIQIUdRs5yooodEsKvLKoIieJWFzem4zGptukBEinAELlncN9LrPpj6hXMaMVhjMXEmRMQlFqMn4PeOAsGmERbLn/NlukgNrwfFVRddws1c9tX0E+fXmET6f1yvQVRi7hlwXWboshJip4SqwcNS6l/Hwb6FKketGJICvFI5oaiUWaWc28zYHkeCmFt8CHu5dJRxjLy313pdBjZqECzWhyRBxVM5F5igGhUzPYipGeZ6e4al324ANCZoYKkG9+Rbzr9sziwb068Au85YZwy9UGhOZTMslpkR1FdoFnQ8B2opFXIxeG6vFmn6zFgT9BN7mZAbJTNKLkEho17xVQV7E8/NA8fFrQwP7E5gXUFzoYciRhdVXt4g+KkKzhtKJ1ROmiFlVd8A2LIIXJYAoCBYE21pjCCY+DArKTmwKZkBe2NP4QIZKC1D5zY0u40xIXO8A8sa742diva7TcrpEkEwS6sECt6FGdgAQr4t56XZz7KyfQQ/r0LpgrpGWKumeV09BG9EOuVHnTN8Tr0ujYHMKJlVLP5I0d0OCM5/tmbYXym9Z1IIxhj6uWU8g6deKaiqdg7QRRUc1WAua+KIlknnfYVjPH3vwmUlll6pVH3BMaNvPIURrIK1Fi+e3Amu5xAMXmeQ5Rgdo9KHW9y/azKk9775cyDfBI9Sc4sZ5nyfN/M+sDFVFeddGAh/N9a2Y3TXy4RRT7h54OlnATRMZA14CRngd56oeOms46G7LQ9eyBn1Q5Y2LoAe9ATsMaLWwzI/5w1FJUwKZVwEynY9qLxK/owlmRxo/FxhHqqfh88Yh0+WXpMqnN3M+NoTEyYH0W6mZ2MJzGGaYdLQQDwZh3jngmBdyjOmHQQytiWoaF3C95aqog3dlwRv6VxfN9sM4Dctg5RgUQnOV6FPboXMRAUZ6arBzANUM4cYjWp7WZQbswbBxwrG8YLZpqSqCRvUwWQWXnkwg2mpPPF8yZVtT1nGa0j6oTaD9VFgweaZtDKAJxWPn38l1pgGrFyc0wqjKLe2Gc1PG2QiQpaF/pD4DH+HDTIePwsMdif9XNgYATfh2q7j7Drs3WjVNJyHooSbO8qT3nF1x3F2w/Do5R5bIxOiy+ZB16WAJUdsWk5DxjcuPNsHys64YjzzFNXq2Tuv3dJVd7EIg9ywNrBsDGG9r/QzXeh7aCcblo7bfa3LaI3HWIups1ztaHafHO+4klUcDE9KoWJi7GJCpcF7E/phS2oVepjZM4EUVnkJGroTz3gmlC4ocGTG08slCqZLc07RtNKROsqDqzc1CW2IzAqZbZ/SY6nA1HqiiRl1WSmTEhTL7kR56tWgALN7oOwfKOMDFrLe4RA2h8LexDEpDHdt2Sh6cQJ9P6/DOYe1tsnW6u9CRLDWNmB420qgVVUhYqh8hURh23db5hfaYmWc8YONgXBqJKwP4Ow6HNwIg/D1RlJVMB6Hh2o8hcnMU7mCD9zb48y6MKvCg7uMVXbUHJTXUPKclcruRLm257my49gZK8WbjD2MwCD3bI485zYtft2wMTBk1h25eTYAWAUxgiyf03T0J6OAdwwIJgvNROf3GpW8VxRHTQnu9WPf5jC2clP6FMYzZWcs7E1Cc3GYK1mPZmi+1QWVToBW9+UaTPGCU8Wrx3nbiEaImPhzvgkqDwsGnQvzezWoFg7AsjetePKVgis3lZ09ZTYNii/x1J1K6+kt4fQanF4zbA4tgzw426fZ8Mnx9h41+NVl0Lr8qaoErLq9X0qW5zlWweMCo+ddGu+LlBhj6GWetb6wPlCsKMN+C041/b9mglYOZkXQG8wzzwtXK4a9PAoELypDHPXV1Cw1rzCrgkHk9T3HC9eUa3u6QFJoviTbKnNAePChS2ro53BuwyHiGWSWXpaDGKwc//sMG1mGMUFBtSm3n2wEdwQIko5F0PYGVSWINc/LCJEqCbEwfxcIWobKhdLnwUw5mFX0M8X0hTwL/fReBrlNZ2XnAMtLo0BUOahiLzIqOrYElGaYMfQMl4JgnVFqDX6tIPasFG4eKDf3wq/97eQaZO6zAuc2hc2RsjWC9UbtSZrzngDg23/cuHGjyfiyLGM4HJJlt4+refPmzcUMMNTGXaC1vovdPOqeRW6VQQ+yDKYx6xJTzwAmM3BxRnA6Db2FSamUTqm86dSi9RgEqNSiqX5gC6dMCs/eBK7twXSJM/YgD/29Ua89X+mIPY70dUo/E2ZlcLyovNJrRGOTzW5Zf9EYxHtUPeINYpJIy5w8lHcWEkZrLqURlfBOUe32couZZziSQ9N7RaKykMb+XSCO9TLoZ2Fdhj8LeRb0QxsATOZJa9Fq66RpHajSDKmbOK9q6xpu9Baq58pS4NLkPevsMhDKwpq/uRf66TX4Vcl9SUXyAwM09M2HudLPIbfaCl2cgN/P5ciyDOei+fAc6SUtj9628xljyCQU7o2+O3uAC3VIQtT4wHnDlW3H83SDY1kAzQCc9fD4fOnzDY80NVJTbemoPvarRQBMM8H5LHG/gvUM1vow6ocRD2ttU05a+OCH3BT1ijSGMeYEAO/kZd5hMvpmZGAxbTxsDddgEMhigzwQv4Y9oZfX2V8AvyYDlHqetC1X+tjLCXqeAWS8hnWfZy2Rxph4SXHOsR7fOKwcSsxwwx9ceF5NILi4KlyPX/GzNfC2QhcnBY+f91GDX/hetVPyvN3gB5EFqkbw6vGuehdngD08BUWljAtlMlMmRSC81Is+Bb/6ochzWBsJ60PLvWcz+rnGcqS+KbqYiJIZoZ/BqAdrA8PWyHGuCOMWgzkADOUlIlGg/b2XtQB5agRnN4SzG4bNUcawJzGaTcc1VmvYSGPyZlAM6usmdMVtnko5OX4OwV7LTPFz6kOHD5sKYa3nVuhnwqhvyS3kmTZr0sbMzdQ9wGSYvll2PpZjE3NlqyFY62VCz2oDnj7aQKgDFQkM9TlIrp9Na4JQhYk9yNzCxTPC9oGweUq5eS06t0tXKjCtpMzK0I6YlUrpQjkXOckAf25Ltg6WEhZomgnedhZo3WQ0mQkr9F2646kq3nlmZei97UwC4IxnbTDcebgy6PeF9TXh3CnD/XcZNkdh6q6XtYIHugQ8l5+/u6n0c8PaQDm9ljE9Yxn0PeNZeAibeqx6jPiOIa9XA2IofZuaDXM4vQZn1oXTI2Gt7+lZba/xEHfttgyaBXUcFO+zGH2fAOCdhncm8XOkNjuONT/vW9mh3sB0BtCXC8SEtdrLDKNemDosXRZEtOO4TQ2EmY02SomXZPNMNW+oiZMFEVjDCIIRCSX4+mXNUL20JJrYljAG8vjUWZV4jQ4wrPXh9LpQVuFZOzgIpDaXOMfXBZxrO8pdG4HdejBVNgbKsNeWWU9A8O0/auCbnwNclhG+WRzoAOC7LNRdCjwh2M1xXplWwQNs+0DYPoCDSVvpU8LD1cuF9XXh1IZwekP40H2WrVEo64zq7GoZpfw4yaAo1nh6mWF9IJzbCGWk8xthNKJqVPwtInZhrqou39ZkHSOhjDTqwcZAWevDsGfIbK3pdzwEs6Ym/jicr5lXbRnqhAr6zj4k1aytF6MnNgQtSghwdX49LPH0anRlRTF4MmuCTZIJbgvWmpgJQi+OMdgkAzSJOlItQ+YVnJWFWcA8C+VP8Hg0DPLXgagmKjTp5dY9QxUyFZwNM4Wlg42h5eFLwsXTnp+9VLEzgJ1dZRarPakazPaesjNWdg/CnnC6Cr3E3Eg47y3sOyfHnXFkd/pDv7AQZbC8tlwdUHmYlZ7dsWdn7NgdB3JLLYZSl1X6ffjVjww4v6GsD0KjvHSetb6hn/ugJWqWZ1VHgaBEEMyMZ5gbzLqw1g+zVs4L3mvbM4kCv50CpgZCg/Pa9DasIWHjtY4VLhn0m7+mynfVbHxV4XxGUUJVaSuhJd0B+pOo+J36MKQjPzFI8kEdBRyqFu9NDK48xdQzWktAj659kiRr1eKRzAQJQdrZvTwpgdZVCmuWuJ5EqT+vbRYGLWNU8EFrl3kAbpVt0l6gJEGb80pmDM4og9wzLWFrZBj1DafXDOPC85UfV1zfDgxUSWyiZjPYnwq7kzA4X1SK89J0wLvr/YQccwKAP+8nfGX0pbG4aECLBtHq4fYgFB1Ao6yUSaHsT5SDiVJM2ofOKGSZMOjD2XXl7IawN/H0szAw28s8mW090lId0DcKgkY0vp8wyKV17lZpmvpmiU5iTWsP+qEaGagpmUbx6nDeBvB03Wuqr3tWeC6ctezuaqMG4x3MCktVVjhnGhA0ROcAe8jneiMGhrd7Kbzdgfnbff7jqK9La4mkJszJaQX0faMP2/53caGuaq10QNAYMhPBLwuygiYBPjNn1Nthb8aAzXc0QaNYw5KTSz2zF7XeZJ7C0wStoddtrQT7L/UUzqEepqVwai3jlx+DL3y7pCzBl60hblEK45lwUIQ+YOXbFLhxf1+ibyrdojMnfYITAHyby5xZXd+hdUGIkS7dB7rWHqyVKEofIr1JoYxni1HnA/dn7O85Rn3h2l7F2bXAeutnNRP0uCLVR4BgZJMaDQSAjnkpq2Wp6tfUY13zr/UR9d3KbDkc45lnc2S6Q8te2Z9CUTiqSqmcx3o4twkfuRzu3WIWaBBMFOXW6DzhO9faSIQ0eay/hU2jfS+R9r26e2h6vrjRvslzyhHXLsmwOVpr6d7KZ+yeT/Dx2hczq86GHPdijbKap3rgCvBDcE5RX1H5JQ4IyR3Spa7n9edUjLiG4WlFGv1PK3NaoI22LJ3aZd02aO6kHuf+L3GknwdKfBRyF9RCTzSq4ig39pRBzzDoCbNCmU1aQ1zvoCg1jA1VYV5RNVxnbQnVtiCk686iiRr5UgvrE1A8AcC37KiSy3eduHYeSOoHPpVNqpxSVjArYkc8WbuDzDM4FRr/6oVeruRZS5deBiiriATHyQRV5p21F3en+fdv+iMsElxEj04kBLh4JueVG2Xnqr0q13c90xnMZo4885ie8r7PCn/3UyzY6wgGkQzVLEb99RhFRcs4NKAZPr4mlLra18gKgF620YPBSBa+92Z3bd+L5Hyh71U3vX2zQo5bwWqunQwhawBXqFCqCPTta1p+YxjE0yPSwPmkRzvvFc8XP59Ie76OK1DdG4vAV8eEWoH2wJ0KG3tt57U3jsLvyT3X+bLjEdesCXiZ+cpEzVJJ3ksT2y7tVDHo2BTpCgNp0aVtynl7+U7mZoxgVINGqVGssWQZzLPoxQTnePXR7qy2/xKDIcxl1CNP0sn05sW7T5rkJwD4cwNCWdhUtLZJSf9ftVnoTpXKQ1kmoBDXthHl/rt6iQZdEokeB9DeAAjqilLWqspXZyNYBpxv4Bmso1vmLJKevw4fe8kz21J6eXhPe1/LKE2H6oNjRBaAyfggZ2Wy+L1om616G0WZPd7XP1MbUc0BuCzZdJOsxBgbz6MNYKhW0Pm+pRmQJvqWBQaZXySBHJKht2SksPmFz2sjKGknA6z/P3xeE687oNKyzE1XjuTZ+H4+/p7Fe+s6Vms1QakmKS17Py3bgE9fgVdvwHgK3oDxdEQS0gWmy9ZbojLTiFD7enQhvJ83IfsTXQINOgd8LIKf6tGLeaU10tyDlZKC0qOeDww4J82cYvqAG3sKuInBHfK8t0LxJ33BEwB8B5dL5/62MciMD4gNG4UkWdD+zEd3jFgWoevF+WZAcNn/LLtK0aM+xeqPfNTrUyWYpgQ190M/ueL5za/Dwf3hpXkeSA9iugSEmjEbIu4SY2YYM4xRtiTgFdVEKgdM8L6fZEspsC0HP+ayXBGDtQ5jJoiMmrvesFUJRKIAgrMIvqm47jFvZ7OxhSzMmCkiwwiCkgC2NObBqi6okrisERefDxpS8FoOiDGFkwnGDDDGBsajbe9T/Vm9DwDnI+GlUzVIxNqrCvw34Puvhv6gdsqH9Y+t8AFcuC8apM2iga7zgT2mEQSXBW7zGWvKDNUlwHjUSl5iC7jwAu9qyTSCsatfDBJieBRKt7Tfp0h+jIfvBPVOAHDJUeSKVBXk0POB3FxYQZzDWcXoIBTHrEN9mELPraX0iq3A52/0jKtDwaWN/WSxNxt4khQUFfzo2YpHLmVMCqWoDHkWB3Vr970jtAJlSXaXPuRLsz+5tUdrFSFnFfi9fqMMkmr74e9qYsBz1+DP/hJ+cwOqvxHk4lBtALCbrfrgHmE8eS8nyzzGaAsIUUarqhxVJXiXx8zMNWWkhvIeSTb1OAaETd251pXAWMhsRZZDZnsYGzIs7zX+Ikp+mQgMUQTa184YeqTlTpr9SewrB8ueHMFjrcfY8D6BABI+vxKyPucU53yQHwv2sR1xiRq0aqZmOkgaSogBRMXkiHisFXp5OKc17YhoDXxFzPJqer+YxPg5j+L2/xr+5Avwg1ehTEqXdd9ufv1IsibTOdfag9DVLU60ybbTbGvZrFaj7jEHdt6npXxNQHBVRj4Prq3PoPrUecIG2UIXMv/Kxb+fE5rIbCyZzol4v5ezugNbseEdmVVmviTPhlSVQzFkKFNTsF4OOZADhm5EZQq8hNEY79/jAJi5LJAwnA9RqTEYsVi1gbJMhcNgVEKfJpaqvPE4c3tNJ9OHJi1hNgaeBub9FV98VVkbwGSmXDpj2d5vBzKFMLvEcYkw2jI2nZeFiPcwIHujIUCtsdgMPB9xjAaGp58tFv5+r4B/+WN4cRt+5atw792gW+CSleKae+rJs5LchiChl4W+oTWt23zlPLOyoqzCnGPI0AKxIw1EchuynHoj9T70qyoXNvjaqqmXV/RzSy9TrHGouugsUG98HlUX/uyI992hWjUlt6PksZrMOPb8JBIsrA2fzRrfMB9DAOAQBKeGslJmpcd7h1eNG2wLDu11tVlJvU4Dq7eipouEObmSXubp5eEe1a8tqxCszapQyne+7ZdZIPeQ78Orr8OXn4OvvQjXJrF3lzSD0yx1ocSfGChrdIYAofKB+FLVEmbS+ug1WaXMB4LSBUC6XoEL2ZnUXUU9FAjTEnAtYF84yySqPZ1eN1zfC6SuefBTVXpZILj1ouC8eY/ndj3NqRhgXEZVKXgbSsrOYE2GStE0Z8WaOKYiiFecujvOSuq2A6D1BsVGyxPBRad301i0FhifYRTEB4dplTiyYN/Cmyd1BhEILoOeb/QHOwCwDb1z8MffnPJv/9KAtYEls7A79ujQMMiVHEKWs8q8ljoSDUaiRaXB8Na3D/ztPEwcKq5dI0zsWS1Y9kaOgmoYlk+jtdpORxVuTOHPn4UvPUdQ95g3OW3Oq4g4bOYbJwCJoxukpS4fMkDnSBi7XRJFummypGRWlyOtrbDWk2W1dmQAfFfVPa1wTarh70KG8WZJCo38clMtsFm4qz4YZmCNYqyL5UbBO6gqH8uwukCh1yXl6vbq2k1faoalVazVhRJx41oSA4R6xo5Epc8pVPHXMikwOU5RJQKf80LhhMpFskidZRoSDdCugsvyoFA7WaBPfk/Xs7WQGYMRT9rfTW9aDX5OhcoHcflpZSOQe/LMs33g+P6znqKIgUK8LKuhujHoC4N+8AmtS/2SDN+/18DQSwUGnHi8EbyAt4GNbExF4UPp2/sKJLSKxIQCuvEGFX1vA6A3FUiFZoFG70VxUoFYvC9xWSjTBG2KsLDFSKQdayts+xYchqA8MYrC0dYuAiDAdBb6HJ/7ypRf+XCf9YGwORJ2Djy6FnQVc2jGF2RJ06N20d6bKNtjz/40OGl7Xcw+bgUQJQJUUIOJn60n9DI9lImYblBqupty44ghYfN0uryxU4OdUQkzgoncVbrbe5f0rZYggCYbtNLN2M1cT0sAox7j2/P5mE3RvF5jabQLuLfSS5ZI+KBMWbMx+/GKkXC+AEja3eBXfC5Z0JJtG4Jh3MAjGhi/Mo9YdQnRxYH3+cBKkp6aLidw6TFaXLUl0rQMaikHszBQbiL457UMWiIqnX5XS881NxTv56oi1vioPWqCR5/RxaCIxOpJDVWlzMrwnrvjiheuOnbHysEUdg+U6ay1OaPOkrMgfbbWEwa9WIUQeC9bQM/8mMpEUkT0lPQSFplKRU9GOFOiVbStivfKisWIobrD7t1tB8BKCsQoWMVoFDhqWFYKto8TQY3BqgHTztxYTexLbmfyV0d9FvqZMuoJmwMYDVot0CohYxzswQHKYAhf+NaMs6eFjz/c48KWsD/1ZMaQ9VZvIcH01jAplJ2J59VteH0HDma6AH63o2ae2eAGsTUMM3usa9NXO/aGR1cI3MY+z2hIR4t0vtdohKaX15iJzpE+Qh9PG6JGvRxUiZF7KOdVfvF7q0WW8xzynjR09lboO+y43ktatWMl7smh/3tov2nVuqpf75xSVUoxg6Jo1Vjm3yso94RfmVkEt8YkNikrzosQpDJ1dY90HiScxvsaJfb8MT9T9zyhdD2eeW7sw7U9ZVqG7722RcqskltZKIUaWX4/695fDUhOtbNeMhMCuSDjZ+jnBiu+qSzQgKfi1MRSeXjt3lR5+tWKKzsB+Gbxu6jLxM2aVejnwnpf2RgGEOzZoEmKlu9ZAOxnfazNMZWFwpBLHaE4LIr48EBqEa3T1KBUoSXsfcpve4/2ACVDfYUWkDlLnzyQGyqLsUI1yzDOYPCo0ZAya6BOGydv6Q00Ehb9Wl/YGAlrAzh7yvCVa77pbXlHM9g7mcF0EqTJfvx8weDhPhe2DIWDvg8mm/ON87D5hE1jUnh2J2HTePlGxc5BjJZM9C9zQZG+vMWgadQLSjXntwyZzRnmQm67gtjHzXVMnDG0Bs5vwpk+rGUwyGjsoJjPaGRJVpNmKXMRftgAlZmDSQU3C7gxhr1ZGLZvQNgG0eXRCNZHwsYoDDTnkbiQAsdygoREk9XlSCcsKu3Mo6jOvV/9atMpWgbmqXPKeObZPfDsHSjjSdh88W3fNDOw0YczIzjdg1HSf1p2CanIgZnL7HQue6ozfuej4bKDmzPYnsJkEsp/XpeMN6zoPyux9Fkp4wJuHiivbZccTIMi0jAXhv3gB1jrgXZ67CuEHLx2xanTvmSwSTI4tVFsOwy5SxbkFur1VHltXOF9FLiohe5vHsC1bWX/oMuQbbI/bQPHfh68Nod94rzve5vdqQcWSiiLCs0Eo4JGtpbLPN7M8MMSSsU6G8TKMYEBfQfeutsPgEWfcs+h3iNOg9egMSHFygRxHquCKRWsUOLCmHIU7X3LWoDSsuMGPaFngxp9bStUe5HVtf/Kt5t6MYODqTItHEVlyW3YCEV0KSM0GHTCrBImhTItKvYnnkkRyizQgl9tbLt/C6nvegbjIrjWn12vmJY5a86QmbDDLN+E2r6K+BYg6mrT5S14aB0ubQinBrCWJ72euYwlBcGlc3xLfvdemFawP4PXx/CsKE+7CBjxyHPY2IAzm4a7ThnObYSgpZcFaDBJz6kLVC3ihe8wPKTpdYv4CDCy1Mx4QVgh6VshdIYGfCx5F5WwOxau7cKVbc91o+wpVLMW0IYZvG8E928JF9cCGA6yNnOatxZZdW+Xg2Dos3sP4zIA3+v7ylMCz9cKKCvUZJbOEiINsWRWwnhWsXPg2Z8G4Kgv1HltdEHr/p9N2KjpclFSpmbLDq1L8qqKN56ihFmZMavCM6oqjWavT37ee4/zNvbb4cqOZxqFr6uKDgs7zf4Go+gZbIIxbs/WbvACFO9ZALSDqD/sPOIsGRZX+ZCoeI+fOHQWmNw930c0JC2++Q7f4z1AmWS4HaWYleSVwxmHigdncJkj8w6T9ckK8MMoY9WodNi3pP6e0vcDYGm0WUnm0FxLyvCRzitzm8PBTGn1Io5S+giD3+odfq7OWd3mj7hfBR/BsgosSK8ODT44C9dZb6STmV/IDuv1uzWCB0fw2Gnh9JawPhRGGeQpAM6XAedKaMsAZL48N3MwmSpnd5VRJexNlb0irACJouSn1g2Xz1ruPStcOA1bw+CekY5PIKa7wfpaGzboRAZljy5qi9iYAcZRAFlRLtb2PXVpqTSMATgNnnI39kM5ug5ypkXoTVVxfZ3vwQfXhQdOw+aWMOwL/dh7WqWKs+zv54GZJNMuPYwrOD1VTuegTtldC5nRKqiXFf+kSRZfB21FFe57UYXstS4tOl+LYWuTaVYinUywKYvH++majF9bBq7Xhs2LZkHjN5UW0xYIVcM3m1th34N637B451d/U9onlJSrqh0h0Rj96nvc+mQ/O4j3piAjw9IDC04cXmDdblDIjEJn5PSwYim1RE1M4++wW5e1D5TeFsZTsT7GTnJG5KhRDLbRKbTeAhZfKcXAgnosBh8vZGbfugZq03dwEiLaKmiC9t3ip07BECDvwbAPp9dNcL6OfHNhebZTZ5S5lVBi6Qvrg8U6XOidhE3l7C1+vmEvjDb0MkvP2hjN+hUivkH15fJdlpdedEszyrsHwaIp3xTMQPC5UEk7Z7aqHzYf7acgkgYjgQSh9AZwXkDHynM78Fwk3lgL+9uwdZ/hri3hnrOGe88attYkjgOkPad2jFvr7Da5jnrQWRomSKviYupsbsWXOa9kAolQcprROGVSwuYwNE/3p8LuWLnZAzMJcZ0VuBgzwHNbAluC7wlVZNuJOaQ3u0ITdv7PdVnRVoHgkTvYH8MzY10AQNUWVjrfj7R+gLUrSWaVPAuG0LPSB/LLnJt6qr6y7HZ2pQQlkn26AF63CBqnifh7OlbROU98z7IKRtN57Evmeej7ede+b7Zk1OJgFr63ompnKbv34L119HwGHrIIDVWcuQWXAAAgAElEQVRcewaL8TDNx+DCv5e2oIxLM/O3J3nZ2trqVAN2dnY6VYJbNcQ9ffr0W5sBvhMPSfoLpQtO8JMZTAvIV1Q71jbbkuiZU9FtfWgY5AlbbIV8lwhkxjPIYXOYcXYdnHNMSu3YwTBXRnuzfU3E0MsMG0PD6XVh1A9OEzV7bj4LE+DC6YzXtitG6zDe7wZuAwn9qY2B4IbCrCfMskARN8lM5PKd+pAGY2ejVbSCgYX1StE+bJl4X5PXj/rC5lA5vS6c3RBOr1t6NoouJxmpSJoNSZIFSAN83Y1Zk017XgdFGyCNkNcSuZrNu1Xr9IR+1LQMdPGdsbA5lNivbH/OSPiM53uwMYK9oTAxwjSTzqwpx723S+6rj5J/WBgJjKZwtqeMVv28LgZvaY+09q4c9uDUmmFWZWwMfeNmEp4HWQqEnTW6JIP1USUoHXURArlmfWAZ9TMGuZCZ9ruqAdCaNvPObCBH9YFLZyz7E8csEnVmUyimSwL1KbhRsEHan4aSceGUyis5PWDGyfHuP7L30of1GrK+g5lnXChFBU++sEjD3DgV+kxGlI014ZPvzzi1lrE1CiF6L2/JE2lk21DORcmsMOwpmyNByRj2gheh11sDvEUANIix5DYQRDYGysYABlnY3aojaKar3CYGEnpT01zQLKjsGwmZymFZyVFpuCZ9UjKwFQwywRtlYNvN0piQnWTN7Cb0c8Mwl6BRirQb7pLvubOpNyDX6p/WwGiivFo7FuIT3UpJyueS9OIsSB8ka8S2nQ9p3rAHgzz0yPJ6zKbObAR6BoYmEEcOjOCtxDGgW7ivaZCjgpcg72U0EjzqwGI1/tXThwuYa0TJrWfUj+bNNmNahp69NUEtJJtjfoqw0Btf0LWNwuhdJZja4NmwNsiiF2cUE5jridaKQVYVK0rfhhLt5tDwyN0ZgqOfKTv7UAyUve3u+auSSFoS9mcwnoX9IGR/eVcw4OQ4AcA7+khAZ1Yp01KDAWYpQe2AyNGJr8st9Hrw1z7WZ31ggh1SHgBxrU9whI+SWrKCXZgZCU0zPNYIa/2c0ulbNwgfy6mDPIK3CYor4peX5XfHbk5kOv6Da9VQ6tJTDSAqtygRJYtlvZrdaOaU/7tamd0SZlrSXNK+a5mqqb5kogMqTbYhrX9dU76RdkPW7nWGnzFRHHsczhE3eiMDcjsJA9x2iXtI0lNLVYk6JeXbdm8jQcuEJZitaFkvjGjMqSZp/G4yo4x6gUY0zAPZxHnBSIaN4w/z38cCAMqic0kqrJ3qk+Y2sLUHuZJbE4OINnismaY2emJaq+QYhhrYqqfXMz70vsDC/sEznht7YM7Azo05EPRCUQaB8Mks2CKFdZedIMMJAN7RRc/OE9/0cHzNaIvEBK9UZTsIDjBcD75/v/6xPuc3DZXzOKfYvmGYE0YLDhvyjX5ogo8O2gZrYNjTVlhbV2D0EleIQytfSa/CGrASSlahVJTh1JB6JKY/trPvmTllGg2BjesCSOVbWr1ERRdzVJmO1Z51y76ZWvygDk7cCvUNSXzm6hGVw3o03XPQcSJIXyCpH1X45J1xgs5NrmFYBCgXgNyIXRCUbkSvE2AvI2Xf66Kbx1H4J3L0fZ2fqUfaezu/xNJgTJfc+DTb6gGmH8qhLpo21wSitBc3P/qwavh+GVGqPmpz5wb8IlFOEmk20doRPgzkZx7IQwQ3Lgybo1B1+cT7PV/+UcHBBDZPK7s3k8/vAgjOKmVWBdKS0xNboxMAvBPTuzdQKqqiSHPaj5Nk8zcS5upeu1ly4ZRhfWAY9KBnfUfX8dBsSBSDR2zIxnp1OW3FZrYMGGUJ43LZ62twqFmJbWlJu4PhyTHsG556plg6J2gECt+yCg/7sIfdg6PAMP0GXb1Ja4enEn9Waagusvy+LE2IZPV9bcqnRpuRiEYyreM+IcmfiYJ+3c/UsehJnDB8YkBbA0PhoUg1MFk9jH/csnV6HX7Fei+Sf8h0MQOUFFmWIGFgd4Z5P9U2KOmWJhe/n2WBoh4CjPU/GlqloZYEkYgo1I7w0dpJDBgN5JlhzzMrhUkRSqKf+mCfP/3ObOGcla91eqPmrNeFvupxg5KT4wQAf/553zEUPOZLLjoXodcMRWuE9YFh1JdQVozg1ynpHFmNCiwaK23R6yieyLKy6mFZoCTCUI0+5ArBbU21QPsBkIvZ6vuoerjt2ht5+Jd9N6rtNZc+sD+VLgB6bYGl7pW1snmLhIuVQLjCZaB2fWhUOHXxs7VC0d0tsfOZdJac0MaSXXuNJjo5FBrAqNIl7hq3eVOt7YAK354nZUIaEzf9uctYYGzW5dD4xUjCDk1HSMzc73IEc3UVkIe13L05mqIec+XeSGjyBowaekk4cHO/YtDLyDJBp92f9VVXYq2OAbzfDaL3tzHwPjlOAPBtywDb0pYuzKcdvSkH0sW1Hc+Z9bqsqI11zJER4dL/bwWil71WWeEIf8h7y5z1TJP5LTFcXeax1s/Nahd2OaygfHu/uXSTnvnFACMMO2tjY2XmhsKXGekuK9TqITZIhwG8HPJNaNSDEcLMmqYmwEj4pcEvsHahKDSWQH1b8uUWTVXnXU/S4KcWxJ4fsRhttN6CKeg1rNkVa68tdQb909RUOe0DLvuZo9Z3J0Ocq3Y0fdvEmHcBSIOwMGIUq6GVkUWN2iwLYuzz5+sYPXtixjllUT3VgljQ6QkAngDgOybnIx3kTR+cGgTrZeyjZmXw/ZIo5ru4tftYVtlYs2jjbC5zrgTHA7/lG/OyDG85xCyPlqVDNGhII/EBXkZDX36tcmjm0GHzcfRneLMIWINcEX+ZOXk55xSvpjOQbSS5D8f0cKtn/Q4txerh79V41rVX1xVnjiMIWmcw2mZaqu1mW6WC6Md1Wn6TAYb3wYUkgEBUR0nXu0+zLmlEIg7L+JsAIxkvOQowm5+S1fdemgyz60B/3ABMRIMySR1+qKXu60L72X0t92elCW41qRTQBJKpSIAHLZPr1xWh7J19VMSRnBhwqrooxu6DSpTPIyHONwzowJbzgXQnd5YY6B0OgElvoN7Skxpao8yjbQRpTJA8sibMDi28Y4zOB7kwnUEVB+dNnDurORPzpIGjXOJlSYrXzTBkZSY2TyefZ9ulYXMYz5BjbBbd/3dxJKHeFLPU4uat+OYSQ9Q6SylippLV5TmCSIBzwfKmue646c6zDI+Cd0VXZv2r7o0eApLzBsfzPT0fv5M6M6/Fq6tkAJ3bwQA9alOLF9UbQBUNkMtZkABrMsA3EMgdZ12lPb26N3hYOb/9Xrpwd9y2Rvg9GBM7r6GvF3+JROWZufcxRmMZt11PtZZvfV/ELBKqup/1zpP/OuzIfdrjbgfQRQxqBLVJBOWliRqMCkbNHacH+i7pAdb8uzpy0SifFEug4rE+MCQzo2TW0MtckNQyArV9TRycnc6C2sXeVBn2TBBeNvXT3M5UHWfpq0YB5SUrQ0RWZIRzyYG2ZbIOhZ7Euoh6U/XLBZ6XoPK8sajYwAZVrWXP5K3Zl9PzRqAooxOINa0YuQOc81TeBCkyX5M1pLXQWhick4SoMk/BaLODVQzEozLDNAvsAmD9PdcbrWk3kHh/o2pX1wT2LcwAaeO/hilcH2Ux1xdd4lEoncpIuO/pyrLazmIuYy0vmO0e0ROcv/LaFPcoick2Aw/rpPKWolImhXJmPePanqMslWIa9mxN5NAyGzLBYI4trTlxwoCuCT6q85/z3QV+EO5FrQqm3kQ7JFCx4XcKwIRsUASJi9oTTKPde90O6W3N/zSNwvzc/u46D6SxWxjZxpogTzboCf0eZFEyKX3onYftA8/5rZwrOxWVD3NPawNDX3yjQOKPKImGvpyh8kFVXxtXbO30WpbSxmV1Jjm/mUhCTtAORV2TuzIXWaebmw3/5LSVgDNvx+MdN5fUIzGw+RL1Hh+GrhuxdBGMXUfYe8OJU3hta6uuSzKM42Q6HWcLnWMnJtlfim0NgUrfGlPkY374JWs07RDrEeu4FqE2TSm6dgjpsD8PEUpfFNxevExjNFRoTOgzHibTWBONamWZWaVMCxgXMOpn7EyUbz9ZMJ7owjmt0SCEnQXlJtMYOWuraZvQqJcJ37/bjgm152Yt4hD63FWsYPTFxv2lVi8yeK0Ah0vT5RMAfAc83ekmLv2ozxnKm6N+YEKmZdCmV1Aqf/rtGRvrcPGMZVoK1WZwupeBoZcFv8NUCWY+YlYVnBqKSpiWUJRuwZMt1U1cBYLSKdl0ndOVrgJHKO3WFeDji/oaBy5RCxGJ7vBvw9dWk0MkLUHrMiZhK5mut5QtuYX4/ch+6TxILiuNJiCSMoxr4o4mJfTW9/2tdx1vTIj1OKVLWQr2wekiKBlNSmFWhqAwCDBIxyVkmQDAqp50athbv9hKrfwjQQbNBld4Te2naj1Z35KoKheUXCaFx5jATr2+5/j2UwU3d5WiTMrS9eaXhb0gqPe0hJnVYY+s2l3ePYdmTTJhkn5wpqGiIdpLam6Cx4W9QjxePFZPeoDv0KMK2V9mWOt7NgfCWi94gEkarRsYH4T5ML8H05ljWhq8CmJ8Q/2uZ8hWFG8a8BsXGq2QHGXlF2Sf0rnChdJRUiZFFgki6eutCSWIXmboZQSHctVjP57GgKna96s8K4f2b0u8Mqc6kpmQdc77w6X6kseavzy6yNOpFhyfYDGX7a3IBnSZmEH6PR+zb/xGQe7w0uDi6/LeknnK+dJ7BPHSCZNS2R0ru5Ngi+S8D8PqmZCbNAgjtBXmANCsGOL32g3k6gH4Yd/itdbeNZFpqx1Qb2yRokPFpAjf785YeeLFgmvbnvFEKSuYHizCWGZh0FPWB8F9vmel0954L874jbCoCIhHccGRwwcVIKvCGMVKhkVQF14nRhBj8HLnBQPvGQAUymiIC+uDYPHzkfszxqXnm9c1iXbDFnmwB349qMy/csVFTcSM3AatwswmZcIlfoCVC5nf/sT//+y92Y8kWZbe9zvnXjNfYsultu4ezcYBORyRlAhJJAEKAqF/V2960gshDAGBIDAUMDMcDkWJw5nu6erpriUrM2NxN7N7jh7uNXMzXyIi0yOrMivjFgKVGRnhbm527/nO+n28vE5crhJdmjJh9Grnhwz7uDt0UEc/2JiSUC18mXXMYw4ljruPkR/XtkWKOHBpHPA9BvvYyGXMHBIVKoFaijRNB13Vp6lkSyvv2Hcu92RrdOAQgNyJ4EyB/FC9ymEn4vMHvJ/7YpX+q292Xo1Ioat5n3KWHWDqu4D7xp02ZcLoby6Fr145l6tcXphXRXkhbsAvO3U+2cN9VKj7noRPCR2CQBXTIBkm5beibsoc5gxiuMlyg1TT5bGUlzeJv/x5w6+/yTVAd1hdFxd4K5KPmqO/ZS2c1EoV5aPn/+y6VT6MMyFFo5OsFxVcCQhdKHumE9KNIZ0QUJSAWPrg4uGPKAJsByaLWZUP77L2Qqy8MUapNMNEh5vSMZcSfP3SOVsa5wvhbAFWjcTFtj1nl0yx1Cau14lXN4nLVT6sY29Yi3BoLx469jq3Kbm2AVNGoqyOD7pqs85w74BIFXYPdP/XVWO3mvfk+2mrbrPYeyORO9pje69/Vr62ScJ7OZyJEK+nI9DCd0D4oaLcsVzSuIY0UKIVncl9M6XbFGo7nah+v/ffBlgZdTh27e5tODQ2s5nR9NJQkhXhv3yReH3jzCIlcnLquBHD7R2WIbMgPokOt+97st0abBVy+30okktBw/D8+/uZfNPp2SZokrBqnL/+dce3r5zX1876BtLIsYsOrW4U4t1zHXBWZTtQjRjD9xEifAyriYmwCOhFjZ7UhCqL36oIajCTlsoquBZc1thV7u6SDtzkg0OUqKpgaeB6PNoWtE76I8HTGvE5nTZUCVKoEG+xYOhqBn/TEGWOk4g4TQxUnWBi7yq5PXijOYpyYvAJQPgoLTluZrq5gpuT0h3abdKDE3QYFcv71EwWDzXWbVZ9H1hBhpSel58dASH7jUMv5mm2AULBh2HmLvWpQ8/8pZaN0aGh8a9fdvzOTyNt10z4ETmQMtsq1dwfOPz2TGgfAc4CzHX03m0fAfaCtVKYYxxnRRa/eRjJmocEQhlJLPXzf2bZ6CYHrzI59TgdupcazN9qi08As2/M2adTul5tSLt9SzZqeh1S9qKw7uBqbXx76SxrGZpV8phB2dPmO1JI+xw9P7DH+8++7nL2pSv3zXwzr+ejVLm7YCZFEDexaoybtXP9ev8tqiyDIJbnArPwtZdzcngG8mNZlS+Qq2vSFzUpNszMaMqN76SfoWxIFwH5hRJFwCtSuM4KKdiR56dMXZoNf1ZVzPKcoaoehwJbByuaGVIMS7KEHlnDDKHI52hACEQiUQwJWkYT8gdCdTO68D20xPXt68PcWTeNNnrwc5kSQwPMl0zapOWeCSsZpHbSkLrREl1JeS/zrC6BySRN1HcKJiu0U2y6TgeashGgBc21TC0zO5sk037guThVrhs/6OFOGlPkfjWrNw0AGUUodZEK2lF20NHr9NfkhtOhWjEmpubeKUuZeiyja3mIregISq6R9QDYpfyMouQMk27xZfot983f4pCLQtl2RBkPvJesRjv93j61hvxnLVygWfao0qxusuHP3Hy2bYei30dBp47ecA220Vwc0sglS1N+eyRzfOhm9A1NxcG9x92qRp+7KWK4yTbSTI/rh1+qmzGiVCh8HkIQd9vBiSJCjDEfGIulpfWYK8/D2JSBbO+9KhF0BIC+h0xzLDr6cHFfv2qSNTSdc73OStBNygXyAawCxG4/qJ+dKGdzYVkXr1edyeVOjIZTBWcWhXkdmFfOqs0inWNANgogWmm6MZkMvw88jXf0XqsIWuaZ5pUyqzRr0YkfZOY/WwRWbbf3xCfLRtM3LspO9LdttH0r+hh3qu4Y1VG6sKer6+s/228wEVkdYN0RUmlXjxTd6n3uGNN+1oEXaCuPux8E37jHrxRM+2hC1Qlh9E++if76uU0roVDYOg97B/C3HoQc2PB9l2QfYfWpfZjKfjHWvBs6MmVHdb0KMIvO+QKenwXalLhe5wzHqs0XFraaucYUY31GpAdCtpy8XnpLZTMDWkelrpQqCEGmbawyytSoCSHket+8gt/5XPn6pXF96kMJ47bHdd04102WSMtyZTIiNn9c3/dKKRHKoRmzEokIIYQBDB8sBdp1HSJKZx1iHB0Bdt5hZQhdxTAsKz+rIX6XOKvg78gH6xW+1y1cr+HVTR6JXncbazduBDk579lfhJOF8OmF8PRUOC2q8EEOiH0KKBkAF7XQdMp6EUku1NFIyabmeE96qL/eatSur1o6JfsBddlcQ/59JcbAvBLq4MSQBqPhvg+kN00OO1lsy6mOLjneOlXsjaWwr9HLfePJb2u79Z9R9zkm7mhyUue0vlFJmACgKqpaKJZkwrjiRexQdLYho96Y4Gkee3jRu1M092GLGf9sTuOVnVsUDHoyAh3VnMzzFTUGbetI8iKaKyV1KHtTNRPydt++t7vXKZ7fKDXOTQvXhWy8vwOpT6ePEhkuvgG/0d0KmjX5ljN4soSmi4jA5SphlrUnq5BrtftkkSb/7/eCTEGw/5kYcsdnbuTa6HDGkf7mJuORN4gFIaSsTh+CcrZQnpwZV6t8Tasb6JoDxtacmxVcrjYO8RtMDz2ud7B68OvToH36093JWPWwnkmsqiofBhIqAX/gSf7NwOQmhO2/vtclFWZ5Vuj1Krdzuyh//eVuWuTiuTCLhUxX4WefKj99Kjw/U05mSh1tQpW780yKLl8dldN5JkSeVYGmC0OaRUae8nadbjvikhHQbSIi2emkDCGTd2fvW0tae7/Bv1oZ69a52lMr6YDLNtNlLVbOaci0WSHs6iD24GcjJYqJpz4wu8i07uW5QaFbw/U1fLWC79IGLMYpuAyAZfh2JDfkPUuytwiBaUHW9gDim9cn7ooCd9hUPMeoKjZICI0bYi4TfLWG5RUsZs4ToHafpAbHQDeOknxP6niHE7Y8h6aB9bVzeQ3frp2b7n4poe3ws89mLGrhyYmhKixnkZsmjobh2RmGl61Gre1OZ9kpFfRzhXkw/WQmLGuhjtmRHs/NaJ+Wl8zl2c8NXq/hZB75R7+ruLd89cI4mQudO69fQbPacvS6XJu/WuXB+XWb07l1eASiH2p9++23w76MMbJYLIjx4TprXrx4sRsBukOShLseHQHeVZsYszpkr/l7qv9JHBpTbhrndz+r+I+/9L0R0KwS/sUf1fyHv2r43S8Cn18oz87gYiks6mLY9HajiThRjXmV5/PO5nk2cLsUNakF3VaPk03kt8240afcMonvBoy6ZAejYQe+/Crt/fxXCb68hp986ywNlitnGSXXrrbAL5WOvNagSRui5z5NVWn/VdJkbPg8VwavVs433zn/9TV8OQridAZ2U6iq1AsrSI6slHHrvg9Ox1iGapo1fnPw28cQs2nZ9x0nAJ9SjkUVqpivfQyk33Tw80vn7Fv4qTvzubAYjdSMXz31z9H7RpDNv8d+aHyLwMA9V0VTC82V83ffOT+/hOs0ihAPzVPu+Xt+jjmdL4tAHfM5aNOGkGAfEIscBrt9WZMpoYNRBQbw28kLjTpKg2dqQzNhViWu18LTE+Wf/4MZTWfcNMa/+8sOP3O+3QLArqMQVQirZqu5zbe0kh/X97JijKSUyxvbTS/j9OiDvZ+qEiV3UKgfXwMUc8TKhu0NvjtiUsiIZS8wfh8g6A5tyvn+Vet8e5nTs2ODdvokG91nJ8rvfKb81nPh2alwuoB5LCmrsAGCQ6DVUzrNxKhiTwVi0zravoH3PfWyqUDorjEZpG8sf762ywoK+3rbez3AZa0H03rfreAv1ZEE313BH55CI86dW68Y4zB6v1Sua8XuyMgK+K9X8DfXzn+8hr9r9zkjsKgkt9r3Xbs76T/f4bI8NvK7LQoc84BO5tjo62V5sHpRG7MqR+T973+X4C9f5/Tyd6+F57Wz6NPZ46i6AGBr0ACNbXIzUgCwVqGm3PORsW4dXhn8eg1/fQ3/3w2sUk5ri+8ffpf94lzFAXMk5OhvFqXUpn1XN/AgTd9hij85sOf7LmnwHRaeUqosIGulyzRHge7Gqsm1/csb+OxJ5J/9IfybP+sI0XPWodRBU8oRX9Plbm1LOnJwHqHvh6oBbggPpnjx0OAHpQvUVTA3LHVHR4CqERdByliFiiKppIJcy+xXT1ot3ysnopU6YK8E///8dbOTcus98XmtPD8XnizhbG7MSq1DVe4Ev8lB1qwat0kLyQ6wcQv7xNSoyEF1BiuRtZnT3ePsym0yDwZ/ewkv1vAXL51/P88dmrcNCYtmx0BG0bFbjkT2lX6tGPffrOCVw7dbdZq6gj/4vcDZXDidZ1qsfq5RZQ9B9V6V+OPAb18tcFunbqwAoUGoonAyD5wvhfOFs5xBiCBlDKY1+MUaXrTwF6+cZeiJx6cA27f5d4UkPDmTSXLto58ScQ6Rda+vWNKtl0Vmam9paw947Y0EydXXPpqdiMXK3UC2/aIHBZ+3GnImjVUHz1juuBVV1JQq9oK9+X3+9uuOJ6e5J6GewU23TbsndOajsYrHEuAPuXrQG3eBjiPBB+8C7YuMGrXQ1R8/h2dieViZhFkq1GBlCKK4zGOpje9rx/Up2HFdpU8rWZiOP8Qggxp8DEII07rbm0Se+zTRhGlrv8i+qHA/6O0XLJVJhCj38GHv+verFq6AX66/30OgS7g4U56dCE9P4GIJJ3MKAI5mG2VX+HdKwnX8Xh4TXR/Sp+vBK5T0+ek8X/fzM3h6JpyfZsL1rtukjV8meAmHG1hHz/CuyafE4TEKeZvPy/7O37Hjxi37cZ8W4EE5pD3f961U9jjLse96GRxMGerOVYQ6eeb4VC0Ezr5zNm3QcHwEv/dh9cC3PQe4LyJ8WwzYDng+Es9i/5/xooaw/SDEhyaGh+C8nbDt+34v+S7w2ycAK/dQ2P6gDsACnl8ov/6l8fmTwGfnwpMT5WSWoyvR3chY9hrk7y+FJSMgrAOczYVnp8pnF4GfPgt8/ixwcZGJ10Xu/hqPftz1MWzP72yLGR9sbDyggyi7Adm9z9b45933ANn26219330rxfyGD0L2dJ/ell1xG1GysVEneZNT/bg+3PVBU6HtV2fe/qE5+Aq3yzd6TXfbsFR433X29uArcrd7PlaW4B6yK/u94smU3Vv8/ve/qhpOz4UQ4WwhfHIR+J//YcVvf6p8/kR5elJSoLqZobtrePzdGqcSbfu0DitFyWBew5MT4Ysnwutroe0CQeGrOnF5nSPB9U3mPb3vnqzn23yzuatR7/kx7a4hfz/Mjbo3OruLJYjtmUqfypdtZ+D9AZ7aqCbbdyR3fT/CyFEKI+mtYYTFN8y7etvZfVyPAPgexnfs66NzBPcWR3G/3vV0e3XxkSGygj5t8kyp1GTPXTXP94luKK7u6x/KnsO0zf04UdAeIhwfDSvLBBR3xxH8Xoa/f+3rdeIuVqGLZ7Ccw6yWydzjNIWUmwlO57CcC9+8siyd0+R6aU+3FftZMd2McWgRta1KfXU5cy4WwrMz5YsL5WfPlc/OlfOFMq91Mlx9P0MZ4M6xnrtfaTsNyshB0fIKofyMam7YOZsJn50H2uSIOidz4dfnuanoegVN5wP7CHuEaXsGGStsKSnt3v/zsyKBFabpRisk0auVs1pnsFXfA4IHZOCdA92a42vz3d+/LU0/fj3VUg7p+VB9X+S4Owd5qPZto3KGA53lLtV1mztJ25S1/raftAYZAXRuGtup02/doF0wvC8b0eN6BMB3CoC+Uaveq9rtU0V1gSfnytXKdoy6O6wa+PbSmdd5HlAQZFb4O+1ucUy/NWIdGSvPX4NRwRGxrVmqAoa3SOmMGyf2dURuZwWvVsbPPov84uf7AeLsCTy/EP6+SaoAACAASURBVJ6fC//4t5VZVeRkNQ8/J3O6VFrH3fnp08ByJjRd7rC9afL3k0MdspxObtTJLUFBNYNGueeXK+NkJpwv4emJ8PxUeXoauFjCoi5AeoCzc7tGmp/LvlnAh8w8SJlH2wiyqmaDHFSY18KzUxCJzKPxZGl8flHx4tI5X2rmlE2piP3e/l4DpR6Z5s496+N56S7q/w3RIYpJZvznX3b8ly8T30lWRBhAcJ/jNHxT9uZAHQqNYFFgKALPhe+p7E/f2+E8gr6B+KEfb1FhqM3tS6X6lrDwPufRvKQyLXd/Nq1yvXZe3Rifnitfv87cQds1wN6h6rlKVTbNRJl/ttQWkQOMSs6tRdzH9QiA7yoFtW3ifQR648O03aYeQh5kdxH+wW/V/Oqrm8nrXL7MDRf/5s9X/K///ZzlTPnqZUs6zarYMleq0M+jHdCE20JA35PuSQgpKZ05XfItUmCdAKDephy/ZSysUKhl4ygH794Xzyp++U3L4iSTfY9XPYfzE+HzJ8LvfCr89GngyYlSxdzkUYcMfKvGuW6MlOCzi0AsxAFZObxvNsqNCKt2RMtlpXNRN0KmTgbZs7lwNlfOFnkQela6P8cR6P2Z+g8lS98O9Lap7/L3CjtNAcJeRqiKyon4MEN3OodPzuDVtXHVFFL1JLjpSEXeR1mI8efMYFN+arggRQv4a4mKrXSBOjdr+PxC+YOfBP7qNx3/7s+MZrVbf/N9WYit22dF3aTpMnNSWwjhzTaNCRuyi8MdoRlsPDO+xDwuUkXKjOc9H+W+PW9Ca06T4LrJ92Je5zP68rrjT/5zZhJptpq5QhT+xz+sCZ7KdRRiCt3q2D7E+vTAe+xxPQLgG0R8G57H7bb0MSD2jPF9VSyqMJ8JMQmLOu09eE1ytBH+jz9Z8c//4YxlHTDgu2tHVVlUma1iXwHmLumabGh6uSTnam2sWqdLfdtvbgFGNi3/ekADcLfZAKLaQKt2a8uwH26iqWv4n/5BZm79vc8DP32WGXBi2KQv3bMkzsk8ZAmbUWq4CrkWJkAQKRylh02FSk7jzWL+uXklzKoMImOOybGT0yep9t0XkTdLcb4pCE5Sd0MXomB9navsu6iwmOXooq4Cpwv45FxYNUZnWhwWG5yozBK00WAUz1R8Q0JOfAM4I7Vd6aWzJA8QJneu1vB33xn/75eJX3+T5X6aVa5/pT3P3IZC9+4+MRduGnh147y8Vm5aitOWz58OpO+HO5bz99NmVrJyzuaZk3ZZKzHYYRD0Kdb00ahZpjM0dJBsEhFe33R89TLxH/4qa0+ZZVWJbfKKOjondabcXtRKHWWYY52AXv+8BxD0N06jP65HAHzglcrlJ9hiiui19Mbpzz4tNotwOsvF8dOZEUpFfDwG8eJruHiWU6b/+t+vmc+Ef/mPaj47F17dJIJormcxFcQ9BH6yFaUmz/WJ1zfGt1eZl7RnbVH1fKQ1p16m9Gdbc2hbNRkVmNdwMsuf85AW4DhW3lcHjAGeLHNd7osL5dNzZTnTjVzWhKR2Y6xbGw8xb4xGL2XTc4JqIUwfc0PmtJhs/i9S7gWTcQc/GAPfNyo8LvOQP7cOgFQS2TiGlnGfTX2qaB1WQoyZSL1NAfNQug9L7do3e8fYsBMN92yIRqbgv4nifHOPSoryau1UQXh1Y3xyEfjf/rhjtsj1wNtuoZOZdmRIq+d62vXa+c0r48sXiVc3eXicUVSnYsN+0q1O5WFMRDd0Z8ta+ORMcBJBQwYZtc3n9P0Jn/HeN3csKV3KdT5QXl4bf/ZXHV9+teFm7c/INgDGkEH5ZCacznK2IYTMij9Q703UX4a7wi29tY/rEQC/r9VNM/J+u0emkgesT2bCn/+84XQ+lXEZg+DLbzevtTx1/v1/XvMv/9uazy4C6y5HKa6bubuDkd9O2iarxa+77KW/vHZeXHW0nQ3ad3qgJX7c3j2uf1gRBlVRThdhGA6OwW8FBD0QAWox2vMIp4vchHIy10GvMDcXbYxxb6S7tDGAA18pU8OTIxjdqRNlmreASgQxhHZvG/4k8vveu/N2eUYzQMeehRqVDQj26VAtg+pRc2TrW+zk41p1rgkXaZ+B6FwyuQQRlRlIVWqHa2A94tfNzyYlqGImeD6fw3Wba6h7xwB2GlnG15TT/k0nXDe5gedvv+749cvETdNH+vn6xsPxnW2G5Xuid9X88z0Ini+VoBWLWljUTlQp+pV+cPzBRuLCQy3SnWRC0wqdwW++S3zzMjdhjVP7i1NYXU7T/DHAf/pFx7/6xxWLuugjiqB6Ary+Y5ZW2ceG87geAfD9SpaOTlLQrL23rDMItuPuz96mhV09wOtLuFpkFfWmUxQdIqjBlN0L/HK9IlkW+1y1cNMkrlY21MW2RXrHXvQYbMYg2BWSb0iIwKKKzKqAaCKMrtNHdRS7pY42dMfqJjKrwpTceByH9GA0/IxsGgeCbNdsZIdjUwbF90TfVJDTrPuiOzl4i384Q5QVVXJkXKJqG/Umy1T1gAmXaQG/vovUNwA/OEQ9qTaOe7bqooDMcQ+4pwJWeahbSnqvjrlhadV5pu8b+DP31ehk4LDt/8GRgVO26ZxV03G9Nr5+nUmkzxf5mS9qH3QB2/LVryr0Kc9MMF/F3skyrteJdRtpWqerhCqrre49u2MQ7Jlbcq3ZSBby3xN89dJZrX2nrr0tjdTXaxczYVHnkkYV+gi/ubsm+UiV9giA78e6vQDde+Eyqjf1TBF9IX/8Kmb7QdCBv/068fmTODXeh+arDhTNbTjEPQ9hJuJtUu8FMwiCjomqS1lw0PjTEZB1KTPZq8C8MrqUMAtF3mYrSi3r8tomjTf7nIYhyhLfibTHKhb7Gh/Gg9ybGp5gAyD7prturHEnOin4+HY6jNtJln/QfVhASosO5rhxo3dgbIuMelvn77aheCFsVG4BWIFk6kHz3OJpAxvQRoFlrKSwHenvaygaohqfZhl6kvXVVtd/lzZ7tk3T9Oj0BMng6OUOYhmiuLE+5jb47TvhY+oyyJ3C+brutxHadf5Mgw5hkMl5fiTCfgTAH80az40lE5IJZkLT2Q5DzD5DujiFxRx+9/NAFabppL3wewD8fBSG9dRl5kqThFWTBVT7TrRDYK5bemrdaBwhN5ooqgHVTL59SO0mmfPNq8Trl7cBoAw1TrM8NzU0z+wBv8N+sgwGF5eh2WdifGVkdfHSuOR7we32ppcfaqVJ+lBFJiMS4yzEvqTqWIRVDjU+iU5m5qSPPgkoVqInQUc0g74l9jve09tAPI3wSwwoeWQlBqeOkeXc+eIi0aZpCnQoF9T5++MIEBipvPfgq3kURiGIlkd/S0pRpkK4oYx0WPn+qsk1z1kNdS0sbhHEbTXLnzVdFsBtuyyNZFZ4a+MjMDwC4I8NBIvR6FL2YJtO+Iuf24a42Ufci8XJ7kVx60r45EI5X0Sazjhf6ODR+wHiRDkIKrlhoI7CrFJO587FSWRWdbhNo7sdANSpIXMHSaX+IpmE+WyhzGuoQyKUJpJ94q6LmfLrrzch5rj+2bPf6Cg9RqGNkpH2Wx8F7qTTJgDZp5VyaJ3/zfbLYLlvCQ3dnXB6H+svG+doE/He53f26UJuR4T77od7QiQgbgMfbNGL3TRp3V0sGI1abCS2sjSRsqidiyX87FnkbH5Iaksm6fXeadt24mJQTheRi6WymAl1NdX880ONZGXPxVKDDoAXRfiqEoLB738ReXXlNK0RL3Jq9vr1FPx6cvamydHqdSvcNM6qc5bmzKhxXz/W9h4B8EcEfp7bodvkrBu4brIOmHv21PuMVV8fefKsEOpG4YtPlP/h79XgzvkiM32oboBvK0N4Z0QU1ZlF53yhmAvzOrBuhGR5MPquxpVxGigbCyWGkOfoFnA+z+CKO2lLiqG3LXXUadrLMhe6GPT6k1I6UftUWBbc3Rii26Rvpga7H1lJCHVOL7ntFE73sX18SMC3aRFi6MrsO2f3Ab77/vTx5u+yE0UfzHB4L2htEweFLZL0cQqwrsbXIUNqWkfvK2RB3JMZfHou1JWyamQY4n9z56Dfq8r5QrhYOPNoOfNxD3WVAQS9V4SHaDCLWdj2Yhn4p3+g/Kms+eZlvr6q8qGpra+RAjRtJrx4fQOXa6NpNUeBUu2v6T+uRwD80FcyZ9Vmocx1B2mkB9h7zafn8M/+KPLJWeB0rixnws068eQ0kzLXoVe2eHNjLJKBZCaAWPaGZ0KbQmbXeEuzGzQP6NdVZl4REcwMS4crpH1E6ToNuIKWbk021Dm6J+0pe1W+ZZf3VAJ4XzhqEJmTm11yzmkfO5dwt1Dw+7dsP21aiQYn5NDsi+jkFgoxB1/dIw1bFSahVOjm9t/Drt1E+vuirj4lq+SO0iWZuu5kJjRd3qv2FptVBKIqVcjD6vPK8siO3q/mNgbBgGRJKM1NLD5TrtbG0xPlX/zhjOvG+Ld/kaffT8/h8tWGsg4gdXDTwOXKuVrBuuvJCOJBh+xxPQLgB7t6KZom5c3edpnHcpxyAvgnfz+ynAU+OVO+fp2oQlaZXs6FOtogysrI038TI61llGxWFMOzyOiGEs3vYQQmQCGb5h6VPEdorhjKhgvz/tZKdZf3sK+/HErVbXMlbn9/+iBW5WfmwHqwSHvld2RbqPV9d82nZOTOfgo8RsKrB0WV5dBTv221iCwRudk8swNOhO+z8KPvCbkZRD1HgSryxnt1397P1GNODJuat8r9ydkn6vECKQiVAzEhKNeN0XbO05PAv/rv5nzzuuX/+rPcmdPXDikMMutWuFk76zbX0vusymP68yMDQHd/EG/npjIubk5oTtasbUXNDDwQpKPVhGlklgytQFcNSCBpIFpWA5YoD2qEtnNpwwCtb5NI54zcbA4ns0zC/NWrjudngfNFZiipgu02J9yhV3YYjUs0KI7G3P+WTN74tTZdl2U+zb101eVRjYnMzH1d2m1R01vee1/kshPZHIpcfMWGTvq25pb7EX2/NxAou8oKO+oIWyHiQRB8uzzHzm/uratNjknfdbvnJPXsKJprgu45dX/MXs1Rqm+6TXk7dZK+M1qDEsmp4J604XKVOFsG1l0kxsRs7qxXWw6xbb6yXfBJevpjW51fo6cRuYYoNU3leKlki4N6S5Ca+ArWZw3dpRFxYpphZviRIdXFxcUmHe/Oy5cvJ9qxxwriPn369B1HgNrR+JrG1ph3JA+oOU4HOJVFgoFLGGojPTmyqpDeEYHxFgXwQcsgAnWVeSndldMZzGL2VoPIznD225yRsXHUnlAUy12Wb/eK/cDAoC7Qj1vcRs1mdsf3y+XcPnQuB+uA9xK047Dy+geaZ9j7meQtEG2aAn6TG7JxfuwWcJFtYuzxbOtWJDiMs7iDWC9B+9Z7te/QEd7SgRylljMZtxXmJMHccqRaO1+/bFnMAv/4Dyr+7Z82O+ewH+9wnzrDHysA1inCSnHrsKuES8r1es9Ud2sMV0E7wW5AkiJB8MBmmPVjToHOZU7VRMxbhIpIQLrM+hBCIDUdchOwDhDFxOjcCK7IOwO/u4237KRplKi5k3Lgo5SiOHFQhfzw9+6eFXD2VgDfYD+NVe7dx+wiI6WIMdClYqy3eLMt5fZ3t2md6FAqai/4cXfzxjZg/NiW7FGvOCQyew834Z5vOsdtXVodDw+Vx2ozoN87PQOl3pbj5CMQ3N6rxyaltykMt/eQ3/Z7Mtqf3kcqhTjA81hEFfrRCd17B4cz8pgNLMavQrocGuuNU6OIa5lSUipCTmETUIvFGXKMROYx+LAO8oMDoH1ntG1HSyrDpAlrOlQz2PmsQ63C05Tc0t8oR3dccnS7jrXNNWg+9lN9qHckY9AVE7kdo/weQCa3fFZ/U7PiI4bC4s1a0ZsbNNPMkTKPdb1OE0AU3cxYd6MMWh/EFX6Ag57xYQDr3sAE7jN7H3473t4I94137X1kdyrcG9xto3LuIyKB0TVUs/LvtgHCHgSTFTYimQr/HOmfvVF2xO/wAdynoJ0GGrkNwbh7ODhLOo2C/Yhn8+Nabcgct8GUIBXQ8x8qHc6MTRpSNdM+JE9YSpgYSvjII8C6zqMFKWWvy5XkjloAcdpVN9xQKQW1UAZu0b5d/uEMza0gKCOeSt9OixRtsS43qjhyUJXhTa9tM3jsO+mxfTN7947+yAYtmRSmGaezLEcEkFyHubSXV4mffBa4fN2Rul3mm8Hzf8NU0NsZj80A/I91ibx9jeueyatcZihagSpTUm7RaQagWeUxnyydBW0y2pRFhzfSU36v979XtuOA1t8+iauDZYYtFehUGJVagzYpXdoA+Obr9vp37qAW9DH2y7dVuzwSJptKsuF0YiRzFiFhlnsWVDW3kDuIK+EDTOM8OACu4g0iQpKEFmEtF4eQW/KjV6hEWkrUR593AX/nNzCfriCZmDiUdure9PadaOsO1o3x3bURVGlaqOIGAHcGi7cOpo+Nl+8GNz1Dxzav53Zaple1uG8E2Nd8clE/0SXjbCGFmspAshJBm5yLk8BN65ycwqvvdo3uxuiMKMtkN/J9mCf2cQxdHezEPAr4ctoTWrwv3pbX7Mm0KUZ+/GapMJ+0Rd9v3TqzG2g7GZRIfHdr3/m877ruSYS3DYAHKO6m+5FhLjV5pl7rUtbUNLPs5JmWcSfnk/PAt5fO65u0e68lz7zGkEeTdCQz9bF2gc4KwUHOyCkuXRk7CUiApCnPTVufcM6MtyKZjyiRPm4ATGqI91PimZvQynBtoiMg2JB6NMRykdXdsSMM6gRr9kVbMJAzx0LKW1UQo2wkU8hG4U/+Ys0v/k4GQdyoh7scD2DsYW94q/EgjRhobhpjUesONdYbYHvu1JKclceU767Hnn9O5/78K6OqlL/5stuvBiHjr0LMzEb+6ZaP9nYG/CNee+uE8gYOgszBm8kLSJnf00FiyosKRf73TiA6dB3crOH1yjlbBH71wqgrHRG6TQ/XDnfpllO34+/JfpD34q3tpFVlt/Fnmw1nmwO3M3h+FkneN+bkaDeqMhfh2yvnxZXzn/6qm0pB0aueZB3KOgoxyEBw8dFuX9fNIy/ZzyiSy1kiJM9SUa6jpgIx1Iue6Qd24x4cAGOqCF7lepJLzg+LEaTGW6ELDeqeW6tVCb0QqGTplfBA5WjZk1KUAn6zSpjXzqLOrC5ealxSoqd14/zmW/jf/+2KxSwzZsRClvvk5DAJ6LZA7SGPf3ygL5bK2bKALPDtpdGa86tv0k5X2m2x03eXm/btnzxXLk7CqL17/P6OqPAnf7kiJXj5Yvp6pxcb6Zo6FomaPXRcU8LlRyB7F9HhvSI/b0rH56a1OGvr+fAMqyAsZiMqvT5bcw0hOH/8p2v+l39SM6tDAZctAnK/4/+3bdC3TQRsoaOUrI1Lz2WbeXBTgl9+222kkoamnqwQ/x//65qra1ivi+7nyCZUFSxncDIXFnW+Tyrg9h2E+ZTMYGeU58fZvLVWGdEXFpo9y8z9IoKmKqc+tZfgSrhmQHT/8DI572QQPkk7eWUBkq2RCoSsnaaSN6lBZgoxe0vw2x1jGKidJh6JEEMeOj+dCxcLOJsLdQWhowxp5APUtvlAd51zdTPVzfvbNzRqfmBOcKyawLbGn90OonetL79KiLRTIuZeokg3BuLmes+GCLCYC7/81vjiiW403EZAqL13LndFf498Uu9mVTml7evJLKttOeBBhHktvL5xzpfCYuFcXxYJrXL4r15nEeb/8/9e5xI8m/3nd3T/vs0g/D4nVQrPrN/yc6WasqkWl3OyK2QvG83ABN1owN22HOK6dk7ncLZwljMpxBSAt4UvtITSRWFzanN+nH2jEd99sCJZ8xLwaJtRtVGj1IcIfu8MAL+/lYXX9mDd8AcZpRMrh0WtXCyc56fOs1Pj6Sn8TTlgVugpuwTWwfrYm1vubndLA9/ZxRT8ri837DRvbR6r/HnbZr+zMJszGQie/G4Qnp3B0xPlbKHM6pEqvcotFGi3AfZdTe2P655wUVZXAiSfZB1k62TEKJzUwsVSeHKSI51vRs8gSWZGef0SlmcbxYa+43kAwYlc1Ya2zEdgubeZ9w7ncMNXehh0e13EcfZhmN2zTUq2XY3Sq4CNtT73ZIJwOF0KFydwsYSzec709M0/VkoJ+ANF64/rEQAfftnOptwHhP2/Bc/il6cL5dmp8fkF/OyZ8Or34cuvndeXm/GpNJameUtnr7tH5/o+OaJjV9vekebYA369GkSMzqcXyqfnwsVCmFdaaiMlPcRh0uLDhuER+B4yP7g9KuBbKe6+XlcFWM6EJyfCkxPl7CQRYgaHPmXYg+BYMeHHstIemyCS6/9nZ/DJRSb4fn6qnMwLyf1IFcP3+B8DF8AjCD4C4PvlHcue9Nu0UKeak0fLWnl25vzsmXK5CrRdyt6kOTc3OQr0EYNGkjcz4arTsQILu6wr2z9z6Oc44n3710yem3/0lshSE5w/gYsT4SdPlc8uAhcngXklVENdYMMt+QhwPxAM+u0gaWxkqKrgnM6VJ0vl+ZlxsRQunjgvvsvkB/0e70FwrxO39bCj3/0z9zI8R7zffd5zm9ii/14IcHoKP/tE+L3PlJ89U56fZkL6OgiipUP7XuGsPO79RwB8X7xj37M967GfTCih3Lw2zufw+YXRdCGDjiRmNXz9nfP1b45sZkq74KJ3/MzBnzvife/7mudPcvrrZCH83k+E/+aTwGfnOQVaR0FDJEhpdx6cjfXj6XkPl5amspw6FJYzeHqqfHKu/OSpcXmT9fNev86p9nb9dkD1pj9zLDAe+55VLRn8zuDzp/D3fqL8/heBnz0PPD1VFrUS4wgs72UAdP+he1yPAPj9Rn+HTsPu6Q5hTuUdy5nz/CzzkfbNAk9P4FcvnN9cGC8ujeurPEg7ORB3tUjv0Xdzn/IrjlO0kzZ4P4ITcXR9KoejheH9yLWVKubUz6dPlN/5TPj7P1F+91Pl2VngZCZUURGJwAqRx8P+obiDIrlzeVY5F0vlJ09CUU1XLk6cX78wvnrhtHXp+NTpmME2Fdo4FpItRhZ/0ySAbPGLUro3b3u/e56TSdNZ30imwslSeHIGn54rP3sOv/uZ8tufBD6/CJzPNTfDlSzHoehPDmSdHtcjAH5A1mGF6imz6przAn6zaJzMlacnzifnxq8vhK9fK6+vE6umdJHZWIB2i1dzi0ZtG3zGw7y+dUh3ukHvA7Jbrz9uAR8TbW83cuGbjtYQYFbnMYxPLpTfei781vPAbz1TvrgIXCyVeRUIGlFWjzWP99j123Qb5/H1oLmJJSjUQTmbGT95GohBOFs4z88T539U88d/vuarl8aryw211QA2W/t4zAe6PRy/aV7xnTMxRQ6ZdH5Oxww2QLrzXluk2dMz5DvnYjPmkMcblgvh2Sl8ciF8fhH4yRPl8yfCp2fK+VKZl/Rn39yzLfZ8SKPxfvR0j+sRAN+3NJF2wJJZXKPLjirCvFLO5saTE/jsHF5ew9VaWLc2SKWMD5fvOeg+OqAyJgzdoncSdr1m2XfY2Zas2fKQR01qVl7QffoeciAyDZpV40/mwrMT+PQ88NlF4PmZcDoLzOtACAuUy8dT8j6BntzOHuNF4FnLbGtUWMwy4tRRWNbGk6Xzm1eJf/r7kZ8+C7QpvZW47d5NyVvw2LLFi/tAzlau+UXqIPzqRcuzU+X5mfL0RLg4yeNQ8yiZ5Um3yd1vi/4e148GAFUVLA9dq8jRJV0LHU1yaq2ZmZC8oyljS7NOwatMp6MdZkZFjYjQpoTG70lOw1e5rb+ao8kzO0wwZpVyOhc+PTeu15kHtEtpMhi8dz5qX8rRR1kS3yhzjz1VOSCme+fQgE8Njd9Dk3AcLfRfMQTmEea1cL4UTufK2UyoKyWE2QB+j9Hf+wuC21GgjDwfBawopy9nTgxCFZTTOTw7VS6fwVXjrBshmdMmhpT/duagd6DMp/t7TJl3n/p1//vJtiI+8Y3Q7eiFNxkXH74/FsQdO3djkvtQupZDcGYRPvntyMlcWNbCyVyYVzn1H3UKfvsEnz+m1WhCVehSQyQyp8YsD95YSJw1Z9xU15gm6vWMQKCdtazSipkucOuO3Nv5hpvZ8GdVxcwwszyEf4zp3zKS0cyQosaQLA3DsG+7ZlYG3ZMgqYyQBkUsM4x3ISHqqOiwifvB9bcTPDyiE8tXhDAHEVQSlSYWNTxJYeAY7Bnmx/WJ2z3waeTVp5R01KV6iPfwPh9pnPrZvpa7NAq3RxhUswGIIRuDzBqiJe15/Qh+HywIbgZjFXJ9z4Q6Zomg5SxwsYQ2eaETy6ohPWtQ3wVZ9Oq3an5TRtCB/srvcNpGv2V++xlS2RjC/UoNMtWo3CKNGANZKAAXJM+4xpAbvnpmmf3gN+0o/5go/k67JRYSXdH/E89zM0Gc4IHLdEkTV4ReidEzx56iHJ9GGNsmHbAglcHohxDEla0HGEWEGGNmWrCI+XEIHlLM6gnuJAp9zhB6GC0dQQK4Zv44HLM04sh80x12JCODrwg6RwqDfghZRigNjCyyL8uzN+Lax30t5XRJz8QiMzR+ujfFcvej7Ujd10XyZkvq5g1dhrGxUMnD7iEIqgGleQS/Dw0Q2a75TkHQFGImdKRyZ1EJ5hXoM4xqcPLswIba2QveYOm7sheLkvqhYfit79/mtPUjHD0QidRI+IR9vcxaSOXHhm3KVdtg9g0yqtXpFqfoIErNFPzuTn3+OMcf0roh1U6qs7p7IuHJAEMC+LxC54qaYl3CLebsIVmD8Vg5wJQSIYTBudo4I5L1ZNPDNuLFrusQUTrrEOPoCHDdtaSl0ElHSI46tLEQqNKRKid4JK0TwRSCY2IF3W/XyHuXKVEppyMUD1ptNEj7hnvetzgMe2LpnKrpgF/c6iHftqoYij5hHtjd1B39noZSdgxbr0qhkgmfHoHvw4sCh/0zi864HwAAH+9JREFUIYLfgGDo97KBhg3/mPN3Q0bC9wnhyn6nTwRcwzAx5/4GAYDv0raN30+HmdMsxiN8uXNQ5I77MrymhKL5J6Uz1KcpU5lGlbdFeh/DuZClw9yonlWwFFz6tECOAulqUlSkhfYy4evMByoiO8LabxVAhQ0A9SlPsyyWkLHqYR9CrKqK4GAkVAJ+5FyLVUb1dI5VoCllNWECGrLgag3ERulaEJciquhIUKzzH9SYCDJ4wC6+pWz+Jg9XtgBwHAEGjpsdipmrsDc47hNi6je/wi2PWB6V0X4MINiDUgbBDapp2AjgipS5zlvGZu5IVBXB6PJeb/D74YBTKbpJuypS9qO89b2BMESo+yS9tqO+u/ltf9xr3SXa1GSVnBo6MaIopEQnQoiGRyeIYpJKxKRFQ/L4m/Ttt98OzluMkcViQYwP16v54sWL3Qgws50k3PXoCJCZEC4qdJZQzylRMwhVQLqM5HpdIa/XiGUBXMMIKGJFZuMHNihaIiWXXBvxI15rVxrp2E0iB43dm4LedkpUH+T6Htf7AII7jTFbzzX0mn9jVZs3fvSKkEpzjCDyFin5PQ1g/de2Kv1bO7XkCHj32vZHffIRRn4b832Cm1J7TTIjWYvGABYIojSss3CBVHkfSaSjxbBcLjryXsUYSSk3ZW43vYzTow8WAapqRnhx1I+vAXZNS+2JjpbkLZXO6FxAOpw11II2FR1OLNOqrSVUA25ZZuMHNCW5XjfKAfWCov52r8a4YN+nG4+j0M+pBhvSXb1Y29t/5klDwbHX97jeSxDcdsry424Gjcy3Ww2OllrN8eoQZTdOG1J6xeojXlEn51cORIqP4Ne7NJgTHNwSaglNgZQc06z6nqAowevQmCLkeqEex2U1gF926n0SVT40+EHpAnUVzA1L3dERYERy6sJL4VQ2DAsuxpqOSmfDluw/rBAQFewHlRnZro/tckK8iYc76UrjAaMr2SblfajXfYz+fqwguH+v+4OdGOd4507eBfqMqI/2OQOHYfHjPA6N3tBwzSwIUiVILVQRb41OjAoleYcncnNMTxyuuZx17OpBb9wFOo4EH7wLtC8yatSiBnwcAAWvuZ7doOaonNHYmhlG01WozJhbTXJFUjsUAuYeaDGqH3zHyaTe5w8shZKVVlYPBNR+61D0G4P0sANXPK4fFwiOMxgyclWheZCYYSJJ9kCYJQ9+rn2vMyAfmy8oku29JWLIBeFEGfcSmHWREGd4VdNaA0mJInSdEeoKUsBwQhTEJdd9g+cylteZZf2Y3VSAb3sOcF9E+LYAOw3YHtdotUxitQdkpHiw5asBpv2hnOR3cZ2P6wcFwe1sxW79t3mgdwv0lGAPDRoP4jD6CkEns7n+MQLfGGREIMTN6Eu5K4pgJGIdSpe5IxIms8fJO4JGUpOGiN89qw3LAxCpfO/34tFcvFvgmhIIy4Ne2bF0vLu//wiBP0YgfLdr/eARm/wA51rkIwE/SwPpiAkkHCHk/1xpQkdcRFx8iLg6tyEySySiBLqbrryGIa6oywcJJo8R4L50yaibTR702KXv7UC/n8bycf0QILhb93o3OnbywNf9kA6jvIOSxge53BFzXAVHSz9BySmZk2aJ+XJOV6J6lYClhGhm83JxKmram4ZKQh6fczCTHE99YIoxjxHgzrIHPxjv4pC979f3uN4vEJxGOB9Kosof9HWm3dgf6b7vKeZG3oBL7uBsafATR2clshMwyclR1TIpoKCtIOuIiAxsX2JydP3vEQDfz73yQK/hD24Yjr2+XSPwqPf3Y85svO9nZXdPPpRBtQP34yPU9AuaQc1kIC7ovKOTDqscOVFS1SFqBRwUVym8oLmLMl07VRsRByOnQvNE6IdnPz7yFKjeckj0gQ72tlr9sYdu+noix7awHLo+43H9mJbfAgIPsRflgdKW7/a8PHx0+aHtAsU9oQgqQucdLh1SB+IiYCeJlnZgyElmo4jREI+0lx0xRUzbAfREBJfM/PUIgB/Msrf8N97BgTzWsPl7en2P6/2MBv0Bn7W8g334uB/fxUoj8nAt/Q5oTnvOz5ZYfUXXtETNs9lZJi8O9tDdSauO2mqSjIjIRT7I5/TeAWB0wRA6BJGEimV+UleSBPRID0OTIJXTdh2qSiWRrusgUDyY45gAkiaCBSQFQgh0NCSzPHOTwI4cFnVNSEqlKwucmPlFu5aoQncHIaOjhJS9OauyzpdbYqYxk85SHedSiKHmRFFcAp0lrGf47+xoogVTRxJEakS9eLCGEhCTgcD87c1tQ5D8Wrmun7vhvFMqKlJo79i/geSGi2x+l1SaMByhfodO231jpduEI9u8f8s+6CTTXIkoSiZiP+adkRYlop7njyVmzVBxJVLT3TGeEUzxoLSpQQME0awQoPnZB3u3Jk3NEfG8N97m6fmaWmd4W0YMQqLzVCLogB9Zlep0hXokliFrcwcxxPI9V20x8yyAkBq6cEX1pEKfV7yOa2LnqAupc2KAuQgmicYTKcL5L2oab1npJeqBOcvMEoMjVt1ZB+w7S0UEH91ELUDafd948/7FZAkTRSWgRMRT7lHySHAlyXFzQaKZYFfUQLx4RCkf8KwNfNwBUTILjiRMA+KGaM6TJ7GjE0QxzTMzO5mSyNA83KqOqxUyg9vvsEue+QkSiuZTJBDQlOjuMPB3rSCCY4VOLoEYQQRFEXXsyAMunrlrDRu4YyUXJzKp+pHMAJXPEALmhhawMfK9vU+No5NUfjcWwCg8t4ViKmn7zs5OpqU60oB6xNCh3uNEBCN6QBxcjpsfVKsQLywfvtF3M3da1ncOOhtpwFolEExy2s1C1hbl3RI5mOhRw9gpzFgTNpSPkofRQ9kl5uujrm9uMzApFImGimeTIIZ7wlIiaE0CfBGIZ+fIuWPSEol0Xpz1MgaxdkNQKovUyek6G2xklq3aDJ36PeybjJtwRs76D1Vwee8AsJN8w9XB3BC3MpTZgQckzo57fbKwo3mWdExl2DN6yHPwRxooaWcgQkdCrScSrXAC5h3hSABo2w4k4ZJ1uAzQMuib7pGFMLEMGh7QLtMZeVC61vDOQI+7v26GumOmg+ROJlTVrLgaj/Px1E9A8wEy8ULkbKhlLA9yLBehAV1WD1AHNCuDiORayR0A42LFuBkkBQMvA8LmCZHZOzs7WdboyPsrMxAfIoch6sJx86P3R6+D5MX2OxC0wjVbVb/Dx7DgaJFPExPcFEsJjU4yR3X2Tu2TlPs7UDiORFrvA4wLWWCWyvxcQkozirij7jThuOsX68ElU1GqanYQs3dKSqAhR80WlXgxJy1aunRNTQa9TLziNCS6oFQe0FbRy5Z2ZYj1s4M5hZqyi/hWUnbj+3cszdmPAgB7OjJxx0ikaEiVIwf3xHx1JNWOZAMtJRLKUYMQTfCbhJ0cGQGuE6FWoB1GTB1HzIgY6sddf3uaLYQGI60M6TwfonvuveBgrjnytY5Eiy7nNN4SUqJaH/v4HMeyWKmQD4kbaoqnBPG4GDheJ3SmWbVaEtF1YKNQEnpkCrs92TwfTwJtQiyrpNg9D7QUouBEyuBZg9TQWcf85rjPLyWtus+jzk7BcZ+/um6gcqxOgxacSo4IXRPB50e9/qpOePkMFZGu6aiKqKqLEeR2k6RsBFIpKjZWGcyFJB3V9bttwrCYuSlxeatI0FZrkrTIItDaOpcyEijZ0a/Skfe3SjkTRBlQTwKpJyx3LEbwTIVmTaJdCV47KBgtIdSYGWYdFowQKrSrkKuEvzBIIc9NlI8+rimKZkaY2700HST3MjBnr91wfoihhPcOAHtWAtyR6IRTQc8jVJkpfHWkg5c1Nh2S5/qDOsEEGli96Ji9Ou6W3HzRUT+pSTFrCiYcOkE0BwTHForrthSmk+AvgUsjuIKHomV4++sHi5jkwxDU0Qjhk5g3uzmr2XHXZ2Sdxz4SM3KqSzqjWyfmvzru/r7+5IbqokaWTieZfT6fO8maknZkCnRd5p1Q/FKw7wxtQV3vJTrsiexgSa5XepXgNCBnmYJqVT9U08nUg/aiyTf7L8e98uoC5FThPJCq/Lras/6rI0eqxczaCtfs4FZdxfprITY1mGFiGVxujXAkxxqer8foYAn6LKBBWFXvNoqo/0bze2eRpVyKkCE9cScA+Ay62pl/HjEF0dwbQMjZmdWRs3R1E/P9VUcbhdeKv3Y85e+tA3StMUPRJrH69iaD3HmkoaPyTSQWZYakgFw5dpWQa8/SSJJDTZON41UUku+0Pz7KUoUiVdXbfQ/f/0jK+weAEnIuz0rOuhLkRPFaSCkR7cgakpQ6kQoSHJGWqIHYKm2Tjr4lse2YSaCrCm2QC2b5cMgDqHmkWfaiaKC7NGKpLQmKu3EXG5+4YprKz7eZyWEJXZWoPLc3H5WGI6c9o+QDLUWOBgP39vj7u07UGFTZb4yeU81GKkbkyBT8SX5WlYMkzQBoERHFaYqg8S0OBpJVXcn1qqRGnAt+IpgmYnqYQz5pJihGKBuu4zbZbNUhS4VgpJjTyuoBU8E1NzgdFWGHhEfLjkKjNKlBuyyG7dHuFVVlEOxLJgmtcj3L6kTs3rERVcHER6r0yhvNz7YNXbhBZhUW21ybS7lhKmli1h1n39bzBDGnMWOIcJXrq94ZEgp4uyNBiEmorhxdCLqINLHDzUCUECKaAnbj2HcNfuNErTeD9Ftp4N62vI2gurngqqhW3zvevIc1wJRTLmStOzOjMyAlUkq5RnFMhCJZe1BcMXE6zUYjyLwUUY70IH2BU9PKDclBqFCJJE0k6Y7uYk1p1WtBlPuT600uno3tnQ5kTjaYgHR9IVrpkpHc4dgu0GwjcickeehWvHqwkWOVE0Kc02mLAe5z3PMwLyTkSAAwa/KclEgB7QIsJW0nd+4vyyCIZoLg0qThBiZ+dJftUDMZlIekKJfYvSLUu1Oss9ysFDospJx0LHUq8we4fnITE5qQmAE867wlij72HRk0xz03brnlvewpD3Z7Mpz5O7VPMmrc0JLBU+85Ne9eS5a01hJFUTcCMfeKIXSWcI6zb9Gd5M0oLZnn/SwIEo3YgAVhHTLNWegCXEFadFTnMTeZqZI8kG6M8G2HXhmSIl2laNrliOw7OgXH71uK6a2YZVFmIQ7NOR81ACbvcK2oTHPHFRH1iCAEF+zIJhULLeIVEDAxOlaoVKAVqB1tpl3bwrawoqUlSK4fubSYjhpj3jbFYTVoLJVSRzHUoVXDJd0ZAbh2WFCCK1Ei0ROVzKhYU1lHG47vUuyHZnt3QjV33wbpjgZYtxZzaFjTeIuoED3i1oBa0aF8+zX3BR0t6lIU0x2VkIv90qF3tNl3fdTrmlOHLkSrEJfSKdcea4GnTtqgZ2UZGI4cs2gt5MxLSasqgUhFoLz+kdcfbU6yhs5LJNd/eaK1huoOAGhpUXKHeCAQLBA8UFNlB07ad2qfXHrJOJnu93uajUYqbhCiwNqFWpQoARHDPSJHXv8sLVljGB3eOpo8O8mWSMk4sZpV7VyTCBJZhIq0WtG97KiXC4iJTgNd49hVy/yyo24iFgMrcep+rKKQZUgZTctNUvKGCQgtezYgEpDw/cNRnKRUHgLASESfcV1fU3VOFRes1ytm6qxFUGlZyIxLCUSvMVrWIVF7Jp6LdcTbPGdjZM/K1TFv8jjBkR5+TDWt50HPWiuSBVJos3Frj78DIa0xnWM6I3YQpCOFXHaMnR5d5+2kwzGCR1IZKlDNUz/tfdo0YkW0FjWl0Yp1aAl2SfIrVmHGsVlaJXfxikaSd1AOYyDgfnwUWHVgGugUKoTacyevayjzjcfu35bOW6JqrgP2Xd4S7rX3pHR9YqUpxXOtEhXae0XodzkXvWK2TjroMEceoPxl2ma+x5AbI7QIWyc1Oj2+BHHDGqGji9BqnkmNBkkj9YiE/uD5Gt7fMAmYBhAliedrf8dMJKaC4YOvnNyHOddc65A7UsArIgkPSjQjyoomKjWRJiizI/dwF9d0aiSUUAVSzI5bjBHRmhs6tIPT0mzUsSYFIbQL5FcV9rMaf9ERXjXEKyX4HKsDBkTrO6NlBGC51IPcL/sutIRKYAXEwKpuqNyR7oz47I+YX1xMGrtevny5t+P2bdfTp0/fbQQozMFrJC2gU9xqxBxEEQ103tCkDvdUZvxyx2CfV05th7Sa51TUQJUQBKlKz3R7ZJtw+c9TxFPAqRCpEJ+j4nDkHJGwhFSjKU8AKBUdhSnBjhd01KoDBLWQwVCcNrV0nkhmVKp3lCASpkYQQ7UiVkpVVVQSqSTi7bE1wExakLPJeTBWRHFT/D45rjtTdBGoUKtwozR/lDlDieDHRUBV6BDPzQ39nJqkhLSldnzHIQ8hEF3REHIXZchGw/Gy508e4IwNN3v4e462HY4cJZ53EZLjjedRniRIcoKCqOJ6XIrxnBYsRxPRIHmi847GDbdEfcf+lX4Or3Rh9pGIWabtCizfbQrU072aPQ7e3xRxm1G3MQ+nqxDMqBAqFeTIOliQllqV5E50QZJBZ2AJxfFYyh+ysYfqjicjSYd91ebO5zXoqKBuJMzs6DEjdyV1QqyUdp2oZzNYNQRu4PQPfrgI8MFSBHqN0hG0yUAjCYk5VYUE1CNIwKXNM2NDRicPbC6qOj+W0nadWTgSnbc4iXDkAY8SkNDi1uApop71zNDMyiDtkTUkvUJFEGlR6cpcVs7Fy9Due8Trd4BraVbJTQOqSoVko3tHF10QydImCF3qaLqO2NzQaUsyqMKRcxCB0omi4AnRzDqBSJmxXBwZoXUobR7wFQcvauQhQ4D51VGvv+pybTiiqGjuTKsCGjMTjN1Vo0rZqCeczhNmHSGVe6JgXL0Do1xqwjh6ZAp0XbVIELxyulrx5IT+QwuoHXf9N0n4/9s7uxXbsqQKfxEx59o7d+ap81dd1RYUVDcWNI2I1TaCguCliHYLXooX3vkCPoGNlz6AeCn4Av0GvomCNNhVp06d/Nl7rTkjvIi1M/NUS2XVWXmyLFwD8iI3SWbk+plzRsQYI1ScpoKFI5blr6Kpy6Xd7WR0I5XqiM9rhFiygPvFW10wje38ft9IMUSyzyZ+NwlrLxOtdtw63QRVYUSR7kwaiC8zGtj3ZFrGnJ2FCZQsYwOM3r80HHmeEzVzAOrnLUX0ISiW66KmVEzvwe0syI0/NiN6Cd6NghBxCe//4Xd/A5wCvCliAxZGRCE07c28KxtLSyUrgvRGeKCq88YgtLHDlJ9dX3hXihZioQsDgHfJFUMUoWIo4YF3Y4q+3KhKN/SeTFYlMx9XsiwofbHSRSJS3+M688+Cqbe0lXJm0flXnaDl5sXVoJphUtjVU6xXpoWTwtvs8KEx93AjiPkxu48i+2R7SmTvwIjZ0ceTndemxSSpVnMutnihizOVRhsnpAsRjeGOE7rOc9MgEM3vzQybF/key9r8fusErnErJQy5HxJXP8d6hWbokFIODSctI50uyzKUqx0MLnQRDoe5ZzSNtC6U3pFidxyAkguQxhjMVRtDUaptlls53VniPmaft2re32QD8EsqHcYRrY5KHlrrsdqw8Pr24WZTa5IkwonO1IHulFLmfOOmGqVzaVE8KF6TXCZZ/vfIyofgVFEWkoBz6TVljAO74ZSrKQ/xTYztez/97m+AugtcGjoTDJ1O0PPk7BXfg+wjeyTzCKl0ZMmFbFDFQ9BihAqGULvRZ0bhVJZlKOFx7SyTaq+Z3SYdtsBSIfjGM3OloZHN/e6CaUDvhC4kwfQtdMe6oDLQLVKN5A3VOw/Qt2ronuxMV/zgaCg2GeN2+RMeMxFFI6kwHi1Pj+YspenHZsB1k24UkX+rScpAaEqUZfFvp1yAtAvek04eJQ0EYLizB2HIdd9QVVOfOApcBTKRouOFJWadtVoht3sx/uUC6ZvdvpMdoZaZ7JT9ru5piEZA2LINZjNtGFKxgoyKyTBXR4QaxnQHD1BDZ5VJZl3IzAK9SiE5b1kHiGcbX0NvMXGP75bcefljs0EHp3dDROmzKYW3fD+Ojjhvfn23WU3rjnmlR1o8GkocXWy4tbHNB+J01gm6KKikR/DMgJaesiYVWGrmaNKZZi2pWkN7MKnQnnyMPv6IhzaDufcN8NGvduz3mYjYlTBZ4MWoXSgMXJ1e4Z66n3QiPxqj5s1wz1S8tUiWX+/sL5NW78DZYVkJrfrAOBxwd0qvSWEuEyaF4VVlPF3Gwtr955b4dSFEqCE0h1ChWGBj0BZugC+3LyGcihEj0Atd5hNa3C02kHnRmN8CYupc/PceVNH9gZNp2Qm0GGi3WS/n+DwwU10pB6dtl/UAH/3a8Euna6N4utmEpZ2WTL7YCeXCPk16eKQhtnTQECzS3Pxwx+1rreVCbJkJ+uiMX4zEpdNxdpfLafq5CR9VaPba524LDwAvDS4Ef+F5YDkuVpFZlvaFOjV9gYfgxQgpyGUah7eSrM67N6CUpORzlQbPbd+ZPuuM1jl9NbzVBdMr1ybO/gYlweG8Umjs9y21zpoeudLBNJC27Pm9tBdIejlQrcJo6ATFspc7Vq5HHfnsnnCzCaZ/VRzLu0jqPq+NFmQxiU/FmWLkJCoHP2Cy5UoG6kd/Rr82dfwul0BP4cJGLKBqYZSkzU7zK1vOZab8Cq4zk0109qCL+eYYqgNDTxstaymCR4xx4QvewxnFaTLRtVDCONAoFuhpXVyk22+DriMejYFCV2EUoUhQo9Nt2RN0Np3SPVmKThBW8rTRG5Ep1x2LZ0/DW5SqBdgQ41yq21fGYaktbQcJjJhz/z67zhgMuphlOu42jBq0UDaRriCTd9TSTjhsWfwnbZeTJWJ2rTg+b6EghTSM/YoDRsmSls7PtvUOI8SUmsBx8Rt3M5ZGxH5D9rHw9aCd1CzhumFzOTFFzwVQxjItvL4nDF04uNJNqNppmmWxr5Nd5E/JbPvWUSpMnhuIwGhvd8FU99lwXW7dB7ku7911+dtWMTZ432NR0LBkDvc8vPSlz29/BG0iwiml4hhRAtFcZ+XYckqh9WuC9mMJ3WYJUK7NkdvEzHhdSmLTHkRRrG8YZWQThp29x8kP/0osHn6g0r1vgFWc4o3qxiYUm/3e1LPgWNjS1BmZsvToQZ1r0CbK1KYb7Vhojtbp2fNCK7ZwGkT1gsZEwziRnDDhPRBaikgXspyqzAbePag4hYIYbEi9TF8470OjZMtLPQ8Oqjdm4e53jtsJPTqHpEBbwtBJKFapcYK1hSQCyT6lhdBVaD4RAVWz1L1UpVX1QAuwHmzVcvKEBBY9T6sLnUC6bAClh9MlCPOZATdhM3PxqzPsmbI9j5xJLaDeWMO1hVdAb6zPdO4/3R4tszBBYy+XFBEs26oUjXmCeBBubBY+v/s64AijCY6jmjKR7o70uHNcVlEj1F8r4VUGBNhExdvb7QE2lf+lncDX5ibs7ZISQveRSkG00lWQ6ATOpi1kYetmtkUcmag0CSYJBhN6dLZu1/HqnAUee4I5DG3O+K6bfZpm7gKhttjLWACrhTivyG6gXzrbd95Dnv0O9An0YbWA8sW/PA/1wK0jXr5xU3fFihUrVqzIBGDD5BMe7brFFTMrXMSoTQi7pOjAS/mCxmO+/xf/Lp+/+zFPWiEsvts6wBUrVqxY8f8TY08W+bEFIBKE3swMHDcjROFw6TwfPmD/45/T3v0+T3qB2XrvIaHrLVuxYsWKFfcC8Vn7O0+H6NkOcG94HLjyIPTASX1Kt0fUn/yDFLZJ3oqHnwaxboArVqxYseJ+9r/j8GhPa7ijh4JJEpiGSdiVZzT9FRd//PeU+pj9dIKGgD78drRugCtWrFix4l6QfgxpcwlZClXVWVhfeKrGxcWnjL/3d5x9/DdCvOSo/XemB4933QBXrFixYsU97YCaDHORZPJLMPWgdWHyQmuXxA9+zu6n/yQ2BVEfsWlwIQ3tD0/AXEkwK1asWLHiXiBiRLSciynQPGguYFvKsOHVB3/A0z/5VwkaWEq6Rj6j8CyNjh84JVs3wBUrVqxYcU/Q7P/pPCIJAS1sTnbszp7Bn/6bEAUCGjmWDj1hswe2x6HkDxjtQZRJGxLKpOPiX5iOLprmzLO1maqmk8v/hQx9jW+Nb41vjW+N783isxGPYPJKjxOOA5jNobpQCIpsQAamIiATLkL/6Gfw57+U0HeuJ7eYFCIKNbbEJl5z1fFbY1d0JsccY17ydfQ3PX6Vs6jp8eZ7hA2+0Oz22o8+54UgWQhGzVCzb/1BWONb41vjW+Nb43uz+GzcUMxxuaTTU+OnyiGcaZo4jcpegl0/Y7hyzp/9kNM/+ke2H/5McPi6Vr06O1wB1zHdhxD+y449hX7gIAekXCFlhx3a4os8b/BId8Qd7473jtjN8NtvE2t8a3xrfGt8a3zfPL5DBZNtDnbxPTGdEyKYPGXQJ+z1ikc+cah7Dj/6ax7/5BfC2btc+WdYfXbnuLneO2Z2na1e+6yKYG/hAFFqU6S+C/45fZxwXcbEydQ10phZJJ3JRZCSbvqttW/15q/xrfGt8a3xrfG9WXxDKNP+c+jG6fAI06ccfGSKc2LzX4hPXP3gLzn75BcyPf1t9v2KbbvgRJ4xcQnsvnpzNnst1mO5NiJorS2eB/sbGeH5P9doCiW2BCf0hfOojjXmiMDdMbPrf0D121ddrPGt8a3xrfGt8b1ZfCfhEAUxpcnIhe/Z84R3nn/CO9/7BD78W7nYPGdvQZERN+M8RkrvfM+f8aqMd8Z3zPhKKZycnFDK/XE1X7x48XoGOJ29D4fPkDbkGIyFKaaqEvPvcHeqWU4Yzg8W13CXYo1vjW+Nb41vje/N4rvafQD1FNn9Fvb8xzx5//cp7/2uxNmH7Nlw/ukrrAQuE3tR3vHHPI3KKw78R73i6R3/fimF3vv1Bn17U75dHr0v/A9VzGwxzR0kWAAAAABJRU5ErkJggg==`;
  }
  initialize() {
    this.mControler = setInterval(this.mControlerThreadF.bind(this), this.mControllerThreadInterval);
    this.mVMContext.addUserLogonListener(this.userLoggedInCallback.bind(this));
  }

  //Fetch login details i.e. the user State-Domain from the system's credentials storage since now available.
  userLoggedInCallback() {
    let userSessionDesc = this.mVMContext.getUserSessionDescription;
    if (!userSessionDesc)
      return; //should not happen

    this.mDomainID = gTools.arrayBufferToString(userSessionDesc.mAddress);
    this.loadLocalIdentityData();
    this.retrieveBalance(true);
    this.updateStateChannels(true);
  }

  mControlerThreadF() {
    if (this.mControlerExecuting)
      return false;

    this.mControlerExecuting = true; //mutex protection

    //operational logic - BEGIN
    //this.retrieveBalance();
    this.updateStateChannels();
    //operational logic - END

    this.mControlerExecuting = false;

  }

  //State update
  async newStateChannelStateCallback(event) {
    this.channelUITransaction(event.channel, gridElemUpdateMode.update);
  }

  async commitStateChangedCallback(state) {
    //[todo:Paulix:low]: consider making the events mechanics more uniform. I.e. en event object with data field instead of an atomic value passed through argument. Just a thought.
    let qr = this.mVMContext.getRecentQRIntent;


    if (!this.mImmediateActionPending)
      return; //most likely it does not concern us

    switch (state) {
      case eCommitState.none:

        break;
      case eCommitState.prePending:

        break;
      case eCommitState.pending:
        if (qr) {
          qr.hide(true);
        }
        break;
      case eCommitState.aborted:
        this.mCommitState = eCommitState.none;
        this.mImmediateActionPending = false;
        if (qr) {
          qr.hide(true);
        }
        this.showMessageBox("Result", "The Commit Operation was aborted.", eNotificationType.notification);
        break;
      case eCommitState.success:
        this.mCommitState = eCommitState.none;

        this.mImmediateActionPending = false;
        if (qr) {
          qr.hide(true);
        }
        this.showMessageBox("Result", "The Commit Operation succeeded.", eNotificationType.notification);
        break;
      default:
        break;
    }
  }


  //entirely new channel
  async newStateChannelCallback(event) {
    this.channelUITransaction(event.channel, gridElemUpdateMode.add);
  }
  newTTCallback(event) {

  }

  //Fired when a new token-pool was generated.
  async newTokenPoolCallback(event) {


    //Local Variables - BEGIN
    let tp = event.pool;
    if (!tp)
      return;
    let awaitedTemplate = this.getAwaitingTPTemplate;
    let cmd = '';
    let reqID = null;
    let msg = null;
    let tpB64 = null;
    //Local Variables - END
    let qr = this.mVMContext.getRecentQRIntent;
    if (awaitedTemplate) {
      if (qr)
        qr.hide(true);
      //we've been awaiting a token pool, thus it needs to match the a-priori defined template.

      if (!gTools.compareByteVectors(tp.getID(), awaitedTemplate.getID()) || tp.getDimensionsCount() != awaitedTemplate.getDimensionsCount() || tp.getTotalValue() != awaitedTemplate.getTotalValue()) //[todo:CodesInChaos:medium]: improve verification?
      {
        console.log('The generated TP is invalid. Aborting.');
        this.setAwaitingTPTemplate = null;
      }

      //Register Token Pool on-the-chain - BEGIN

      //[todo:PauliX:high]: make this happen on the chain.

      //[todo:Mike:high]: notify user data being prepared
      tpB64 = gTools.encodeBase64Check(tp.getPackedData());


      //-------------Direct On-The-Chain support - BEGIN
      cmd = 'data64 ' + tpB64 + ' regPoolEx';
      if (this.mImmediateActionPending)
        cmd += " ct"; //we were to commit the Token Pool immediatedly
      let metaGen = new CVMMetaGenerator();
      reqID = metaGen.addRAWGridScriptCmd(cmd, eVMMetaCodeExecutionMode.GUI, this.mID);

      this.revokeCurrentAction();

      if (!this.mVMContext.processVMMetaData(metaGen, this)) {
        return false;
      } else {
        this.addVMMetaRequestID(reqID); //success await result then
      }
      //-------------Direct On-The-Chain support - END

      //Register Token Pool on-the-chain - END

    }
  }

  async newTransitPoolCallback(event) {

  }


  saveWalletID() {
    this.mDomainID = $(this.mDiv).find('#walletAddressTxt')[0].value;
    this.mVMContext.getLocalDataStore.saveValue('walletAddress', this.mDomainID);
    this.loadLocalIdentityData();
    gTools.showMsgAtControl('#' + this.mID + ' #walletAddressTxt', 'Saved!');
    this.retrieveBalance(true);
    this.updateStateChannels(true);
  }

  finishResize(isFallbackEvent) { //Overloaded window-resize Event
    super.finishResize(isFallbackEvent);
  }

  //attempt to deliver Transmission Token to peer.
  //might attempt to use the current Swarm the user is connected to.
  sendTTToTarget(token, useActiveSwarm = true, useWebTorrent = false, swarmID = null) {
    if (token == null)
      return false;
    if (!useActiveSwarm && !useWebTorrent)
      return false;

    if (token.getRecipient == null || token.getRecipient.byteLength == 0)
      return false; //supports only targeted Transmission Tokens otherwise everyone in swarm would receive it!

    if (useActiveSwarm && useWebTorrent)
      return false;

    let serializedToken = token.getPackedData();
    if (serializedToken == null || serializedToken.byteLength == 0)
      return false;

    if (useSwarm) {
      if (this.mVMContext.getSwarmsManager.getSwarmsCount == 0)
        return false;

      return this.mVMContext.getSwarmsManager.sendData(serializedToken, token.getRecipient, swarmID)


    } else { //web-torrent

    }
  }

  useTP(doIt = true) {
    if (doIt) {
      $(this.getBody).find('#sendTPSelectorContainer').addClass('shown animate__fadeIn');
    } else {
      $(this.getBody).find('#sendTPSelectorContainer').removeClass('shown animate__fadeIn');
    }
  }



  getRowNodeId(data) {
    return data.id;
  }

  //Data-Grid modifications  - Begin

  //Either in-bound or Outbound data-grid is udpated based on the channel's 'direction'-parameter -getIsOutgress()
  //Events are fired per-channel, thus the function supports singulary/unitary per channel updates.
  //That is quite allright since the updates are executed asynchronously in UI anyway thus the main overhead stems from the
  //number of function's invocations.
  channelUITransaction(channel, mode = gridElemUpdateMode.add) {

    if (channel == null)
      return false;
    if (!(channel instanceof CStateLessChannel))
      return false;

    //local variables - BEGIN
    let elem = null;
    let fid = channel.getFriendlyID; //todo: add column
    let tp = channel.getTokenPool;
    //local variables - END

    let elements = [];
    //operational logic follows
    let time = gTools.getTime();
    let idB58 = gTools.encodeBase58Check(channel.getID);
    let stv = BigInt(tp.getSingleTokenValue());
    if (channel.getIsOutgress) {

      elem = {
        id: gTools.arrayBufferToString(idB58), //needs to be comparable i.e. not object
        status: tp != null ? tp.getStatus() : 'unknown',
        friendlyID: fid,
        TPID: gTools.arrayBufferToString(idB58),
        value: tp != null ? (tp.getTotalValue()) : 'unknown',
        tokensLeft: tp != null ? tp.getTokensLeft() : 'unknown',
        totalTokens: tp != null ? tp.getTokensCount : 'unknown',
        banksCount: tp != null ? tp.getDimensionDepth() : 'unknown',
        tokenValue: tp != null ? (stv> 1000000000000000000n ?(this.mTools.numberToString(this.mTools.attoToGNC(stv)) + ' GNC') : (this.mTools.numberToString(stv) + ' Atto') ) : 'unknown',
        valueLeft: tp != null ? (this.mTools.attoToGNC(tp.getValueLeft())+ ' GNC') : 'unknown',
        usSA: channel.getLastTimeUsed(),
        uSA: channel.getLastTimeUpdated(),
        cSA: channel.getLastTimeCashedOut(),
        info: tp != null ? tp.getInfo() : '', //additional textutal information containing per-bank info.

      }
    } else {


      elem = {
        id: gTools.arrayBufferToString(idB58), //needs to be comparable i.e. not object
        friendlyID: fid,
        tokensLeft: tp != null ? tp.getTokensLeft() : 'unknown',
        totalTokens: tp != null ? tp.getTokensCount : 'unknown',
        peer: gTools.arrayBufferToString(channel.getOwnerID),
        lastUpdate: (gTools.getTime() - channel.getLastUpdate),
        TPID: gTools.arrayBufferToString(idB58),
        valueLeft: tp != null ? (this.mTools.attoToGNC(tp.getValueLeft())+ ' GNC') : 'unknown',
        tokenValue: tp != null ? (stv> 1000000000000000000n ?(this.mTools.numberToString(this.mTools.attoToGNC(stv)) + ' GNC') : (this.mTools.numberToString(stv) + ' Atto') ) : 'unknown',
        value: channel.getValue(),
        uSA: channel.getLastTimeUpdated(),
        cSA: time - channel.getLastTimeCashedOut(),

      }

    }

    elements.push(elem);

    //formulate GRID transaction - BEGIN
    let transaction = {
      add: [],
      update: [],
      remove: []
    };

    switch (mode) {
      case gridElemUpdateMode.add:
        transaction.add = elements;
        break;
      case gridElemUpdateMode.update:
        transaction.update = elements;
        break;
      case gridElemUpdateMode.remove:
        transaction.remove = elements;
        break;
      default:

    }

    //formulate GRID transaction - END

    //Commit transaction to UI
    //due to possibly many invocations per sec we update the grid asynchronously

    let gridDebugInfoResultCallback = function() {
      console.log('grid updated.');
    };

    if (!channel.getIsOutgress) {
      this.mIngressChannelsGrid.gridOptions.api.applyTransactionAsync(transaction, gridDebugInfoResultCallback);
    } else {
      this.mOutgressChannelsGrid.gridOptions.api.applyTransactionAsync(transaction, gridDebugInfoResultCallback);
    }
    //  let res = gridOptions.api.applyTransaction({ update: toUpdate });
    //this.gridDebugInfo(res);
    return true;
  }

  gridDebugInfo(res) {
    console.log('---------------------------------------');
    if (res.add) {
      res.add.forEach(function(rowNode) {
        console.log('Added Row Node', rowNode);
      });
    }
    if (res.remove) {
      res.remove.forEach(function(rowNode) {
        console.log('Removed Row Node', rowNode);
      });
    }
    if (res.update) {
      res.update.forEach(function(rowNode) {
        console.log('Updated Row Node', rowNode);
      });
    }
  }

  updateChannelInUI(channel) {
    {

    }
  }

  //Data-Grid modifications  - End
  refreshChannels() {
    let channelIDs = this.mVMContext.getChannelsManager.getChannelIDs(eChannelDirection.outgress); // we want only channels owned by user


    this.clearComboBox(this.mTokenPoolsSendTabCB);
    this.clearComboBox(this.mTokenPoolsPoolsTabCB);

    if (channelIDs.length > 0) {

      for (let i = 0; i < channelIDs.length; i++) {
        let id = gTools.arrayBufferToString(channelIDs[i]);

        this.addComboBoxElement(this.mTokenPoolsSendTabCB, id);
      }
    } else {
      this.addComboBoxElement(this.mTokenPoolsSendTabCB, 'no available', true);
    }
  }

  refreshSwarms() {
    let swarmIDs = this.mVMContext.getSwarmsManager.getSwarmIDs;

    this.clearComboBox(this.mSwarmsSendTabCB);
    if (swarmIDs.length > 0) {


      for (let i = 0; i < swarmIDs.length; i++) {
        let id = gTools.arrayBufferToString(swarmIDs[i]);

        this.addComboBoxElement(this.mSwarmsSendTabCB, id);

      }
    } else {
      this.addComboBoxElement(this.mSwarmsSendTabCB, 'no available', true);
    }
  }
  useChannel(id) {
    if (id == 'no available')
      return;
    this.mChannelIDToUse = id;
  }

  get getChannelIDToUse() {
    return this.mChannelIDToUse;
  }
  useSwarm(id) {
    if (id == 'no available')
      return;
    this.mSwarmIDToUse = id;
  }

  get getSwarmIDToUse() {
    return this.mSwarmIDToUse;
  }
  doUseSwarm(doIt = true) {
    if (doIt) {
      $(this.getBody).find('#sendSwarmSelectorContainer').addClass('shown animate__fadeIn');


      this.refreshSwarms();
    } else {
      $(this.getBody).find('#sendSwarmSelectorContainer').removeClass('shown animate__fadeIn');
    }
  }
  showTPInfo(id) {
    $(this.getBody).find('#tokenPoolInfoContainer').addClass('shown animate__fadeIn');
  }
  cashOutSelected() {
    let count = 0;
    let selectedRows = this.mIngressChannelsGrid.gridOptions.api.getSelectedRows();
    this.showMessageBox("Cash-Out", "Cashing out " + selectedRows.length + " Tokens!", eNotificationType.notification);
  }

  cashOutEverything() {
    let count = 0;
    //let selectedRows = this.mIngressChannelsGrid.gridOptions.api.getDisplayedRowCount();//todo replace with actial data-source info
    this.showMessageBox("Cash-Out", "Cashing out " + ingressRowData.length + " Tokens!", eNotificationType.notification);
    //operational logic
  }
  closeWindow() {
    if (this.mControler > 0)
      clearInterval(this.mControler); //shut-down the thread if active
    super.closeWindow();
  }

  open() { //Overloaded Window-Opening Event
    this.mContentReady = false;
    super.open();
    this.initialize();

    this.mFieldValidator.makeControlsVerifiable('.txtInput');

    let tHistory = this.getControl('transfersHistory');

    if (tHistory != null) {
      let tHistoryTable = document.createElement('table');
      tHistoryTable.style.width = '100%';
      tHistoryTable.style.height = '100%';
      tHistoryTable.setAttribute("id", "tHistoryTableT")
      tHistory.appendChild(tHistoryTable);
      //first try to fetch wallet address from system's credentials-store
      let sd = this.mVMContext.getUserSessionDescription;

      if (sd != null) {
        this.mDomainID = gTools.arrayBufferToString(sd.getAddress);
      }

      //if still empty then request qr-logon explicitly
      if (this.mDomainID == null || this.mDomainID.length == 0) {
        this.mVMContext.requestQRLogon();
      }

      //load local data
      this.loadLocalIdentityData();

      this.retrieveBalance();
      this.updateStateChannels();

      //
      if (CWallet.prototype.xInstance && this.mDomainID.length) {
        setTimeout(this.updateKnownChannels.bind(this), 2000);
        //
      }



      // Add slideDown animation to Bootstrap dropdown when expanding.
      $(this.getBody).find('.dropdown').on('show.bs.dropdown', function() {
        $(this.getBody).find('.dropdown-menu').first().stop(true, true).slideDown();
      });

      // Add slideUp animation to Bootstrap dropdown when collapsing.
      $(this.getBody).find('.dropdown').on('hide.bs.dropdown', function() {
        $(this.getBody).find('.dropdown-menu').first().stop(true, true).slideUp();
      });

      // setup the grid after the page has finished loading
      var ingressChannelsDiv = $(this.getBody).find('#ingressGrid')[0];
      var outgressChannelsDiv = $(this.getBody).find('#outgressGrid')[0];
      this.mIngressChannelsGrid = new agGrid.Grid(ingressChannelsDiv, ingressChannelsGridOptions);
      this.mIngressChannelsGrid.gridOptions.getRowNodeId = this.getRowNodeId;
      this.mOutgressChannelsGrid = new agGrid.Grid(outgressChannelsDiv, outgressChannelsGridOptions);
      this.mOutgressChannelsGrid.gridOptions.getRowNodeId = this.getRowNodeId;

      $(this.getBody).find('#availableTokenPoolsDrop2').editableSelect({
        effects: 'fade'
      });

      $(this.getBody).find('#availableTokenPoolsDrop1').editableSelect({
        effects: 'fade'
      });

      $(this.getBody).find('#fSwarmsAvailableDrop').editableSelect({
        effects: 'fade'
      });

      this.mTokenPoolsSendTabCB = $(this.getBody).find('#availableTokenPoolsContainerTPTab .es-list'); //send tab
      this.mTokenPoolsPoolsTabCB = $(this.getBody).find('#tokenPoolsTabContainer .es-list'); //token-pools tab
      this.mSwarmsSendTabCB = $(this.getBody).find('#sendSwarmSelectorContainer .es-list');
      CWallet.prototype.xInstance = true;
    }
  }
  updateKnownChannels() {
    this.mKnownChannels = this.mVMContext.getChannelsManager.findChannelsByOwner(this.mDomainID);
    for (let i = 0; i < this.mKnownChannels.length; i++) {
      this.channelUITransaction(this.mKnownChannels[i], gridElemUpdateMode.add);
    }

  }

  getRecipientID() {
    this.mRecipientID = this.getStringFromControl('recipientIDTxt');
    return this.mRecipientID;

  }
  getERGLimit() {
    this.mERGLimit = this.getNumberFromControl('ergLimitTxt');
    return this.mERGLimit;
  }
  getERGBid() {
    this.mERGBig = this.getNumberFromControl('ergBIDTxt');
    return this.mERGBig;
  }
  getValue() {
    this.mValue = this.getNumberFromControl('valueTxt'); //this.getControl('#valueTxt').value;
    return this.mValue;
  }

  get getTotalPendingOutgressTransfer() {
    return BigInt(this.mPendingTotalValueTransfer);
  }

  incTotalPendingOutgressTransfer(value) {
    this.mPendingTotalValueTransfer += BigInt(value);
  }

  decTotalPendingOutgressTransfer(value) {
    if (this.mPendingTotalValueTransfer >= value) {
      this.mPendingTotalValueTransfer -= BigInt(value);
    } else {
      this.mPendingTotalValueTransfer = 0;
    }
  }

  getPendingNewTPValue() {
    let value = this.getNumberFromControl('pendingTPTotalValueTxt');
    if (value) {
      return value;
    }
    return 0;
  }

  getPendingNewTPDimensions() {
    let value = this.getNumberFromControl('pendingTPBanksCountTxt');
    if (value) {
      return value;
    }
    return 0;
  }

  getPendingNewTPTokenValue() {
    let value = this.getNumberFromControl('singleTokenValueTxt');
    if (value) {
      return value;
    }
    return 0;
  }

  resetFields() {
    this.clearControl('valueTxt');
    this.clearControl('ergBIDTxt');
    this.clearControl('ergLimitTxt');
    this.clearControl('recipientIDTxt');

    //new TP fields - BEGIN
    this.clearControl('pendingTPTotalValueTxt');
    this.clearControl('pendingTPBanksCountTxt');
    this.clearControl('singleTokenValueTxt');
    //new TP fields - END
  }

  setDefaultsInUI() {
    this.setValue('ergBIDTxt', 1);
    this.setValue('ergLimitTxt', 1000);
  }

  set setAwaitingTPTemplate(template) {
    this.mTPTemplate = template;
  }

  get getAwaitingTPTemplate() {
    return this.mTPTemplate;
  }

  revokeCurrentAction() {
    this.mImmediateActionPending = false;
  }

  createTP(immediateOne) {
    this.revokeCurrentAction();
    let ephKeyPair = this.mVMContext.getKeyPair;
    if (this.getAwaitingTPTemplate) {
      console.log('Warning:' + ' cancelling current TP generation request.')
      this.setAwaitingTPTemplate = null; //clear the current one.
    }

    //Local Variables - BEGIN
    let cmd = '';
    let pendingTPValue = this.getPendingNewTPValue();
    let pendingTPBanks = this.getPendingNewTPDimensions();
    let pendingTPTValue = this.getPendingNewTPTokenValue();
    //Local Variables - END

    //Data Validation - Begin
    if (!pendingTPValue || !pendingTPBanks || !pendingTPTValue) { //this shouldn't happen as controls have dynamic validators of their own.
      this.showMessageBox("Invalid Token Pool details!", this.mErrorMsg, eNotificationType.error);
    }
    //Data Validation - End

    //WARNING ** Operational Logic - BEGIN **
    console.log('Initiating Token-Pool generation process.');
    //------------------- Immediate TP Deployment - BEGIN
    this.mImmediateActionPending = immediateOne;
    //first create a token pool on mobile

    //create a token pool request and save the ID of it locally - BEGIN
    let tokenPool = new CTokenPool(null, 100, gTools.base58CheckEncode(gTools.getRandomVector(15)), new ArrayBuffer(), 1, 10000, 0, "newPool");
    this.setAwaitingTPTemplate = tokenPool; //we'll verify the incomming token-pool to match what was expected i.e. ordered beforehand
    //create a token pool request and save the id of it locally - END

    //route to websocket conversation's ID, instead of our IP (web-peer might be behind a NAT)
    let qr = new CQRIntent(eQRIntentType.QRProcessVMMetaData, this.mVMContext.getConversationID, this.mVMContext.getFullNodeIP, eEndpointType.WebSockConversation);

    qr.setPubKey(ephKeyPair.public);

    let meta = new CVMMetaGenerator();
    meta.addGenTokenPoolRequest(tokenPool);
    let packedMeta = meta.getPackedData();
    qr.setData(packedMeta);
    qr.show(); //let the mobile app take it from here

    //------------------- Immediate TP Deployment - END

    //WARNING ** Operational Logic - END **
  }
  //Issues a #crypto transfer based on the in-UI selected criterias and parameters.
  issueTransfer(immediateOne) {
    this.revokeCurrentAction();
    //Local Variables - BEGIN
    let recipientID = this.getRecipientID();
    let value = this.getValue();
    let ergLimit = 10000; //this.getERGLimit();
    let swarm = null;
    let reqID = null;
    let msg = null;
    let task = null;
    let channel = null;
    let cmd = '';
    let doSwarm = false,
      doOffTheChain = false;
    let tt; //'exotic' options
    this.mChannelIDToUse = this.getComboBoxValue(this.mTokenPoolsSendTabCB);
    this.mSwarmIDToUse = this.getComboBoxValue(this.mSwarmsSendTabCB);
    //Local Variables - END

    //Data Verification - BEGIN
    if (recipientID.length == 0 || value <= 0) { //might be a friendly ID thus don't do any validation
      this.mErrorMsg = "Invalid data provided 🤡";
      this.showMessageBox("Result", this.mErrorMsg, eNotificationType.error);
      return false;
    }

    if ((value + (ergLimit)) > this.mBalance) {
      this.mErrorMsg = "Insuficient balance 👾";
      this.showMessageBox("Result", this.mErrorMsg, eNotificationType.error);
      return false;
    }
    //-------------Swarm Support - BEGIN
    if (this.mSwarmIDToUse.length > 0) {
      swarm = this.mVMContext.getSwarmsManager.findSwarmByID(this.mSwarmIDToUse);
      if (swarm == null) {
        this.showMessageBox("The requested Swarm is unavailable 🙁", this.mErrorMsg, eNotificationType.error);
        return;
      } else {
        doSwarm = true;
      }
    }
    //-------------Swarm Support - END

    //-------------State-Less channels support - BEGIN
    if (this.mChannelIDToUse.length > 0) {
      channel = this.mVMContext.getChannelsManager.findChannelByID(this.mChannelIDToUse);

      if (channel == null) {
        this.showMessageBox("Error", "The requested Token-Pool is unavailable 🙁", eNotificationType.error);
        return;
      } else {
        tt = channel.getTTForPeer(value, recipientID);
        if (tt == null)
          this.showMessageBox("Error", "I was unable to generate a Transmission Token 🙁", eNotificationType.error);
        return;
        //after token-pool verification
        doOffTheChain = true;
      }
    }
    //-------------State-Less channels support - END

    //Data Verification - END

    //WARNING ** Operational Logic - BEGIN **

    //-------------State-Less channels support - BEGIN
    if (doOffTheChain) {

      //If we want to deliver token through Swarm-mechanis, proceed
      //-------------Swarm Support - BEGIN
      if (doSwarm) {

      }
      //-------------Swarm Support - END

    }
    //-------------State-Less channels support - END
    else {
      //-------------Direct On-The-Chain support - BEGIN

      if (immediateOne && !this.mVMContext.tryLockCommit(immediateOne ? this : null)) //lock only if the process is to be carried out right now
      {
        this.showMessageBox("Result", "Another commit is pending.", eNotificationType.error);
        return;
      }

      cmd = "bt send " + recipientID + " " + this.mTools.numberToString(value) + (this.mDoGNC ? ' -gnc ' : '') + "\r rt"; //make transfer and mark the thread as ready right away

      //if (immediateOne) //add and finalize with commit instruction only if the process is to be carried out right now
      //  cmd += " ct"
      //As discussed with TheOldWizard on 07.08.21 => user mode UI application MAY  execute Commit *directly* from #GridScript (which is to prevent race conditions) and to allow current
      //app to fully control the VM should it will to.
      //In any case they MUST  first call tryLockCommit() to indicate an intention to issue a commit, check for the result returned and precceed appropriately.
      //The VM (running on full-node) MUST confirm the Commit operation before a timeout (specified within VM Context), otherwise the commit-lock would be autoamtically broken.
      //In ANY CASE the isCommiting flag of the VM Context can be set ONLY by the remote VM running on the full-node.
      //ONLY kernel-mode UI dApps are allowed to break any pending Commit-Locks by providing the second parameter equal to TRUE.


      let thread = this.getThreadByID(this.getThreadID);
      this.incTotalPendingOutgressTransfer(value);
      this.mVMContext.incTotalPendingOutgressTransfer(value);
      // /  processGridScript(cmd, threadID = new ArrayBuffer(), processHandle = null, mode = eVMMetaCodeExecutionMode.RAW, reqID = 0) {
      if (!this.mVMContext.processGridScript(cmd, this.getThreadID, this)) {
        this.showMessageBox("Ooops", "Unable to process right now.", eNotificationType.error);
        return;
      }
      /*
            let metaGen = new CVMMetaGenerator();

            this.mImmediateActionPending = true;
            reqID = metaGen.addRAWGridScriptCmd(cmd, eVMMetaCodeExecutionMode.GUI, this.mID);

            if (!this.mVMContext.processVMMetaData(metaGen, this)) {
              this.mErrorMsg = "Unable to process at this time.";
              this.showMessageBox("Result", this.mErrorMsg, eNotificationType.error);
              this.mImmediateActionPending = false;
              return false;
            } else {
              this.addVMMetaRequestID(reqID); //success await result then
            }
            */
      //this.mVMContext.commit(); <= no need for that it's within source code already
      //-------------Direct On-The-Chain support - END
    }

    //WARNING ** Operational Logic - END **

    //Refresh UI - BEGIN

    if (!immediateOne) {
      task = new CConsensusTask('Value Transfer (' + value + ' ' + this.getDenomination() + ' to ' + recipientID + ')');
      this.mVMContext.addConsensusTask(task);
      this.mErrorMsg = "Enqued.";
      this.showMessageBox("Result", "Operation enqued. Use Magic Button to Commit.", eNotificationType.notification);
      this.mErrorMsg = "Awaiting result..";
    } else {
      task = new CConsensusTask('Value Transfer (' + value + ' ' + this.getDenomination() + ' to ' + recipientID + ')');
      this.mVMContext.addConsensusTask(task);
      this.mVMContext.getMagicButton.commitActions();
    }

    this.resetFields();
    this.setDefaultsInUI();
    this.refreshBalanceUI();
    this.mVMContext.playSound(eSound.sent);

    //Refresh UI - END
    return true;
  }

  getDenomination() {
    if (this.mDoGNC) {
      return 'GNC';
    } else {
      return 'Atto';
    }
  }
  //The functions loads local identity-related data and updates the UI.
  loadLocalIdentityData() {
    //if domain-id already discovered then do not attempt to load it  from local storage

    //Get the ID - BEGIN
    let addr = this.mDomainID.length > 0 ? this.mDomainID : this.mVMContext.getLocalDataStore.loadValue('walletAddress');
    //Get the ID - END


    //Get other ID-related data - BEGIN

    //Get other ID-related data - END


    if (addr != null) {
      //Update the UI - Begin

      //Update displayed domain ID - BEGIN
      //&& gTools.isDomainIDValid(addr)) {
      this.getControl('walletAddressTxt').value = addr;
      this.getControl('addressTxt').value = addr;
      this.mDomainID = gTools.arrayBufferToString(addr);

      //Update displayed domain ID - END

      //Update other data in UI - BEGIN
      //Update other data in UI - END

      //Update the UI - END
    }
  }

  retrieveBalance(forceIt = false) {

    if (!this.mDomainID || this.mDomainID.length <= 4) //|| !gTools.isDomainIDValid(this.mDomainID)) Note:user might be using Friendly ID
      return;

    let now = gTools.getTime(true);
    //Security - Begin
    //here we throttle the maxim frequency of balance updates. Note that full-node does the same.
    //Should we attempt to exceed the maximum code-execution interval as specified at full-nodes the connection would be  dropped,
    //with the IP address possibly getting banned.
    if ((now - this.mLastBalanceRefreshAttempt) < 2000) {
      console.log('Excessive balance-update requests.');
      return;
    }
    //Security - END

    if (!forceIt && (now - this.mLastBalanceRefreshAttempt) < this.mBalanceRefreshInterval)
      return false;

    this.mLastBalanceRefreshAttempt = now;
    let balanceTxt = $(this.getBody).find('#currentBalanceTxt')[0];
    balanceTxt.innerHTML = '<img class ="animate__animated animate__lightSpeedInLeft" src="/images/wait.gif"/>';
    this.addDFSRequestID(this.mVMContext.getFileSystem.doCD('/' + this.mDomainID, false).getReqID);
    this.addDFSRequestID(this.mVMContext.getFileSystem.doSync(false, this.sBalanceUpdateThreadID).getReqID);
    this.addDFSRequestID(this.mVMContext.getFileSystem.doGetFile('GNC').getReqID);
  }

  updateStateChannels(forceIt = false) {

    if (this.mDomainID == null || this.mDomainID.length <= 4)
      return;

    let now = gTools.getTime(true);

    if (!forceIt && (now - this.mLastChannelsRefreshAttempt) < this.mChannelsRefreshInterval) //provisioned under additional Channel manager's limits
      return false;

    this.mLastChannelsRefreshAttempt = now;

    this.mVMContext.getChannelsManager.syncAgentChannels(this.mDomainID);

  }
  async newGridScriptResultCallback(result) {
    if (result == null)
      return;

    if (!this.hasVMMetaRequestID(result.reqID)) { //the vm-operation indeed was triggered by our application

      //was is the 'transaction'? if so notify about the result
      if (result.reqID == this.mRecentTransactionID)
        this.notifyResult(result, this.mImmediateActionPending);
    }


    this.retrieveBalance();
    this.updateStateChannels();
  }

  notifyResult(result, immediateOne = false) {
    if (result.status == eVMMetaProcessingResult.success) { //showNotification(caption,text,notificationType, fullScreen=true;windowID)
      let text = immediateOne ? "Processing of an immediate transfer succeeded!" : "Transfer has been successfully unqued at full-node.\n Commit once ready.";
      this.showMessageBox("Result", text, eNotificationType.notification);


    } else {
      this.showMessageBox("Result", "Transfer Failed!", eNotificationType.notification);
    }

  }
  async newDFSMsgCallback(dfsMsg) {

    if (!this.hasDFSRequestID(dfsMsg.getReqID))
      return;

    if (dfsMsg.getType == eDFSCmdType.error) {
      let errorMsg = gTools.arrayBufferToString(dfsMsg.getData1);
      return;
    }


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
              let path = dataFields[1];
              let fileName = gTools.parsePath(path).fileName;
              if (fileName != "GNC")
                return;
              let data = dataFields[2];

              switch (dataType) {
                case eDataType.bytes:
                  break;

                case eDataType.signedInteger:
                  data = gTools.arrayBufferToNumber(data);
                  break;

                case eDataType.BigInt:
                  this.mBalance = gTools.arrayBufferToBigInt(data);
                  this.refreshBalanceUI();
                  break;
                case eDataType.unsignedInteger:
                  //Note: Conversation to UNSIDGNED number shall be employed below:
                  data = gTools.arrayBufferToUNumber(data); //here will be the GNC value
                  this.mBalance = data;
                  this.refreshBalanceUI();
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
  refreshBalanceUI() {
    if (!gTools.isNumber(this.mBalance))
      return;

    let balanceTxt = $(this.getBody).find('#currentBalanceTxt')[0];
    balanceTxt.innerHTML = '<i class="fal fa-money "></i> ' + gTools.attoToGNCStr(BigInt(this.mBalance) - BigInt(this.mVMContext.getTotalPendingOutgressTransfer)) + ' GNC';
    this.mBalanceRefreshInterval = 60000; //once the balance has been updated at least one; lower the update-interval
  }
  async newVMMetaDataCallback(meta) { //callback called when new Meta-Data made available from full-node
    //this will contain the result of our #Crypto transfer
    if (!this.hasVMMetaRequestID(meta.getReqID))
      return;
    if (meta.getData.byteLength > 0) {
      let metaData = this.mMetaParser.parse(dfsMsg.getData1);
    }
  }
}
export default CWallet;
