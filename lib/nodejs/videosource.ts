import native from '../../binding';
import { RTCVideoFrameInit } from './videoframe';

export interface RTCVideoSourceInit {
    isScreencast?: boolean;
    needsDenoising?: boolean;
}

export class RTCVideoSource { 
    constructor(init?: RTCVideoSourceInit) {}
    readonly isScreencast!: boolean;
    readonly needsDenoising?: boolean;
    createTrack(): MediaStreamTrack { return undefined as any; }
    onFrame(frame: RTCVideoFrameInit): void { }
}
(RTCVideoSource as any) = native.RTCVideoSource;