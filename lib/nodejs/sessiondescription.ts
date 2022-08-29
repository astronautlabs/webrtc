export class RTCSessionDescription implements globalThis.RTCSessionDescription {
  constructor(descriptionInitDict: RTCSessionDescriptionInit) {
    if (descriptionInitDict) {
      this.type = descriptionInitDict.type;
      this.sdp = descriptionInitDict.sdp ?? '';
    }
  }

  sdp: string;
  type: RTCSdpType;

  toJSON() {
    return { sdp: this.sdp, type: this.type };
  }
}