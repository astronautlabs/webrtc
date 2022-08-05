import { expect } from 'chai';
import { describe } from 'razmin';
import { RTCVideoSink, RTCVideoSource } from '..';
import { I420Frame } from './lib/frame';

describe('RTCVideoSink', it => {
  it('works', async () => {
    const source = new RTCVideoSource();
    const track = source.createTrack();
    const sink = new RTCVideoSink(track);
    expect(sink.stopped).to.be.false;
    const inputFrame = new I420Frame(160, 120);
    const outputFramePromise = new Promise<I420Frame>(resolve => { sink.onframe = ({ frame }) => resolve(frame); });
    source.onFrame(inputFrame);

    let outputFrame = await outputFramePromise;

    expect(inputFrame.width).to.equal(outputFrame.width);
    expect(inputFrame.height).to.equal(outputFrame.height);
    expect(inputFrame.data).to.eql(outputFrame.data);
    sink.stop();
    expect(sink.stopped).to.be.true;
    track.stop();
  });
});
