/* eslint no-console:0, no-process-env:0 */
import SimplePeer from 'simple-peer';
import * as simplepeer from 'simple-peer';

import * as wrtc from '..';

var log = process.env.LOG ? console.log : function() {};

/**
 * These tests take quite some time, it is mainly stress testing the connection process that simple-peer uses.
 * simple-peer is a good test case because it uses a lot of functionality to ensure the connection is working correctly:
 * - normal ICE candidate exchange / peer connection establishment
 * - data channel creation with message passing
 * - getStats
 */
describe('multiconnect', () => {
  it('connect once', done => {
    connect(err => done(err));
  });
  it('connect once and then later', done => {
    connect(err => {
        setTimeout(() => {
            connect(err => {
                done(err)
            });
        }, 3000);
    });
  });
  it('connect loop', done => {
    connectLoop(10, err => done(err));
  });
  it('connect concurrent', done => {
    var n = 10;
    for (let i = 0; i < n; i += 1)
      connect(err => done(err));
  });
  it('connect loop concurrent', done => {
    var n = 10;
    for (var i = 0; i < n; i += 1)
      connectLoop(10, (err: Error) => done(err));
  });
});

var connIdGen = 1;

function connect(callback: (err?: Error) => void) {
  var connId = connIdGen;
  var connName = 'CONNECTION-' + connId;
  connIdGen += 1;
  log(connName, 'starting');

  // setup two peers with simple-peer
  var peer1: simplepeer.Instance | null = new SimplePeer(<any>{ wrtc: wrtc });
  var peer2: simplepeer.Instance | null = new SimplePeer(<any>{ wrtc: wrtc, initiator: true });

  function cleanup() {
    if (peer1) {
      peer1.destroy();
      peer1 = null;
    }
    if (peer2) {
      peer2.destroy();
      peer2 = null;
    }
  }

  // when peer1 has signaling data, give it to peer2, and vice versa
  peer1.on('signal', function(data) {
    log(connName, `signal peer1 -> peer2: ${JSON.stringify(data, undefined, 2)}`);
    peer2?.signal(data);
  });
  peer2.on('signal', function(data) {
    log(connName, `signal peer2 -> peer1: ${JSON.stringify(data, undefined, 2)}`);
    peer1?.signal(data);
  });

  peer1.on('error', function(err) {
    log(connName, 'peer1 error', err);
    cleanup();
    callback(err);
  });
  peer2.on('error', function(err) {
    log(connName, 'peer2 error', err);
    cleanup();
    callback(err);
  });

  // wait for 'connect' event
  peer1.on('connect', function() {
    log(connName, 'sending message');
    peer1?.send('peers are for kids');
  });
  peer2.on('data', function() {
    log(connName, 'completed');
    cleanup();
    callback();
  });
}

function connectLoop(count: number, callback: (err?: any) => void) {
  if (count <= 0) {
    log('connect loop completed');
    callback();
  } else {
    log('connect loop remain', count);
    connect(err => {
      if (err) {
        callback(err);
      } else {
        connectLoop(count - 1, callback);
      }
    });
  }
}
