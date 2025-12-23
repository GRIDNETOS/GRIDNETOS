//todo: RENAME (urgent)
//An SDP Entity. Encapsulates ICE messages as well as provided Swarm-specific signaling.
export class CSDPEntity {
  constructor(cryptoFactory, type = 0, source = new ArrayBuffer(), destination = new ArrayBuffer(), SDPData = new ArrayBuffer(),SDPSessionID = new ArrayBuffer()) {
    this.initFields();
    this.mCrypto = cryptoFactory;
    this.mType = type;
    this.mSDPData = gTools.convertToArrayBuffer(SDPData);
    this.mSDPSessionID = gTools.convertToArrayBuffer(SDPSessionID);
    this.mSourceID = gTools.convertToArrayBuffer(source);
    this.mDestinationID = gTools.convertToArrayBuffer(destination);
  }

  set setSwarmID(id) {
    if (id != null)
      this.mSwarmID = id;
    else {
      this.mSwarmID = new ArrayBuffer();
    }
  }

  get SDPSessionID() {
    return this.mSDPSessionID;
  }

  set SDPSessionID(id) {
    this.mSDPSessionID = id;
  }

  get getSDPData() {
    return this.mSDPData;
  }
  set setSDPData(data) {
    this.mSDPData = data;
  }
  get getCapabilities() {
    return this.mCapabilities;
  }

  set setCapabilities(capabilities) {
    this.mCapabilities = capabilities;
  }

  validate() {
    if (this.mSwarmID == null || this.mSwarmID.byteLength == 0)
      return false;

    if (this.getType != eSDPEntityType.control) { //control datagram needs not to have these. Since it may be issued by the sginaling server itself.
      if (this.mSourceID == null || this.mSourceID.byteLength == 0)
        return false;

      if (this.mDestinationID == null || this.mDestinationID.byteLength == 0)
        return false;

    }

    return true;
  }

  get layer0Datagram() {
    return this.mLayer0Datagram;
  }

  set layer0Datagram(datagram) //i.e. the CNetMsg
  {
    this.mLayer0Datagram = datagram;
  }



  initFields(newSrcSeqNr = true) {

    //Network Layer-0 Data - BEGIN - not serialized.
    //these fields are copy pasted from Layer-0 of the protocol stack (CNetMsg containers).
    //we do not want to hold a reference,thus we copy the particular fields.

    this.mLayer0Datagram = null;

    //Network Layer-0 Data - END
    this.mSDPData = new ArrayBuffer();
    this.mSDPSessionID =  new ArrayBuffer();
    this.mCapabilities = eConnCapabilities.audioVideo;
    this.mSwarmID = new ArrayBuffer();
    this.mSourceID = new ArrayBuffer();
    this.mDestinationID = new ArrayBuffer();
    this.mSig = new ArrayBuffer();
    this.mExtraData = new ArrayBuffer();
    this.mTimeCreated = gTools.getTime();
    this.mType = eSDPEntityType.joining;
    this.mPending = true;
    this.mVersion = 1;
    this.mSeqNr = (newSrcSeqNr == false ? 0 : CVMContext.getInstance().getNewSDPMsgSrcSeq()); //could be overrridden (ex. when deserializing).
    this.mTT = null;
    this.mSig = null;
    this.mStatus = eSDPControlStatus.ok;
  }

  get getTT() {
    return this.mTT;
  }

  set setTT(tt) {
    this.mTT = tt;
  }
  set setStatus(status) {
    this.mStatus = status;
  }

  get getStatus() {
    return this.mStatus;
  }

  validateSignature(pubKey) {
    if (pubKey.byteLength == 0 || this.mSig.byteLength == 0)
      return false;

    let packedData = this.getPackedData(false);

    return this.mCrypto.verifySignatue(this.mSig, packedData, pubKey);
  }

  sign(privKey) {
    let packedData = this.getPackedData(false);
    if (packedData == null)
      return false;

    let sig = this.mCrypto.signData(packedData, privKey);

    if (sig.bytelength == 64) {
      this.mSig = sig;
      return true;
    }

    return false;
  }


   get description() {
    let res = "";
    let tools = CTools.getInstance();
    //Notice: there's special logic for handling of 'control' SDP datagrams.
    res += "{ " + tools.sdpTypeToString(this.mType) + (this.mType == eSDPEntityType.control ? (" - " + tools.sdpControlTypeToString(this.mStatus)) : "") + " } \n";


    res += (" [Datagram-From]: " + (tools.getLength(this.mSourceID) ? tools.arrayBufferToString(this.mSourceID) : 'signaling node') +
        " [To]: " + (tools.getLength(this.mDestinationID) ? tools.arrayBufferToString(this.mDestinationID) : 'implicit')) +
      " [Src Seq]: " + this.mSeqNr;
    return res;
  }

  get getSourceID() {
    return this.mSourceID;
  }

  get getDestinationID() {
    return this.mDestinationID;
  }

  set setSeqNr(seqNr) {
    this.mSeqNr = seqNr;
  }

  get getSeqNr() {
    return this.mSeqNr;
  }
  get getType() {
    return this.mType;
  }

  get getSig() {
    return this.mSig;
  }


  getPackedData(includeSig) {

    let sig = includeSig ? this.mSig : new ArrayBuffer();
    let packedTT = new ArrayBuffer();
    if (this.mTT != null) {
      packedTT = this.mTT.getPackedData();
    }
    //we construct the encoding iteratively
    let wrapperSeq = new asn1js.Sequence();
    wrapperSeq.valueBlock.value.push(new asn1js.Integer({
      value: this.mVersion
    }));
    wrapperSeq.valueBlock.value.push(new asn1js.Integer({
      value: this.mType
    }));

    let mainDataSeq = new asn1js.Sequence();
    mainDataSeq.valueBlock.value.push(new asn1js.Integer({
      value: this.mCapabilities
    }));
    mainDataSeq.valueBlock.value.push(new asn1js.Integer({
      value: this.mStatus
    }));
    mainDataSeq.valueBlock.value.push(new asn1js.Integer({
      value: this.mSeqNr
    }));
    mainDataSeq.valueBlock.value.push(new asn1js.Integer({
      value: this.mTimeCreated
    }));


    mainDataSeq.valueBlock.value.push(new asn1js.OctetString({
      valueHex: this.mSwarmID
    }));
    mainDataSeq.valueBlock.value.push(new asn1js.OctetString({
      valueHex: this.mSourceID
    }));
    mainDataSeq.valueBlock.value.push(new asn1js.OctetString({
      valueHex: this.mDestinationID
    }));
    mainDataSeq.valueBlock.value.push(new asn1js.OctetString({
      valueHex: this.mSDPData
    }));
    mainDataSeq.valueBlock.value.push(new asn1js.OctetString({
      valueHex: this.mSDPSessionID
    }));
    mainDataSeq.valueBlock.value.push(new asn1js.OctetString({
      valueHex: this.mExtraData
    }));
    mainDataSeq.valueBlock.value.push(new asn1js.OctetString({
      valueHex: sig
    }));
    mainDataSeq.valueBlock.value.push(new asn1js.OctetString({
      valueHex: packedTT
    }));

    //fix-sequences into their slots

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
      let toRet = new CSDPEntity();
      let msg, version, typE;
      //local variables - END

      let decodedASN1 = asn1js.fromBER(packedData);

      if (decodedASN1.offset === (-1))
        return 0; // Error during decoding

      let wrapperSeq = decodedASN1.result;
      toRet.mVersion = wrapperSeq.valueBlock.value[0].valueBlock.valueDec;
      toRet.mType = wrapperSeq.valueBlock.value[1].valueBlock.valueDec;
      if (toRet.mVersion == 1) {
        let mainSeq = wrapperSeq.valueBlock.value[2];
        toRet.mCapabilities = mainSeq.valueBlock.value[0].valueBlock.valueDec;
        toRet.mStatus = mainSeq.valueBlock.value[1].valueBlock.valueDec;
        toRet.mSeqNr = mainSeq.valueBlock.value[2].valueBlock.valueDec;
        toRet.mTimeCreated = mainSeq.valueBlock.value[3].valueBlock.valueDec;
        toRet.mSwarmID = mainSeq.valueBlock.value[4].valueBlock.valueHex;
        toRet.mSourceID = mainSeq.valueBlock.value[5].valueBlock.valueHex;
        toRet.mDestinationID = mainSeq.valueBlock.value[6].valueBlock.valueHex;
        toRet.mSDPData = mainSeq.valueBlock.value[7].valueBlock.valueHex;
        toRet.mSDPSessionID = mainSeq.valueBlock.value[8].valueBlock.valueHex;
        toRet.mExtraData = mainSeq.valueBlock.value[9].valueBlock.valueHex;
        toRet.mSig = mainSeq.valueBlock.value[10].valueBlock.valueHex;
      }

      return toRet;
    } catch (error) {
      console.log(error);
      return null;
    }
  }

  get getIsPending() {
    return this.mPending;
  }

  get getTimeCreated() {
    return this.mTimeCreated;
  }

  set setIsPending(isIt) {
    this.mPending = isIt;
  }
  get getSwarmID() {
    return this.mSwarmID;
  }
};
