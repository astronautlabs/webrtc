import { inherits } from 'util';
import native from '../../binding';
import { EventTarget } from './eventtarget';
export const MediaStreamTrack: typeof globalThis.MediaStreamTrack = native.MediaStreamTrack;
export type MediaStreamTrack = globalThis.MediaStreamTrack;
inherits(native.MediaStreamTrack, EventTarget);