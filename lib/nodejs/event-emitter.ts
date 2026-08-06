/**
 * @author mrdoob / http://mrdoob.com/
 * @author Jesús Leganés Combarro "Piranna" <piranna@gmail.com>
 */

import { debug as debugSetup } from 'debug';

const debug = debugSetup('webrtc');
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

        // Dispatch synchronously. This method is already invoked from the native layer via
        // napi_make_callback (which drains the microtask/nextTick queue inline when its callback
        // scope closes), so deferring with process.nextTick bought no additional yielding — it only
        // added a scheduling hop. Loop fairness is enforced natively by bounding the work done per
        // uv_async wakeup (see AsyncObjectWrapWithLoop::Run), so running listeners directly here is
        // both cheaper and more predictable in ordering. The listener set is snapshotted first so a
        // handler that adds/removes listeners during dispatch doesn't mutate the set mid-iteration.
        let listeners = new Set(listenerMap[event.type] || []);
        const dummyListener = this.subject['on' + event.type];
        if (typeof dummyListener === 'function') {
            listeners.add(dummyListener);
        }

        // NB: never serialize the event itself here. Media events (e.g. a video 'frame') carry a
        // multi-megabyte raw pixel buffer, and a template literal is evaluated eagerly — so
        // `JSON.stringify(event)` would serialize ~1.4MB of pixel data into a huge string on EVERY
        // frame even when this debug namespace is disabled. That alone consumed ~190ms/frame and was
        // the dominant cause of event-loop starvation under live media. Log only cheap scalars.
        debug(`dispatch event ${event.type} to ${listeners.size} listeners`);
        listeners.forEach((listener: EventHandler) => {
            if (typeof listener === 'object' && typeof listener.handleEvent === 'function') {
                listener.handleEvent(event);
            } else if (typeof listener === 'function') {
                listener.call(this.subject, event);
            }
        });

        return true;
    }

    removeEventListener(type: string, listener: EventListener) {
        this._listeners?.[type]?.delete(listener);
    }
}