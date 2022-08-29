import { RTCPeerConnection } from '..';
import { describe } from 'razmin';
import { expect } from 'chai';

describe('RTCDataChannel', it => {
  it('does not segfault when closed after RTCPeerConnection has been garbage collected', () => {
    var dc = (function() {
      var pc = new RTCPeerConnection();
      var dc = pc.createDataChannel('foo');
      pc.close();
      return dc;
    })();
  
    dc.close();
  });
  it('open is not called during closing state', async () => {
    let peer1 = new RTCPeerConnection({ iceServers: [] });
    let peer2 = new RTCPeerConnection({ iceServers: [] });
  
    [[peer1, peer2], [peer2, peer1]].forEach((peers) => {
      peers[0].onicecandidate = function(event) {
        if (event.candidate) {
          peers[1].addIceCandidate(event.candidate);
        }
      };
    });
  
    var channel1 = peer1.createDataChannel('data', { negotiated: true, id: 0 });
    expect(channel1.readyState).to.equal('connecting', 'Expected initial readyState to be "connecting"');
  
    var closeCount = 0;
    var waitingFor = 1;
  
    function ready() {
      expect(channel1.readyState).to.equal('open', 'Expected readyState to be "open" in open');
      --waitingFor;
      if (!waitingFor) {
        peer1.close();
        peer2.close();
      }
    }
  
    function close() {
      ++closeCount;

      expect(closeCount).to.equal(1, 'Expected close count to be 1');
      expect(waitingFor).to.equal(0, 'Expected waiting for to be 0');
      expect(channel1.readyState).to.equal('closed', 'successfully closed');
    }
  
    channel1.onopen = ready;
    channel1.onclose = close;
  
    let offer = await peer1.createOffer({});
    await peer1.setLocalDescription(offer);
    peer2.setRemoteDescription(peer1.localDescription);
    let answer = await  peer2.createAnswer();
    await peer2.setLocalDescription(answer);
    await peer1.setRemoteDescription(peer2.localDescription);
  })
});
