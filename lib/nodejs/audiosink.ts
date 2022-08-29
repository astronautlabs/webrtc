import { inherits } from 'util';
import * as native from '../../binding';
import { EventTarget } from './eventtarget';
export const RTCAudioSink = native.RTCAudioSink;
export type RTCAudioSink = typeof RTCAudioSinkT;
declare class RTCAudioSinkT { 
    // TODO
}
inherits(native.RTCAudioSink, EventTarget);