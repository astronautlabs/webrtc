/* eslint no-undefined:0 */

import { expect } from 'chai';
import { describe, it } from 'razmin';

import { RTCAudioSink, RTCAudioSource } from '..';

function createData(bitsPerSample) {
  const sampleRate = 8000;
  const channelCount = 1;
  const numberOfFrames = sampleRate / 100;  // 10 ms

  const length = channelCount * numberOfFrames;
  const byteLength = length * bitsPerSample / 8;

  const samples = {
    8: new Int8Array(length),
    16: new Int16Array(length),
    32: new Int32Array(length)
  }[bitsPerSample] || new Uint8Array(byteLength);

  samples[0] = -1 * Math.pow(2, bitsPerSample) / 2;

  return {
    samples,
    sampleRate,
    bitsPerSample,
    channelCount,
    numberOfFrames
  };
}

function createTest(bitsPerSample) {
  it(`(${bitsPerSample}-bit)`, async () => {
    const source = new RTCAudioSource();
    const track = source.createTrack();
    expect(track.enabled).to.be.true;
    const sink = new RTCAudioSink(track);
    const receivedDataPromise = new Promise(resolve => { sink.ondata = resolve; });
    const data = createData(bitsPerSample);
    expect(source.onData(data)).to.be.undefined;
    let receivedData = await receivedDataPromise;
    expect(receivedData).to.eql(Object.assign({ type: 'data' }, data));
    track.stop();
    sink.stop();
  });
}

describe('RTCAudioSource', it => {
  // createTest(8);
  createTest(16);
  // createTest(32);
  // createTest(64);
});
