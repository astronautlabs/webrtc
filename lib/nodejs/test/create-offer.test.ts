import { describe } from 'razmin';
import { expect } from 'chai';
import { RTCPeerConnection } from '..';

describe('RTCPeerConnection', it => {
  var peer;
  var localDesc;

  it('create a peer connection', () => {
    peer = new RTCPeerConnection({ iceServers: [] });
    expect(peer instanceof RTCPeerConnection).to.be.true;
  });
  
  it('createOffer function implemented', () => {
    expect(typeof peer.createOffer).to.equal('function');
  });
  
  it('can call createOffer', async () => {
    let desc = await peer.createOffer();
    // save the local description
    localDesc = desc;

    // run the checks
    expect(desc).to.exist;
    expect(desc.type).to.equal('offer');
    expect(desc.sdp).to.exist;
  });
  
  it('setLocalDescription function implemented', () => {
    expect(typeof peer.setLocalDescription).to.equal('function');
  });
  
  it('can call setLocalDescription', async () => {
    await peer.setLocalDescription(localDesc);
    
    expect(peer.localDescription).to.exist;
    expect(peer.localDescription.sdp).to.exist;
  });
  
  it('cleanup connection', async () => {
    peer.close();
  });
});
