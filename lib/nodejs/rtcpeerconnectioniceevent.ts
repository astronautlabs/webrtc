import { RTCEvent } from "./rtc-event";

export class RTCPeerConnectionIceEvent extends RTCEvent implements globalThis.RTCPeerConnectionIceEvent {
  constructor(type: string, eventInitDict: RTCPeerConnectionIceErrorEventInit & { target: EventTarget | null, candidate: RTCIceCandidate | null }) {
    super(type, eventInitDict);
    this.candidate = eventInitDict.candidate;
    this.target = eventInitDict.target;
  }
  
  readonly candidate: RTCIceCandidate | null;
}
