/*
// Copyright (c) 2016 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include "overlaylayer.h"

#include <drm_mode.h>
#include <hwctrace.h>

#include "hwcutils.h"

namespace hwcomposer {

void OverlayLayer::SetAcquireFence(int32_t acquire_fence) {
  // Release any existing fence.
  if (imported_buffer_->acquire_fence_ > 0) {
    close(imported_buffer_->acquire_fence_);
  }

  imported_buffer_->acquire_fence_ = acquire_fence;
  acquire_fence_signalled_ = false;
}

int32_t OverlayLayer::GetAcquireFence() const {
  return imported_buffer_->acquire_fence_;
}

void OverlayLayer::WaitAcquireFence() {
  if (imported_buffer_->acquire_fence_ > 0 && !acquire_fence_signalled_) {
    HWCPoll(imported_buffer_->acquire_fence_, -1);
    acquire_fence_signalled_ = true;
  }
}

OverlayBuffer* OverlayLayer::GetBuffer() const {
  return imported_buffer_->buffer_;
}

void OverlayLayer::SetBuffer(ImportedBuffer* buffer, int32_t acquire_fence) {
  imported_buffer_.reset(buffer);
  imported_buffer_->acquire_fence_ = acquire_fence;
}

void OverlayLayer::ReleaseBuffer() {
  imported_buffer_->owned_buffer_ = false;
}

void OverlayLayer::SetIndex(uint32_t index) {
  index_ = index;
}

void OverlayLayer::SetTransform(int32_t transform) {
  transform_ = transform;
  rotation_ = 0;
  if (transform & kReflectX)
    rotation_ |= 1 << DRM_REFLECT_X;
  if (transform & kReflectY)
    rotation_ |= 1 << DRM_REFLECT_Y;
  if (transform & kRotate90)
    rotation_ |= 1 << DRM_ROTATE_90;
  else if (transform & kRotate180)
    rotation_ |= 1 << DRM_ROTATE_180;
  else if (transform & kRotate270)
    rotation_ |= 1 << DRM_ROTATE_270;
  else
    rotation_ |= 1 << DRM_ROTATE_0;
}

void OverlayLayer::SetAlpha(uint8_t alpha) {
  alpha_ = alpha;
}

void OverlayLayer::SetBlending(HWCBlending blending) {
  blending_ = blending;
}

void OverlayLayer::SetSourceCrop(const HwcRect<float>& source_crop) {
  source_crop_width_ =
      static_cast<int>(source_crop.right) - static_cast<int>(source_crop.left);
  source_crop_height_ =
      static_cast<int>(source_crop.bottom) - static_cast<int>(source_crop.top);
  source_crop_ = source_crop;
}

void OverlayLayer::SetDisplayFrame(const HwcRect<int>& display_frame) {
  display_frame_width_ = display_frame.right - display_frame.left;
  display_frame_height_ = display_frame.bottom - display_frame.top;
  display_frame_ = display_frame;
}

void OverlayLayer::ValidatePreviousFrameState(const OverlayLayer& rhs) {
  OverlayBuffer* buffer = imported_buffer_->buffer_;
  if (buffer->GetFormat() != rhs.imported_buffer_->buffer_->GetFormat())
    return;

  // We expect cursor plane to support alpha always.
  if (!(buffer->GetUsage() & kLayerCursor)) {
    if (alpha_ != rhs.alpha_)
      return;
  }

  if (blending_ != rhs.blending_)
    return;

  // We check only for rotation and not transform as this
  // value is arrived from transform_
  if (rotation_ != rhs.rotation_)
    return;

  if (display_frame_width_ != rhs.display_frame_width_)
    return;

  if (display_frame_height_ != rhs.display_frame_height_)
    return;

  if (source_crop_width_ != rhs.source_crop_width_)
    return;

  if (source_crop_height_ != rhs.source_crop_height_)
    return;

  layer_attributes_changed_ = false;

  const HwcRect<int>& previous = rhs.GetDisplayFrame();
  if ((previous.left == display_frame_.left) &&
      (previous.top == display_frame_.top)) {
    layer_pos_changed_ = false;
  }
}

void OverlayLayer::SetSurfaceDamage(const HwcRegion& surface_damage,
                                    const OverlayLayer& rhs) {
  ValidatePreviousFrameState(rhs);
}

void OverlayLayer::Dump() {
  DUMPTRACE("OverlayLayer Information Starts. -------------");
  switch (blending_) {
    case HWCBlending::kBlendingNone:
      DUMPTRACE("Blending: kBlendingNone.");
      break;
    case HWCBlending::kBlendingPremult:
      DUMPTRACE("Blending: kBlendingPremult.");
      break;
    case HWCBlending::kBlendingCoverage:
      DUMPTRACE("Blending: kBlendingCoverage.");
      break;
    default:
      break;
  }

  if (transform_ & kReflectX)
    DUMPTRACE("Transform: kReflectX.");
  if (transform_ & kReflectY)
    DUMPTRACE("Transform: kReflectY.");
  if (transform_ & kReflectY)
    DUMPTRACE("Transform: kReflectY.");
  else if (transform_ & kRotate180)
    DUMPTRACE("Transform: kRotate180.");
  else if (transform_ & kRotate270)
    DUMPTRACE("Transform: kRotate270.");
  else
    DUMPTRACE("Transform: kRotate0.");

  DUMPTRACE("Alpha: %u", alpha_);

  DUMPTRACE("SourceWidth: %d", source_crop_width_);
  DUMPTRACE("SourceHeight: %d", source_crop_height_);
  DUMPTRACE("DstWidth: %d", display_frame_width_);
  DUMPTRACE("DstHeight: %d", display_frame_height_);
  DUMPTRACE("AquireFence: %d", imported_buffer_->acquire_fence_);

  imported_buffer_->buffer_->Dump();
}

}  // namespace hwcomposer
