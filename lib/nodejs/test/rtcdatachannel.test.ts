import { expect } from 'chai';
import { describe } from 'razmin';
import { RTCPeerConnection } from '..';

describe('RTCDataChannel', it => {
  it('Calling .send(message) when .readyState is "closed" throws InvalidStateError', () => {
    const pc = new RTCPeerConnection();
    const dc = pc.createDataChannel('hello');
    pc.close();
    expect(() => dc.send('world'))
      .to.throw(/RTCDataChannel.readyState is not 'open'/);
  });
  it('.maxPacketLifeTime', () => {
    const pc = new RTCPeerConnection();
    const dc1 = pc.createDataChannel('dc1');
    const dc2 = pc.createDataChannel('dc2', { maxPacketLifeTime: 0 });
    expect(dc1.maxPacketLifeTime).to.equal(65535);
    expect(dc2.maxPacketLifeTime).to.equal(0);
    pc.close();
  });
  it('.negotiated', () => {
    const pc = new RTCPeerConnection();
    const dc1 = pc.createDataChannel('dc1');
    const dc2 = pc.createDataChannel('dc2', { negotiated: true });
    expect(dc1.negotiated).to.equal(false);
    expect(dc2.negotiated).to.equal(true);
    pc.close();
  });
});
