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

function log(...args: any[]) {
    //console.log(...args);
}

describe('Bridge Example', () => {
    it('works', async () => {
        await new Promise<void>(async (resolve, reject) => {

            log('client: initializing');
            let pendingDataChannels: Record<string, RTCDataChannel> = {};
            let dataChannels: Record<string, RTCDataChannel> = {};
            let pendingCandidates: (RTCIceCandidate | null)[] = [];
            let ws: WebSocket | null = null;
            let pc = new RTCPeerConnection(RTC_CONFIGURATION, /*{ optional: [] }*/);

            log('client: connecting to websockets...');
            ws = new WebSocket(`ws://${location.hostname}:8080`);

            pc.onicecandidate = event => {
                log('client: onicecandidate');
                if (ws.readyState === WebSocket.OPEN) {
                    log(`sending:`, pc.localDescription);
                    ws.send(JSON.stringify({ type: 'ice', sdp: event.candidate }));
                } else {
                    pendingCandidates.push(event.candidate);
                }
            };


            ws.onopen = async () => {
                log('client: websocket established');

                log('client: setting up datachannels');
                for (let [label, channelOptions] of Object.entries(DATA_CHANNEL_CONFIG)) {
                    log(`client: opening channel '${label}'`);
                    var channel = pendingDataChannels[label] = pc.createDataChannel(label, channelOptions);
                    channel.binaryType = 'arraybuffer';
                    channel.onopen = () => {
                        log(`client: channel '${label}' is open`);
                        dataChannels[label] = channel;
                        delete pendingDataChannels[label];
                        log('log8');
                        if (Object.keys(dataChannels).length === Object.keys(DATA_CHANNEL_CONFIG).length) {
                            log('log9');
                            dataChannels.reliable.send(new Uint8Array([97, 99, 107, 0]).buffer);
                            dataChannels.reliable.send('Hello bridge!');
                            resolve();
                        }
                    };
                    channel.onerror = err => reject(err);
                }

                log('client: creating offer / setting local description');
                await pc.setLocalDescription(await pc.createOffer());

                log(`client: applying ${pendingCandidates.length} pending ICE candidates`);
                pendingCandidates.forEach(sdp => ws.send(JSON.stringify({ type: 'ice', sdp })));

                log(`sending offer:`, pc.localDescription);
                ws.send(JSON.stringify(pc.localDescription));
            };
            ws.onmessage = async event => {
                var data = JSON.parse(event.data);
                log('client: message received:', data);

                if (data.type === 'answer')
                    await pc.setRemoteDescription(data);
                else if (data.type === 'ice')
                    await pc.addIceCandidate(data.sdp);
            };
        });
    });
});
