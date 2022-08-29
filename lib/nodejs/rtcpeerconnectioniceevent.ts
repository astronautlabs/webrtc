export class RTCPeerConnectionIceEvent implements globalThis.RTCPeerConnectionIceEvent {
  constructor(readonly type: string, eventInitDict: RTCPeerConnectionIceErrorEventInit) {
    this.candidate = eventInitDict['candidate'];
    this.target = eventInitDict['target'];
  }

  candidate: RTCIceCandidate | null;
  bubbles: boolean;
  cancelBubble: boolean;
  cancelable: boolean;
  composed: boolean;
  currentTarget: EventTarget | null;
  defaultPrevented: boolean;
  eventPhase: number;
  isTrusted: boolean;
  returnValue: boolean;
  srcElement: EventTarget | null;
  target: EventTarget | null;
  timeStamp: number;

  composedPath(): EventTarget[] {
    return [];
  }

  initEvent(type: string, bubbles?: boolean | undefined, cancelable?: boolean | undefined): void {
  }

  preventDefault(): void {
  }

  stopImmediatePropagation(): void {
  }

  stopPropagation(): void {
  }

  AT_TARGET: number;
  BUBBLING_PHASE: number;
  CAPTURING_PHASE: number;
  NONE: number;
}
