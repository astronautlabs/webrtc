import {
  RTCPeerConnection,
  getUserMedia,
  RTCAudioSink,
  RTCAudioSource,
  RTCVideoSink,
  RTCVideoSource,
} from '..';

import { negotiateRTCPeerConnections } from './lib/pc';
import { createDeferred, trackDestructors } from './destructor/util';
import { describe } from 'razmin';

async function waitUntilOpen(dataChannel) {
  if (dataChannel.readyState === 'open') {
    return;
  }
  await new Promise<void>(resolve => {
    dataChannel.addEventListener('open', function onopen() {
      dataChannel.removeEventListener('open', onopen);
      resolve();
    });
  });
}

async function setupRTCDataChannels() {
  let dc1;
  const dc2Deferred = createDeferred();

  const [pc1, pc2] = await negotiateRTCPeerConnections({
    withPc1(pc1) {
      dc1 = pc1.createDataChannel('test');
    },
    withPc2(pc2) {
      pc2.addEventListener('datachannel', function ondatachannel({ channel }) {
        pc2.removeEventListener('datachannel', ondatachannel);
        dc2Deferred.resolve(channel);
      });
    }
  });

  const dc2 = await dc2Deferred.promise;
  await Promise.all([
    waitUntilOpen(dc1),
    waitUntilOpen(dc2)
  ]);

  return {
    pc1,
    pc2,
    dc1,
    dc2
  };
}

describe('RTCPeerConnection', it => {
  it.skip("RTCPeerConnection's destructor fires", async () => {
    const { destructor, stop } = trackDestructors();
    const pc = new RTCPeerConnection();
    pc.close();
    await destructor(pc['_pc']);

    stop();
  });
  it.skip('Destructors fire in RTCDataChannel use-case', async () => {
    const { destructor, stop } = trackDestructors();
  
    const { pc1, pc2, dc1, dc2 } = await setupRTCDataChannels();
  
    const destructors = [
      destructor(pc1['_pc']),
      destructor(pc2['_pc']),
      destructor(dc1),
      destructor(dc2),
      destructor(pc1.sctp),
      destructor(pc2.sctp),
      destructor(pc1.sctp.transport),
      destructor(pc2.sctp.transport),
      destructor(pc1.sctp.transport.iceTransport),
      destructor(pc2.sctp.transport.iceTransport)
    ];

    pc1.close();
    pc2.close();

    await Promise.all(destructors);
    
    stop();
  });
  it.skip('Destructors fire in MediaStreamTrack use-case', async () => {
    const { destructor, stop } = trackDestructors();
  
    await (async () => {
      const stream1 = await getUserMedia({ audio: true, video: true });
  
      const [pc1, pc2] = await negotiateRTCPeerConnections({
        withPc1(pc1) {
          stream1.getTracks().forEach(track => pc1.addTrack(track, stream1));
        }
      });
  
      const localTracks = stream1.getTracks().map(destructor);
      const remoteTracks = pc2.getReceivers().map(receiver => destructor(receiver.track));
      const senders = pc1.getSenders().concat(pc2.getSenders()).map(destructor);
      const receivers = pc1.getReceivers().concat(pc2.getReceivers()).map(destructor);
      const transceivers = pc1.getTransceivers().concat(pc2.getTransceivers()).map(destructor);
  
      const dtlsTransports = pc1.getReceivers().concat(pc2.getReceivers()).map(receiver => receiver.transport);
      const iceTransports = dtlsTransports.map(dtlsTransport => dtlsTransport.iceTransport);
  
      const destructors = [
        destructor(pc1['_pc']),
        destructor(pc2['_pc'])
      ].concat(
        localTracks
      ).concat(
        remoteTracks
      ).concat(
        senders
      ).concat(
        receivers
      ).concat(
        transceivers
      ).concat(
        dtlsTransports.map(destructor)
      ).concat(
        iceTransports.map(destructor)
      );
  
      stream1.getTracks().forEach(track => track.stop());
      pc1.close();
      pc2.close();
  
      return Promise.all(destructors);
    })();
  
    stop();
  });
});

async function testSink(kind) {
  const { destructor, stop } = trackDestructors();

  const Source = {
    audio: RTCAudioSource,
    video: RTCVideoSource
  }[kind];

  const Sink = {
    audio: RTCAudioSink,
    video: RTCVideoSink
  }[kind];

  await (() => {
    const source = new Source();
    const track = source.createTrack();
    const sink = new Sink(track);

    track.stop();
    sink.stop();

    return Promise.all([
      destructor(track),
      destructor(sink),
    ]);
  })();

  stop();
}

describe('RTCAudioSink', it => {
  it.skip('RTCAudioSink\'s destructor fires', async () => {
    await testSink('audio');
  });
});

describe('RTCVideoSink', it => {
  it.skip('RTCVideoSink\'s destructor fires', async () => {
    await testSink('video');
  });
});

