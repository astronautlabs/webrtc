import { describe, it } from 'razmin';
import { performance } from 'perf_hooks';

import { RTCVideoSink, RTCVideoSource } from '..';
import { fromBits, toBits } from './lib/bits';
import { I420Frame } from './lib/frame';
import { negotiateRTCPeerConnections } from './lib/pc';
import { printBitsI420, readBitsI420 } from './lib/timestamp';

function average(xs) {
  return xs.reduce((y, x) => y + x, 0) / xs.length;
}

async function measureTimeFromRTCVideoSourceToRTCVideoSink(source, sink, width, height, n) {
  n = typeof n === 'number' ? n : 300;

  const inputFrame = new I420Frame(width, height);
  const timestamps: number[] = [];

  const times: number[] = [];

  sink.onframe = ({ frame }) => {
    if (times.length < n) {
      const receivedAt = performance.now();

      const outputBits = readBitsI420(frame);
      const i = fromBits(outputBits);
      const sentAt = timestamps[i];

      const time = receivedAt - sentAt;
      times.push(time);
    }
  };

  while (times.length < n) {
    const i = times.length;
    const inputBits = toBits(i);
    printBitsI420(inputFrame, inputBits);

    const sentAt = performance.now();
    timestamps[i] = sentAt;

    source.onFrame(inputFrame);

    await new Promise(resolve => setTimeout(resolve));
  }

  return average(times);
}

async function measureTimeFromRTCVideoSourceToLocalRTCVideoSink(width, height, n?) {
  const source = new RTCVideoSource();
  const track = source.createTrack();
  const sink = new RTCVideoSink(track);
  try {
    return await measureTimeFromRTCVideoSourceToRTCVideoSink(source, sink, width, height, n);
  } catch (error) {
    throw error;
  } finally {
    sink.stop();
    track.stop();
  }
}

async function measureTimeFromRTCVideoSourceToRemoteRTCVideoSink(width, height, n?) {
  const source = new RTCVideoSource();
  const localTrack = source.createTrack();
  try {
    const [pc1, pc2] = await negotiateRTCPeerConnections({
      withPc1(pc1) {
        pc1.addTrack(localTrack);
      }
    });
    try {
      const remoteTrack = pc2.getReceivers()[0].track;
      const sink = new RTCVideoSink(remoteTrack);
      try {
        return await measureTimeFromRTCVideoSourceToRTCVideoSink(source, sink, width, height, n);
      } catch (error) {
        throw error;
      } finally {
        sink.stop();
      }
    } catch (error) {
      throw error;
    } finally {
      pc1.close();
      pc2.close();
    }
  } catch (error) {
    throw error;
  } finally {
    localTrack.stop();
  }
}

function testTimeFromRTCVideoSourceToLocalVideoSink(width, height) {
  it(`Average Time from RTCVideoSource to Local RTCVideoSink (${width} x ${height})`, async () => {
    const averageTime = await measureTimeFromRTCVideoSourceToLocalRTCVideoSink(width, height);
    console.log(`    (***) Latency: video|source->sink|local[${width}x${height}]: ${averageTime} ms`);
  });
}

function testTimeFromRTCVideoSourceToRemoteVideoSink(width, height) {
  it(`Average Time from RTCVideoSource to Remote RTCVideoSink (${width} x ${height})`, async () => {
    const averageTime = await measureTimeFromRTCVideoSourceToRemoteRTCVideoSink(width, height);
    console.log(`    (***) Latency: video|source->sink|remote[${width}x${height}]: ${averageTime} ms`);
  });
}

async function measureTimeThroughRTCDataChannel(localDataChannel, remoteDataChannel, n?) {
  n = typeof n === 'number' ? n : 300;

  const times: number[] = [];

  remoteDataChannel.addEventListener('message', ({ data }) => {
    if (times.length < n) {
      const timestamp = Number.parseFloat(data);
      const time = performance.now() - timestamp;
      times.push(time);
    }
  });

  while (times.length < n) {
    const timestamp = performance.now();
    localDataChannel.send(`${timestamp}`);
    await new Promise(resolve => setTimeout(resolve));
  }

  return average(times);
}

async function measureTimeThroughUnorderedUnreliableRTCDataChannel() {
  let localDataChannel: RTCDataChannel | null = null;
  let remoteDataChannelPromise: Promise<RTCDataChannel> | null = null;
  const [pc1, pc2] = await negotiateRTCPeerConnections({
    withPc1(pc1) {
      localDataChannel = pc1.createDataChannel('test', {
        maxRetransmits: 0,
        ordered: false
      });
    },
    withPc2(pc2) {
      remoteDataChannelPromise = new Promise(resolve => {
        pc2.addEventListener('datachannel', ({ channel }) => resolve(channel));
      });
    }
  });
  try {
    const remoteDataChannel = await remoteDataChannelPromise;
    return await measureTimeThroughRTCDataChannel(localDataChannel, remoteDataChannel);
  } catch (error) {
    throw error;
  } finally {
    pc1.close();
    pc2.close();
  }
}

describe('Latency', it => {
  it(': Average Time through Unordered, Unreliable RTCDataChannel', async () => {
    const averageTime = await measureTimeThroughUnorderedUnreliableRTCDataChannel();
    console.log(`    (***) Latency: data-channel: ${averageTime} ms`);
  });
  describe(': RTCVideoSource to', () => {
    describe('LocalVideoSink', it => {
      testTimeFromRTCVideoSourceToLocalVideoSink(160, 120);
      testTimeFromRTCVideoSourceToLocalVideoSink(320, 240);
      testTimeFromRTCVideoSourceToLocalVideoSink(640, 480);
      testTimeFromRTCVideoSourceToLocalVideoSink(1280, 720);    
    });
    describe('RemoteVideoSink', it => {
      testTimeFromRTCVideoSourceToRemoteVideoSink(160, 120);
      testTimeFromRTCVideoSourceToRemoteVideoSink(320, 240);
      testTimeFromRTCVideoSourceToRemoteVideoSink(640, 480);
      testTimeFromRTCVideoSourceToRemoteVideoSink(1280, 720);
    });
  });
});
