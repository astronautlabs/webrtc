import { RTCEvent } from "./rtc-event";

export class RTCDataChannelEvent extends RTCEvent {
    constructor(type: string, eventInitDict: EventInit & { channel: RTCDataChannel, target: EventTarget }) {
        super(type, eventInitDict);
        this.channel = eventInitDict?.channel;
        this.target = eventInitDict.target;
    }

    readonly channel: RTCDataChannel;
    readonly target: EventTarget;
}
