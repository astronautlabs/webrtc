import { inherits } from 'util';
import * as native from '../../binding';
import { EventTarget } from './eventtarget';
export const MediaStreamTrack: typeof globalThis.MediaStreamTrack = native.MediaStreamTrack;
export type MediaStreamTrack = globalThis.MediaStreamTrack;
inherits(native.MediaStreamTrack, EventTarget);