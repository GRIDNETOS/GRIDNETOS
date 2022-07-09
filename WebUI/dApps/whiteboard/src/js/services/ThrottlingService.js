import Point from "../classes/Point.js";
import { getCurrentTimeMs } from "../utils.js";
import ConfigService from "./ConfigService.js";

/**
 * Class to handle all the throttling logic
 */
class ThrottlingService {


  constructor (window, configService)
  {
    this.mConfigService=configService;
    this.mWindow = window;
    this.mLastPointPosition = new Point(0, 0);
    this.mLastSuccessTime = 0;
  }

  get getWindow()
  {
    return this.mWindow;
  }
  set setWindow(w)
  {
    this.mWindow = w;
  }

    /**
     * @type {number}
     */
    get lastSuccessTime() {
        return this.mLastSuccessTime;
    }

    /**
     * @type {Point}
     */

    get lastPointPosition() {
        return this.mLastPointPosition;
    }

    /**
     * Helper to throttle events based on the configuration.
     * Only if checks are ok, the onSuccess callback will be called.
     *
     * @param {Point} newPosition New point position to base the throttling on
     * @param {function()} onSuccess Callback called when the throttling is successful
     */
    throttle(newPosition, onSuccess) {
        const newTime = getCurrentTimeMs();
        const { lastPointPosition, lastSuccessTime } = this;
        if ((newTime - lastSuccessTime) > this.mConfigService.pointerEventsThrottling.minTimeDelta) {
            if (
                lastPointPosition.distTo(newPosition) >
                this.mConfigService.pointerEventsThrottling.minDistDelta
            ) {
                onSuccess();
                this.mLastPointPosition = newPosition;
                this.mLastSuccessTime = newTime;
            }
        }
    }
}

export default ThrottlingService;
