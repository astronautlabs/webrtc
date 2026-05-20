import { RTCEvent } from "./rtc-event";

export class RTCPeerConnectionIceErrorEvent extends RTCEvent implements globalThis.RTCPeerConnectionIceErrorEvent {
    constructor(type: string, eventInitDict: RTCPeerConnectionIceErrorEventInit & { target: EventTarget | null }) {
        super(type, eventInitDict);

        this.address = eventInitDict.address ?? null;
        this.port = eventInitDict.port ?? null;
        this.url = eventInitDict.url ?? '';
        this.errorCode = eventInitDict.errorCode ?? null;
        this.errorText = eventInitDict.errorText ?? '';
        this.target = eventInitDict.target;
    }

    readonly address: string | null;
    readonly errorCode: number;
    readonly errorText: string;
    readonly port: number | null;
    readonly url: string;
    readonly target: EventTarget | null;
}