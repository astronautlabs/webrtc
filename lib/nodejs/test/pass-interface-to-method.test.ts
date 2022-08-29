import { expect } from 'chai';
import { describe } from 'razmin';
import { RTCPeerConnection } from '..';

describe('Pass interface to method', it => {
  it(': passing something that\'s not a MediaStream fails', () => {
    const pc = new RTCPeerConnection();
    expect(() => pc.addTransceiver('audio', { streams: [<any>{}] }))
      .to.throw(/This is not an instance of MediaStream/);
    pc.close();
  });
  
  it(': passing something that\'s not a MediaStreamTrack fails', () => {
    const pc = new RTCPeerConnection();
    expect(() => pc.addTrack(<any>{}))
      .to.throw(/This is not an instance of MediaStreamTrack/);
    pc.close();
  });
  
  it(': passing something that\'s not an RTCRtpSender fails', () => {
    const pc = new RTCPeerConnection();
    expect(() => pc.removeTrack(<any>{}))
      .to.throw(/This is not an instance of RTCRtpSender/);
    pc.close();
  });
});
