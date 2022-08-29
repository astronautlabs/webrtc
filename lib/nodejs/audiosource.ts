import * as native from '../../binding';
export const RTCAudioSource: typeof RTCAudioSourceT = native.RTCAudioSource;
export type RTCAudioSource = RTCAudioSourceT;

export interface RTCAudioData {
  samples: Int16Array;
  sampleRate: number;
  bitsPerSample?: number;
  channelCount?: number;
  numberOfFrames?: number;
}
declare class RTCAudioSourceT { 
    createTrack(): MediaStreamTrack;
    onData(data: RTCAudioData): void;
}
