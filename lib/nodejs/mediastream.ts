import { inherits } from 'util';
import * as native from '../../binding';
import { EventTarget } from './eventtarget';
export const MediaStream: (typeof globalThis.MediaStream & typeof MediaStreamExtensions) = native.MediaStream;
export type MediaStream = globalThis.MediaStream;
declare class MediaStreamExtensions extends globalThis.MediaStream {
    constructor(options: { id: string });
}
inherits(native.MediaStream, EventTarget);