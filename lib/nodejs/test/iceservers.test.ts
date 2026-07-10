/* eslint no-console:0, no-process-env:0 */
import { describe, it } from '@jest/globals';
import { expect } from 'chai';

import { RTCPeerConnection, RTCSessionDescription } from '..';

// NOTE(mroberts): Not sure why we need to do this. This may be a firewall issue.
// TODO(liam): Remove this

var isDarwinOnCircleCi = process.platform === 'darwin'
    && process.env.CIRCLECI === 'true';

var skipReflexive = isDarwinOnCircleCi || !process.env.CHECK_REFLEXIVE;

var pc: RTCPeerConnection;

describe('RTCPeerConnection', () => {
    it('assign ICE server and get reflective candidates', async () => {
        pc = new RTCPeerConnection({
            iceServers: [
                {
                    urls: ['stun:stun.l.google.com:19302']
                }
            ]
        });

        var gotReflective = false;

        await new Promise<void>(async (resolve, reject) => {
            function finish() {
                if (pc.signalingState === 'closed') {
                    return;
                }
                pc.close();

                if (!gotReflective && !skipReflexive)
                    reject(new Error(`Expected to receive a reflective candidate`));
                else
                    resolve();
            }

            pc.onicecandidate = function (candidate) {
                if (candidate.candidate) {
                    if (candidate.candidate.candidate.indexOf('typ srflx') > -1) {
                        gotReflective = true;
                        finish();
                    }
                }
            };

            pc.onicegatheringstatechange = function () {
                if (pc.iceGatheringState === 'complete') {
                    finish();
                }
            };

            pc.createDataChannel('test');

            let e = await pc.createOffer();
            await pc.setLocalDescription(new RTCSessionDescription(e));
        });
    });

    it('dont assign ICE server and get no reflective candidates', async () => {
        pc = new RTCPeerConnection({
            iceServers: []
        });

        var gotReflective = false;

        pc.onicecandidate = function (candidate) {
            if (candidate.candidate) {
                if (candidate.candidate.candidate.indexOf('typ srflx') > -1) {
                    gotReflective = true;
                }
            }
        };

        pc.onicegatheringstatechange = function () {
            if (pc.iceGatheringState === 'complete') {
                pc.close();

                expect(gotReflective).to.be.false;
            }
        };

        pc.createDataChannel('test');

        let e = await pc.createOffer();
        await pc.setLocalDescription(new RTCSessionDescription(e));
    });
});
