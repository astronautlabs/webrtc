/* eslint no-console:0, no-process-env:0 */
import SimplePeer from 'simple-peer';
import * as simplepeer from 'simple-peer';

import * as wrtc from '..';
import { describe } from 'razmin';

var log = process.env.LOG ? console.log : function() {};

describe('multiconnect', it => {
  it('connect once', () => {
    log('###########################\n');
    connect(err => { if (err) throw err; });
  });
  it('connect loop', () => {
    log('###########################\n');
    connectLoop(10, err => { if (err) throw err; });
  });
  it('connect concurrent', () => {
    var n = 10;
    log('###########################\n');
    for (let i = 0; i < n; i += 1)
      connect(err => { if (err) throw err; });
  });
  it('connect loop concurrent', () => {
    var n = 10;
    log('###########################\n');
    for (var i = 0; i < n; i += 1)
      connectLoop(10, err => { if (err) throw err; });
  });
});

var connIdGen = 1;

function connect(callback) {
  var connId = connIdGen;
  var connName = 'CONNECTION-' + connId;
  connIdGen += 1;
  log(connName, 'starting');

  // setup two peers with simple-peer
  var peer1: simplepeer.Instance | null = new SimplePeer(<any>{
    wrtc: wrtc
  });
  var peer2: simplepeer.Instance | null = new SimplePeer(<any>{
    wrtc: wrtc,
    initiator: true
  });

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
    log(connName, 'signal peer1 -> peer2:');
    log(' ', data);
    peer2?.signal(data);
  });
  peer2.on('signal', function(data) {
    log(connName, 'signal peer2 -> peer1:');
    log(' ', data);
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

function connectLoop(count, callback) {
  if (count <= 0) {
    log('connect loop completed');
    callback();
  } else {
    log('connect loop remain', count);
    connect(function(err) {
      if (err) {
        callback(err);
      } else {
        connectLoop(count - 1, callback);
      }
    });
  }
}
