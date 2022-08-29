
import * as native from '../../binding';
export const getUserMedia: typeof globalThis.getUserMedia = native.getUserMedia;
export type getUserMedia = typeof globalThis.getUserMedia;
