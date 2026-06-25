import { inherits } from 'util';
import native from '../../binding';
import { EventTarget, mixinEventTarget } from './eventtarget';
import { RTCVideoFrame } from './videoframe';

export interface RTCVideoSinkEvent extends Event {
    frame: RTCVideoFrame;
}

export class RTCVideoSink extends EventTarget {
    constructor(track: MediaStreamTrack) { super(); }
    stop(): void {}
    readonly stopped!: boolean;
    onframe?: (ev: RTCVideoSinkEvent) => void;
}

(RTCVideoSink as any) = mixinEventTarget(native.RTCVideoSink);
//inherits(native.RTCVideoSink, EventTarget);