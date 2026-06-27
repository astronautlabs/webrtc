export function test(): number;
export function setDOMException(constructor: unknown): void;
export const getDisplayMedia: MediaDevices['getDisplayMedia'];
export const getUserMedia: MediaDevices['getUserMedia'];
export function rgbaToI420(rgbaFrame: any, i420Frame: any): void;
export function i420ToRgba(i420Frame: any, rgbaFrame: any): void;
export class MediaStream extends globalThis.MediaStream {
    constructor();
    constructor(stream: MediaStream);
    constructor(tracks: MediaStreamTrack[]);
    constructor(options?: { id: string });
}
export class RTCSessionDescription extends globalThis.RTCSessionDescription {}
export class MediaStreamTrack extends globalThis.MediaStreamTrack {}
export class RTCPeerConnectionFactory {}
export interface RTCIceCandidate extends globalThis.RTCIceCandidate {}
export class RTCDataChannel extends globalThis.RTCDataChannel {}
export class RTCIceTransport extends globalThis.RTCIceTransport {
    component: string;
    role: string;
}
export interface RTCError extends globalThis.RTCError {
    readonly reasonName: string;
}
export class RTCDtlsTransport extends globalThis.RTCDtlsTransport {}
export class RTCPeerConnection extends globalThis.RTCPeerConnection {}
export class RTCRtpReceiver extends globalThis.RTCRtpReceiver {}
export class RTCRtpSender extends globalThis.RTCRtpSender {}
export class RTCRtpTransceiver extends globalThis.RTCRtpTransceiver {}
export class RTCSctpTransport extends globalThis.RTCSctpTransport {}
export class RTCStatsResponse {}
export class RTCStatsReport extends globalThis.RTCStatsReport {
    frameWidth: number;
    frameHeight: number;
}
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
export interface RTCVideoSinkEvent extends Event {
    frame: RTCVideoFrame;
}
export interface RTCVideoSinkEventMap {
    "frame": RTCVideoSinkEvent;
}
export class RTCVideoSink {
    constructor(track: MediaStreamTrack);
    stop(): void;
    readonly stopped: boolean;
    onframe?: (ev: RTCVideoSinkEvent) => void;

    // ~Begin EventEmitter interface
    addEventListener<K extends keyof RTCVideoSinkEventMap>(type: K, listener: (this: RTCPeerConnection, ev: RTCVideoSinkEventMap[K]) => any, options?: boolean | AddEventListenerOptions): void;
    addEventListener(type: string, listener: EventListenerOrEventListenerObject, options?: boolean | AddEventListenerOptions): void;
    addEventListener(type: string, listener: (ev: Event) => void): void;
    dispatchEvent(event: Event): void;
    removeEventListener<K extends keyof RTCVideoSinkEventMap>(type: K, listener: (this: RTCPeerConnection, ev: RTCVideoSinkEventMap[K]) => any, options?: boolean | EventListenerOptions): void;
    removeEventListener(type: string, listener: EventListenerOrEventListenerObject, options?: boolean | EventListenerOptions): void;
    removeEventListener(type: string, listener: (ev: Event) => void): void;
    // ~End EventEmitter interface
}

export interface RTCVideoSourceEventMap {
}

export interface RTCVideoSourceInit {
    isScreencast?: boolean;
    needsDenoising?: boolean;
}

export class RTCVideoSource {
    constructor(init?: RTCVideoSourceInit);
    readonly isScreencast: boolean;
    readonly needsDenoising?: boolean;
    createTrack(): MediaStreamTrack;
    onFrame(frame: RTCVideoFrameInit): void;

    // ~Begin EventEmitter interface
    addEventListener<K extends keyof RTCVideoSourceEventMap>(type: K, listener: (this: RTCPeerConnection, ev: RTCVideoSourceEventMap[K]) => any, options?: boolean | AddEventListenerOptions): void;
    addEventListener(type: string, listener: EventListenerOrEventListenerObject, options?: boolean | AddEventListenerOptions): void;
    addEventListener(type: string, listener: (ev: Event) => void): void;
    dispatchEvent(event: Event): void;
    removeEventListener<K extends keyof RTCVideoSourceEventMap>(type: K, listener: (this: RTCPeerConnection, ev: RTCVideoSourceEventMap[K]) => any, options?: boolean | EventListenerOptions): void;
    removeEventListener(type: string, listener: EventListenerOrEventListenerObject, options?: boolean | EventListenerOptions): void;
    removeEventListener(type: string, listener: (ev: Event) => void): void;
    // ~End EventEmitter interface

}

export interface RTCAudioData {
    samples: Int16Array;
    sampleRate: number;
    bitsPerSample: number;
    channelCount: number;
    numberOfFrames: number;
}

export interface RTCAudioDataInit {
    samples: Int8Array | Int16Array | Int32Array | Uint8Array;
    sampleRate: number;
    /**
     * If not specified, defaults to 16.
     */
    bitsPerSample?: number;

    /**
     * If not specified, defaults to 1.
     */
    channelCount?: number;
    numberOfFrames: number;
}

export interface RTCAudioSinkEvent extends Event, RTCAudioData {
}

export interface RTCAudioSinkEventMap {
    "data": RTCAudioSinkEvent;
}

export class RTCAudioSink {
    constructor(track: MediaStreamTrack);
    stop(): void;
    readonly stopped: boolean;
    ondata: (event: RTCAudioSinkEvent) => void;

    // ~Begin EventEmitter interface
    addEventListener<K extends keyof RTCAudioSinkEventMap>(type: K, listener: (this: RTCPeerConnection, ev: RTCAudioSinkEventMap[K]) => any, options?: boolean | AddEventListenerOptions): void;
    addEventListener(type: string, listener: EventListenerOrEventListenerObject, options?: boolean | AddEventListenerOptions): void;
    addEventListener(type: string, listener: (ev: Event) => void): void;
    dispatchEvent(event: Event): void;
    removeEventListener<K extends keyof RTCAudioSinkEventMap>(type: K, listener: (this: RTCPeerConnection, ev: RTCAudioSinkEventMap[K]) => any, options?: boolean | EventListenerOptions): void;
    removeEventListener(type: string, listener: EventListenerOrEventListenerObject, options?: boolean | EventListenerOptions): void;
    removeEventListener(type: string, listener: (ev: Event) => void): void;
    // ~End EventEmitter interface
}

export interface RTCAudioSourceEventMap {
}

export class RTCAudioSource {
    constructor();
    createTrack(): MediaStreamTrack;
    onData(data: RTCAudioDataInit): void;
    
    // ~Begin EventEmitter interface
    addEventListener<K extends keyof RTCAudioSourceEventMap>(type: K, listener: (this: RTCPeerConnection, ev: RTCAudioSourceEventMap[K]) => any, options?: boolean | AddEventListenerOptions): void;
    addEventListener(type: string, listener: EventListenerOrEventListenerObject, options?: boolean | AddEventListenerOptions): void;
    addEventListener(type: string, listener: (ev: Event) => void): void;
    dispatchEvent(event: Event): void;
    removeEventListener<K extends keyof RTCAudioSourceEventMap>(type: K, listener: (this: RTCPeerConnection, ev: RTCAudioSourceEventMap[K]) => any, options?: boolean | EventListenerOptions): void;
    removeEventListener(type: string, listener: EventListenerOrEventListenerObject, options?: boolean | EventListenerOptions): void;
    removeEventListener(type: string, listener: (ev: Event) => void): void;
    // ~End EventEmitter interface
}

export const path: string;