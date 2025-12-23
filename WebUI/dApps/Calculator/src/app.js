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
  CWindow
} from "/lib/window.js"


var calcBody = ``;

$.get("/dApps/Calculator/src/body.html", function(data) {
  calcBody = data;
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
class CCalculator extends CWindow {


  constructor(positionX, positionY, width, height) {
    super(positionX, positionY, width, height, calcBody, "Calculator", CCalculator.getIcon(), true); //use Shadow-DOM by default

    this.setThreadID = null; //by default there's no need for a dedicated Decentralized Processing Thread.
    //Developers should prefere usage of public 'data' thread for retreval of information instead.
    this.mTempDiv = document.createElement("div");
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
    this.mNumWasLast=false;

    //Init app items - BEGIN
    this.setIsMaxWindowEnabled = false;

    // Variables
      this.mViewer = this.el("#viewer"), // Calculator screen where result is displayed
      this.mEquals = this.el("#equals"), // Equal button
      this.mNums = this.el(".num"), // List of numbers
      this.mOps = this.el(".ops"), // List of operators
      this.mTheNum = "", // Current number
      this.mOldNum = "", // First number
      this.mResultNum = null, // Result
      this.mOperator = null; // Batman

    /* The click events */

    // Add click event to numbers
    for (var i = 0, l = this.mNums.length; i < l; i++) {
      this.mNums[i].onclick = this.setNum.bind(this);
    }

    // Add click event to operators
    for (var i = 0, l = this.mOps.length; i < l; i++) {
      this.mOps[i].onclick = this.moveNum.bind(this);
    }

    // Add click event to equal sign
    this.mEquals.onclick = this.displayNum.bind(this);

    // Add click event to clear button
    this.el("#clear").onclick = this.clearAll.bind(this);

    // Add click event to reset button
    this.el("#reset").onclick = function() {
      //  window.location = window.location;
    };
    //Init app items - END
  }

  // Shortcut to get elements
  el(element) {
    if (element.charAt(0) === "#") { // If passed an ID...
      return this.getBody.querySelector(element); // ... returns single element
    }

    return this.getBody.querySelectorAll(element); // Otherwise, returns a nodelist
  };
  static getDefaultCategory() {
    return 'productivity';
  }
getAttribute(at)
{
  return this.mTempDiv.getAttribute(at);
}

  // When: Number is clicked. Get the current number selected
  setNum(e) {
    this.mNumWasLast=true;

    if (this.mResultNum) { // If a result was displayed, reset number
      this.mTheNum = e.target.getAttribute("data-num");
      this.mResultNum = "";
    } else { // Otherwise, add digit to previous number (this is a string!)
      this.mTheNum += e.target.getAttribute("data-num");
    }

    this.mViewer.innerHTML = this.mTheNum; // Display current number

  }

  // When: Operator is clicked. Pass number to this.mOldNum and save this.mOperator
  moveNum(e) {
    if(this.mNumWasLast)
    {
      this.mOldNum = this.mTheNum;
      this.mTheNum = "";
      this.mEquals.setAttribute("data-result", ""); // Reset result in attr
    }

    this.mOperator = e.target.getAttribute("data-ops");

    this.mNumWasLast=false;
  }

  // When: this.mEquals is clicked. Calculate result
  displayNum() {

    // Convert string input to numbers

    if(this.mOldNum=='')
        this.mOldNum = '0';

    if(this.mTheNum=='')
    this.mTheNum = this.mOldNum;

    this.mOldNum = parseFloat(this.mOldNum);
    this.mTheNum = parseFloat(this.mTheNum);

    // Perform operation
    switch (this.mOperator) {
      case "plus":
        this.mResultNum = this.mOldNum + this.mTheNum;
        break;

      case "minus":
        this.mResultNum = this.mOldNum - this.mTheNum;
        break;

      case "times":
        this.mResultNum = this.mOldNum * this.mTheNum;
        break;

      case "divided by":
        this.mResultNum = this.mOldNum / this.mTheNum;
        break;

        // If equal is pressed without an this.mOperator, keep number and continue
      default:
        this.mResultNum = this.mTheNum;
    }

    // If NaN or Infinity returned
    if (!isFinite(this.mResultNum)) {
      if (isNaN(this.mResultNum)) { // If result is not a number; set off by, eg, double-clicking this.mOperators
        this.mResultNum = "You broke it!";
      } else { // If result is infinity, set off by dividing by zero
        this.mResultNum = "Look at what you've done";
        this.el('#calculator').classList.add("broken"); // Break calculator
        this.el('#reset').classList.add("show"); // And show reset button
      }
    }

    // Display result, finally!
    this.mViewer.innerHTML = this.mResultNum;
    this.mEquals.setAttribute("data-result", this.mResultNum);

    // Now reset this.mOldNum & keep result
    this.mOldNum = 0;
    this.mTheNum = this.mResultNum;
    this.mNumWasLast=true;

  }

  // When: Clear button is pressed. Clear everything
  clearAll() {
    this.mOldNum = "";
    this.mTheNum = "";
    this.mViewer.innerHTML = "0";
    this.mEquals.setAttribute("data-result", this.mResultNum);
  }


  static getIcon() {
    return `data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAgEAAAIBCAYAAADQ5mxhAABFS0lEQVR42u2dCXhU1cG/R1TqXgsBskz2ZGYSSAIJkDB3EoYtILtA2BMWERW1gCIwd1IHUVHrTvupFVfAWi3q59aKVtyqVlFr/7bWWqtSFUSrlvoBypLzPychFjAks8+9d97f87wPfvpVJ3fuPb839557js1GCCGEEEIIMW7sAxYfX1C51O7wLCtzuhsHOz3+OqfmP9vp1v0uTb9OcrdT0zc43P5H5d/fKP/+sw5Nf1n++bpD8/1Z/vXf5T/fIv/8VP75lWSnZN8B1F9/deCfqf+fv6v/Tcv/tvnf8az6d7b8u/UN6r+l/pvqv938GdRnkZ9JfTb1GdVn5RsjhBBCOkhxcaBzgbsxv9DTOFQW7jyH5r9cFu09Ts33tCzYN50e/SP5f++SCJOxq+Wzy5+h+WfR71E/m/oZ1c+qfmb1s3MGEEIIsXACnVxVy3Ncnkav0+2b49L8lzjd/rWyFJ8/UPD7TVjw0WL/gWPwvDomLcdGHiN5rNQxU8eO84cQQogp4vCuSGm5Va8vdLr122W5bTbpb/HGuZugjqE6ls3HtHGwOsacaYQQQhIWdRtbPf92eHz1Ts1/tSyoJ2RZbaW048bWlmPuv1p9B+q74NECIYSQmEQ9v3Z49AaXx3+rLKC3JHspYsOhvpO31Hekviv1nXHmEkIICSleb+AYh3t5P/lb5iI1E17+uY2CNSvN3536Dhep71R9t5zhhBBC/vtbfmXgFNcAfbgsi5WSTQdel6NArcnOA9/xSvWdq++eK4AQQpIqgU5Fmr+qeSa6R38lyWfnJzvq7YRX1LmgzgneRiCEEAum2HtRqkvzzXJ59HvlwP8F5QdH4At1jqhzRZ0zXDmEEGLCqGe/hW5/jUPTV8nf8t6Qg3sTBQch0qTOHXUOqXOJ+QSEEGLg5FUs+6H6DU4O2g/IAXwHJQZRZoc6t9Q5ps41rjhCCElwnNrSk52af4YcoB+RfEtRQZz4tuWcU+fe0pO5EgkhJE4prV1yokPzT3Fp+oNyIN5NIUGC2a3ORXVOqnOTK5QQQqIctSudy+2fKAfc+3mFDwz+CuL96lxlJ0VCCIkogU5Oz/IRDrfvl3Jg/ZqCAZPxdfO5K89hXj0khJAgo/ajd2i+i9W+9hQJWIQt6pxW5zZXOCGEHJ66uqOdmm+sw+1/VA6Y+ygNsCj7Ws5x31h1znPhE0KSOs6axlynR79MDo6fUBCQZHzSfO7La4CRgBCSNKmomH+s0+2b5ND0J1nEB0Bvar4W5DWhrg1GCEKIJePwrkhpedbv287AD9AWvu3qGlHXCiMGIcQSKappLJQD3M2SXQzyAEGhrpWb1bXDCEIIMedv/prf43TrD7FLH0AEuxzKa0hdS4wohBDjR83yd/smuTT9DwzgANGj+ZqS1xZvFRBCDBe1XKrLrZ8vB6t/MGADxJT31bXGEsWEkISnoDJwyoGFfb5kcAaIK1+2LEAUOIWRiBAS99/8nR59mRyIvmAwBkgoX6hrkTsDhJCYJ8cbOM6h6Yt5zQ/AiK8X6ovVNcpIRQiJaoqLA50dHv8CVvYDMP5KhOpaVdcsIxchJKJ4vYFj5G8X89jMB8CMmxbp89Q1zEhGCAkxgU4Oj69eDiTvMZgCmJr31LXMdsaEkKDi8jR6nZr/TQZPACvhf1Nd24xwhJA2o3Yzc2j6AwyWANal+Rpn50JCSGuKvYGT5MCwSg4Q3zBIAiQF36hrXl37jICEJG+Ocrn12U7Nv41BESApHxFsU2OAGgsYDglJplv/1T5NDgKbGQQBoHkskGMCIyMhVr/1ry3Lcnn0exn0AOB7mxTJsUGNEYyUhFgtdXVHuzTfEnmh72SwA4B22KnGCnYrJMQiKXTrveWF/RqDGwCEwGtq7GAEJcSkUWuIO936lfJi3suABgBhsFeNIexHQIjJ0rzgj1t/l0EMACJGjiUsNESIOX77P1VetGskTQxeABBF1JiyRo0xjLSEGPG3f803QV6kWxmsACCGbFVjDSMuIQZJsfeiVJemP8jgBABxe51Qjjlq7GEEJiSBcWq+sfKC/JxBCQASwOdqDGIkJiTOqahYcYK8AG9hEAIAA3CLGpMYmQmJQ4qq9XJ50b3DwAMABuIdNTYxQhMSswQ6Od2+pfJi28OAAwAGZI8ao9RYxXhNSBRTULnULi+wTQwyAGACNqkxi5GbkCjE6fHXyYvqSwYWADARX6qxixGckDBT7A2c5ND0OxlMAMCsqDFMjWWM6ISE8tu/1uh0evS3GUQAwPQ0j2WNTkZ2QoKIo9o3Xl44Oxg8AMBC7FBjGyM8IUdMoJND01ex7j8AWHX/ATXG8fYAIYfF1d/X1an5NzJIAID18W9UYx4jPyG27xb/+ZCBAQCSiA9ZXIhwB8Ctz5YXw24GBABIQnarMZAmIEmX4uJAZ3kB3MQgAACg36TGRJqBJEV6Veo95En/Ehc+AMB3vKTGRhqCWDqFVY1F8mT/gAseAOB7fKDGSJqCWDLyBB8k+YoLHQDgiKgxchCNQSwVh0dvcLL7HwBAMOxRYybNQSwRl0dfwUUNABAaauykQYhp0/wGgNu/losZACBM5BjKmwPEdMnxBk51ar5nuIgBACLF94waU2kWYo4JgDWNuewACAAQRdSYKsdWGoYY/Pn/8gpprdu5aAEAon5HYLsaY2kaYsg4NL/HyRbAAACxZIcaa2kcYqgUafoweXLu5AIFAIg5O9WYS/MQY8wBcPvHyZPyGy5MAIC48Y0ae2kgkuA5AP5p8mTcywUJABB39qoxmCYiCZoDoM+TJ+F+LkQAgISxX43FNBKJ7yMAzb9InnxNXIAAAAmnSY3JNBOJjwB4fI1cdAAARltLwNdIQ5EYC4D/Ki42AACjioD/KpqKIAAAAIgAITwCAADg0QAhkU0C5KICADAVTBYkEebAa4C8BQAAYMK3Bnh9kISdAwsBsQ4AAICJ1xFgQSES+iOAlqWAWQkQAMACKwuyxDAJOgc2A2IvAAAA6/ANmw6RIOYANG8HzG6AAADWYyfbEJN25gAsr5AnyQ4uFAAAy7JDjfU0Hjl0DkBNY65T823nAgEAsDpyrJdjPs1HmpPjDZzq9Ohvc2EAACTLYkL622rspwGTPMXFgc7SCp/hogAASLo7As+oDqAJk/tVwLVcCAAASYrsAJowaScC6iu4CAAAkhvVBTRisr0K6NEbOPkBAEChOoFmTJZHAJo+SLKHEx8AAA6gOmEQDWnxFFY1Fskv+itOeAAAOIyvVEfQlBZNr0q9h/ySP+BEBwCAI/CB6goa05KvAuovcYIDAEAHvMSrg9abB3ATJzYAAATJTTSnVV4FdOuzOaEBACCkVwdld9CgJk9RtV4uv8zdnNAAABAiu1WH0KRmvQPQ39dVfokfciIDAECYfKi6hEY1XQKdnJp/IycwAABEhn+j6hR61URxaPoqTlwAAIjKioKyU2hWswhAtW+8/NKaOHEBACBKNKluoWEN/ypgo1N+WTs4YQEAIMrsUB1D0xp1QSBv4CSnR3+bExUAAGKC7BjVNTSuMecB3MlJCgAAMZ4fcCeNa7THAB5/HScnAADE546Av47mNUgKKpfa5ZfyJScmAADEiS9V99DAhlgPQN/ECQkAAHFmE+sHJPoxgNu3lBMRAAASguwgmjhBObAvwB5ORAAASBB72F8gAamoWHGCPPjvcAICAECCeUd1Es0c10WB9Fs48QAAwCDcQjPHTQB8YznhAADAWPjG0tAxXxXwolR5sD/nZAMAAIPxueoomjqGcWn6g5xoAABgRFRH0dQxEwDfBE4yAAAwtgj4JtDYUU6ON3CqPLhbOcEAAMDgbFWdRXNH922ANZxYAABgEtbQ3NF6DOBp9MoD2sRJlRz09iwV5wyZKa6qHSvuOG2YeHSUJp4f01e8AFHnxUHZ4uXKUyBG/L6qi3jKnSE2aA7xC0+FmOSZxTWePDSp7qLBI38McJzTrb/LCWVtemrLxfKhdeKZMf3FNxPtQkxKgziwZeCJ4pUyG8SR1/r9QPyvViDGe+Zy7Vsd2V2qw2jySB4DuPUrOZmszflDpost452UMhKQVLza+yjxqJYrqrXzGQesLQJX0uRhptCt95YHcS8nkjXxVC8Sr47tQxkjAcl9Z6D8aHFF9UDGBOuyV3UZjR5q6uqOlgfvNU4gazLJO198eno+RYwEwAF+rTkZG6zLa6rTKPbQ1gRYwoljTc4aXM9zfyQA2uCZqhRR5PYxTlhz7YAlNHuwSwNry7LkQdvJiWM9xgw8R+ycmEUBIwFwpDsCfe1qj3rGC+uxU3UbDR/UK4H6vZww1qOq+gLx0ekOyhcJgA5Y1bsUEbDi3QDZbTR8R28DVPs0ThZr8vgoN8WLBEAQbO5zlBhQMAYRsCKy42j6I+coeZA2c6JYj4ne+aKJ0kUCIGgeL08RadnTEAHrsVl1HXXf1mMAtz6bE8SavDy2nNJFAiDEdQRGZA4QaVmIgOUeC8iuo/EPnwzoDZzk1PzbOEGsx3jv2RQuEgBh8Fh5iujyo9MQAcvh36Y6j+Y/KA5NX8WJYU1+NuI0ChcJgHAWEqo4plkCWkRgKiJgIVTn0fytkwFrGnPlQfmGE8OavD2uJ4WLBECYTM3piwhYk29U92EALXcBHuCEsCbu6gsoWyQAImBt74zvJAARsNzdgAeYDNiyTTAnhIWXB6ZskQAIn99UdD1EAhABq60dkNTbDQc6OTX/m5wI1uXcITMpWyQAIuCFvid9TwIUqYiAVSYJvqm6MDkfA3h89ZwA1mblsNMpWyQAInlVsO8P2pQARMBCjwVkFyadAHi9gWPkD/8eJ4C1uXb4aMoWCYCI3hA49ogSgAhYhvdUJybbZMB5fPFIACABEJkEIAKWmSQ4L3kWBioOdJY/9Ba+eCQgHB5//rfi3r/+BaLAT9/7p7hoy78gijz844lxlwBEwBJsUd2YJHMB/Av4wpGAcHngjT+IOz7+FKLA5R/9Syz+51cQRR5aOjMhEoAIWGFugH+B5QUgxxs4Tv6wn/CFIwFIABKABERXAppFIHMKImBePlEdafW5AIv5opEAJAAJQAJiIwGtIuBABMw6N2CxZQWgtHbJiU7Nt50vGglAApAAJCB2EoAImBnfdtWV1twjwKMv4wtGApAAJAAJiL0EIAImRnal5QSgoDJwivzhvuALRgKQACQACYiPBCACpuUL1ZkWmwvgu5gvFglAApAAJCC+EoAImHVugO9ii80F0L/ki0UCkAAkAAmIvwQgAqbkS8vMDXC59fP5QpEAJAAJQAISJwH/FYHljE1m2WFQdqf5DaCu7mj5w7zPF4oEIAFIABKQWAlABEzH+6pDzf1GgNs3iS8SCUACkAAkwBgSgAiYDNmh5n4UoOl/4ItEApAAJAAJMI4EtIjAZETADI8EZIea+I0Av4cvEQlAApAAJMB4EoAImOlNAb/HpI8C9If4ApEAJAAJQAKMKQHNImBHBIz/SEB/yHQCUFTTWCg//H6+QCQACUACkADjSgAiYAr2q041110ATb+ZLw6QACQACTC+BCACpuBm88wF8K5IkR94F18aIAFIABJgDglABAzPLtWtLBEMSAASgAQgAYgASwkbMxUV849lu2BAApAAJMCcEoAIGHubYdWxLA4ESAAFjgQgATGlhxKBAYgAiweF/ChAf5IvCpAAJAAJMLcEIAJGfSSgP2ncuwA1jbnyQzbxRQESgAQgAeaXgBYRqEMEjEWT6lpjSoBHv4wvCJAAJAAJsI4EtIpAkbZMeEcuFFW1FzK2JRrZtUbdLfATviBAApAAJMDcEpDS5TQxylMjblpaJv60rkB89ptssf/FDPHt73LFZ2vLxOfrS8Wn95SKv99VLtatHC6Gjf4x4118+cRwuws6Nd9YvhhAApAAJMC8EpCdUSuu+XEf8flvs4V4KeN7tEpAW3xwdx9x0dkzGPfi96bAWGNNCHT7H+VLASQACUACzCcBPbqNEP45/cS/nmi7/IORgFb+dmeFmDFtHuNfrCcIys41jAAUVC61yw+1jy8GkAAkAAkwlwTkZ9WK3//C2W75hyIBrY8Lrls2njEwtuxT3csKgYAEABKABIRFVelg8f4DuUEJQCgS0Mpvr/UyDlp/BcFAJ/lhtvCFABKABIB5JKDMOeSIz/6jJQGKJ68fyFgYO7aoDk7wa4HLR/BFABKABIB5JCAzfbj4yy/zQxKAcCVAsXr5GMbDmL0uuHxEgicE+n7JFwFIABIA5pGAh39aHLIARCIBn60rFTMnTGFMjMkEQd8vEyYA9gGLj5cf4mu+CEACkAAwhwSMqakOSwAikgDJu7eViJySBYyL0edr1cUJkQCX2z+RLwCQACQAzCMBr99VmBAJUJw/QUMEYoDq4gQtEKTfzxcASAASAOaQgOkjtLAFIBoS8OGdJaJLl5GIQPS5P+4CUFq75ET5H97JwQckAAkAc0jAr1f1TKgEKNylQxCB6LNTdXKc1wbwT+HAAxKABIA5JECtCrjjqayES8DdS/u2fCZEIMprBvinxHc+gKY/yIEHJAAJAHNIwITB1REJQLQk4L3bS/77uRCB6M0LkJ0cx7kAS0+W/9HdHHhAApAAMIcEqL0BjCAB29eXHPrZEIFosVt1c5wkwD+DAw5IABIA5pGA1Rf2NoQEfLau7PufDxGIEv4Z8Xor4BEONiABSACYRwLuu7yXMSRAUuEaigjEhkdiLgB5Fct+KP9D33KwAQlAAsA8EvCb64oMIwHDK71tf05EIFK+VR0d4wmBvlkcaEACkAAwlwTcfXGJYSQgP7P2yJ8VEYhwgqBvVoxfDdQf4EADEoAEgLkk4KfnlxtkTkBpx58XEYjgVUH9gZgJgNcbOEb+R3ZwoAEJQALAXBJwwfT+hpCAbetKgvvMiEC47FBdHRMJKHT7azjAgAQgAWA+CXD3HmQICXjj56XBf25EICxUV8fqUcAqDjAgAUgAmHPZ4H9syEu4BKycUxXa50YEwnkksCpGkwL9b3CAAQlAAsCcEhDpWgERS8C6UpGdPjz0z44IhDg50P9G1AWg2HtRqvyXN3GAAQlAAsCcEtC/ZLDY+4I9YRLwyo1l4X9+RCAUmlRn82ogIAGABCABh3BXBK8KRiQB60pFZa8hkX1+RCBxrwq6PPq9HFhAApAAMLcEFOUPFTs3ZcZdAp5Y1Sc6PwMiEJwEyM6OogIEOsl/6RccWEACkAAwtwQoFk6tjKsEfLy2RORkDI/ez4AIBMMXqrujogBFmr+KAwpIABIA1pAAxa2+svhIwPoSMbDP4Oj/DIhAh6jujtZbAZdwQAEJQALAOhLQPWWEeGq1K7YSsK5ULBivxeTzIwJBvSVwSXR2DfTor3BAwSgSANFjy8ATo1pYEH1iJQGtIrDGVxoTCdi+vkRMHVITOwFABDpGdnfEAlBQGThF/sv2c0ABCUACwFoS0MqiaZVi97OZUZOA9+8oEX2cQ2MvAIhAR+xXHR7Zo4AB+nAOJCABSABYVwIUPQuGinsu6SX2vxi+BKh9Aa44szJ+5Y8IdPxIQHZ4ZI8CNH0lBxKQACQArC0BrXj6DGp+RPDRwznBScC6UvGP20vEmgv6ibTuIxIjAIhAe6yMVAI2cRABCUACIDkk4GBqygeJZQ39xA0X9Ba/urSXePnmYvG3NSXi1RvLxOOX9RFXn9VfOHNqE1v8iEBHbIp06+CdHERAApAASD4JMC2IwMHsDHtrYYd7eT8OICABSAAgAYiAiXcVlF0e5qMA/yIOICABSAAgAYiAmfEvCnc+wAYOHiABSAAgAYiAqdkQ7p2AbRQZIAFIACABiICp7wRsC32RIHdjPiUGSAASAEgAImB+VKeHNinQozdQYoAEIAGABCACFpgcKDs9tJUCPf5bKTFAApAAQAIQAQusHCg7PdRJgW9RYoAEIAGABCACluCtoAWguDjQWf4P9lJiYEgJmJUnxJyCxDHVHvpnnpmT2M88IytkCdjc/yjxRs3RCeP1AZ1CLsxXKxL8mbVOSAAiYFT2qm4Pcj7AsjIKDIwqAU2vLRVNW65IGOLygaF/5ofnJvQzN905KWQJ+OfSk0Paaz7a/Pu2riEX5jvTj0voZ97zRBoSgAgYeF7AsrIgJcBXT4EBEoAEIAFIACJgJQnw1Qe7PsDVFBggAUgAEoAEIAKWWi/g6uAkwKM/QYEBEoAEIAFIACJgIWS3B/tmwFYKDJAAJAAJQAIQAUuxteP5AN4VKZQXIAFIABKABCACFpwXIDu+/bsA7sbBlBcgAUgAEoAEIAIWHJdlx3c0H2Ah5RU/ho8/T5w1r0H85IJJYvWKUeJX1w0ST6+pEu+s6yPevbNCvHZLldh4/UCxbuVwcdWSCWLMhHORACQACUACABEId17Awg7uBOi3U86xXLpRF1OmnynWrBouPni46IiDyX8edorP1pa1ycfryprFYF7DHCQACUACkABABEK4E6Df3tGkwM2UdfTxjloo7rlmsPh8Y0FQg0l7EnAw29eXimdXe8TA0xYhAUgAEoAEACLQEZvbUYBAJ/n/sIvSjh79hi4Rt19RK759PiukwSRYCfgOKQOPXj1Y9Bm0FAlAApAAJAAQgSOxS3V92zsHVi3PobijQ+nAZeJq/1ix4+ncsAaTkCXgoDsDdwZOE8XVPiQACUACkABABL7/WFp2/RG2D270UuCRM2L8ueL9h4sjGkzClYBWPry7jxg8aiESgAQgAUgAIAKHzU1r9B5hUqBvDiUeGfPmzhL/2ZQb8WASqQQoPl1fKubWz0ECkAAkAAkAROCgyYG+OW3fCdD8l1Dk4XOlb7zY93t7VAaTaEhA61yBny45HQlAApAAJAAQgZY7AbLrj3AnwL+WMg+P+64fFNXBJGoScID7Vg1DApAAJAAJAERAqK4/0uuBz1PooXPdxWOiPphEWwIUV15wOhKABCABSAAgAs8fabXAjyj10Dhnfr1oejHDFBLw2bpSMf30KUgAEoAEIAGQzCIgu/57AlBcHOgs/+F+ij14Rk9cIHY+kx2TwSQmEqAmC64rEW7PbCQACUACkABIXhHYrzr/EAkocDfmU+yhLQL00WPOmA0msZIAxT/uKBG5vc5BApAAJAAJgCQVAdX5h0hAoadxKOUePGoVwFgOJrGUAMV151SKnDiIABKABCABSAAiYDxU5x8iAQ5Nn0e5B78PwDchLgNsNAlQjwXSup8WcxFAApAAJAAJQASMh+r8wyTAfzkFHxwPrh4Y88Ek1hKgWLe8Qp60I2MqAkgAEoAEIAGIgBElwH/54a8H3kPBBzcZcP+LdktIgFpIqCCrNqYigAQgAUgAEoAIGJJ7DpMA39OUfMc8+Qt3XAaTuEiA5H9XlB84aWMjAkgAEoAEIAGIgBH7zPf0YRLgf5OSb58+g5eGvCWw0SXgk7UlB5200RcBJAAJQAKQAETAiCLgf5OFgkJk4TnT4zaYxEsCFIP7Do6ZCCABSAASgAQgAgYUgcMXDJJ/cxdF3z6P/LzakhJwr7/isJM2eiIQCwkQN48RTeunJAyxsDT0z3zl4MR+5ovdIUvAX6f8QGy77IcJ44PzTgy5MP80/JiEfuZPGk9BAsAsIrDrOwGwD1h8PCXfPj1rlosdT+daUgI+uLOkjZM2OiIQEwmAqNGeBIAxQAIQgVihur9ltcDKpXaKvn1mz54T19uK8ZSAz9aVidKCYTERASQACQAkAIwpAqr7W9YI8Cwro+jbZ9Xy8daVAMm547UjnLSRiQASgAQAEgDGFAHV/S3zAdyNgyn69lmzarilJeDac/q3c9KGLwJIABIASAAYVARk9x94M8BfR9G3z//+rMbSEnBfY3kHJ214IoAEIAGABIBBRUB2f+saAWdT9O3z0t19LS0Bz11TFsRJG7oIIAFIACABYFQR8J994HGA7qfo2+fvD/W0tAS8dUtpkCdtaCKABCABgASAQUVAdn+zBLg0/TqKvn3+9WS+pSXg/TtKQjhpgxcBJAAJACQAjCkCqvtbJeBuir593n+42NIS8JdflIZ40gYnAkgAEgBIABhTBFT3t64WuIGib59X1lVYWgJeuLYsjJO2YxGIiQRc0FuIZf0Sx+y80D/zuT0T+5nPcoUsAX8cdLT4a90PEsZbI48NuTBfd3dK6Gf+y+mdkQAwkwhsaFknwO1/lKJvn8f+x2NpCdhwcXmYJ237IhCTvQM2m3HvgDnsHcDeAUgAGEoEVPe3vh2wkaJvnzuuqLW0BNx4bv8ITtojiwASgAQgAUgAGFUE/Btb3w54lqJvnyv1cZaWgEWT3BGetG2LABKABCABSAAYVARk97c8DtD0lyn69jlrXoOlJaBfzyFROGm/LwJIABKABCABYEwRUN3feifgdYq+fcq8y8Tu57ItKQEf3V0SxZP2UBFAApAAJCC5JSC9x3AxpdYjls/qJ65f3Efce2kvsennLvGXX+aLbb8qEh+vLRHv3lYiXvtZmdi4qo+4e2lfMW90tUjpigDEXARk9x+4E+D7M0XfMU/eOsCSEvDwJeVRPmn/KwJIABKABCSfBORn1YpzJlWJx64tFrueyTzi8drxoOvIY9P6EvHq6jLhnzlAZKSOQARiIAKq+1sfB/ydku+YZQunWFICTq8ZGIOTtkUEkAAkAAlIHgkYXV0jnrvJKfb93h7U8WpXAg7Z7rxUvP7zUjGoYjAiEEURUN3fuk7AFkq+YyqHXRj0yW0WCdi2riSGJ+1IcdXAIUgAEoAEWFwCBpQNEr+9vijk4xW0BBwkA89c3Vv0KhiGCESn17a03gn4lJIPjhfv6mcpCXjqyt4xPWFX9qlAApAAJMCiElCcP1SsDZSE/ctRyBLw3aOCUnH/T8pFZupwRCCyOwGftt4J+IqCD45pM+ZZRwKkVfdxDo2xBJQjAUgAEmBBCRjnrRZfbsyK6HiFLQEHTWoeUDoEEQifr1olYCcFHzy/W1NlCQl4dGWfmJ+oSAASgARYTwIuqu8v9r4Q+aPRSCVAsX19iZgzshoRCI+drRKwj3IPnlETFsR8bkDMJUBeOLn2WiQACUACkICg6dFthLjr4pKoHa9oSIDi83Wl4voF/RGB0NmHBITJhhu9ppaAu5b2jcsJigQgAUiANSQgVQqAmvkfzeMVLQmIfA+UpBWBfTwOCJPq0xaJL5/KM6UEqMU51AWNBCABSAASECzrVpRE/XhFWwIUl82tQgTCeBzAxMAwmDHzDLH3hUxzScD6ElHTO37v2iIBSAASYH4J8M2OzVtRsZAA9Whg6pAaRCCUiYG8Ihg+jRfUmUcC5MUxf4wnriclEoAEIAHmloAJg6tjNgcqFhLQOlmwwjUUEQjhFUEWC4qAu64aZgoJWH1e/CfOIAFIABJgXgkocw4RXz2ZFbPjFSsJUHx4Z0ny7j8QvAhsYdngKFBc7RO/XV1qaAn4zeV9EnIyIgFIABJgXgnYcEXPmB6vWEqA4oZkfWMgSBH4btlgNhCKHJe2XNyi9zeeBKwrFbcu7pewExEJQAKQAHNKwKB+XtH0YoapJeDT9SXJvflQByLw3QZCbCUcpc0Y3D6xqN4rvn3ebgwJWF8qzp+gJfQkRAKQACTAnBLw/M3OmB+vWEuA4lf+8uSVgI5E4L9bCesvU+LRE4ExQ08Tn/8mO6ES8MnaEjG0X+J33EICkAAkwHwSMHmYJy7HKx4SoH4ZKs4bhgi0IQKq+1vvBDxLgUdXBEp6TRC36aViT5h3BcKWAHnCP3JJucjJMMbGGkgAEoAEmE8C3rir0DoSIHnssj7JLQFHEgHZ/QfeDvBvpLyjLwKpmVNERfGQ5sk1oT5bC1kC1pWK568tE2UOY70WgwQgAUiAuSRAvREQr+MVLwlQd0aTXgLaFAH/xpbHAW7/oxR37ERAHXxvX6949JpisfvZzOhKgPzN/7Wfl4rBfQcb8qRDApAAJMBcEhCrhYESKQGKIX0HIQGHiYDq/tZ1AjZQ2rEXAUVG6nBRP9ItfnVpr3a34WxPAtRCGC/fUCaWTh0g0robe+YrEoAEIAHmkoAXbnFaUgLu1SsQgO+LwIZmCXBp+t0UdvxEoJVuXUeI0dU1YtG0SnHVeeXi7otLxJM3usT/W18gtt3nal7j/93bSsSrq1ueaalX/RqGm2s5TCQACUACzCMB+Vm1Md8hNVES8MEdPBI4XASyiudvapWA6yjrxIiA1UECkAAkwDwScPaEqrger3hKwGfrykRRsr8lcPjW0FlT/tj6doCfokYEzCIB4jJZwtfVJo6zXKF/ZrWQVCI/8wV9QpaAt0YdKz44/8SE8bcZx4VcmH/0Hp3Qz/yPs04wtQRc8+M+1pUAScPwasr/4O2hs6f/rvXtgLMpaUTANBIAUaM9CQBjEE8JiMV2wUaSgEtmVVH+B5GeM/PeFgnw+OsoaEQACUACILkl4KnVLktLwB1L+lL+B5GWN/O6A48DGgdTzogAEoAEQHJLgJqUbGUJ+O3lLBp0MBn59Re2rBPgWVZGMSMCSAASAMktAZ9FuNy50SVg88/KKP+DJaBgdl2zBBRULrVTyogAEoAEQHJLQDQ2PzOyBLx9aynlfxBZrlkVzRJgH7D4eAoZEUACkABIbgnY9liOpSXglRu5E3AweXnzf2hrjSyjXRQyIoAEIAGQvBLwx7utPSfg8UuZE9BK166jhO3gOD36R5QxIoAEIAGQvBLw2+uLLC0Bay7ohwAcIKX7uL2HSoDmf5MiRgSQACQAklcC7rrY2usENNYPQAAO0D11ws7DJMD3NCWMCJhCAq4cLMTq0xLHOUWhf+bGAYn9zBeVhywBfx7TWWxZfFLCeLch9BUDX6/uLP46t1vCeLu+q6kl4Mpzyy0tAdOG1CAArUsGZ0z612ESoN9DASMCptg74DUz7h0wl70D4rB3wP8bd3JcS+Vwtt3S09QSoHY4tfLeAbkZwxGA1iWDM6f84xAJcGj+yylfRAAJQAKQgOSVAHvacPHNs9bcRfDdNbweeMhqgTnTnj9MAvR5FC8igAQgAUhA8kqA4okbiiwpAbddyJLBh+wbkDfzjkMkoNDTOJTSRQSQACQACUhuCVg4tdKSElDZcwjlf8iSwbMuPEQCCtyN+RQuIoAEIAFIQHJLgDN3mNj/orUk4J93lVD8hz/6KZgz8BAJKC4OdJZls5/CRQSQACQACUheCVA8f7PTUhLwK38FxX8II0V6xfwTbIeHBYMQASQACUACkIDaAQMtIwHb15eI7HTeCjh0oaCx+2xtRZbM8xQtIoAEIAFIQHJLgOKRq4stIQE3L2SVwO8tFJQ28d9tS4Dbv5aSRQSQACQACUAC+vYcLPa+YDe1BHyytkSkdKX0v79GwOT32pQAl+a/hIJFBJAAJAAJQAIUt/lLTS0B/pksE9zmGgHZ0549wp0A3xzKFRFAApAAJAAJUBRmDxMfP5xjSgn48y9YHOiIawTkzryt7TsBnkYvxYoIIAFIABKABLQysMIrdm7KNJUEqMcAufZaCv9IawTk1S9qWwKqludQqogAEoAEIAFIwME0jHKLphdNIgHrS8XAPoMp+/buBDhnaba2E+gkC2UXpYoIIAFIABKABBzMqgXlxpeAdaXirLEeir4dunYdJWx1dUfbjhRZJpspVEQACUACkAAk4HDuvrjEuBIgBeCas/tT9B2+Hjjh/2ztxenWb6dMEQEkAAlAApCAtvjJGX2jtqxw1CRgfak4d7xGyQfzemDWlHfalwCPvpAiRQSQACQACUACjsSUWo/4z++yDCEBahLg4L7MAQh6UmDOjAc6uBPQOJgSRQSQACQACUAC2mNA2SDx/gO5CZWAd9aUiPxM3gIISQIKZi1uVwIc3hUpFCgigAQgAUgAEtBhoaQOFyvPqhA7nsqKqwRsW1cirphXSamH8525Ghy2jiLLYysFigggAUgAEoAEBENBVq34n4t6i2+es8dWAtaXijsv6itSu42g0MPZOKjbmP22YOL06E9QnogAEoAEIAFIQCiUOYeI2/RSse2xnKhKwNa1JeLhS8pFXia7AUZCj/RJ/wpOAjT/1RQnIoAEIAFIABIQ1rvoXU4TQysHiusX9RF/uy8vdAlYVyY+vLNE3N9YLoZXDqLAo/ZmwNTNQUmAw+OrpzQRASQACUACkIBoUF40REwa4hHnTa4Sl59TIW73l4rfXl8k/np7sXj71lLx4vVl4qEV5eJn5/cXF0x2izLHUEo7FvMBcmf+IkgJWFZGYSICSAASgAQgAWCh5YLz66cGJQHFxYHOsiz2UpiIABKABCABSABYgC4jRZeCGafYgo0sircoS0QACUACkAAkACywXHDq6btsocTl8d9KUSICSAASgAQgAWCJSYF/DkkCHB69gZJEBIwoAWJBsRA/Lkkc9dmhf+YzHYn9zHMLQpaANzydxFujjk0Ybw45BglAAiBakwLz6m8KSQIK3I35FCQiYEgJgKjRngSYESQAoG0yC8+osYUap+bfRkEiAkgAEoAEIAFg5pUCx+63hRNZDBsoR0QACUACkAAkAEy8UmBG3cdhSoB/EcWICCABSAASgASAeUnLmf5QWBLgcC/vRykiAkgAEoAEIAFg4kWC8mbNCUsCvN7AMbIQdlKKiAASgAQgAUgAmI+uXUaK9PT5J9jCjSyDTRQiIoAEIAFIABIAJlwkKG3Cv22RRBbBSsrQGBRX+8TAUQvFqPHnip7Vy00hAkgAEoAEIAGQwPkAWVNfjEgCXAP04RRwYjh98tniZ5eMFH+8t7fY/kSh2P+iXTT93v7dVpufrS8VW9b2Fr+51ivm1M81pAggAUgAEoAEQALnA+TMDEQkAQWVgVNkCeynlONDz5rlYuXSCWLrbxxtLqX6nQS0wadSCm4PjBRFHuOIQEyWDb5zomja0JAwxAV9Qv/c19Um9jNfWhOyBPxtxnHi8+t+lDC2XHgSEoAEQISbBtkdDRm2SOP06K9Q0LHnvLNniA8fKWp3PfX2JKCVrevLxIqFdYYQgZhIwGYz7h0wh70D2DsACYA4zweY+B9bNOLS/JdQ0rH97f/eawcHNTAGIwGtPH1DdUzuCoQiAkgAEoAEIAGQqEcB0zdFRQKKNH8VZR0bqmovFK+uLw96YAxFAhR/u6Nc9B+yOPqfPUgRQAKQACQACYBE7RcQ5voA30+gkxz4v6C0o0uZd5n4y69LQxoYQ5UAxXt3lIpiz9KEiAASgAQgAUgAJGB9gJTRTTZv4BhbtOLy6PdS3NHl8Zu1kAfGcCRA8fz1FcLhXh53EUACkAAkAAmABOwXYK/bYotmXJpvFsUdPa7Sx4U1MIYrAYobfuyJuwggAUgAEoAEQALmA+TOvC2qElDsvShVDvhNFHjkuIdfIHY+kx13Cdi+vkQUOibFVQSQACQACUACIAESUNDQxxbtuDT/G5R45NxzzeCwB8ZIJEDxvyvKm8s6XiKABCABSAASAPGlW49xu22xiEPTV1HikeEdtVDsfSEzYRKgVhnMz6yNmwggAUgAEoAEQHxJzZz6SkwkoNDtr6HII2PV8vERDYwRS4DkqvmVB06U2IsAEoAEIAFIAMR9PsCSmEjAga2Fd1Dm4RPKmgCxkoA/3VR6kDHGVgSQACQACUACII6vBnYd1ZTiHHuyLVZxaPoDlHl4VA67UOyTJZ5oCVCPBLp3PS0uInB5pRsJQAKQACQA4rVUsH3yR7ZYhlcFw2fK9DMjHhijIgEST9ngw54hxUYErq0djQQgAUgAEgBxwp436+cxlYC8imU/lIP7t5R6eBsEGUUC5oysbmMySfRF4NrhSAASgAQgARCnRwEip3hBqi3WkYP7I5R66Ky8aIJhJODSOZVHmFUaXRFAApAAJAAJgHitEjh5qy0ecWr+GZR6GIXYOMYwErD6vP7tvF4SPRFAApAAJAAJgHi9FVC/Ok4SsPRkOcDvpthDQ19UZxgJWDZtQAfvmUZHBJAAJAAJQAIgDo8CuowSub3m9bDFKy5Nf5BiD40zz5hlGAmYOHBgEAtORC4CSAASgAQgARCHRwEZdZ/Y4hmH5p9CsYfG2EnnGEYCehUMC3LlqchEAAlAApAAJADi8Shgxo1xlYDS2iUnykF+J+UePMXVPvHV7/ISLgFb15WEuARl+CKABCABSAASALF+FDBS9Miv726Ld+Qgfz/lHhoPra5JuAQ8eUXvMNaiDk8EkAAkAAlAAiDWjwImfWxLRFxu/0SKPTQWzK9PuAQ0DK8Jc1OK0EUACUACkAAkAGJLRl799QmRAPuAxcfLgf5ryj14SgYuE5887kyYBHx0d0lEJ1uoIoAEIAFIABIAsVwgaHSTvecZXWyJisPt+yXlHhrLfjwlYRJwwWR3FLapDF4EkAAkAAlAAiCG2wZnTf27LZFxepaPoNhDo8jjE397oFfcJeC920uid+IFKQJIABKABCABEDuynHMX2RKbQCc52G+h3ENj4pSzxO7nsuInAetLxfBKb3QNNAgRQAKQACQACYDY0K3H+D2qg22JjkPzXUyxh87iBdPjIwHrysTiSe7Y3IrqQASQACQACUACIEZrA+TM2GgzQgoql9rlgL+PYg+dn17ojbkE/GJxv9g+k2pHBGIhAWKKXYipCaQuPYzPnJHYzzw5I2QJeLWPTWzue1TCeLX8KCQACYAj0WWkyC44o4/NKHG4/Y9S6uGxaM4I8c1z9uhLwPpScXF9VXwmpxxBBGIiARA12pMAM4IEQBKtDfCpzUhxar6xFHoESwqfNlF89HBO1CRg27oSMb5mYHxnqbYhAkgAEoAEIAEQi0cB9VcYSgJsdXVHy0H/Ewo9fAp6zhaXnNlX7HgqK3wJWF8i7l7aV6R2G5GY11UOEwEkAAlAApAAiPLaACmj9+fkzD7OZrQ4PfpllHlkZDrmioKsWnHp2RXi9bsKRdOLQUjAujLx4V0l4r7GclGYXZv491YPEgEkAAlAApAAiPYYO/UVmxHjrGnMlQN/E2UeuQi0ftnF+UPFOZOqxKoF5eLOn5SKJ28sEu/eViJeXV0mHr20j7hhQX9R5hhqwJO0RQSQACQACUACIMrja/aM02xGjUPTn6TIoysC5rXVKeKa2lGULRKABCABECW6p034ymbkON2+SZQ4ItDK5ZVuyhYJQAKQAIjWZkH59dcYWgIqKuYf69R82ylxRECxsk85ZYsEIAFIAESBlO7j9hlyQiArCCICSAASgAQgARDj1wJzZzxhM0Mc3hUpsrx2UeCIQCwkoOnxeaLp5QsThvBXhf6Z75yU2M9844iQJeD9s08QO+/vnjC2X3EqEoAEwHdbBo8SWYVz8mxmiSyumylvRCAmEsDeAewdgARA8m0Z/JbNTCmqaSyUxbWf8k5uEUACkAAkAAmAKGwZXDh7mM1scbr1hyju5BYBJAAJQAKQAIhwn4D0SdtsZoxD83so7eQWASQACUACkACIjLS8mQttZo1L0/9AaSevCCABSAASgARA+HTrMX6Xzcxh8aDkFgEkAAlAApAAiOAuQPb0m0wtAQd2F3yfwk5OEUACkAAkAAmAMBcH6jZ2r91ed7zN7HG59fMp6+QUASQACUACkAAId3GgmffbrJDS2iUnyqL6krJOPhFAApAAJAAJgLDuAuy39zyji80qYSnh5BQBJAAJQAKQAAhjo6C8mb+xWSkFlYFTZEl9QVEnlwggAUgAEoAEQKh3Acbsz+01r4fNanF69GWUdHKJABKABCABSACEvFHQYzYrpmVuANsMJ5MIIAFIABKABEAIdwFSxuxPLZjTzWbVODR9MQWdPCKABCABSAASACHcBcie/qjNysnxBo6T5fQJBZ0cIoAEIAFIABIAQW4XnDJ6f7pjWorN6nF4/Aso5+QQASQACUACkAAIervgh23JkOLiQGdZTFsoZ+uLABKABCABSAAEdxcgI6Ohqy1Z4tD0eRSz9UUACUACkAAkAILYIyBnxi9tyRSvN3CMLKX3KGZriwASgAQgAUgAdLRT4Lg9ltgjIPS5Ab56StnaIoAEIAFIABIAHawOmD/7BltyJtDJqfnfpJStKwJIABKABCABcGS6p0/8P9WFtmSNy9PopZCtKwJIABKABCAB0M5dgMK5Z9uSPQ5Nf4BCtqYIIAFIABKABMARXgm0T/7IRmw2Z01jriyjbyhk64kAEoAEIAFIALTxSmCXkXLcnV2NAfz3bsAqyth6IoAEIAFIABIAbdwFyJ76B5r/4AWEvIGTnJp/G2VsLRFAApAAJAAJgO8vDGR3NGTQ/IdPEnTrsylia4lATCTg8Xmi6eULE4bwV4X+mWUJJ/Qz3zgiZAl4/+wTxM77uyeM7VecGnJh/mnkieKTG1wJ459XFCAB0PHCQNnT76Hx285RsoQ2U8TWEYFYSABEj/YkAIwBEmC1hYHG77LZ5h9L3R9pkmC1T6OErSMCSAASAEgAHPRKYN5sXgnseO0A/V5K2Boi0FjWl7JFAiACNld0pjytMhkwc8o/aPhgJglqy7JkAe2khM0vAme6qihbJAAi4KW+J1CgFiAlZUxTbq9zSmn4YO8GaL4lFLD5RWB0bjVliwRABDzV91RK1BL7AzTcR7OHkrq6o2X5vEYBm1sEKtIHU7ZIAETAr/v0oETNvj9A2sSv1c65FHuIKXTrvWX57KWAzSsCKV1OE5+NzaRwkQAIE5+rmCI1M11GCnvh3Ik0erhvC7j1Kylfc4vAWncvChcJgHAmBfY5SmSl1FKkJiY9c+rLNHkEyfEGjpMi8C7la14RmFqoUbhIAITBc31PoUjNPBmw29i9dvsZXWjyiF8ZbN5uuInyNacIpHUdIbaOyaJ0kQDgUUCSrQw4U6fBo/VYQNPXULzmFYHze1ZSukgAhMAf+h5PkZqYHhl179Hc0X0scKosna0UrzlFoFuXEeKvp+VSvEgABMkZ+b0pU9M+BhizP9tZn0tzR3/tgAmUrnlFQK0ZsHdiOuWLBEAHbKzoQpmae02Ay2nsmImA/iCla14RWFLSj/JFAqA9+h4n7LwRYN6lge1TPqSpY7mksPeiVFk4n1O65hWB26tKKGAkANrcLOgYUZE2kDI162OA7mP324vOKqSpYz5J0DeWwjWvCHSVXNu3NyWMBMBBvNr3B8KdXk2ZmpjMwtk/oaHj97bALRSuue8InOEaIHZPyKCMkYCkZ1PfH4rslGEUqZkfA2RN/TPNHMdUVKw4QZbNOxSuuUVA7S3wcE0RhYwEJOnt/2PFymIHJWr6xwDj9qQWzOlGM8c5RdV6uSybPRSu+dcRGJ5TI54e5BB7eHsACUiS8r+1NEukdx1OiVphaeDc+jNo5EQ9FnD7llK21lliOLdbrZhfVCU2VBeLN4fnN680yGuFSICpC79PJ7FZlv7LfU8Q63uni9Oz+1OcVnoMkDn5OZo4oQl0kkWzibK13jbEB08kzJFykAtRx9l9qOjVYzDECDb/sfgWwamn7ygoOO0H9HCCU1C51C6L5kvK1roiAABgJLqmjG7KKJhdRQMb5bGAx19H0SICAADxwJ7f8FOa12BxaPqdFC0iAAAQ290Bp/E6oDFXEwyc5PTob1O0JhABJyIAACacB5A2YVdWyTk/onENu4hQo1OWzA6KFhEAAIj6PADn3EE0rdEfC1T7xsuSaaJoEQEAgKjtDphXv4qGNc/8gFWULCIAABCV9QCyp/6BZjXd+gH+jZQsIgAAEOF6AP9mPQATxtXf11UWzIeULCIAABDWvgDdxuyz580ooVFNmgP7C+ymZBEBAICQJgJ2GSky8htm0aRmvyPg1mdTsIgAAEBoGwPNXEODWubVQf0mChYRAAAIckGgP9KcVlpIqDjQWZbLSxQsIgAA0B49Muq+zPHOPo7mtFh6Veo9ZLl8QMEiAgAAbdGtx/g99p4LCmhMi6awqrFIlstXFCwiAABw+IqAma45w2lK688PGCTZQ8EiAgAAzTS/CVB/IQ2ZJHF49AbKFREAAGiZCDh9Lc2YbK8OevQVlCsiAABJviRw5hSWBE7aRwNu/1rKFREAgKR9E+ADWQVH0YZJ/eqg7xnKFREAgCQTgPSJX3YrXnASTZjkyfEGTnV69LcpV0QAAJJmU6DdGQXz7DQgaXksUNOY69R82ylXRAAALL4WQPdx+zJ7ntmP5iOHTRRcXiGLZQfliggAgEV3BUwZ05TpnDeOxiNtvzqo+T2yWHZSrogAAFhsMaCuo6QAzJ5L05F2U6Tpw2SxfEO5IgIAYJ3FgOx5DctoOBLsq4PjZLHspVwRAQAwP+m5M66h2UiIcwT802Sx7KdcEQEAMC8ZOTN/QaORMOcI6PNksTRRrogAAJhwOeDcmXfTZCSyRwOafxHFiggAgMkeAeTPvI8GI9ERAY+vkWJFBADAJI8A8hseprlIlEXAfxXFiggAgMEFoGDWkzQWQQQAEQBIvjsAT9FUhEcDgAgAJNscgLyZj9FQJJ6TBXlrABEAACMIQM6MB2kmEtcceH2QdQQQAQBI5GuAOdPvoZFIQnJgQSFWFkQEACABSwGn5874OU1EEvtooGWJYfYaQAQAIF6bAXUZJTLy6y+lgYghcmDTIXYfRAQAIOa7AY5uyiiYs5jmIQabI9C8DfEOyhURAIDYkJIypimzcM4cGocYdI7A8gqn5ttOuSICABBlAeg+bp+96MzTaRpi7DkCNY25To/+NuWKCABAdOjWY/w3Ga65A2gYYorkeAOnOjXfM5QrIgAAEQpA6un/ySqck0ezEFOluDjQ2en2r6VcEQEACI/u6RM/ttvrutAoxMTzBPQVlCsiAAChkZo19SWbLdCJFiGmj8OjN8hy2UPBIgIAEMQiQDkzbqc5iLUmDGr6IMlXFKyZROAMBmSAeK4BkDK6KSOvfhGNQSyZwqrGIlkuH1CwiAAAfO8NgD2ZBXNraQpi6fSq1HvIcnmJgkUEAKCFHhmTvrIXnVVIQ5CkSPObA5p+EwWLCAAwAXDqW/YBi4+nGUjSxeXWZ8uC2U3JIgIAyTgBMC17+lqagCR1iqr1clkwH1KyiABA0iwB3G3svvS8mewBQEjzHYH+vq5Ozb+RkkUEAJJgAaB/ZeQ1OBj5CTkkgU4OTV8lS6aJokUEAKxIWvbU39u8gWMY7wk5QhzVvvFsSYwIAFjt/X97waxLGOEJCSJOrdHJToSIAIAlbv+nTdhld83zMrITEkKKvYGTHJp+J0WLCACY9vZ/1rS37T3PYAMgQsK+K+Dx18mi+ZKyRQQATHP7v+sotfzv9YzghEQhBZVL7bJoNlG2iACA4W//p074d3r+LI2Rm5CoJtDJ6fYtZTdCRADAsIv/ZE17oqJi/rGM14TEKAcWF3qHwkUEAAzz23+P8d/a82bNYIQmJA6pqFhxgiybWyhcRADACGv/Z7gaujIyExLnODXfWFk4n1O6iABAApb+3Z9e0BBgJCYkgSn2XpTq0vQHKV1EACBuW//aJ3+c45jvYgQmxCBxab4JsnS2UryIAEAMV/7bn5Ez/QZGXEIMmBxv4FRZOmvYfwARAIj6b/8Zde/Z86YXMtISYvS7Ap5Gr9Otv0v5IgIAkdKt+7g96bkzlzCyEmKuuwLHSRG4UpbPXgoYEQAI573/9OypL2dlTf8RIyohJk2hW+8ty+c1ChgRAAhh05+vMwrmTGIEJcQKqas72qX5lsgC2kkJIwIA7W35m5FXv8HrDRzDwEmIxVKsLctyefR7KWFEAKCN1/62ZBfNL2ekJMTicVb7NFlCmyliRACgW+rpO9NzZ57HyEhIcuUol1uf7dT82yhjRACSccW/MfvTc2bcbrPVHc1wSEiyPiLwBk5yaPoqWUbfUMiIACTBc3+121/2tBdzc+f1YAQkhLQ8IqhpzJUy8ACFjAiAhTf7sU/+Z7pzlsaIRwhpM80LDWn+NyllRAAs9crfzsyC2QsY4QghQSTQyeHx1ctSeo9iRgTA1Dv97U3Lq79JvSbMuEYICSnqXWGHps+TxbSFckYEwFTlvy89d+b69PT5JzCSEUIiSnFxoLPD418gy+kTChoRAIOXf86M+1Kcc09m5CKERDVqPwKHpi92ar7tlDQiAAZ73S9vxoN5efN/yEhFCIlpSmuXnOj06MtkSX1BUSMCkODyz53xaIbr3K6MTISQuKagMnCKQ/NdLIvqS8oaEYA4ln/KmCZZ/k/k9uJdf0KIAe4MuNz6+bKs/kFhIwIQwyV+u4/bk5Y949epBXO6MfIQQoyVurqjnW7fJJem/4HSRgQgqu/5/8eeO/OnFRXzj2WgIYQYPg7N73G69Ydkce2nvBEBCIeRame/jzPyZ81nRCGEmDJFNY2FsrhuluyiwBEBCGJt/66jRGr2tD/ZXfO8jCCEEGvcGfCuSGmZRMjrhYgAHHmBnxkbswrn5DFiEEIsGfVMU80bcGj6k7LImihzRCDZ6ZEx6XN7fsNP7fbFxzNCEEKSJmrnQqdHv4yVCBGBZPytPy1n2vPs6EcIIeqtAs031uH2PypLbR/FjghYdqJfRt2nGfkNK4uL6zpz4RNCyGEpqFxqP7AAEZsWIQLW+K2/+7i96dnTn85yzargCieEkKAS6OT0LB/hcPt+KQvua0oeETDXDP/RTakZkz/IKGjQ2caXEEIiiH3A4uNdbv9EWXL3S3ZS9oiAYV/ts0/ekl7QcGVO79mncuUSQkiUo5Yodmj+KS5Nf1AW3m5KHxFIdPF3t0/+KCOv4dp0x/wUrlBCCIlTnNrSk52af4YsvUck31L+iEBcir/LKLWS39b03PrV+flnd+dKJISQBCevYtkPXZpvlkPTH5AFuAMJQASivGVvU6p9yof2vFk/zy6an8YVRwghBo3XGzim0O2vkUKwyqX530j2RYkQgfBe5+ueNuH/0nKmP2d3zJnHxj2EEGLSFHsvSlV3CVwe/V5Zil8gAtDmb/spY1pm9OfV35zjmO/iyiGEEMsl0KlI81e5NP8lTo/+SjLtcogIfP/Zfve0if9Jy57+bHrB7Lk2b+AYrg9CCEmiFFQGTnEN0IfLklwp2WT1VxCTWQTUu/s9Mib9Kz1n+tOZjtkXFPQ5vxtXACGEkO+i5hM43Mv7OTX/IlmaG+Sf2xAB007m29/8+l7OjAfshbOns1QvIYSQ0O8WuBvzHR69weXx3ypL9C3JXkTAgO/rp0/cmZo19c/puTPXZOQ3DOLMJYQQEvUUFwc6OzzLyhweX71T81/t9OhPyGLdigjEbwe+Hhl1n6VmT38pPb/+xizXmaMLCs7/AWcmIYSQhMXhXZHidDcOllKw0OnWb5dFu1myCxEIf7Z+82t69invpuVMfyijoOECe9HsQs40QgghJkmgk6tqeY7L0+h1un1zmt9KcPvXygJ+XsrCR0Z4OyGRIqB+q++eNvHfqZlT/q4m7GUU1N9kd8w9M6/kvBJ17Dh/CCGEWDbq0YKac1DoaRzq0PR5Ds1/uSzme5ya72mn5n/zgCjsMpsIdO06WnTrPm6vLPj/62Gf/Fla1tR307Knv5CWO319Rm59Y3rBrNouBTNO4QwghBBCOojaSbGgcqldzUVoeeTgr5OScLbTrftdmn6d5G71NoPD7X9U/v2N8u8/K6XiZfnn6w7N92f513+X/3yL/PNT+edXB16F3HcA9ddfZTrmfp2SMma//O18b7ce47/plnr6zu5pE/7TI33Sl83P4O2TP5G/sX+QljXtHclr6VnTNqVlTnswLWf6TWm5M3V7XsP0jILZVVkl5/yIb4wQ4+f/A+wHyd1r4sb2AAAAAElFTkSuQmCCG3Bo+pOyyJooc0Qg2emRMelze37DT+32xcczQhBCkiZq50KnR7+MlQgRgWT8rT8tZ9rz7OhHCCHqrQLNN9bh9j8qS20fxY4IWHaiX0bdpxn5DSuLi+s6c+ETQshhKahcaj+wABGbFiEC1vitv/u4venZ05/Ocs2q4AonhJCgEujk9Cwf4XD7fikL7mtKHhEw1wz/0U2pGZM/yCho0NnGlxBCIoh9wOLjXW7/RFly90t2UvaIgGFf7bNP3pJe0HBlTu/Zp3LlEkJIlKOWKHZo/ikuTX9QFt5uSh8RSHTxd7dP/igjr+HadMf8FK5QQgiJU5za0pOdmn+GLL1HJN9S/ohAXIq/yyi1kt/W9Nz61fn5Z3fnSiSEkAQnr2LZD12ab5ZD0x+QBbgDCUAEorxlb1OqfcqH9rxZP88ump/GFUcIIQaN1xs4ptDtr5FCsMql+d9I9kWJEIHwXufrnjbh/9Jypj9nd8yZx8Y9hBBi0hR7L0pVdwlcHv1eWYpfIALQ5m/7KWNaZvTn1d+c45jv4sohhBDLJdCpSPNXuTT/JU6P/koy7XKICHz/2X73tIn/Scue/mx6wey5Nm/gGK4PQghJohRUBk5xDdCHy5JcKdlk9VcQk1kE1Lv7PTIm/Ss9Z/rTmY7ZFxT0Ob8bVwAhhJDvouYTONzL+zk1/yJZmhvkn9sQAdNO5tvf/PpezowH7IWzp7NULyGEkNDvFrgb8x0evcHl8d8qS/QtyV5EwIDv66dP3JmaNfXP6bkz12TkNwzizCWEEBL1FBcHOjs8y8ocHl+9U/Nf7fToT8hi3YoIxG8Hvh4ZdZ+lZk9/KT2//sYs15mjCwrO/wFn
`; //data:image/png;base64
  }

  static getPackageID() {
    return "org.gridnetproject.UIdApps.calculator";
  }


  initialize() //called only when app needs a thread/processing queue of its own, one seperate from the Window's
  //internal processing queue
  {
    this.mControler = setInterval(this.controlerThreadF.bind(this), this.controllerThreadInterval);
    this.disableVerticalScroll();
    this.disableHorizontalScroll();
  }

  controlerThreadF() {
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

  open() { //Overloaded Window-Opening Event
    this.mContentReady = false;
    super.open();
    this.initialize();
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

export default CCalculator;
