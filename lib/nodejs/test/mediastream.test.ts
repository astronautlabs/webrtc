import { expect } from 'chai';
import { describe } from 'razmin';
import { getUserMedia, MediaStream, RTCPeerConnection, RTCSessionDescription } from '..';

var sdp = [
  'v=0',
  'o=- 0 1 IN IP4 0.0.0.0',
  's=-',
  't=0 0',
  'a=group:BUNDLE audio video',
  'a=msid-semantic:WMS *',
  'a=ice-ufrag:0000',
  'a=ice-pwd:0000000000000000000000',
  'a=fingerprint:sha-256 00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00:00',
  'm=audio 9 UDP/TLS/RTP/SAVPF 109 9 0 8 101',
  'c=IN IP4 0.0.0.0',
  'a=mid:audio',
  'a=sendrecv',
  'a=rtpmap:109 opus/48000/2',
  'a=rtpmap:9 G722/8000/1',
  'a=rtpmap:0 PCMU/8000',
  'a=rtpmap:8 PCMA/8000',
  'a=rtpmap:101 PCMA/16000',
  'a=rtcp-mux',
  'a=ssrc:1 cname:0',
  'a=ssrc:1 msid:stream 123',
  'm=video 9 UDP/TLS/RTP/SAVPF 120 121 126 97',
  'c=IN IP4 0.0.0.0',
  'a=mid:video',
  'a=sendrecv',
  'a=rtpmap:120 VP8/90000',
  'a=rtpmap:121 VP9/90000',
  'a=rtpmap:126 H264/90000',
  'a=rtpmap:97 H264/180000',
  'a=rtcp-mux',
  'a=ssrc:2 cname:0',
  'a=ssrc:2 msid:stream 456'
].join('\r\n') + '\r\n';

describe('MediaStream', it => {
  it('should have no tracks after `new MediaStream()`', () => {
    var stream = new MediaStream();
    expect(stream.getTracks().length)
      .to.equal(0, 'the MediaStream does not contain any MediaStreamTracks');
  });
  it('new MediaStream(stream)', async () => {
    let stream1 = await getRemoteMediaStream();
    let stream2 = new MediaStream(stream1);

    expect(stream2.id).not.to.equal(stream1.id);
    expect(stream2.getTracks().every((track, i) => track === stream1.getTracks()[i]))
      .to.be.true;
    expect(stream1.getTracks().every((track, i) => track === stream2.getTracks()[i]))
      .to.be.true;
  });
  it('new MediaStream(tracks)', async () => {
    let stream1 = await getRemoteMediaStream();
    var tracks = stream1.getTracks();
    var stream2 = new MediaStream(tracks);

    expect(stream2.getTracks().every((track, i) => track === tracks[i]))
      .to.be.true;
    
    expect(tracks.every((track, i) => track === stream2.getTracks()[i]))
      .to.be.true;
  });
  it('new MediaStream(mediaStreamInit)', () => {
    var stream = new MediaStream({ id: 'testStreamId' });
    expect(stream.getTracks().length).to.equal(0);
    expect(stream.id).to.equal('testStreamId');
  });
  it('.clone', async () => {
    let stream1 = await getRemoteMediaStream();
    var stream2 = stream1.clone();
    var stream3 = stream2.clone();

    // NOTE(mroberts): Weirdly, cloned video MediaStreamTracks have .readyState
    // "live"; we'll .stop them, at least until that bug is fixed.
    // stream2.getVideoTracks().forEach(function(track) {
    stream2.getTracks().forEach(track => track.stop());
    // stream3.getVideoTracks().forEach(function(track) {
    stream3.getTracks().forEach(track => track.stop());

    expect(
      stream1.id !== stream2.id 
      && stream2.id !== stream3.id 
      && stream3.id !== stream1.id
    ).to.be.true;

    expect(
      stream1.getTracks().length === stream2.getTracks().length 
      && stream1.getTracks().length === stream3.getTracks().length
    ).to.be.true;
    
    expect(
      stream1.getTracks().every(
        (track, i) => track.kind === stream2.getTracks()[i].kind 
                      && track.kind === stream3.getTracks()[i].kind
      )
    ).to.be.true;

    expect(
      stream1.getTracks().every(
        (track, i) => track.id !== stream2.getTracks()[i].id 
                      && track.id !== stream3.getTracks()[i].id
      )
    ).to.be.true;
  });
  
  it.skip('.clone and .stop', async () => {
    let stream1 = await getUserMedia({ audio: true });
    var track1 = stream1.getTracks()[0];

    expect(stream1.active).to.be.true;
    expect(track1.readyState).to.equal('live');

    var stream2 = stream1.clone();
    var track2 = stream2.getTracks()[0];

    expect(stream2.active).to.be.true;
    expect(track2.readyState).to.equal('live');
    expect(stream1).not.to.equal(stream2);
    expect(track1).not.to.equal(track2);

    track1.stop();

    expect(!stream1.active).to.be.true;
    expect(stream2.active).to.be.true;
    expect(track1.readyState).to.equal('ended');
    expect(track2.readyState).to.equal('live');

    track2.stop();

    expect(!stream2.active).to.be.true;
    expect(track2.readyState).to.equal('ended');
  });
  it('.removeTrack and .addTrack on remote MediaStream', async () => {
    let stream = await getRemoteMediaStream();
    let tracks = stream.getTracks();
    tracks.forEach(track => stream.removeTrack(track));
    expect(stream.getTracks().length).to.equal(0);
    tracks.forEach(track => stream.addTrack(track));
    expect(stream.getTracks().length).to.equal(tracks.length);
    expect(stream.getTracks().every((track, i) => track === tracks[i]))
      .to.be.true;
  });
  
  it('.enabled on local MediaStreamTrack', async () => {
    let stream = await getLocalMediaStream();
    testSettingEnabled(stream);
  });

  it('.enabled on remote MediaStreamTrack', async () => {
    let stream = await getRemoteMediaStream();
    testSettingEnabled(stream);
  });

});


function testSettingEnabled(stream) {
  const tracks = stream.getTracks();
  tracks.forEach(track => { track.enabled = false; });
  expect(tracks.every(track => !track.enabled)).to.be.true;
  tracks.forEach(track => track.enabled = true);
  expect(tracks.every(track => track.enabled)).to.be.true;
  tracks.forEach(track => track.stop());
  expect(tracks.every(track => track.enabled)).to.be.true;
  tracks.forEach(track => track.enabled = false);
  expect(tracks.every(track => !track.enabled)).to.be.true;
}

function getRemoteMediaStream() {
  var pc = new RTCPeerConnection();
  var offer = new RTCSessionDescription({ type: 'offer', sdp: sdp });
  var trackEventPromise = new Promise(function(resolve) {
    pc.ontrack = resolve;
  });
  return pc.setRemoteDescription(offer).then(function() {
    return trackEventPromise;
  }).then(function(trackEvent: any) {
    pc.close();
    return trackEvent.streams[0];
  });
}

function getLocalMediaStream() {
  return getUserMedia({ audio: true, video: true });
}
