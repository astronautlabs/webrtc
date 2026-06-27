/**
 * @author mrdoob / http://mrdoob.com/
 * @author Jesús Leganés Combarro "Piranna" <piranna@gmail.com>
 */

type EventHandler = ((event: Event) => void) | { handleEvent(event: Event): void; };

const EMITTER = Symbol('EMITTER');
const GET_EMITTER = Symbol('GET_EMITTER');

export class EventEmitter {
    constructor(private subject?: any) {
        if (!this.subject)
            this.subject = this;
    }

    static mixin(constructor: any) {
        Object.assign(
            constructor.prototype, 
            {
                [GET_EMITTER]() {
                    return (this as any)[EMITTER] ??= new EventEmitter(this);
                },
                addEventListener(type: string, listener: (ev: Event) => void) { 
                    return this[GET_EMITTER]().addEventListener(type, listener); 
                },
                dispatchEvent(event: Event) { 
                    return this[GET_EMITTER]().dispatchEvent(event); 
                },
                removeEventListener(type: string, listener: (ev: Event) => void) { 
                    return this[GET_EMITTER]().removeEventListener(type, listener); 
                }
            }
        );
    }

    private _listeners: Record<string, Set<(event: any) => void>> = {};

    addEventListener(type: string, listener: (event: Event) => void) {
        this._listeners ??= {};
        this._listeners[type] ??= new Set();
        this._listeners[type].add(listener);
    };

    dispatchEvent(event: Event) {
        let listenerMap = this._listeners ??= {};
        console.log(`DISPATCH ${event.type} on next tick `);
        process.nextTick(() => {
            let listeners = new Set(listenerMap[event.type] || []);
            const dummyListener = this.subject['on' + event.type];
            if (typeof dummyListener === 'function') {
                listeners.add(dummyListener);
            }

            console.log(`DISPATCH ${event.type} NOW to ${listeners.size}`);
            listeners.forEach((listener: EventHandler) => {
                if (typeof listener === 'object' && typeof listener.handleEvent === 'function') {
                    listener.handleEvent(event);
                } else if (typeof listener === 'function') {
                    listener.call(this.subject, event);
                }
            });
        });

        return true;
    }

    removeEventListener(type: string, listener: EventListener) {
        this._listeners?.[type]?.delete(listener);
    }
}