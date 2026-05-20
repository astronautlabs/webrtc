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