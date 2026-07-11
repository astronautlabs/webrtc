import { describe, it } from '@jest/globals';

const RTC_CONFIGURATION: RTCConfiguration = {
    iceServers: [{ urls: 'stun:stun.l.google.com:19302' }]
};
const DATA_CHANNEL_CONFIG: Record<string, RTCDataChannelInit> = {
    'reliable': {
        ordered: false,
        maxRetransmits: 10
    },
    // '@control': {
    //     outOfOrderAllowed: true,
    //     maxRetransmitNum: 0
    // }
};

describe('Bridge Example', () => {
    it('works', async () => {
        let pendingDataChannels: Record<string, RTCDataChannel> = {};
        let dataChannels: Record<string, RTCDataChannel> = {};
        let pendingCandidates: (RTCIceCandidate | null)[] = [];
        let ws: WebSocket | null = null;
        let pc = new RTCPeerConnection(RTC_CONFIGURATION, /*{ optional: [] }*/);

        await pc.setLocalDescription(await pc.createOffer());

        ws = new WebSocket(`ws://${location.search || `${location.hostname}:8080`}`);
        ws.onopen = () => {
            pendingCandidates.forEach(sdp => ws.send(JSON.stringify({ type: 'ice', sdp })));
            ws.send(JSON.stringify(pc.localDescription));
        };
        ws.onmessage = async event => {
            var data = JSON.parse(event.data);
            if (data.type === 'answer')
                await pc.setRemoteDescription(data);
            else if (data.type === 'ice')
                await pc.addIceCandidate(data.sdp);
        };

        pc.onicecandidate = event => {
            if (ws.readyState === WebSocket.OPEN)
                ws.send(JSON.stringify({ type: 'ice', sdp: event.candidate }));
            else
                pendingCandidates.push(event.candidate);
        };

        await new Promise<void>((resolve, reject) => {
            for (let [label, channelOptions] of Object.entries(DATA_CHANNEL_CONFIG)) {
                var channel = pendingDataChannels[label] = pc.createDataChannel(label, channelOptions);
                channel.binaryType = 'arraybuffer';
                channel.onopen = () => {
                    dataChannels[label] = channel;
                    delete pendingDataChannels[label];
                    if (Object.keys(dataChannels).length === Object.keys(DATA_CHANNEL_CONFIG).length) {
                        dataChannels.reliable.send(new Uint8Array([97, 99, 107, 0]).buffer);
                        dataChannels.reliable.send('Hello bridge!');
                        resolve();
                    }
                };
                channel.onerror = err => reject(err);
            }
        });
    });
});
