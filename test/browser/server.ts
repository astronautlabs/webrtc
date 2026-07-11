import http from 'http';
import * as ws from 'ws';
import webrtc from '../..';

export type WireMessage = IceMessage | OfferMessage;
export type IceMessage = { type: 'ice'; sdp: RTCIceCandidate; };
export type OfferMessage = { type: 'offer' };

const RTC_CONFIGURATION: RTCConfiguration = {
    iceServers: [{ urls: 'stun:stun.l.google.com:19302' }]
};

export function startServer() {
    return new ws.WebSocketServer({ server: http.createServer().listen(8080) })
        .on('connection', ws => {
            let pc = new webrtc.RTCPeerConnection(RTC_CONFIGURATION, /*{ optional: [{ DtlsSrtpKeyAgreement: false }] }*/);
            let pendingDataChannels: Record<string, RTCDataChannel> = {};
            let dataChannels: Record<string, RTCDataChannel> = {};
            let pendingCandidates: IceMessage[] = [];
            let hasOffer = false;

            pc.onicecandidate = ev => {
                if (ws.readyState !== ws.OPEN)
                    return;
                ws.send(JSON.stringify({
                    type: 'ice',
                    sdp: ev.candidate
                }));
            };
            pc.ondatachannel = ({ channel }) => {
                pendingDataChannels[channel.label] = channel;
                channel.binaryType = 'arraybuffer';
                channel.onopen = () => {
                    dataChannels[channel.label] = channel;
                    delete pendingDataChannels[channel.label];
                };
                channel.onmessage = ({ data }) => {
                    console.log('onmessage:', typeof data === 'string' ? data : new Uint8Array(data));
                    if ('string' === typeof data)
                        channel.send('Hello peer!');
                    else
                        channel.send(new Uint8Array([107, 99, 97, 0]).buffer);
                };
                channel.onerror = error => { throw error; };
            };

            ws.on('close', () => pc?.close());
            ws.on('message', async (rawData: ArrayBuffer) => {
                let data: WireMessage = JSON.parse(new TextDecoder().decode(rawData));
                if (data.type === 'offer') {
                    await pc.setRemoteDescription(data);
                    hasOffer = true;
                    pendingCandidates.forEach(candidate => pc.addIceCandidate(candidate.sdp));
                    await pc.setLocalDescription(await pc.createAnswer());
                    if (ws.readyState !== ws.OPEN)
                        return;
                    ws.send(JSON.stringify(pc.localDescription));
                } else if (data.type === 'ice') {
                    if (hasOffer)
                        pc.addIceCandidate(data.sdp);
                    else
                        pendingCandidates.push(data);
                }
            });
        })
    ;
}
