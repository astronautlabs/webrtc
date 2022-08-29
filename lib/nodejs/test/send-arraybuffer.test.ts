import { expect } from 'chai';
import { describe } from 'razmin';
import { RTCPeerConnection } from '..';

// NOTE(mroberts): These limits were tested on macOS.
const maxOrderedAndReliableSize = 262144;
const maxUnorderedAndUnreliableSize = 262144;

async function test(size, options) {
  const pc1 = new RTCPeerConnection();
  const pc2 = new RTCPeerConnection();
  [[pc1, pc2], [pc2, pc1]].forEach(([pc1, pc2]) => {
    pc1.onicecandidate = ({ candidate }) => {
      if (candidate) {
        pc2.addIceCandidate(candidate);
      }
    };
  });

  const dc1 = pc1.createDataChannel('test', options);
  const dc2Promise = new Promise<RTCDataChannel>(resolve => {
    pc2.ondatachannel = ({ channel }) => resolve(channel);
  });

  const offer = await pc1.createOffer({});
  await Promise.all([
    pc1.setLocalDescription(offer),
    pc2.setRemoteDescription(offer)
  ]);

  const answer = await pc2.createAnswer();
  await Promise.all([
    pc2.setLocalDescription(answer),
    pc1.setRemoteDescription(answer)
  ]);

  const dc2 = await dc2Promise;

  const remoteBuf1Promise = new Promise<any>(resolve => {
    dc2.onmessage = ({ data }) => resolve(data);
  });
  const buf1 = new Uint8Array(size);
  buf1.forEach((x, i) => { buf1[i] = Math.random() * 255; });
  dc1.send(buf1);
  const remoteBuf1 = new Uint8Array(await remoteBuf1Promise);

  const remoteBuf2Promise = new Promise<any>(resolve => {
    dc2.onmessage = ({ data }) => resolve(data);
  });
  const buf2 = new Uint8Array(size);
  buf2.forEach((x, i) => { buf2[i] = Math.random() * 255; });
  dc1.send(buf2);
  const remoteBuf2 = new Uint8Array(await remoteBuf2Promise);

  expect(remoteBuf1.length).to.eql(buf1.length);
  expect(remoteBuf1.every(function(x, i) {
    return x === buf1[i];
  })).to.be.true;

  expect(remoteBuf2.length).to.eql(buf2.length);
  expect(remoteBuf2.every(function(x, i) {
    return x === buf2[i];
  })).to.be.true;

  pc1.close();
  pc2.close();
}

describe('Send ArrayBuffer', it => {
  it('receiving two ArrayBuffers works (ordered, reliable)', () => {
    test(maxOrderedAndReliableSize, {});
  });
  
  // if (process.platform !== 'win32') {
  //   it('receiving two ArrayBuffers works (unordered, unreliable)', () => {
  //     test(maxUnorderedAndUnreliableSize, {
  //       ordered: false,
  //       maxRetransmits: 0
  //     });
  //   });
  // }
});
