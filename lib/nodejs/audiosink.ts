import { inherits } from 'util';
import native from '../../binding';
import { RTCAudioData } from './audiodata';
import { EventTarget } from './eventtarget';

export interface RTCAudioSinkEvent extends Event, RTCAudioData {
}

const RTCAudioSinkNative: typeof RTCAudioSink = native.RTCAudioSink;
export { RTCAudioSinkNative as RTCAudioSink };
declare class RTCAudioSink extends EventTarget { 
    constructor(track: MediaStreamTrack);
    stop(): void;
    readonly stopped: boolean;
    ondata: (event: RTCAudioSinkEvent) => void;
}

inherits(native.RTCAudioSink, EventTarget);