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
import { describe, it } from '@jest/globals';

async function waitUntilOpen(dataChannel: RTCDataChannel) {
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
    let dc1!: RTCDataChannel;
    const dc2Deferred = createDeferred<RTCDataChannel>();

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

describe('RTCPeerConnection', () => {
    it("RTCPeerConnection's destructor fires", async () => {
        const { destructor, stop } = trackDestructors();
        const pc = new RTCPeerConnection();
        pc.close();
        await destructor(pc);

        stop();
    });
    it('Destructors fire in RTCDataChannel use-case', async () => {
        const { destructor, stop } = trackDestructors();

        const { pc1, pc2, dc1, dc2 } = await setupRTCDataChannels();

        const destructors = [
            destructor(pc1),
            destructor(pc2),
            destructor(dc1),
            destructor(dc2),
            destructor(pc1.sctp!),
            destructor(pc2.sctp!),
            destructor(pc1.sctp!.transport),
            destructor(pc2.sctp!.transport),
            destructor(pc1.sctp!.transport.iceTransport),
            destructor(pc2.sctp!.transport.iceTransport)
        ];

        pc1.close();
        pc2.close();

        await Promise.all(destructors);

        stop();
    });
    it('Destructors fire in MediaStreamTrack use-case', async () => {
        const { destructor, stop } = trackDestructors();

        await (async () => {
            const stream1 = await getUserMedia({ audio: true, video: true });

            const [pc1, pc2] = await negotiateRTCPeerConnections({
                withPc1(pc1) {
                    stream1.getTracks().forEach(track => pc1.addTrack(track, stream1));
                }
            });

            const dtlsTransports = pc1.getReceivers().concat(pc2.getReceivers()).map(receiver => receiver.transport!);
            const iceTransports = dtlsTransports.map(dtlsTransport => dtlsTransport.iceTransport);

            const destructors = [
                ...stream1.getTracks().map(destructor),
                ...pc2.getReceivers()
                    .map(receiver => receiver.track)
                    .map(track => destructor(track)),
                // ...pc1.getReceivers().concat(pc2.getReceivers()).map(destructor),
                // ...pc1.getSenders().concat(pc2.getSenders()).map(destructor),
                // ...pc1.getTransceivers().concat(pc2.getTransceivers()).map(destructor),
                destructor(pc1),
                destructor(pc2),
                ...dtlsTransports.map(destructor),
                ...iceTransports.map(destructor)
            ];

            stream1.getTracks().forEach(track => track.stop());
            pc1.close();
            pc2.close();

            return Promise.all(destructors);
        })();

        stop();
    });
});

async function testSink(kind: 'audio' | 'video') {
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

describe('RTCAudioSink', () => {
    it('RTCAudioSink\'s destructor fires', async () => {
        await testSink('audio');
    });
});

describe('RTCVideoSink', () => {
    it('RTCVideoSink\'s destructor fires', async () => {
        await testSink('video');
    });
});

