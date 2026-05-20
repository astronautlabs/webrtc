import { inherits } from 'util';
import native from '../../binding';
import { EventTarget } from './eventtarget';
import type { RTCIceTransport } from './icetransport';

export type RTCDtlsTransport = globalThis.RTCDtlsTransport & { 
    iceTransport: RTCIceTransport;
};
export const RTCDtlsTransport: RTCDtlsTransport = native.RTCDtlsTransport;

inherits(RTCDtlsTransport, EventTarget);