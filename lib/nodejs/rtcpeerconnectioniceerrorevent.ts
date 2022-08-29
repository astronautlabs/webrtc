export class RTCPeerConnectionIceErrorEvent implements globalThis.RTCPeerConnectionIceErrorEvent {
  constructor(readonly type: string, eventInitDict: RTCPeerConnectionIceErrorEventInit) {
    this.address = eventInitDict.address ?? null;
    this.port = eventInitDict.port ?? null;
    this.url = eventInitDict.url ?? '';
    this.errorCode = eventInitDict.errorCode ?? null;
    this.errorText = eventInitDict.errorText ?? '';
    this.target = eventInitDict['target'] ?? null;
  }

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
  timeStamp: number;
  address: string | null;
  errorCode: number;
  errorText: string;
  port: number | null;
  url: string;
  target: EventTarget | null;

  AT_TARGET: number;
  BUBBLING_PHASE: number;
  CAPTURING_PHASE: number;
  NONE: number;

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
}