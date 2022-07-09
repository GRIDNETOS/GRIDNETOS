import { getThrottling } from "./ConfigService.utils.js";

/**
 * Class to hold the configuration sent by the backend
 */
class ConfigService {
    /**
     * @type {object}
     */

    constructor (window)
    {
      this.mWindow = window;
      this.mConfigFromServer =  {};
      this.mPointerEventsThrottling = { minDistDelta: 0, minTimeDelta: 20 };
      //do not spam others with a frequence higher than the one specified by above thresholds
      this.mIsReadOnly = false;
      this.mCorrespondingReadOnlyWid = '';
      this.mOnWhiteboardLoad = { setReadOnly: false, displayInfo: false };
      this.mShowSmallestScreenIndicator = true;
      this.mImageDownloadFormat = "png";
      this.mDrawBackgroundGrid = false;
      this.mBackgroundGridImage = "bg_grid.png";
      this.mRefreshInfoInterval = 1000;
    }

    get getWindow()
    {
      return this.mWindow;
    }
    set setWindow(w)
    {
      this.mWindow = w;
    }


    get configFromServer() {
        return this.mConfigFromServer;
    }

    /**
     * Associated read-only id for this whiteboad
     * @type {string}
     */
    get correspondingReadOnlyWid() {
        return this.mCorrespondingReadOnlyWid;
    }

    /**
     * @type {boolean}
     */
    get isReadOnly() {
        return this.mIsReadOnly;
    }

    /**
     * @type {{displayInfo: boolean, setReadOnly: boolean}}
     * @readonly
     */

    get readOnlyOnWhiteboardLoad() {
        return this.mOnWhiteboardLoad.setReadOnly;
    }
    get displayInfoOnWhiteboardLoad() {
        return this.mOnWhiteboardLoad.displayInfo;
    }

    /**
     * @type {boolean}
     */

    get showSmallestScreenIndicator() {
        return this.mShowSmallestScreenIndicator;
    }

    /**
     * @type {string}
     */

    get imageDownloadFormat() {
        return this.mImageDownloadFormat;
    }

    /**
     * @type {boolean}
     */

    get drawBackgroundGrid() {
        return this.mDrawBackgroundGrid;
    }

    /**
     * @type {string}
     */

    get backgroundGridImage() {
        return this.mBackgroundGridImage;
    }

    /**
     * @type {{minDistDelta: number, minTimeDelta: number}}
     */
    get pointerEventsThrottling() {
        return this.mPointerEventsThrottling;
    }

    /**
     * @type {number}
     */

    get refreshInfoInterval() {
        return this.mRefreshInfoInterval;
    }

    /**
     * Init the service from the config sent by the server
     *
     * @param {object} configFromServer
     */
    initFromServer(configFromServer) {
      /*  this.#configFromServer = configFromServer;

        const { common } = configFromServer;
        const {
            onWhiteboardLoad,
            showSmallestScreenIndicator,
            imageDownloadFormat,
            drawBackgroundGrid,
            backgroundGridImage,
            performance,
        } = common;
*/
      //  this.mOnWhiteboardLoad = onWhiteboardLoad;
      //  this.mShowSmallestScreenIndicator = showSmallestScreenIndicator;
        //this.mImageDownloadFormat = imageDownloadFormat;
      //  this.mDrawBackgroundGrid = drawBackgroundGrid;
      //  this.mBackgroundGridImage = backgroundGridImage;
      //  this.mRefreshInfoInterval = 1000 / performance.refreshInfoFreq;

      //  const { whiteboardSpecific } = configFromServer;
        //const { correspondingReadOnlyWid, isReadOnly } = whiteboardSpecific;

        //this.mCorrespondingReadOnlyWid = correspondingReadOnlyWid;
        //*/
        this.mIsReadOnly = false;

      //  console.log("Whiteboard config from server:", configFromServer, "parsed:", this);
    }

    /**
     * Refresh config that depends on the number of user connected to whiteboard
     *
     * @param {number} userCount
     */
    refreshUserCountDependant(userCount) {
        const { configFromServer } = this;
        const { common } = configFromServer;
        const { performance } = common;
        const { pointerEventsThrottling } = performance;

        this.mPointerEventsThrottling = getThrottling(pointerEventsThrottling, userCount);
    }
}

export default ConfigService;
