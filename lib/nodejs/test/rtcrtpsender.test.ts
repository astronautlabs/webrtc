import { expect } from 'chai';
import { describe } from 'razmin';
import {
  RTCPeerConnection,
  RTCRtpSender,
  RTCSessionDescription,
  MediaStream
} from '..';

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

describe('RTCRTPSender', it => {
  it('.addTrack(track, stream)', async () => {
    let stream = await getMediaStream();
  
    var pc = new RTCPeerConnection();
    expect(pc.getSenders().length).to.equal(0);

    var tracks = stream.getTracks();
    var senders = tracks.map(function(track) {
      return pc.addTrack(track, stream);
    });

    expect(pc.getSenders().length).to.equal(senders.length);

    expect(pc.getSenders().every(function(sender) {
      return sender instanceof RTCRtpSender;
    })).to.be.true;

    expect(pc.getSenders().every(function(sender, i) {
      return sender === senders[i];
    })).to.be.true;

    expect(senders.every(function(sender, i) {
      return sender.track === tracks[i];
    })).to.be.true;

    senders.forEach(sender => pc.removeTrack(sender));

    expect(pc.getSenders().length).to.equal(senders.length);
    expect(senders.every(function(sender) {
      return sender.track === null;
    })).to.be.true;
    pc.close();
  });
  
  it('.addTrack(track, stream, stream2, stream3) duplicate stream ids', async () => {
    let stream = await getMediaStream();
    var pc = new RTCPeerConnection();

    expect(pc.getSenders().length).to.equal(0);
    var tracks = stream.getTracks();
    var senders = tracks.map(function(track) {
      var stream2 = new MediaStream({ id: 'testStreamId' });
      var stream3 = new MediaStream({ id: 'testStreamId' });
      return pc.addTrack(track, stream, stream2, stream3);  // Stream2 and Stream3 have the same stream id
    });

    expect(pc.getSenders().length).to.equal(senders.length);

    expect(pc.getSenders().every(function(sender) {
      return sender instanceof RTCRtpSender;
    })).to.be.true;

    expect(pc.getSenders().every(function(sender, i) {
      return sender === senders[i];
    })).to.be.true;

    expect(senders.every(function(sender, i) {
      return sender.track === tracks[i];
    })).to.be.true;

    let offer = await pc.createOffer({});
    
    // even duplicates get added
    // 3 streams per track and 2 tracks (audio + video) = 6 msid lines

    expect((offer.sdp.match(/a=msid:/g) || []).length).to.equal(6)
    pc.close();
  });
  
  it('.addTrack(track, stream) called twice', async () => {
    let stream = await getMediaStream();
    const pc = new RTCPeerConnection();
    const [track] = stream.getTracks();
    pc.addTrack(track, stream);

    expect(() => pc.addTrack(track, stream))
      .to.throw(/Sender already exists for track/);

    pc.close();
  });
  
  it('.replaceTrack(null)', async () => {
    let stream = await getMediaStream();
    var pc = new RTCPeerConnection();
    var senders = stream.getTracks().map(function(track) {
      return pc.addTrack(track, stream);
    });

    await Promise.all(senders.map(sender => sender.replaceTrack(null)));
  
    expect(senders.every(function(sender) {
      return sender.track === null;
    })).to.be.true;

    pc.close();
  });
});

async function getMediaStream() {
  var pc = new RTCPeerConnection();
  var offer = new RTCSessionDescription({ type: 'offer', sdp: sdp });
  var trackEventPromise = new Promise<RTCTrackEvent>(function(resolve) {
    pc.ontrack = resolve;
  });
  await pc.setRemoteDescription(offer);
  let trackEvent = await trackEventPromise;
  pc.close();
  return trackEvent.streams[0];
}
