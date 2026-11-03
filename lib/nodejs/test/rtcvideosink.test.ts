import { expect } from 'chai';
import { RTCVideoSink, RTCVideoSource, RTCVideoFrame } from '..';
import { I420Frame } from './lib/frame';

describe('RTCVideoSink', () => {
  it('works', async () => {
    const source = new RTCVideoSource();
    const track = source.createTrack();
    const sink = new RTCVideoSink(track);
    expect(sink.stopped).to.be.false;
    const inputFrame = new I420Frame(160, 120);
    const outputFramePromise = new Promise<RTCVideoFrame>(resolve => { sink.onframe = ({ frame }) => resolve(frame); });
    source.onFrame(inputFrame);

    let outputFrame = await outputFramePromise;

    expect(inputFrame.width).to.equal(outputFrame.width);
    expect(inputFrame.height).to.equal(outputFrame.height);

    expect(inputFrame.data.byteLength).to.equal(outputFrame.data.byteLength);

    for (let i = 0, max = inputFrame.data.length; i < max; ++i)
      expect(inputFrame.data[i]).to.equal(outputFrame.data[i]);

    sink.stop();
    expect(sink.stopped).to.be.true;
    track.stop();
  });

  // Odd dimensions have ceil(w/2) x ceil(h/2) chroma planes. (WebRTC's resolution adaptation produces sizes like
  // 720x405, so this isn't hypothetical.)
  for (const [width, height] of [[720, 405], [161, 120], [161, 121]]) {
    it(`round-trips ${width}x${height} frames exactly`, async () => {
      const source = new RTCVideoSource();
      const track = source.createTrack();
      const sink = new RTCVideoSink(track);
      const inputFrame = new I420Frame(width, height);
      for (let i = 0, max = inputFrame.data.length; i < max; ++i)
        inputFrame.data[i] = (i * 7919) & 0xff;

      const outputFramePromise = new Promise<RTCVideoFrame>(resolve => { sink.onframe = ({ frame }) => resolve(frame); });
      source.onFrame(inputFrame);
      const outputFrame = await outputFramePromise;

      expect(outputFrame.width).to.equal(width);
      expect(outputFrame.height).to.equal(height);
      expect(outputFrame.data.byteLength).to.equal(width * height + 2 * ((width + 1) >> 1) * ((height + 1) >> 1));

      let mismatches = 0;
      for (let i = 0, max = inputFrame.data.length; i < max; ++i)
        mismatches += inputFrame.data[i] === outputFrame.data[i] ? 0 : 1;
      expect(mismatches).to.equal(0);

      sink.stop();
      track.stop();
    });
  }
});
