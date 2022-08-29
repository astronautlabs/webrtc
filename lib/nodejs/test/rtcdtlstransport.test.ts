import { Certificate } from '@fidm/x509';
import { expect } from 'chai';
import { describe } from 'razmin';
import { RTCDtlsTransport, RTCIceTransport, RTCRtpSender, RTCRtpReceiver } from '..';
import { createRTCPeerConnections, gatherCandidates, negotiate, waitForStateChange } from './lib/pc';

async function testDtlsTransport(createSenderOrReceiver: (pc: RTCPeerConnection) => RTCRtpSender | RTCRtpReceiver) {
  const [pc1, pc2] = createRTCPeerConnections({}, {}, { handleIce: false });
  const senderOrReceiver = createSenderOrReceiver(pc1);
  expect(senderOrReceiver.transport).to.be.null;

  const candidates1Promise = gatherCandidates(pc1);
  const candidates2Promise = gatherCandidates(pc2);

  await negotiate(pc1, pc2);

  senderOrReceiver.transport;
  const { transport } = senderOrReceiver;
  expect(transport).to.be.instanceOf(RTCDtlsTransport);
  expect(transport.state).to.equal('new');
  expect(transport.iceTransport).to.be.instanceOf(RTCIceTransport);
  expect(transport.iceTransport.state).to.equal('new');
  expect((transport.iceTransport as any).component).to.equal('rtp');
  expect((transport.iceTransport as any).role).to.equal('controlling');

  const connectingPromise = waitForStateChange(transport, 'connecting');
  const connectedPromise = waitForStateChange(transport, 'connected');
  const candidates = await Promise.all([candidates1Promise, candidates2Promise]);
  await Promise.all(
    candidates[0].map(candidate => pc2.addIceCandidate(candidate)).concat(
    candidates[1].map(candidate => pc1.addIceCandidate(candidate)))
  );

  if (transport.state !== 'connected') {
    await connectingPromise;
  }

  expect(transport.state).to.be.oneOf(['connecting', 'connected']);
  expect(transport.iceTransport.state).to.be.oneOf(['checking', 'connected']);

  await connectedPromise;

  //t.pass('"statechange" fires in state "connected"');

  expect(transport.state).to.equal('connected');
  expect(transport.iceTransport.state).to.equal('connected');

  const remoteCertificates = transport.getRemoteCertificates();
  expect(remoteCertificates.length).to.be.greaterThan(0);
  remoteCertificates.forEach((derBuffer, i) => {
    // NOTE(mroberts): https://stackoverflow.com/a/48309802
    const prefix = '-----BEGIN CERTIFICATE-----\n';
    const postfix = '-----END CERTIFICATE-----';
    const pemText = prefix + Buffer.from(derBuffer).toString('base64').match(/.{0,64}/g)?.join('\n') + postfix;
    const cert = Certificate.fromPEM(<any>pemText);

    expect(cert.issuer.commonName).to.equal('WebRTC');
    //t.pass(`parsed remote certificate ${i + 1}`);
  });

  pc1.close();
  pc2.close();

  expect(senderOrReceiver.transport).to.equal(transport);
  expect(transport.state).to.equal('closed');
  expect(transport.iceTransport).to.be.instanceOf(RTCIceTransport);
  expect(transport.iceTransport.state).to.equal('closed');
  expect((transport.iceTransport as any).component).to.equal('rtp');
  expect((transport.iceTransport as any).role).to.equal('controlling');
}

describe('RTCDtlsTransport', it => {
  it('accessed via RTCRtpSender .transport', () =>
    testDtlsTransport(pc => pc.addTransceiver('audio').sender));
  it('accessed via RTCRtpReceiver .transport', () =>
    testDtlsTransport(pc => pc.addTransceiver('audio').receiver));
});
