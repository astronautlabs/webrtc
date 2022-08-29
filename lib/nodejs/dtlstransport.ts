import { inherits } from 'util';
import * as native from '../../binding';
import { EventTarget } from './eventtarget';
import type { RTCIceTransport } from './icetransport';
export const RTCDtlsTransport: (typeof globalThis.RTCDtlsTransport) = native.RTCDtlsTransport;
export type RTCDtlsTransport = RTCDtlsTransportExtensions;
declare class RTCDtlsTransportExtensions extends globalThis.RTCDtlsTransport {
    iceTransport: RTCIceTransport;
}

inherits(RTCDtlsTransport, EventTarget);