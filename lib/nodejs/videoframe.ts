export interface RTCVideoFrame {
    width: number;
    height: number;
    data: Uint8ClampedArray;
    rotation: number;
}

export interface RTCVideoFrameInit {
    width: number;
    height: number;
    data: Uint8ClampedArray;
    rotation?: number;
}