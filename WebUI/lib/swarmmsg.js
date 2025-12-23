
let msgTemplate = `<li class="[originFlagField]"><div class="entete"><span class="[statusField]"></span><h2>[sourceField]</h2><h3> [timeField]</h3></div><div class="message"><div class="triangle"></div>[msgTxtField]</div></li>`;
export class CSwarmMsg {
  constructor(type = eSwarmMsgType.text, from = new ArrayBuffer(), to = new ArrayBuffer(), data = new ArrayBuffer(), timestamp = 0, external = false) {
    this.mTools = CTools.getInstance();
    this.mFromID = this.mTools.convertToArrayBuffer(from);
    this.mToID = this.mTools.convertToArrayBuffer(to);
    this.mVersion = 1;
    this.mData = data;
    this.mTimestamp = timestamp;
    this.mExternal = external; //NOT serialized. optimization only
    this.mSig = null;
    this.mInitialized = false;
    this.mType = type;

    //Not Serialized - BEGIN
    this.mProtocolID = 0;
    this.mLayer0Datagram = null;
    //Not Serialized - END

    if (this.mToID == null)
      this.mToID = new ArrayBuffer();
    if (this.mTimestamp == 0)
      this.mTimestamp = this.mTools.getTime();
  }
  set protocolID(id)
  {
    this.mProtocolID = id;//taken from CNetMsg
  }

  get layer0Datagram()
  {
      return this.mLayer0Datagram;
  }

  set layer0Datagram(datagram) //i.e. the CNetMsg
  {
    this.mLayer0Datagram = datagram;
  }

  get protocolID()
  {
    return this.mProtocolID;
  }
  get dataTxt() {
    this.mTools.arrayBufferToString(this.mData);
  }

  set data(val)
  {

    this.mData =  gTools.convertToArrayBuffer(val);
  }

  get dataBytes() {
    return this.mData;
  }

  get type() {
    return this.mType;
  }
  get sourceID() {
    return this.mFromID;
  }

  get destinationID() {
    return this.mToID;
  }
  get timestamp() {
    return this.mTimestamp;
  }
  //Returns a HTML5 rendering of the encapsulated message
  getRendering(sanitize = true) {
    if (this.mType != eSwarmMsgType.text)
      return null; // For now

    try {
      let rendering = msgTemplate;

      // Sanitize and replace fields in the template
      let originFlag = this.mExternal ? 'you' : 'me';
      let statusField = 'status green';
      let sourceField = this.mExternal ? this.mTools.arrayBufferToString(this.mFromID) : 'me';
      let timeField = this.mTools.timestampToString(this.mTimestamp);
      let msgTxtField = this.mTools.arrayBufferToString(this.mData);

      if (sanitize) {
        originFlag = DOMPurify.sanitize(originFlag, {ALLOWED_TAGS: ['b','i']});
        statusField = DOMPurify.sanitize(statusField, {ALLOWED_TAGS: ['b','i']});
        sourceField = DOMPurify.sanitize(sourceField, {ALLOWED_TAGS: ['b','i']});
        timeField = DOMPurify.sanitize(timeField, {ALLOWED_TAGS: ['b','i']});
        msgTxtField = DOMPurify.sanitize(msgTxtField, {ALLOWED_TAGS: ['b','i']});
      }

      rendering = rendering.replace('[originFlagField]', originFlag);
      rendering = rendering.replace('[statusField]', statusField);
      rendering = rendering.replace('[sourceField]', sourceField);
      rendering = rendering.replace('[timeField]', timeField);
      rendering = rendering.replace('[msgTxtField]', msgTxtField);

      return rendering;
    } catch (err) {
      return null;
    }
  }


  initialize() {
    if (this.mSwarm == null || this.mInitialized)
      return false;

    this.mInitialized = true;
    return true;
  }

  newDataChannelMessageEventHandler(eventData) {

  }
  get version() {
    return this.mVersion;
  }
  get sig() {
    return this.mSig;
  }

  sign(privKey) {
    let concat = new CDataConcatenator();
    concat.add(this.mFromID);
    concat.add(this.mToID);
    concat.add(this.mVersion);
    concat.add(this.mData);
    concat.add(this.mTimestamp);
    concat.add(this.mType);

    let sig = gCrypto.sign(privKey, concat.getData()); //Do NOT rely on BER encoding of data. that might differ slightly across implementations.this.getPackedData(false));
    if (sig.byteLength == 64) {
      this.mSig = sig;
      return true;
    } else
      return false;
  }

  verifySignature(pubKey) {
    let concat = new CDataConcatenator();
    concat.add(this.mFromID);
    concat.add(this.mToID);
    concat.add(this.mVersion);
    concat.add(this.mData);
    concat.add(this.mTimestamp);
    concat.add(this.mType);
    if (gCrypto.verify(pubKey, concat.getData(), mSig))
      return true;
    else
      return false;
  }

  getPackedData(includeSig = true) {

    if (this.mSig == null)
      this.mSig = new ArrayBuffer();
    //we need to construct encoding iteratively due to optional fields
    let wrapperSeq = new asn1js.Sequence();
    wrapperSeq.valueBlock.value.push(new asn1js.Integer({
      value: this.mVersion
    }));

    let mainDataSeq = new asn1js.Sequence();
    mainDataSeq.valueBlock.value.push(new asn1js.Integer({
      value: this.mType
    }));

    mainDataSeq.valueBlock.value.push(new asn1js.OctetString({
      valueHex: this.mFromID
    }));
    mainDataSeq.valueBlock.value.push(new asn1js.OctetString({
      valueHex: this.mToID
    }));

    mainDataSeq.valueBlock.value.push(new asn1js.OctetString({
      valueHex: this.mTools.convertToArrayBuffer(this.mData)
    }));

    mainDataSeq.valueBlock.value.push(new asn1js.Integer({
      value: this.mTimestamp
    }));

    if (includeSig) {
      mainDataSeq.valueBlock.value.push(new asn1js.OctetString({
        valueHex: this.mSig
      }));
    }

    wrapperSeq.valueBlock.value.push(mainDataSeq);

    var bytes = wrapperSeq.toBER(false);
    var length = bytes.byteLength;
    return bytes;
  }

  static instantiate(packedData) {
    try {
      if (packedData.byteLength == 0)
        return 0;
      //local variables - BEGIN
      let toRet = new CSwarmMsg();
      let decoded_sequence1, decoded_sequence2, decoded_asn1;
      //local variables - END

      decoded_asn1 = asn1js.fromBER(packedData);

      if (decoded_asn1.offset === (-1))
        return 0; // Error during decoding

      decoded_sequence1 = decoded_asn1.result;
      toRet.mVersion = decoded_sequence1.valueBlock.value[0].valueBlock.valueDec;

      if (toRet.mVersion == 1) {
        decoded_sequence2 = decoded_sequence1.valueBlock.value[1];
        toRet.mType = decoded_sequence2.valueBlock.value[0].valueBlock.valueDec;
        toRet.mFromID = decoded_sequence2.valueBlock.value[1].valueBlock.valueHex;
        toRet.mToID = decoded_sequence2.valueBlock.value[2].valueBlock.valueHex;
        toRet.mData = decoded_sequence2.valueBlock.value[3].valueBlock.valueHex;
        toRet.mTimestamp = decoded_sequence2.valueBlock.value[4].valueBlock.valueDec;

        //decode the optional Variables
        if (decoded_sequence2.valueBlock.value.length > 5)
          toRet.mSig = decoded_sequence2.valueBlock.value[5].valueBlock.valueHex;
      }


      return toRet;
    } catch (error) {
      return false;
    }
  }
}
