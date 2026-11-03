#include "src/dictionaries/webrtc/video_frame_buffer.h"

#include <libyuv.h>
#include <webrtc/api/video/i420_buffer.h>

#include "src/dictionaries/node_webrtc/image_data.h"
#include "src/functional/validation.h"

namespace node_webrtc {

static webrtc::scoped_refptr<webrtc::I420Buffer> CreateI420Buffer(
    I420ImageData i420Frame) {
  // Copy plane by plane: the I420Buffer's strides need not match the packed input's.
  return webrtc::I420Buffer::Copy(
      i420Frame.width(), i420Frame.height(),
      i420Frame.dataY(), i420Frame.strideY(),
      i420Frame.dataU(), i420Frame.strideU(),
      i420Frame.dataV(), i420Frame.strideV());
}

CONVERTER_IMPL(I420ImageData, webrtc::scoped_refptr<webrtc::I420Buffer>, value) {
  return Pure(CreateI420Buffer(value));
}

TO_NAPI_IMPL(webrtc::scoped_refptr<webrtc::VideoFrameBuffer>, pair) {
  auto value = pair.second;
  return value->type() == webrtc::VideoFrameBuffer::Type::kI420
      ? From<Napi::Value>(std::make_pair(pair.first, value->GetI420()))
      : Validation<Napi::Value>::Invalid("Unsupported RTCVideoFrame type (file a bug in @/webrtc, please!)");
}

CONVERT_VIA(Napi::Value, I420ImageData, webrtc::scoped_refptr<webrtc::I420Buffer>)

TO_NAPI_IMPL(const webrtc::I420BufferInterface*, pair) {
  auto env = pair.first;
  Napi::EscapableHandleScope scope(env);
  auto value = pair.second;

  // Pack into the same layout RTCVideoSource accepts (see I420ImageData): tightly-packed Y, U, V planes, with
  // chroma planes of ceil(width / 2) x ceil(height / 2). The source planes may be padded (stride > width).
  auto width = value->width();
  auto height = value->height();
  auto chromaWidth = I420ImageData::chromaWidth(width);
  auto sizeOfYPlane = static_cast<size_t>(width) * height;
  auto sizeOfChromaPlane = static_cast<size_t>(chromaWidth) * I420ImageData::chromaHeight(height);
  auto byteLength = I420ImageData::byteLength(width, height);

  auto maybeArrayBuffer = Napi::ArrayBuffer::New(env, byteLength);
  if (maybeArrayBuffer.Env().IsExceptionPending()) {
    return Validation<Napi::Value>::Invalid(maybeArrayBuffer.Env().GetAndClearPendingException().Message());
  }
  auto data = static_cast<uint8_t*>(maybeArrayBuffer.Data());

  auto dstYPlane = data;
  auto dstUPlane = dstYPlane + sizeOfYPlane;
  auto dstVPlane = dstUPlane + sizeOfChromaPlane;

  libyuv::I420Copy(
      value->DataY(), value->StrideY(),
      value->DataU(), value->StrideU(),
      value->DataV(), value->StrideV(),
      dstYPlane, width,
      dstUPlane, chromaWidth,
      dstVPlane, chromaWidth,
      width, height);

  // FIXME(mroberts): How to create a Uint8ClampedArray?
  auto maybeUint8Array = Napi::Uint8Array::New(env, byteLength, maybeArrayBuffer, 0);
  if (maybeUint8Array.Env().IsExceptionPending()) {
    return Validation<Napi::Value>::Invalid(maybeUint8Array.Env().GetAndClearPendingException().Message());
  }

  return Pure(scope.Escape(maybeUint8Array));
}

} //  namespace node_webrtc
