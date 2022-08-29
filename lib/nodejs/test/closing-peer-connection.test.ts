import { describe } from 'razmin';
import { RTCPeerConnection } from '..';
import { expect } from 'chai';

describe('RTCPeerConnection', it => {
  it('make sure channel is available after after connection is closed on the other side', async () => {
    var peer1 = new RTCPeerConnection({ iceServers: [] });
    var peer2 = new RTCPeerConnection({ iceServers: [] });
  
    [[peer1, peer2], [peer2, peer1]].forEach(function(peers) {
      peers[0].onicecandidate = function(event) {
        if (event.candidate) {
          peers[1].addIceCandidate(event.candidate);
        }
      };
    });
  
    var channel1 = peer1.createDataChannel('data', { negotiated: true, id: 0 });
    var channel2 = peer2.createDataChannel('data2', { negotiated: true, id: 0 });
    var waitingFor = 2;
  
    function ready() {
      --waitingFor;
      if (!waitingFor) {
        expect(channel2.readyState).to.equal('open');
        channel2.close();

        expect(channel2.readyState === 'closing' || channel2.readyState === 'closed').to.be.true;
        peer2.close();
        setTimeout(function() {
          if (channel1.readyState === 'open') {
            channel1.send('Hello');
          }
          channel1.close();
          peer1.close();
          expect(channel1.readyState)
            .to.equal('closed', 'channel on the other side is also closed, but we did not crash');
        }, 100);
      }
    }
  
    channel1.onopen = ready;
    channel2.onopen = ready;
  
    let offer = await peer1.createOffer({});
    await peer1.setLocalDescription(offer);
    peer2.setRemoteDescription(peer1.localDescription);
    let answer = await peer2.createAnswer();
    await peer2.setLocalDescription(answer);
    peer1.setRemoteDescription(peer2.localDescription);
  });
  
  it('make sure onicecandidate handler doesn\'t fire when connection is closed', async () => {
    await new Promise<void>(async (resolve, reject) => {
      var pc = new RTCPeerConnection({ iceServers: [] });
      pc.onicecandidate = () => { reject(new Error('should not have fired')); };
      pc.close();
      await new Promise<void>(r => setTimeout(r, 100));
      resolve();
    });
  });
});
