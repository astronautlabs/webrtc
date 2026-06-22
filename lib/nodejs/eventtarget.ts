/**
 * @author mrdoob / http://mrdoob.com/
 * @author Jesús Leganés Combarro "Piranna" <piranna@gmail.com>
 */

export class EventTarget {
    private _listeners: Record<string, Set<(EventListener | { handleEvent: (event: Event) => void })>> = {};

    addEventListener(type: string, listener: EventListener) {
        const listeners = this._listeners = this._listeners || {};
        if (!listeners[type])
            listeners[type] = new Set();
        listeners[type].add(listener);
    };

    dispatchEvent(event: Event) {
        let listeners = this._listeners ||= {};

        process.nextTick(() => {
            let responders = new Set(listeners[event.type] || []);

            const dummyListener = (this as any)['on' + event.type];
            if (typeof dummyListener === 'function') {
                responders.add(dummyListener);
            }

            responders.forEach(listener => {
                if (typeof listener === 'object' && typeof listener.handleEvent === 'function') {
                    listener.handleEvent(event);
                } else if (typeof listener === 'function') {
                    listener.call(this, event);
                }
            });
        });
    }

    removeEventListener(type: string, listener: EventListener | { handleEvent: (event: Event) => void }) {
        const listeners = this._listeners = this._listeners || {};
        if (listeners[type]) {
            listeners[type].delete(listener);
        }
    }
}

export function mixinEventTarget(superklass: any) {
    return class EventTarget extends superklass {
        #listeners: Record<string, Set<(EventListener | { handleEvent: (event: Event) => void })>> = {};

        addEventListener(type: string, listener: EventListener) {
            const listeners = this.#listeners ??= {};
            if (!listeners[type])
                listeners[type] = new Set();
            listeners[type].add(listener);
        };

        dispatchEvent(event: Event) {
            let listeners = this.#listeners ??= {};

            process.nextTick(() => {
                let responders = new Set(listeners[event.type] || []);

                const dummyListener = (this as any)['on' + event.type];
                if (typeof dummyListener === 'function') {
                    responders.add(dummyListener);
                }

                responders.forEach(listener => {
                    if (typeof listener === 'object' && typeof listener.handleEvent === 'function') {
                        listener.handleEvent(event);
                    } else if (typeof listener === 'function') {
                        listener.call(this, event);
                    }
                });
            });
        }

        removeEventListener(type: string, listener: EventListener | { handleEvent: (event: Event) => void }) {
            const listeners = this.#listeners ??= {};
            if (listeners[type]) {
                listeners[type].delete(listener);
            }
        }
    }
}