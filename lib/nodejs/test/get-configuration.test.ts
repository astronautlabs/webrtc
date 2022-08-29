import { describe } from 'razmin';
import { expect } from 'chai';

import { RTCPeerConnection } from '..';

describe('RTCPeerConnection', () => {
  describe('.getConfiguration()', it => {
    var defaultConfiguration = {
      iceServers: [],
      iceTransportPolicy: 'all',
      bundlePolicy: 'balanced',
      rtcpMuxPolicy: 'require',
      iceCandidatePoolSize: 0,
      portRange: {},
      sdpSemantics: 'unified-plan'
    };
  
    it('before calling close, with defaults', () => {
      var pc = new RTCPeerConnection();
      var actualConfiguration = pc.getConfiguration();
      pc.close();
      expect(actualConfiguration).to.eql(defaultConfiguration);
    });
  
    it('after calling close, with defaults', () => {
      var pc = new RTCPeerConnection();
      pc.close();
      var actualConfiguration = pc.getConfiguration();
      expect(actualConfiguration).to.eql(defaultConfiguration);
    });
  
    let spec = {
      iceServers: [
        {
          urls: ['stun:stun1.example.net'],
          credentialType: 'password'
        },
        {
          urls: ['turns:turn.example.org', 'turn:turn.example.net'],
          username: 'user',
          credential: 'myPassword',
          credentialType: 'password'
        }
      ],
      iceTransportPolicy: 'relay',
      bundlePolicy: 'max-bundle',
      rtcpMuxPolicy: 'negotiate',
      iceCandidatePoolSize: 255,
      portRange: { min: 1, max: 2 }
    };
  
    Object.keys(spec).forEach((key) => {
      let value = spec[key];
      it(`after setting ${key}`, () => {
        var expectedConfiguration = Object.assign({}, defaultConfiguration);
        expectedConfiguration[key] = value;
        var pc = new RTCPeerConnection(<any>expectedConfiguration);
        pc.close();
        var actualConfiguration = pc.getConfiguration();
        try {
          expect(actualConfiguration).to.eql(expectedConfiguration);
        } catch (e) {
          throw new Error(`Expected ${JSON.stringify(actualConfiguration, undefined, 2)} to equal ${JSON.stringify(expectedConfiguration, undefined, 2)}`);
        }
      });
    });
  });
  
  describe('.setConfiguration()', it => {
    it('after closing', () => {
      var pc = new RTCPeerConnection();
      pc.close();
      expect(pc.setConfiguration.bind(pc, {})).to.throw;
    });
  
    it('changing iceServers', () => {
      var pc = new RTCPeerConnection();
      var expectedConfiguration = Object.assign({}, pc.getConfiguration());
      expectedConfiguration.iceServers = [
        { 
          urls: ['stun:stun1.example.net'], 
          credential: 'password',
          "credentialType": "password" 
        }
      ];
      pc.setConfiguration(expectedConfiguration);
      var actualConfiguration = pc.getConfiguration();
      try {
        expect(actualConfiguration).to.eql(expectedConfiguration);
      } catch (e) {
        throw new Error(`Expected ${JSON.stringify(actualConfiguration, undefined, 2)} to equal ${JSON.stringify(expectedConfiguration, undefined, 2)}`);
      }
      pc.close();
    });
  
    it('changing iceTransportPolicy', () => {
      var pc = new RTCPeerConnection();
      var expectedConfiguration = Object.assign({}, pc.getConfiguration());
      expectedConfiguration.iceTransportPolicy = 'relay';
      pc.setConfiguration(expectedConfiguration);
      var actualConfiguration = pc.getConfiguration();
      try {
        expect(actualConfiguration).to.eql(expectedConfiguration);
      } catch (e) {
        throw new Error(`Expected ${JSON.stringify(actualConfiguration, undefined, 2)} to equal ${JSON.stringify(expectedConfiguration, undefined, 2)}`);
      }
      pc.close();
    });
  
    // If the value of configuration.bundlePolicy differs from the connection's
    // bundle policy, throw an InvalidModificationError.
    it('changing bundlePolicy throws', () => {
      var pc = new RTCPeerConnection({ bundlePolicy: 'max-bundle' });
      expect(pc.setConfiguration.bind(pc, { bundlePolicy: 'max-compat' })).to.throw;
      pc.close();
    });
  
    // If the value of configuration.rtcpMuxPolicy differs from the connection's
    // rtcpMux policy, throw an InvalidModificationError.
    it('changing rtcpMuxPolicy throws', () => {
      var pc = new RTCPeerConnection(<any>{ rtcpMuxPolicy: 'negotiate' });
      expect(pc.setConfiguration.bind(pc, { rtcpMuxPolicy: 'require' })).to.throw;
      pc.close();
    });
  
    // If the value of configuration.iceCandidatePoolSize differs from the
    // connection's previously set iceCandidatePoolSize, and setLocalDescription
    // has already been called, throw an InvalidModificationError.
    it('changing iceCandidatePoolSize after setLocalDescription throws', async () => {
      var pc = new RTCPeerConnection({ iceCandidatePoolSize: 1 });
      let offer = await pc.createOffer({});
      pc.setConfiguration({ iceCandidatePoolSize: 2 });
      await pc.setLocalDescription(offer);
      expect(pc.setConfiguration.bind(pc, { iceCandidatePoolSize: 3 })).to.throw;
      pc.close();
    });
  });
});