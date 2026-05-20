import { inherits } from 'util';
import native from '../../binding';
import { EventTarget } from './eventtarget';

export const RTCDataChannel: typeof globalThis.RTCDataChannel = native.RTCDataChannel;
export type RTCDataChannel = globalThis.RTCDataChannel;

inherits(native.RTCDataChannel, EventTarget);