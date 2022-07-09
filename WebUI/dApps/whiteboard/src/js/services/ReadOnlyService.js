import ConfigService from "./ConfigService.js";

/**
 * Class the handle the read-only logic
 */
class ReadOnlyService {


  constructor(window) {
    this.mWindow = window;
    this.mIsReadOnly = false;
    this.mReadOnlyActive = true;
    this.mPreviousToolHtmlElem = null;
  }

  get getWindow() {
    return this.mWindow;
  }
  set setWindow(w) {
    this.mWindow = w;
  }

  /**
   * @type {boolean}
   */

  get readOnlyActive() {
    return this.mReadOnlyActive;
  }

  /**
   * @type {object}
   */

  get previousToolHtmlElem() {
    return this.mPreviousToolHtmlElem;
  }

  /**
   * Activate read-only mode
   */
  activateReadOnlyMode() {
    this.mReadOnlyActive = true;

    this.mPreviousToolHtmlElem = $(".whiteboard-tool.active");

    // switch to mouse tool to prevent the use of the
    // other tools
    $(this.mWindow.getBody).find(".whiteboard-tool[tool=mouse]").click();
    $(this.mWindow.getBody).find(".whiteboard-tool").prop("disabled", true);
    $(this.mWindow.getBody).find(".whiteboard-edit-group > button").prop("disabled", true);
    $(this.mWindow.getBody).find(".whiteboard-edit-group").addClass("group-disabled");
    $(this.mWindow.getBody).find("#whiteboardUnlockBtn").hide();
    $(this.mWindow.getBody).find("#whiteboardLockBtn").show();
  }

  /**
   * Deactivate read-only mode
   */
  deactivateReadOnlyMode() {
    if (this.mIsReadOnly)
    return;

    this.mReadOnlyActive = false;

    $(this.mWindow.getBody).find(".whiteboard-tool").prop("disabled", false);
    $(this.mWindow.getBody).find(".whiteboard-edit-group > button").prop("disabled", false);
    $(this.mWindow.getBody).find(".whiteboard-edit-group").removeClass("group-disabled");
    $(this.mWindow.getBody).find("#whiteboardUnlockBtn").show();
    $(this.mWindow.getBody).find("#whiteboardLockBtn").hide();

    // restore previously selected tool
    const {
      previousToolHtmlElem
    } = this;
    if (previousToolHtmlElem) previousToolHtmlElem.click();
  }
}

export default ReadOnlyService;
