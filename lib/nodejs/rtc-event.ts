/**
 * Base class for events emitted from this library.
 */
export class RTCEvent implements Event {
    constructor(type: string, eventInit?: EventInit) {
        this._type = type;
        this.bubbles = eventInit?.bubbles ?? false;
        this.cancelable = eventInit?.cancelable ?? false;
        this.composed = eventInit?.composed ?? false;
    }

    private _type: string;
    get type() { return this._type; }

    composedPath(): EventTarget[] {
        return this.target ? [ this.target ] : [];
    }

    initEvent(type: string, bubbles?: boolean, cancelable?: boolean): void {
        this._type = type;
        this.bubbles = bubbles ?? false;
        this.cancelable = cancelable ?? false;
    }

    preventDefault(): void {
    }
    
    stopImmediatePropagation(): void {
    }

    stopPropagation(): void {
    }

    NONE: 0 = 0;
    CAPTURING_PHASE: 1 = 1;
    AT_TARGET: 2 = 2;
    BUBBLING_PHASE: 3 = 3;
    
    bubbles = false;
    cancelBubble = false;
    cancelable = false;
    composed = false;
    currentTarget: EventTarget | null = null;
    defaultPrevented = false;
    eventPhase = 0;
    isTrusted = false;
    returnValue = true;
    srcElement: EventTarget | null = null;
    target: EventTarget | null = null;
    timeStamp = Date.now();

}