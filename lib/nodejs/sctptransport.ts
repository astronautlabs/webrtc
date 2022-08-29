
import { inherits } from 'util';
import * as native from '../../binding';
import { EventTarget } from './eventtarget';
export const RTCSctpTransport: typeof globalThis.RTCSctpTransport = native.RTCSctpTransport;
export type RTCSctpTransport = globalThis.RTCSctpTransport;
inherits(native.RTCSctpTransport, EventTarget);