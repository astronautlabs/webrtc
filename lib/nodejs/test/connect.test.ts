import { describe } from 'razmin';
import { expect } from 'chai';

import { RTCIceCandidate, RTCPeerConnection } from '..';

import { captureCandidates } from './helpers/capture-candidates';

describe('RTCPeerConnection', it => {
  let candidatesPromises: Promise<globalThis.RTCIceCandidate[]>[];
  let peers: RTCPeerConnection[] = [];
  let candidates: RTCIceCandidate[][] = [];
  let dcs: RTCDataChannel[] = [];
  let localDesc;
  let dcPromise;
  
  it('create the peer connections', () => {
    peers = [
      new RTCPeerConnection({ iceServers: [] }),
      new RTCPeerConnection({ iceServers: [] })
    ];
  
    dcPromise = new Promise(function(resolve) {
      peers[1].ondatachannel = function(evt) {
        dcs[1] = evt.channel;
        resolve(dcs[1]);
      };
    });
  
    candidatesPromises = peers.map(function(pc, i) {
      var candidatesPromise = captureCandidates(pc, `Peer ${i}`, c => peers[i ? 0 : 1].addIceCandidate(c));
      return candidatesPromise.then(function(_candidates) {
        candidates[i] = _candidates;
        return _candidates;
      });
    });
  
    expect(peers[0]).to.be.instanceOf(RTCPeerConnection);
    expect(peers[1]).to.be.instanceOf(RTCPeerConnection);
  });
  
  it('peers are created and in the expected connection state', () => {
    expect(peers[0].iceConnectionState).to.equal('new');
    expect(peers[1].iceConnectionState).to.equal('new');
  });
  
  it('create a datachannel on peer:0', () => {
    expect(dcs[0] = peers[0].createDataChannel('test')).to.exist;
    expect(dcs[0].label).to.equal('test');
  });
  
  it('createOffer for peer:0', async () => {
    let desc = await peers[0].createOffer({});
    // save the local description
    localDesc = desc;

    // run the checks
    expect(desc).to.exist;
    expect(desc.type).to.equal('offer')
    expect(desc.sdp).to.exist;
  });
  
  it('setLocalDescription for peer:0', async () => await peers[0].setLocalDescription(localDesc));
  
  it('capture ice candidates for peer:0', async () => {
    await candidatesPromises[0];
    expect(peers[0].iceGatheringState).to.equal('complete', 'have candidates for peer:0');
  });
  
  it('setRemoteDescription for peer:1', async () => {
    await peers[1].setRemoteDescription(peers[0].localDescription);
  });
  
  it('provide peer:1 with the peer:0 gathered ice candidates', async () => {
    if (!candidates[0].length)
      return;
  
    await Promise.all(candidates[0].map(candidate => {
      return peers[1].addIceCandidate(candidate);
    }));
  });
  
  it('createAnswer for peer:1', async () => {
    let desc = await peers[1].createAnswer();
    localDesc = desc;
    expect(desc).to.exist;
    expect(desc.type).to.equal('answer');
    expect(desc.sdp).to.exist;
  });
  
  it('setLocalDescription for peer:1', async () => {
    await peers[1].setLocalDescription(localDesc);
  });
  
  it('capture ice candidates for peer:1', async () => {
    await candidatesPromises[1];
    expect(peers[1].iceGatheringState).to.equal('complete', 'have candidates for peer:1');
  });
  
  it('setRemoteDescription for peer:0', async () => {
    await peers[0].setRemoteDescription(peers[1].localDescription);
  });
  
  it('provide peer:0 with the peer:1 gathered ice candidates', async () => {
    if (!candidates[1].length)
      return;
  
    await Promise.all(candidates[1].map(c => peers[0].addIceCandidate(c)));
  });
  
  it('peer:1 triggers data channel event', async () => {
    let dc = await dcPromise;
    expect(dc).to.exist;
    expect(dc.label).to.equal('test');
  });
  
  it('monitor the ice connection state of peer:0', async () => {
    await new Promise<void>(connected => {
      function checkState() {
        if (peers[0].iceConnectionState === 'connected' || peers[0].iceConnectionState === 'completed') {
          connected();
          peers[0].oniceconnectionstatechange = null;
        }
      }
    
      peers[0].oniceconnectionstatechange = checkState;
      checkState();
    });
  });
  
  it('monitor the ice connection state of peer:1', async () => {
    await new Promise<void>(done => {
      function checkState() {
        if (peers[1].iceConnectionState === 'connected') {
          done();
          peers[1].oniceconnectionstatechange = null;
        }
      }
    
      peers[1].oniceconnectionstatechange = checkState;
      checkState();
    });
  });
  
  /**
   * @param {Test} t
   * @param {RTCDataChannel} sender
   * @param {RTCDataChannel} receiver
   * @param {string|Uint8Array} message
   * @returns {Promise<void>}
   */
  async function testSendingAMessage(sender: RTCDataChannel, receiver: RTCDataChannel, message: string | Uint8Array) {
    var messageEventPromise = new Promise<MessageEvent>(function(resolve) {
      receiver.addEventListener('message', resolve);
    });
  
    if (typeof message === 'string')
      sender.send(message);
    else if (message instanceof Buffer)
      sender.send(message.buffer.slice(message.byteOffset, message.byteOffset + message.length));
    else if (message instanceof ArrayBuffer)
      sender.send(message);
    else if (ArrayBuffer.isView(message))
      sender.send(message);
    
    let messageEvent = await messageEventPromise;

    var data = messageEvent.data;
    if (typeof message === 'string') {
      expect(data.length).to.equal(message.length, "String length must match.");
      expect(data).to.equal(message);
    } else {
      data = new Uint8Array(data);
      expect(data.length).to.equal(message.length, "Uint8Array length must match");
      expect([].slice.call(data)).to.eql([].slice.call(message));
    }
  }
  
  /**
   * @param {Test} t
   * @param {RTCDataChannel} sender
   * @param {RTCDataChannel} receiver
   * @param {string|Uint8Array} message
   * @param {number} n
   * @returns {Promise<void>}
   */
  function testSendingAMessageNTimes(sender: RTCDataChannel, receiver: RTCDataChannel, message, n?: number) {
    return n <= 0
      ? Promise.resolve()
      : testSendingAMessage(sender, receiver, message)
        .then(() => testSendingAMessageNTimes(sender, receiver, message, n - 1));
  }
  
  /**
   * @param {Test} t
   * @param {RTCDataChannel} sender
   * @param {RTCDataChannel} receiver
   * @param {string} message
   * @param {object} [options]
   * @param {number} [options.times=1]
   * @param {boolean} [options.type='string'] - one of "string", "arraybuffer", or
   *   "buffer"
   * @returns {Promise<void>}
   */
  function testSendingAMessageWithOptions(sender: RTCDataChannel, receiver: RTCDataChannel, message, options) {
    options = Object.assign({
      times: 1,
      type: 'string'
    }, options);
    if (options.type === 'arraybuffer') {
      let messageStr = <string>message;
      message = new Uint8Array(Array.from(messageStr).map(char => {
        return char.charCodeAt(0);
      }));
    } else if (options.type === 'buffer') {
      message = Buffer.from(message, 'utf-8');
    }
    return testSendingAMessageNTimes(sender, receiver, message, options.times);
  }
  
  it('data channel connectivity', async () => {
    var sender = dcs[0];
    var receiver = dcs[1];
    var message1 = 'hello world';

    // First, test sending strings.
    await testSendingAMessageWithOptions(sender, receiver, message1, { times: 3 });
  });
  
  it('data channel connectivity (arraybuffer)', async () => {
    var sender = dcs[0];
    var receiver = dcs[1];
    var message1 = 'hello world';

    // Then, test sending ArrayBuffers.
    await testSendingAMessageWithOptions(sender, receiver, message1, {
      times: 3,
      type: 'arraybuffer'
    });
  });
  
  it('data channel connectivity (buffer)', async () => {
    var sender = dcs[0];
    var receiver = dcs[1];
    var message1 = 'hello world';

    // Finally, test sending Buffers.
    await testSendingAMessageWithOptions(sender, receiver, message1, {
      times: 3,
      type: 'buffer'
    });
  });
  
  it('getStats (legacy)', () => {
    function getStats(peer, callback) {
      peer.getStats(function(response) {
        var reports = response.result();
        callback(null, reports.map(function(report) {
          var obj = {
            timestamp: report.timestamp,
            type: report.type
          };
          var names = report.names();
          names.forEach(function(name) {
            obj[name] = report.stat(name);
          });
          return obj;
        }));
      }, function(error) {
        callback(error);
      });
    }
  
    function done(error, reports) {
      if (error) {
        throw new Error(error);
        return;
      }
    }
  
    peers.forEach(peer =>getStats(peer, done));
  });
  
  it('getStats', async () => {
    function getStats(peer, callback) {
      peer.getStats().then(response => {
        callback(null, response);
      }, error => {
        callback(error);
      });
    }
  
    function done(error, reports) {
      if (error) {
        throw new Error(error);
      }
    }
  
    peers.forEach(peer => getStats(peer, done));
  });
  
  it('close the connections', async () => {
    peers[0].close();
    peers[1].close();
  
    // make sure nothing crashes after connection is closed and _jinglePeerConnection is null
    for (var i = 0; i < 2; i++) {
      await peers[i].createOffer({}).then(() => {}, () => {});
      await peers[i].createAnswer().then(() => {}, () => {});
      await peers[i].setLocalDescription({}).then(() => {}, () => {});
      await peers[i].setRemoteDescription({}).then(() => {}, () => {});
      await peers[i].addIceCandidate({}).then(() => {}, () => {});
      try {
        peers[i].createDataChannel('test');
        throw new Error('createDataChannel should throw InvalidStateError');
      } catch (error) {
        expect(error.code).to.equal(11);
        expect(error.name).to.equal('InvalidStateError');
      }

      let caughtError;
      try {
        await peers[i].getStats();
      } catch (error) {
        caughtError = error;
      }

      expect(caughtError).to.exist;

      peers[i].close();
    }
    peers = [];
  });
});
