export class RTCError {
    constructor(code, message) {
        this.name = this.reasonName[Math.min(code, this.reasonName.length - 1)];
        this.message = typeof message === 'string' ? message : this.name;
    }

    readonly name: string;
    readonly message: string;

    get reasonName() {
        return RTCError.reasonName;
    }

    static get reasonName() {
        return [
            // These strings must match those defined in the WebRTC spec.
            'NO_ERROR', // Should never happen -- only used for testing
            'INVALID_CONSTRAINTS_TYPE',
            'INVALID_CANDIDATE_TYPE',
            'INVALID_STATE',
            'INVALID_SESSION_DESCRIPTION',
            'INCOMPATIBLE_SESSION_DESCRIPTION',
            'INCOMPATIBLE_CONSTRAINTS',
            'INTERNAL_ERROR'
        ];
    }
}