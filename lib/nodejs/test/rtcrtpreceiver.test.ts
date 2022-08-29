import { expect } from 'chai';
import { describe } from 'razmin';
import {
  MediaStream,
  MediaStreamTrack,
  RTCPeerConnection,
  RTCRtpReceiver,
  RTCSessionDescription
} from '..';

var sdp1 = [
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

var sdp2 = [
  'v=0',
  'o=- 0 2 IN IP4 0.0.0.0',
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
  'a=recvonly',
  'a=rtpmap:109 opus/48000/2',
  'a=rtpmap:9 G722/8000/1',
  'a=rtpmap:0 PCMU/8000',
  'a=rtpmap:8 PCMA/8000',
  'a=rtpmap:101 PCMA/16000',
  'a=rtcp-mux',
  'm=video 9 UDP/TLS/RTP/SAVPF 120 121 126 97',
  'c=IN IP4 0.0.0.0',
  'a=mid:video',
  'a=recvonly',
  'a=rtpmap:120 VP8/90000',
  'a=rtpmap:121 VP9/90000',
  'a=rtpmap:126 H264/90000',
  'a=rtpmap:97 H264/180000',
  'a=rtcp-mux',
].join('\r\n') + '\r\n';

describe('RTCRTPReceiver', it => {
  it('applying a remote offer creates receivers (checked via .getReceivers)', async () => {
    // NOTE(mroberts): Create and close the RTCPeerConnection inside a Promise,
    // then delay with setTimeout so that we can test accessing getReceivers after
    // the RTCPeerConnection's internals have been destroyed.
    
    var pc = new RTCPeerConnection();
    var offer = new RTCSessionDescription({ type: 'offer', sdp: sdp1 });
    await pc.setRemoteDescription(offer);

    var receivers = pc.getReceivers();
    expect(receivers.length).to.equal(2);
    pc.close();
    expect(pc.getReceivers().length).to.equal(0);
    
    receivers = await new Promise<globalThis.RTCRtpReceiver[]>(function(resolve) { setTimeout(resolve.bind(null, receivers)); });
  
    expect(receivers.every(function(receiver) {
      return receiver instanceof RTCRtpReceiver;
    })).to.be.true;

    expect(receivers[0].track).to.be.instanceOf(MediaStreamTrack);
    expect(receivers[0].track.kind).to.equal('audio');
    expect(receivers[0].track.id).to.equal('123');
    expect(receivers[0].track.enabled).to.equal(true);
    expect(receivers[0].track.readyState).to.equal('ended');

    expect(receivers[1].track).to.be.instanceof(MediaStreamTrack);
    expect(receivers[1].track.kind).to.equal('video');
    expect(receivers[1].track.id).to.equal('456');
    expect(receivers[1].track.enabled).to.equal(true, 'the second RTCRtpReceiver\'s .track has .enabled true');
    expect(receivers[1].track.readyState).to.equal('ended', 'the second RTCRtpReceiver\'s .track has .readyState "ended"');
  });
  
  it('applying a remote offer creates receivers (checked via .ontrack)', async () => {
    // NOTE(mroberts): Create and close the RTCPeerConnection inside a Promise,
    // then delay with setTimeout so that we can test accessing getReceivers after
    // the RTCPeerConnection's internals have been destroyed.
    var pc = new RTCPeerConnection();
    var offer = new RTCSessionDescription({ type: 'offer', sdp: sdp1 });
    var trackEventPromise1 = new Promise<RTCTrackEvent>(function(resolve) {
      pc.ontrack = resolve;
    });
    var trackEventPromise2 = trackEventPromise1.then(function() {
      return new Promise<RTCTrackEvent>(function(resolve) {
        pc.ontrack = resolve;
      });
    });

    await pc.setRemoteDescription(offer);
    pc.close();

    let trackEvents = await Promise.all([trackEventPromise1, trackEventPromise2]);
    await new Promise<void>(rs => setTimeout(rs));
  
    expect(trackEvents.every(function(trackEvent) {
      return trackEvent.receiver instanceof RTCRtpReceiver;
    })).to.be.true;

    expect(trackEvents.every(function(trackEvent) {
      return trackEvent.track instanceof MediaStreamTrack;
    })).to.be.true;

    expect(trackEvents.every(function(trackEvent) {
      return trackEvent.track === trackEvent.receiver.track;
    })).to.be.true;

    expect(trackEvents.every(function(trackEvent) {
      return Array.isArray(trackEvent.streams)
        && trackEvent.streams.length === 1
        && trackEvent.streams[0] instanceof MediaStream;
    })).to.be.true;

    expect(trackEvents.every(function(trackEvent) {
      return trackEvent.streams[0].id === 'stream';
    })).to.be.true;

    expect(trackEvents.every(function(trackEvent) {
      return trackEvent.streams[0].getTracks().indexOf(trackEvent.track) > -1;
    })).to.be.true;

    expect(trackEvents.every(function(trackEvent) {
      return trackEvent.streams[0][trackEvent.track.kind === 'audio'
        ? 'getAudioTracks' : 'getVideoTracks'
      ]().indexOf(trackEvent.track) > -1;
    })).to.be.true;
  
    expect(trackEvents.every(function(trackEvent) {
      return trackEvent.streams[0].active === false;
    })).to.be.true;

    expect(trackEvents[0].streams[0] === trackEvents[1].streams[0]).to.be.true;
  });
  
  it('applying a remote offer and then applying a local answer causes .getParameters to return values', async () => {
    var pc = new RTCPeerConnection();
    var offer = new RTCSessionDescription({ type: 'offer', sdp: sdp1 });
    await pc.setRemoteDescription(offer);
    let answer = await pc.createAnswer();
    await pc.setLocalDescription(answer);
    let receivers = await pc.getReceivers();

    // NOTE(mroberts): Flaky
    // t.equal(receivers[0].track.readyState, 'live', 'the audio RTCRtpReceiver\'s .track has .readyState "live"');
    // t.equal(receivers[1].track.readyState, 'live', 'the video RTCRtpReceiver\'s .track has .readyState "live"');
    compareParameters(receivers[0].getParameters(), {
      headerExtensions: [],
      codecs: [
        {
          payloadType: 109,
          mimeType: 'audio/opus',
          clockRate: 48000,
          channels: 2,
          sdpFmtpLine: 'a=fmtp:109 useinbandfec=1; minptime=10'
        },
        {
          payloadType: 9,
          mimeType: 'audio/G722',
          clockRate: 8000,
          channels: 1
        },
        {
          payloadType: 0,
          mimeType: 'audio/PCMU',
          clockRate: 8000,
          channels: 1
        },
        {
          payloadType: 8,
          mimeType: 'audio/PCMA',
          clockRate: 8000,
          channels: 1
        }
      ],
      encodings: [{ active: true }]
    }, 'the audio RTCRtpReceiver\'s .getParameters() returns the expected RTCRtpParameters');
    compareParameters(receivers[1].getParameters(), {
      headerExtensions: [],
      codecs: [
        {
          payloadType: 120,
          mimeType: 'video/VP8',
          clockRate: 90000
        },
        {
          payloadType: 121,
          mimeType: 'video/VP9',
          clockRate: 90000,
          sdpFmtpLine: 'a=fmtp:121 profile-id=0'
        }
      ],
      encodings: [{ active: true }]
    }, 'the video RTCRtpReceiver\'s .getParameters() returns the expected RTCRtpParameters');
    pc.close();
  });
  
  it('negotiating MediaStreamTracks and then renegotiating without them', async () => {
    var pc = new RTCPeerConnection(<any>{ sdpSemantics: 'plan-b' });
    var offer1 = new RTCSessionDescription({ type: 'offer', sdp: sdp1 });

    expect(pc.getReceivers().length).to.equal(0);

    await pc.setRemoteDescription(offer1)
  
    var receivers = pc.getReceivers();
    expect(receivers.length).to.equal(2);

    // NOTE(mroberts): Flaky
    // t.equal(receivers[0].track.readyState, 'live', 'the audio RTCRtpReceiver\'s .track has .readyState "live"');
    // t.equal(receivers[1].track.readyState, 'live', 'the video RTCRtpReceiver\'s .track has .readyState "live"');
    let answer1 = await pc.createAnswer();
    await pc.setLocalDescription(answer1);
    var offer2 = new RTCSessionDescription({ type: 'offer', sdp: sdp2 });
    await pc.setRemoteDescription(offer2);
  
    expect(pc.getReceivers().length).to.equal(0);
    expect(receivers[0].track.readyState).to.equal('ended');
    expect(receivers[1].track.readyState).to.equal('ended');

    let answer2 = await pc.createAnswer();
    await pc.setLocalDescription(answer2);
    expect(pc.getReceivers().length).to.equal(0);
    pc.close();
    expect(receivers[0].track.readyState).to.equal('ended');
    expect(receivers[1].track.readyState).to.equal('ended');
  });
  it('accessing remote MediaStreamTrack after RTCPeerConnection is destroyed', async () => {
    var pc = new RTCPeerConnection();
    var offer1 = new RTCSessionDescription({ type: 'offer', sdp: sdp1 });

    await pc.setRemoteDescription(offer1);
    let answer = await pc.createAnswer();
    await pc.setLocalDescription(answer);
    pc.close();
    await new Promise(function(resolve) { setTimeout(resolve); });
    let mediaStreamTracks = pc.getReceivers().map(function(receiver) {
      return receiver.track;
    });

    expect(mediaStreamTracks.every(function(mediaStreamTrack) {
      return mediaStreamTrack.readyState === 'ended';
    })).to.be.true;
  });
});

function compareParameters(actual, expected, message) {
  expect(actual.headerExtensions).to.eql(expected.headerExtensions, message);
  expect(actual.codecs.length).to.equal(expected.codecs.length, message);
  actual.codecs.forEach((actualCodec, i) => {
    const expectedCodec = expected.codecs[i];
    Object.keys(expectedCodec).forEach(key => {
      if (key === 'sdpFmtpLine') {
        compareSdpFmtpLine(actualCodec[key], expectedCodec[key], message);
        return;
      }
      expect(actualCodec[key]).to.eql(expectedCodec[key], message);
    });
  });
  expect(actual.encodings).to.eql(expected.encodings, message);
}

function compareSdpFmtpLine(actual, expected, message) {
  const [actualHead, ...actualTail] = actual.split(' ');
  const [expectedHead, ...expectedTail] = expected.split(' ');
  expect(actualHead).to.eql(expectedHead, message);
  const actualParameters = actualTail.join(' ').split('; ').sort();
  const expectedParameters = expectedTail.join(' ').split('; ').sort();
  expect(actualParameters).to.eql(expectedParameters, message);
}
