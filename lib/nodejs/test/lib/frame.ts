import { ABGRToI420, I420ToRGBA, RGBAToI420 } from 'libyuv';

export class I420Frame {
  constructor(width = 640, height = 480) {
    this.width = width;
    this.height = height;
    this.data = new Uint8ClampedArray(this.byteLength);
    Object.freeze(this);
  }

  width: number;
  height: number;
  data: Uint8ClampedArray;

  get dataY() {
    return this.data;
  }

  get dataU() {
    return this.data.subarray(this.sizeOfLuminancePlane);
  }

  get strideY() {
    return this.width * 4;
  }

  get strideU() {
    return this.width / 2;
  }

  get dataV() {
    return this.dataU.subarray(this.sizeOfChromaPlane);
  }

  get strideV() {
    return this.strideU;
  }

  static fromRgba(rgbaFrame: RgbaFrame) {
    const i420Frame = new I420Frame(rgbaFrame.width, rgbaFrame.height);
    ABGRToI420(
      new Uint8Array(rgbaFrame.data),
      i420Frame.strideY,
      new Uint8Array(i420Frame.dataU),
      i420Frame.strideU,
      new Uint8Array(i420Frame.dataV),
      i420Frame.strideV,
      new Uint8Array(rgbaFrame.data),
      rgbaFrame.stride,
      i420Frame.width,
      i420Frame.height
    );
    return i420Frame;
  }

  get byteLength() {
    return this.sizeOfLuminancePlane  // Y
         + this.sizeOfChromaPlane     // U
         + this.sizeOfChromaPlane;    // V
  }

  get sizeOfLuminancePlane() {
    return this.width * this.height;
  }

  get sizeOfChromaPlane() {
    return this.sizeOfLuminancePlane / 4;
  }
}

export class RgbaFrame {
  constructor(readonly width = 640, readonly height = 480) {
    this.data = new Uint8ClampedArray(this.byteLength);
    Object.freeze(this);
  }

  data: Uint8ClampedArray;

  get stride() {
    return this.width * 4;
  }

  static fromI420(i420Frame) {
    const rgbaFrame = new RgbaFrame(i420Frame.width, i420Frame.height);
    I420ToRGBA(
      i420Frame.dataY, 
      i420Frame.strideY,
      i420Frame.dataU,
      i420Frame.strideU,
      i420Frame.dataV,
      i420Frame.strideV,
      new Uint8Array(rgbaFrame.data),
      rgbaFrame.stride,
      i420Frame.width,
      i420Frame.height
    );
    return rgbaFrame;
  }

  get byteLength() {
    return this.width * this.height * 4;
  }
}