import { RTCEvent } from "./rtc-event";

export class RTCDataChannelMessageEvent<T = any> extends RTCEvent implements MessageEvent<T> {
    constructor(type: string, init: MessageEventInit<T> & { data: T }) {
        super(type, init);

        this._origin = init.origin ?? '';
        this._lastEventId = init.lastEventId ?? '';
        this._ports = init.ports ?? [];
        this._source = init.source ?? null;
        this._data = init.data;
    }

    private _lastEventId: string = '';
    get lastEventId() { return this._lastEventId; }

    private _origin: string = '';
    get origin() { return this._origin; }

    private _ports: readonly MessagePort[] = [];
    get ports() { return this._ports; }

    private _source: MessageEventSource | null = null;
    get source() { return this._source; }

    private _data: T;
    get data() { return this._data; }

    initMessageEvent(type: string, bubbles?: boolean, cancelable?: boolean, data?: T, origin?: string, lastEventId?: string, source?: MessageEventSource, ports?: MessagePort[]): void {
        this.initEvent(type, bubbles, cancelable);
        this._data = data as T;
    }
}