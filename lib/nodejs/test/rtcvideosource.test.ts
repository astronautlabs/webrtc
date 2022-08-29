/* globals gc */

import { RTCVideoSource } from '..';
import { confirmSentFrameDimensions, negotiateRTCPeerConnections } from './lib/pc';
import { I420Frame } from './lib/frame';
import { describe } from 'razmin';
import { expect } from 'chai';

const frame = new I420Frame(640, 480);

function tick() {
  return new Promise(resolve => setTimeout(resolve));
}

function printSource(source) {
  console.log(source);
}

function printTrack(track) {
  console.log(track);
}

describe('RTCVideoSource', it => {
  it('simple usage', async () => {
    await (async () => {
      const source = new RTCVideoSource();
      printSource(source);
      await tick();
  
      source.onFrame(frame);
      await tick();
  
      const track = source.createTrack();
      printTrack(track);
      await tick();
  
      const clonedTrack = track.clone();
      printTrack(clonedTrack);
      await tick();
  
      source.onFrame(frame);
      await tick();
  
      track.stop();
      printTrack(track);
      await tick();
  
      source.onFrame(frame);
      await tick();
  
      clonedTrack.stop();
      printTrack(clonedTrack);
      await tick();
  
      source.onFrame(frame);
      await tick();
    })();
  
    if (typeof gc === 'function') {
      gc();
    }
  });
  
  it.skip('getStats()', async () => {
    const source = new RTCVideoSource();
    const track = source.createTrack();
  
    const [pc1, pc2] = await negotiateRTCPeerConnections({
      withPc1(pc1) {
        pc1.addTrack(track);
      }
    });
  
    const frames = [
      new I420Frame(640, 480),
      new I420Frame(1280, 720),
      new I420Frame(320, 240)
    ];
  
    for (const frame of frames) {
      await confirmSentFrameDimensions(source, track, pc1, frame);
      //t.pass(`Sent a ${frame.width}x${frame.height} frame`);
    }
  
    track.stop();
    pc1.close();
    pc2.close();
  });
  
  it('constructor', () => {
    const source1 = new RTCVideoSource();
    expect(source1.needsDenoising).to.be.null;
    expect(source1.isScreencast).to.be.false;
  
    const source2 = new RTCVideoSource({ needsDenoising: true });
    expect(source2.needsDenoising).to.be.true;
    expect(source2.isScreencast).to.be.false;
  
    const source3 = new RTCVideoSource({ needsDenoising: false });
    expect(source3.needsDenoising).to.be.false;
    expect(source3.isScreencast).to.be.false;
  
    const source4 = new RTCVideoSource({ isScreencast: true });
    expect(source4.needsDenoising).to.be.null;
    expect(source4.isScreencast).to.be.true;
  });
});
