import native from '../../binding';
import { EventEmitter } from './eventemitter';

import { RTCPeerConnectionIceEvent } from './rtcpeerconnectioniceevent';
import { RTCPeerConnectionIceErrorEvent } from './rtcpeerconnectioniceerrorevent';
import { RTCSessionDescription } from './sessiondescription';
import { RTCIceCandidate } from './icecandidate';
import { RTCDataChannelEvent } from './datachannelevent';
import { RTCRtpReceiver } from './rtpreceiver';
import { RTCRtpTransceiver } from './rtptransceiver';
import { RTCDataChannel } from './datachannel';

export declare class NRTCPeerConnection extends globalThis.RTCPeerConnection {
}

/**
 * At no point should the managed event handler implementations throw an unexpected exception.
 * This ensures that behavior.
 * @param name 
 * @param handler 
 * @returns 
 */
function ManagedHandler() {
    return (target: any, propertyKey: string | symbol, property: PropertyDescriptor) => {
        let handler: Function = target[propertyKey];

        property.value = function (event: any, ...args: any[]) {
            try {
                return handler.apply(this, [event, ...args]);
            } catch (e: any) {
                console.error(`(@/webrtc) INTERNAL: Caught error in managed handler ${handler.name}: ${e.message}`);
                console.error(e);
            }
        }

        return property;
    };
}

export interface RTCStatsReportExtension {
    frameWidth: number;
    frameHeight: number;
}

export class RTCPeerConnection extends (native.RTCPeerConnection as typeof NRTCPeerConnection) {
    constructor(options?: RTCConfiguration) {
        super(options ?? {});
    }

    private emitter = new EventEmitter(this);
    
    addEventListener<K extends keyof RTCPeerConnectionEventMap>(type: K, listener: (this: RTCPeerConnection, ev: RTCPeerConnectionEventMap[K]) => any, options?: boolean | AddEventListenerOptions): void;
    addEventListener(type: string, listener: EventListenerOrEventListenerObject, options?: boolean | AddEventListenerOptions): void;
    addEventListener(type: string, listener: (ev: Event) => void) { return this.emitter.addEventListener(type, listener); };
    
    dispatchEvent(event: Event) { return this.emitter.dispatchEvent(event); }

    removeEventListener<K extends keyof RTCPeerConnectionEventMap>(type: K, listener: (this: RTCPeerConnection, ev: RTCPeerConnectionEventMap[K]) => any, options?: boolean | EventListenerOptions): void;
    removeEventListener(type: string, listener: EventListenerOrEventListenerObject, options?: boolean | EventListenerOptions): void;
    removeEventListener(type: string, listener: (ev: Event) => void) { return this.emitter.removeEventListener(type, listener); }

    getStats(selector?: MediaStreamTrack | null): Promise<RTCStatsReport & RTCStatsReportExtension> {
        return super.getStats(selector) as any;
    }

    /**
     * @internal
     * @param receiver 
     * @param streams 
     * @param transceiver 
     */
    @ManagedHandler()
    _ontrack(receiver: RTCRtpReceiver, streams: MediaStream[], transceiver: RTCRtpTransceiver) {
        this.dispatchEvent(<any>{
            type: 'track',
            track: receiver.track,
            receiver: receiver,
            streams: streams,
            transceiver: transceiver,
            target: this
        });
    }

    addTrack(track: MediaStreamTrack, ...streams: MediaStream[]): RTCRtpSender {
        return super.addTrack(track, <any>streams);
    }

    /**
     * @internal
     * @param receiver 
     * @param streams 
     * @param transceiver 
     */
    @ManagedHandler()
    _onicecandidate(candidate: RTCIceCandidate) {
        try {
            this.dispatchEvent(new RTCPeerConnectionIceEvent('icecandidate', <any>{
                candidate: new RTCIceCandidate(candidate),
                target: this
            }));
        } catch (e) {
            console.error(`ERROR DURING onicecandidate:`);
            console.error(e);
            throw e;
        }
    }

    /**
     * @internal
     * @param receiver 
     * @param streams 
     * @param transceiver 
     */
    @ManagedHandler()
    _onconnectionstatechange() {
        this.dispatchEvent(<any>{ type: 'connectionstatechange', target: this });
    }

    /**
     * @internal
     * @param receiver 
     * @param streams 
     * @param transceiver 
     */
    @ManagedHandler()
    _onicecandidateerror(event: any) {
        var pair = event.hostCandidate.split(':');
        event.address = pair[0];
        event.port = pair[1];
        event.target = this;
        var icecandidateerror = new RTCPeerConnectionIceErrorEvent('icecandidateerror', event);
        this.dispatchEvent(icecandidateerror);
    }

    /**
     * @internal
     * @param receiver 
     * @param streams 
     * @param transceiver 
     */
    @ManagedHandler()
    _onsignalingstatechange() {
        this.dispatchEvent(<any>{ type: 'signalingstatechange', target: this });
    }

    /**
     * @internal
     * @param receiver 
     * @param streams 
     * @param transceiver 
     */
    @ManagedHandler()
    _oniceconnectionstatechange() {
        this.dispatchEvent(<any>{ type: 'iceconnectionstatechange', target: this });
    }

    /**
     * @internal
     * @param receiver 
     * @param streams 
     * @param transceiver 
     */
    @ManagedHandler()
    _onicegatheringstatechange() {
        try {
            this.dispatchEvent(<any>{ type: 'icegatheringstatechange', target: this });

            // if we have completed gathering candidates, trigger a null candidate event
            if (this.iceGatheringState === 'complete' && this.connectionState !== 'closed') {
                this.dispatchEvent(new RTCPeerConnectionIceEvent('icecandidate', <any>{ candidate: null, target: this }));
            }
        } catch (e) {
            console.error(`ERROR DURING onicegatheringstatechange:`);
            console.error(e);

            throw e;
        }
    }

    /**
     * @internal
     * @param receiver 
     * @param streams 
     * @param transceiver 
     */
    @ManagedHandler()
    _onnegotiationneeded() {
        this.dispatchEvent(<any>{ type: 'negotiationneeded', target: this });
    }

    // [ToDo] onnegotiationneeded

    /**
     * @internal
     * @param receiver 
     * @param streams 
     * @param transceiver 
     */
    @ManagedHandler()
    _ondatachannel(channel: RTCDataChannel) {
        try {
            this.dispatchEvent(new RTCDataChannelEvent('datachannel', <any>{ channel, target: this }));
        } catch (e) {
            console.error(`CAUGHT ERROR DURING ondatachannel:`);
            console.error(e);

            throw e;
        }
    }

    get currentLocalDescription() {
        return super.currentLocalDescription
            ? new RTCSessionDescription(super.currentLocalDescription)
            : null;
    }

    get localDescription() {
        return super.localDescription
            ? new RTCSessionDescription(super.localDescription)
            : null;
    }

    get pendingLocalDescription() {
        return super.pendingLocalDescription
            ? new RTCSessionDescription(super.pendingLocalDescription)
            : null;
    }

    get currentRemoteDescription() {
        return super.currentRemoteDescription
            ? new RTCSessionDescription(super.currentRemoteDescription)
            : null;
    }

    get remoteDescription() {
        return super.remoteDescription
            ? new RTCSessionDescription(super.remoteDescription)
            : null;
    }

    get pendingRemoteDescription() {
        return super.pendingRemoteDescription
            ? new RTCSessionDescription(super.pendingRemoteDescription)
            : null;
    }

    addIceCandidate(candidate: RTCIceCandidate) {
        var promise = super.addIceCandidate(candidate);
        if (arguments.length === 3) {
            promise.then(arguments[1], arguments[2]);
        }
        return promise;
    }

    
    createOffer(options?: RTCOfferOptions): Promise<RTCSessionDescriptionInit>;
    createOffer(successCallback: RTCSessionDescriptionCallback, failureCallback: RTCPeerConnectionErrorCallback, options?: RTCOfferOptions): Promise<void>;
    createOffer(...args: any[]): Promise<any> {
        var options = args.length === 3
            ? args[2]
            : args[0];
        var promise = super.createOffer(options || {});
        if (args.length >= 2) {
            promise.then(args[0], args[1]);
        }
        return promise;
    }

    createAnswer(options?: RTCAnswerOptions): Promise<RTCSessionDescriptionInit>;
    createAnswer(successCallback: RTCSessionDescriptionCallback, failureCallback: RTCPeerConnectionErrorCallback): Promise<void>;
    createAnswer(...args: any[]): Promise<any> {
        var options = args.length === 3
            ? args[2]
            : args[0];
        var promise = super.createAnswer(options || {});
        if (args.length >= 2) {
            promise.then(args[0], args[1]);
        }
        return promise;
    }

    setLocalDescription(description: RTCLocalSessionDescriptionInit) {
        var promise = super.setLocalDescription(description);
        if (arguments.length === 3) {
            promise.then(arguments[1], arguments[2]);
        }
        return promise;
    }

    setRemoteDescription(description: RTCSessionDescriptionInit) {
        var promise = super.setRemoteDescription(description);
        if (arguments.length === 3) {
            promise.then(arguments[1], arguments[2]);
        }
        return promise;
    }
}

// NOTE(mroberts): This is a bit of a hack.
//RTCPeerConnection.prototype.ontrack = null;