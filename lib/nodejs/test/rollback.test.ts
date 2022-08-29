import { expect } from 'chai';
import { describe } from 'razmin';
import { RTCPeerConnection } from '..';

describe('rollback', it => {
  it('local rollback (wrong state)', async () => {
    let pc = new RTCPeerConnection(<any>{ sdpSemantics: 'unified-plan' });
    let caughtError;
    try {
      await pc.setLocalDescription({ type: 'rollback' });
    } catch (error) {
      caughtError = error;
    } finally {
      pc.close();
    }
    expect(caughtError).to.exist;
  });
  it('remote rollback (wrong state)', async () => {
    const pc = new RTCPeerConnection(<any>{ sdpSemantics: 'unified-plan' });
    let caughtError;
    try {
      await pc.setRemoteDescription({ type: 'rollback' });
    } catch (error) {
      caughtError = error;
    } finally {
      pc.close();
    }

    expect(caughtError).to.exist;
  });
  it('local rollback', async () => {
    const pc = new RTCPeerConnection(<any>{ sdpSemantics: 'unified-plan' });
    try {
      pc.addTransceiver('audio');
      const localDescription = await pc.createOffer({});
      await pc.setLocalDescription(localDescription);
      expect(pc.signalingState).to.equal('have-local-offer');
      await pc.setLocalDescription({ type: 'rollback' });
      expect(pc.signalingState).to.equal('stable');
    } finally {
      pc.close();
    }
  });
  it('remote rollback', async () => {
    const pc1 = new RTCPeerConnection(<any>{ sdpSemantics: 'unified-plan' });
    const pc2 = new RTCPeerConnection(<any>{ sdpSemantics: 'unified-plan' });
    try {
      pc1.addTransceiver('audio');
      const remoteDescription = await pc1.createOffer({});
      await pc2.setRemoteDescription(remoteDescription);
      expect(pc2.signalingState).to.equal('have-remote-offer');
      await pc2.setRemoteDescription({ type: 'rollback' });
      expect(pc2.signalingState).to.equal('stable');
    } finally {
      pc1.close();
      pc2.close();
    }
  });
  it('remote rollback', async () => {
    const pc1 = new RTCPeerConnection(<any>{ sdpSemantics: 'unified-plan' });
    const pc2 = new RTCPeerConnection(<any>{ sdpSemantics: 'unified-plan' });
    try {
      pc1.addTransceiver('audio');
      const remoteDescription = await pc1.createOffer({});
      await pc2.setRemoteDescription(remoteDescription);
      expect(pc2.signalingState).to.equal('have-remote-offer');
      await pc2.setRemoteDescription({ type: 'rollback' });
      expect(pc2.signalingState).to.equal('stable');
    } finally {
      pc1.close();
      pc2.close();
    }
  });
})
