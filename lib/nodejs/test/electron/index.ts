import { RTCPeerConnection } from '../..';

// We're just testing that the module links with Electron here.
// NOTE: Currently we support Node.js 18 and later. Electron v42+
// no longer supports Node.js 18, so we test with v41.

const pc = new RTCPeerConnection();
pc.close();

console.log(`✅ Looks good!`);
process.exit();
