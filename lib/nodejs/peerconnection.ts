import * as native from '../../binding';
import { EventEmitter } from './eventemitter';

import { RTCPeerConnectionIceEvent } from './rtcpeerconnectioniceevent';
import { RTCPeerConnectionIceErrorEvent } from './rtcpeerconnectioniceerrorevent';
import { RTCSessionDescription } from './sessiondescription';
import { RTCIceCandidate } from './icecandidate';
import { RTCDataChannelEvent } from './datachannelevent';

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
  return (target, propertyKey, property: PropertyDescriptor) => {
    let handler: Function = target[propertyKey];

    property.value = function (event, ...args) {
      try {
        return handler.apply(this, [ event, ...args ]);
      } catch (e) {
        console.error(`(@/webrtc) INTERNAL: Caught error in managed handler ${handler.name}: ${e.message}`);
        console.error(e);
      }
    }

    return property;
  };
}

export class RTCPeerConnection extends (native.RTCPeerConnection as typeof NRTCPeerConnection) {
  constructor(options?: RTCConfiguration) {
    super(options ?? {});
  }

  private emitter = new EventEmitter(this);
  addEventListener(type, listener) { return this.emitter.addEventListener(type, listener); };
  dispatchEvent(event) { return this.emitter.dispatchEvent(event); }
  removeEventListener(type, listener) { return this.emitter.removeEventListener(type, listener); }
  
  /**
   * @internal
   * @param receiver 
   * @param streams 
   * @param transceiver 
   */
  @ManagedHandler()
  _ontrack(receiver, streams, transceiver) {
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
  _onicecandidate(candidate) {
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
  _ondatachannel(channel) {
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

  addIceCandidate(candidate) {
    var promise = super.addIceCandidate(candidate);
    if (arguments.length === 3) {
      promise.then(arguments[1], arguments[2]);
    }
    return promise;
  }

  createOffer(options)
  createOffer(successCallback, failureCallback)
  createOffer(successCallback, failureCallback, options) 
  createOffer(...args)
  {
    var options = arguments.length === 3
      ? arguments[2]
      : arguments[0];
    var promise = super.createOffer(options || {});
    if (arguments.length >= 2) {
      promise.then(arguments[0], arguments[1]);
    }
    return promise;
  }

  createAnswer()
  createAnswer(options)
  createAnswer(successCallback, failureCallback)
  createAnswer(successCallback, failureCallback, options)
  createAnswer(...argfs) {
    var options = arguments.length === 3
      ? arguments[2]
      : arguments[0];
    var promise = super.createAnswer(options || {});
    if (arguments.length >= 2) {
      promise.then(arguments[0], arguments[1]);
    }
    return promise;
  }

  legacyGetStats: () => Promise<void>;

  getStats() {
    if (typeof arguments[0] === 'function') {
      this.legacyGetStats().then(arguments[0], arguments[1]);
      return;
    }
    return super.getStats();
  }

  setLocalDescription(description) {
    var promise = super.setLocalDescription(description);
    if (arguments.length === 3) {
      promise.then(arguments[1], arguments[2]);
    }
    return promise;
  }

  setRemoteDescription(description) {
    var promise = super.setRemoteDescription(description);
    if (arguments.length === 3) {
      promise.then(arguments[1], arguments[2]);
    }
    return promise;
  }
}

// NOTE(mroberts): This is a bit of a hack.
//RTCPeerConnection.prototype.ontrack = null;