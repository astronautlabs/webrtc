export class RTCDataChannelEvent {
    constructor(readonly type, eventInitDict?: Record<string, any>) {
        this.channel = eventInitDict?.channel;
        this.target = eventInitDict?.target;
    }

    readonly bubbles = false;
    readonly cancelable = false;
    readonly channel;
    readonly target;
}
