import { inherits } from 'util';
import * as native from '../../binding';
import { EventTarget } from './eventtarget';
export const RTCIceTransport: (typeof globalThis.RTCIceTransport & typeof RTCIceTransportExtensions) = native.RTCIceTransport;
export type RTCIceTransport = RTCIceTransportExtensions;
inherits(native.RTCIceTransport, EventTarget);

declare class RTCIceTransportExtensions extends globalThis.RTCIceTransport {
    component: string;
    role: string;
}