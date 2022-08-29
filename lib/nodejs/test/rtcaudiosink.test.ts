import { expect } from 'chai';
import { describe } from 'razmin';
import { getUserMedia } from '..';
import { RTCAudioSink } from '..';

describe('RTCAudioSink', it => {
  it('works', async () => {
    let stream = await getUserMedia({ audio: true });
  
    const track = stream.getAudioTracks()[0];
    const sink = new RTCAudioSink(track);

    expect(sink.stopped).to.be.false;
    sink.stop();
    expect(sink.stopped).to.be.true;
    track.stop();
  });
});
