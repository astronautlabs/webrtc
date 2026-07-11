import * as wrtc from '..';
import { expect } from 'chai';

var log = process.env.LOG ? console.log : function () { };

describe('RTCPeerConnection', () => {
    function make(...args: ConstructorParameters<typeof wrtc.RTCPeerConnection>): wrtc.RTCPeerConnection {
        let pc = new wrtc.RTCPeerConnection(...args);
        setImmediate(() => pc.close());
        return pc;
    }

    it('is a constructor', () => {
        expect(wrtc.RTCPeerConnection).to.be.a('function');
    });
    it('can be constructed with no arguments', () => {
        expect(make()).to.exist;
    });
    it('can be constructed with empty configuration', () => {
        expect(make({})).to.exist;
    });
    it('can be constructed with empty ice servers', () => {
        expect(make({ iceServers: [] })).to.exist;
    });
    it('can be constructed with a transport policy', () => {
        expect(make({ iceTransportPolicy: 'relay' })).to.exist;
    });
    it('constructs an instance of itself', () => {
        expect(make()).to.be.instanceOf(wrtc.RTCPeerConnection);
    });
    it('refuses to configure plan B', () => {
        expect(() => make(<any>{ sdpSemantics: 'plan-b' })).to.throw;
    });
    it('allows configuring unified plan', () => {
        expect(() => make(<any>{ sdpSemantics: 'unified-plan' })).not.to.throw;
    });
});