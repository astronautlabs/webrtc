/* eslint no-process-exit:0 */
import { RTCPeerConnection } from '../..';

const pc = new RTCPeerConnection();
pc.close();
process.exit();
