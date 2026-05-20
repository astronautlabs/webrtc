import { EventTarget } from "./eventtarget";
import native from '../../binding';

export class MediaDevices extends EventTarget {
  enumerateDevices() {
    throw new Error('Not yet implemented; file a feature request for @/webrtc');
  };
  
  getSupportedConstraints() {
    throw new Error('Not yet implemented; file a feature request for @/webrtc');
  };

  getDisplayMedia(...args: any[]) {
    return native.getDisplayMedia(...args);
  }

  getUserMedia(...args: any[]) {
    return native.getUserMedia(...args);
  }
}