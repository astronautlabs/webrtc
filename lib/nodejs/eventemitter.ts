/**
 * @author mrdoob / http://mrdoob.com/
 * @author Jesús Leganés Combarro "Piranna" <piranna@gmail.com>
 */

type EventHandler = ((event: Event) => void) | { handleEvent(event: Event): void; };

export class EventEmitter {
    constructor(private subject?: any) {
        if (!this.subject)
            this.subject = this;
    }

    private _listeners: Record<string, any> = {};

    addEventListener(type: string, listener: (event: Event) => void) {
        const listeners = this._listeners = this._listeners || {};

        if (!listeners[type]) {
            listeners[type] = new Set();
        }

        listeners[type].add(listener);
    };

    dispatchEvent(event: Event) {
        let listeners = this._listeners = this._listeners || {};

        process.nextTick(() => {
            listeners = new Set(listeners[event.type] || []);

            const dummyListener = this.subject['on' + event.type];
            if (typeof dummyListener === 'function') {
                listeners.add(dummyListener);
            }

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
        const listeners = this._listeners = this._listeners || {};
        if (listeners[type]) {
            listeners[type].delete(listener);
        }
    }
}