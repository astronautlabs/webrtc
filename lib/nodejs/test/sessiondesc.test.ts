import { expect } from 'chai';
import { describe, it } from '@jest/globals';
import {
  RTCPeerConnection,
  RTCSessionDescription
} from '..';

let peer: any;
let localDesc: any;

describe('RTCSessionDescription', () => {

  it('create a peer connection', () => {
    peer = new RTCPeerConnection({ iceServers: [] });
    expect(peer).to.be.instanceOf(RTCPeerConnection);
  });
  
  it('createOffer', async () => {
    let desc = await peer.createOffer();
    // save the local description
    localDesc = desc;

    // run the checks
    expect(desc).to.exist;
    expect(desc.type).to.equal('offer');
    expect(desc.sdp).to.exist;
  });
  
  it('setLocalDescription with a created RTCSessionDescription', async () => {
    await peer.setLocalDescription(
      new RTCSessionDescription({ sdp: localDesc.sdp, type: 'offer' })
    );
    
    expect(peer.localDescription).to.exist;
    expect(peer.localDescription.sdp).to.exist;
  });
  it('TODO: cleanup connection', () => peer.close());
});
