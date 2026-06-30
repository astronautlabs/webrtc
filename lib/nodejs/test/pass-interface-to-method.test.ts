import { expect } from 'chai';
import { describe, it } from '@jest/globals';
import { RTCPeerConnection } from '..';

describe('Pass interface to method', () => {
  it(': passing something that\'s not a MediaStream fails', () => {
    const pc = new RTCPeerConnection();
    try {
        expect(() => pc.addTransceiver('audio', { streams: [<any>{}] }))
            .to.throw(/Expected instance of node_webrtc::MediaStream/);
    } finally {
        pc.close();
    }
  });
  
  it(': passing something that\'s not a MediaStreamTrack fails', () => {
    const pc = new RTCPeerConnection();
    try {
        expect(() => pc.addTrack(<any>{}))
            .to.throw(/Expected instance of node_webrtc::MediaStreamTrack/);
    } finally {
        pc.close();
    }
  });
  
  it(': passing something that\'s not an RTCRtpSender fails', () => {
    const pc = new RTCPeerConnection();
    try {
        expect(() => pc.removeTrack(<any>{}))
            .to.throw(/Expected instance of node_webrtc::RTCRtpSender/);
    } finally {
        pc.close();
    }
  });
});
