import { inherits } from 'util';
import * as native from '../../binding';
import { EventTarget } from './eventtarget';
export const RTCAudioSink = native.RTCAudioSink;
export type RTCAudioSink = typeof RTCAudioSinkT;
declare class RTCAudioSinkT { 
    constructor(track: MediaStreamTrack);
    stop(): void;
    readonly stopped: boolean;
    ondata: (data: Buffer) => void;
}
inherits(native.RTCAudioSink, EventTarget);