/* eslint no-console:0 */

import { describe, it } from '@jest/globals';
import SimplePeer from 'simple-peer';
import * as wrtc from '..';

type PortRange = { min: number, max: number };

describe('CustomSettings', () => {
    it('custom ports connect once', () => {
        connectClientServer({ min: 9000, max: 9010 }, (err: any) => {
            if (err)
                throw new Error(`connectClientServer callback: ${err}`);
        });
    });

    it('custom ports connect concurrently', () => {
        const n = 2;

        const portRange = { min: 9000, max: 9010 };

        function callback(err: any) {
            if (err)
                throw new Error('connectClientServer callback');
        }

        for (let i = 0; i < n; i++) {
            connectClientServer(portRange, callback);
        }
    });

    function connectClientServer(portRange: PortRange, callback: (arg?: any) => void) {
        const client = new SimplePeer(<any>{
            wrtc: wrtc,
            initiator: true
        });

        const server = new SimplePeer(<any>{
            wrtc: wrtc,
            initiator: false,
            config: {
                portRange: portRange
            }
        });

        client.on('signal', function (data) {
            server.signal(data);
        });
        server.on('signal', function (data: any) {
            if (data.candidate && !isValidCandidate(data.candidate.candidate, portRange || { min: 0, max: 65535 })) {
                callback(`candidate must follow port range (${portRange}): ${data.candidate.candidate}`);
            }
            client.signal(data);
        });
        server.on('connect', function () {
            server.send('xyz');
        });
        client.on('data', function () {
            callback();
            server.destroy();
            client.destroy();
        });
        client.on('error', function (e) {
            callback(e);
            server.destroy();
            client.destroy();
        });
        server.on('error', function (e) {
            callback(e);
            client.destroy();
            server.destroy();
        });
    }

    function isValidCandidate(candidate: string, portRange: PortRange) {
        const port = candidate.replace(/candidate:([^\s]+)\s([^\s]+)\s([^\s]+)\s([^\s]+)\s([^\s]+)\s([0-9]+)\styp.*/, '$6');

        const minPort = portRange.min;
        const maxPort = portRange.max;

        return minPort <= parseInt(port) && maxPort >= parseInt(port);
    }

});
