type NRTCIceCandidate = globalThis.RTCIceCandidate;

const FIELDS = [
    'candidate',
    'sdpMid',
    'sdpMLineIndex',
    'foundation',
    'component',
    'priority',
    'address',
    'protocol',
    'port',
    'type',
    'tcpType',
    'relatedAddress',
    'relatedPort',
    'usernameFragment'
];

export class RTCIceCandidate implements NRTCIceCandidate {
    /**
     * @internal
     */
    constructor(candidateInitDict: Record<string,any>)
    {
        FIELDS.forEach(property => {
            if (candidateInitDict && property in candidateInitDict) {
                this[property] = candidateInitDict[property];
            } else {
                this[property] = null;
            }
        });
    }

    address: string | null;
    candidate: string;
    component: RTCIceComponent | null;
    foundation: string | null;
    port: number | null;
    priority: number | null;
    protocol: RTCIceProtocol | null;
    relatedAddress: string | null;
    relatedPort: number | null;
    sdpMLineIndex: number | null;
    sdpMid: string | null;
    tcpType: RTCIceTcpCandidateType | null;
    type: RTCIceCandidateType | null;
    usernameFragment: string | null;
    
    toJSON(): RTCIceCandidateInit {
        return {
            candidate: this.candidate,
            sdpMLineIndex: this.sdpMLineIndex,
            sdpMid: this.sdpMid,
            usernameFragment: this.usernameFragment
        };
    }
}