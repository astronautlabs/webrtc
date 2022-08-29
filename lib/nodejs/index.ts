import * as native from '../../binding';
try {
  native.setDOMException(require('domexception'));
} catch (error) {
  // Do nothing
}

export * from './audiosink';
export * from './audiosource';
export * from './videosink';
export * from './videosource';
export * from "./datachannel";
export * from "./datachannelevent";
export * from "./icecandidate";
export * from "./peerconnection";
export * from './rtcpeerconnectioniceevent';
export * from './sessiondescription';
export * from "./mediastream";
export * from "./mediastreamtrack";
export * from "./dtlstransport";
export * from "./icetransport";
export * from "./rtpreceiver";
export * from "./rtpsender";
export * from "./rtptransceiver";
export * from "./sctptransport";
export * from "./getusermedia";

import { MediaDevices } from './mediadevices';
export const mediaDevices = new MediaDevices();

import { RTCDataChannel } from './datachannel';

// NOTE(mroberts): Here's a hack to support jsdom's Blob implementation.
RTCDataChannel.prototype.send = function send(data) {
  const implSymbol = Object.getOwnPropertySymbols(data).find(symbol => symbol.toString() === 'Symbol(impl)');
  if (data[implSymbol] && data[implSymbol]._buffer) {
    data = data[implSymbol]._buffer;
  }
  this._send(data);
};