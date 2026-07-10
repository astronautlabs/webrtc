import { RTCPeerConnection, RTCStatsReport, RTCVideoSource, RTCVideoFrame } from '../..';

export function createRTCPeerConnections(configuration1 = {}, configuration2 = {}, options: any = {}) {
    options = {
        handleIce: true,
        ...options
    };
    const pc1 = new RTCPeerConnection(configuration1);
    try {
        const pc2 = new RTCPeerConnection(configuration2);
        if (options.handleIce) {
            let closed = false;

            [pc1, pc2].forEach(pc => {
                pc.addEventListener('signalingstatechange', ev => {
                    if (pc.signalingState === 'closed') {
                        closed = true;
                    }
                });
            });
            [[pc1, pc2], [pc2, pc1]].forEach(([pcA, pcB]) => {
                pcA.addEventListener('icecandidate', async ({ candidate }) => {
                    if (pcB.signalingState === 'closed')
                        return;
                    if (closed)
                        return;

                    if (candidate) {
                        try {
                            await pcB.addIceCandidate(candidate);
                        } catch (e: any) {
                            console.error(`[test-lib] Caught error while conveying ICE candidate between peers: ${e.message}`);
                            debugger;
                            throw e;
                        }
                    }
                });
            });
        }
        return [pc1, pc2];
    } catch (error) {
        pc1.close();
        throw error;
    }
}

export async function doOffer(offerer: RTCPeerConnection, answerer: RTCPeerConnection) {
    const offer = await offerer.createOffer();
    await Promise.all([
        offerer.setLocalDescription(offer),
        answerer.setRemoteDescription(offer)
    ]);
}

export async function doAnswer(answerer: RTCPeerConnection, offerer: RTCPeerConnection) {
    const answer = await answerer.createAnswer();
    await Promise.all([
        answerer.setLocalDescription(answer),
        offerer.setRemoteDescription(answer)
    ]);
}

export async function negotiate(offerer: RTCPeerConnection, answerer: RTCPeerConnection) {
    await doOffer(offerer, answerer);
    await doAnswer(answerer, offerer);
}

export async function negotiateRTCPeerConnections(
    options: {
        configuration?: any,
        pc1Configuration?: any,
        pc2Configuration?: any,
        withPc1?: (pc: RTCPeerConnection) => void,
        withPc2?: (pc: RTCPeerConnection) => void,
    } = {}
) {
    options = Object.assign({}, {
        configuration: {},
        pc1Configuration: {},
        pc2Configuration: {},
        withPc1() { },
        withPc2() { },
    }, options);
    const [pc1, pc2] = createRTCPeerConnections(
        Object.assign({}, options.configuration, options.pc1Configuration),
        Object.assign({}, options.configuration, options.pc2Configuration));
    try {
        options.withPc1?.(pc1);
        options.withPc2?.(pc2);
        await negotiate(pc1, pc2);
        return [pc1, pc2];
    } catch (error) {
        pc1.close();
        pc2.close();
        throw error;
    }
}

export async function getLocalTrackStats(pc: RTCPeerConnection, track: MediaStreamTrack, check = (stats: RTCStatsReport) => true) {
    let stats;
    do {
        const report = await pc.getStats();
        stats = [...report.values()]
            .find(stats => stats.type === 'track'
                && stats.trackIdentifier === track.id
                && !stats.remote
                && check(stats));
    } while (!stats);
    return stats;
}

export async function confirmSentFrameDimensions(source: RTCVideoSource, track: MediaStreamTrack, pc: RTCPeerConnection, frame: RTCVideoFrame) {
    await getLocalTrackStats(pc, track, stats => {
        if (stats.frameWidth === frame.width
            && stats.frameHeight === frame.height) {
            return true;
        }
        source.onFrame(frame);
        return false;
    });
}

export function waitForStateChange(target: any, state: any, options: any = {}) {
    options = {
        event: 'statechange',
        property: 'state',
        ...options
    };
    if (target[options.property] === state) {
        return Promise.resolve();
    }
    return new Promise<void>(resolve => {
        target.addEventListener(options.event, function listener() {
            if (target[options.property] === state) {
                target.removeEventListener(options.event, listener);
                resolve();
            }
        });
    });
}

export function gatherCandidates(pc: RTCPeerConnection): Promise<RTCIceCandidate[]> {
    const candidates: any[] = [];
    return new Promise(resolve => {
        if (pc.iceGatheringState === 'complete') {
            resolve(candidates);
        }
        pc.addEventListener('icecandidate', ({ candidate }) => {
            if (!candidate) {
                resolve(candidates);
                return;
            }
            candidates.push(candidate);
        });
    });
}
