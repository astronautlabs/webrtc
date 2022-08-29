import { inherits } from 'util';
import * as native from '../../binding';
import { EventTarget } from './eventtarget';
export const RTCVideoSink: typeof RTCVideoSinkT = native.RTCVideoSink;
export type RTCVideoSink = typeof RTCVideoSinkT;

export interface RTCVideoSinkEvent extends Event {
    frame: any;
}

declare class RTCVideoSinkT extends EventTarget {
    constructor(track: MediaStreamTrack);
    stop(): void;
    readonly stopped: boolean;
    onframe: (ev: RTCVideoSinkEvent) => void;
}
inherits(native.RTCVideoSink, EventTarget);