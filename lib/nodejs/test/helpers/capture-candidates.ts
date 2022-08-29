export function captureCandidates(pc: RTCPeerConnection, label: string, received: (candidate: RTCIceCandidate) => void) {
  let candidates: any[] = [];
  return new Promise<RTCIceCandidate[]>(function(resolve) {
    pc.onicecandidate = function(evt) {
      if (evt.candidate) {
        // eslint-disable-next-line no-console
        //console.log(evt);
      } else {
        resolve(candidates);
      }
    };
  });
}