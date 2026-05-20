import native from '../../binding';
import { RTCAudioDataInit } from './audiodata';
export const RTCAudioSource: typeof RTCAudioSourceT = native.RTCAudioSource;
export type RTCAudioSource = RTCAudioSourceT;

declare class RTCAudioSourceT { 
    createTrack(): MediaStreamTrack;
    onData(data: RTCAudioDataInit): void;
}
