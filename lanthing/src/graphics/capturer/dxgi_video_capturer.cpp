/*
 * BSD 3-Clause License
 *
 * Copyright (c) 2023 Zhennan Tu <zhennan.tu@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "dxgi_video_capturer.h"

#include <d3d11.h>
#include <dxgi.h>

#include <g3log/g3log.hpp>

#include <ltlib/strings.h>
#include <ltlib/times.h>

#if 0
static int64_t point1;
static int64_t point2;
static int64_t point3;
static int64_t point4;
static int64_t point5;
static int64_t point6;
static int64_t point7;
#define RECORD_T(x) x = ltlib::steady_now_us();
#define LOG_RECORD_T()                                                                             \
    LOGF(INFO, "RECORD_T %lld,%lld,%lld,%lld,%lld,%lld,%lld", point1, point2, point3, point4,      \
         point5, point6, point7)
#else
#define RECORD_T(X)
#define LOG_RECORD_T()
#endif

using namespace Microsoft::WRL;

namespace lt {

DxgiVideoCapturer::DxgiVideoCapturer()
    : impl_{std::make_unique<DUPLICATIONMANAGER>()}
    , texture_pool_(kDefaultPoolSize) {}

DxgiVideoCapturer::~DxgiVideoCapturer() {
    // 父类持有的线程会调用DxgiVideoCapturer的wait_for_vblank()
    // 必须让父类的线程先停下，才能析构DxgiVideoCapturer
    stop();
}

bool DxgiVideoCapturer::preInit() {
    if (!initD3D11()) {
        return false;
    }
    if (!impl_->InitDupl(d3d11_dev_.Get(), 0)) {
        LOG(WARNING) << "Failed to init DUPLICATIONMANAGER";
        return false;
    }
    return true;
}

bool DxgiVideoCapturer::initD3D11() {
    HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)dxgi_factory_.GetAddressOf());
    if (FAILED(hr)) {
        LOGF(WARNING, "Failed to create dxgi factory, er:%08x", hr);
        return false;
    }
    bool out_of_bound = false;
    int32_t index = -1;
    while (true) {
        index += 1;
        Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
        DXGI_ADAPTER_DESC adapter_desc;
        hr = dxgi_factory_->EnumAdapters(static_cast<UINT>(index), adapter.GetAddressOf());
        if (hr == DXGI_ERROR_NOT_FOUND) {
            LOGF(WARNING, "Failed to find no %u adapter", index);
            out_of_bound = true;
            break;
        }
        if (hr == DXGI_ERROR_INVALID_CALL) {
            LOG(FATAL) << "DXGI Factory == nullptr";
            return false;
        }
        hr = adapter->GetDesc(&adapter_desc);
        if (FAILED(hr)) {
            LOGF(WARNING, "Adapter %d GetDesc failed", index);
            continue;
        }
        UINT flag = 0;
#ifdef _DEBUG
        flag |= D3D11_CREATE_DEVICE_DEBUG;
#endif
        hr = D3D11CreateDevice(adapter.Get(), D3D_DRIVER_TYPE_UNKNOWN, nullptr, flag, nullptr, 0,
                               D3D11_SDK_VERSION, d3d11_dev_.GetAddressOf(), nullptr,
                               d3d11_ctx_.GetAddressOf());
        if (FAILED(hr)) {
            LOGF(WARNING, "Adapter(%x:%x) failed to create d3d11 device, err:%08lx",
                 adapter_desc.VendorId, adapter_desc.DeviceId, hr);
            continue;
        }
        luid_ =
            ((uint64_t)adapter_desc.AdapterLuid.HighPart << 32) + adapter_desc.AdapterLuid.LowPart;
        LOGF(INFO, "DxgiVideoCapturer using adapter(index:%d, %x:%x, %x)", index,
             adapter_desc.VendorId, adapter_desc.DeviceId, luid_);
        return true;
    }
    return false;
}

std::shared_ptr<ltproto::peer2peer::CaptureVideoFrame> DxgiVideoCapturer::captureOneFrame() {
    FRAME_DATA frame;
    bool timeout = false;
    RECORD_T(point1);
    auto hr = impl_->GetFrame(&frame, &timeout);
    if (hr == DUPL_RETURN::DUPL_RETURN_SUCCESS && !timeout) {
        RECORD_T(point2);
        std::string name = shareTexture(frame.Frame);
        RECORD_T(point6);
        impl_->DoneWithFrame();
        RECORD_T(point7);
        if (name.empty()) {
            return nullptr;
        }
        LOG_RECORD_T();
        auto capture_frame = std::make_shared<ltproto::peer2peer::CaptureVideoFrame>();
        capture_frame->set_name(name);
        capture_frame->set_capture_timestamp_us(ltlib::steady_now_us());
        return capture_frame;
    }
    return nullptr;
}

void DxgiVideoCapturer::releaseFrame(const std::string& name) {
    for (auto& texture : texture_pool_) {
        if (texture.name == name) {
            texture.in_use = false;
            return;
        }
    }
    LOG(FATAL) << "Should not reach here";
}

std::string DxgiVideoCapturer::shareTexture(ID3D11Texture2D* texture1) {
    if (!pool_inited_) {
        pool_inited_ = true;
        D3D11_TEXTURE2D_DESC desc;
        texture1->GetDesc(&desc);
        desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
        desc.MiscFlags =
            D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX | D3D11_RESOURCE_MISC_SHARED_NTHANDLE;
        for (size_t i = 0; i < kDefaultPoolSize; i++) {
            ComPtr<IDXGIResource1> resource;
            ComPtr<ID3D11Texture2D> texture2;
            auto hr = d3d11_dev_->CreateTexture2D(&desc, NULL, &texture2);
            if (FAILED(hr)) {
                LOGF(WARNING, "failed to create shared texture, hr:0x%08x", hr);
                return "";
            }
            hr = texture2.As(&resource);
            if (FAILED(hr)) {
                LOGF(WARNING, "Cast to IDXGIResource1 failed, hr:%#08x", hr);
                return "";
            }
            std::string name = "Global\\lanthing_dxgi_sharedTexture_" + std::to_string(i);
            HANDLE handle = nullptr;
            std::wstring w_name = ltlib::utf8To16(name);
            hr = resource->CreateSharedHandle(NULL, DXGI_SHARED_RESOURCE_READ, w_name.c_str(),
                                              &handle);
            if (FAILED(hr)) {
                LOGF(WARNING, "failed to create shared handle, hr:0x%08x", hr);
                return "";
            }
            texture_pool_[i].name = name;
            texture_pool_[i].texture = texture2;
            texture_pool_[i].handle = handle;
        }
    }
    std::optional<size_t> index = getFreeSharedTexture();
    if (!index.has_value()) {
        LOG(WARNING) << "No free shared texture";
        return "";
    }
    ComPtr<IDXGIKeyedMutex> mutex;
    HRESULT hr = texture_pool_[*index].texture.As(&mutex);
    if (FAILED(hr)) {
        LOGF(WARNING, "Cast to IDXGIKeyedMutex failed, hr:0x%08x", hr);
        return "";
    }
    RECORD_T(point3);
    hr = mutex->AcquireSync(0, 0);
    if (FAILED(hr)) {
        LOGF(WARNING, "drop frame, hr:0x%08x", hr);
        return "";
    }
    RECORD_T(point4);
    d3d11_ctx_->CopyResource(texture_pool_[*index].texture.Get(), texture1);
    RECORD_T(point5);
    // FIXME: 如果没有人打开过handle, 会持续failed
    mutex->ReleaseSync(1);
    return texture_pool_[*index].name;
}

std::optional<size_t> DxgiVideoCapturer::getFreeSharedTexture() {
    for (size_t index = 0; index < texture_pool_.size(); index++) {
        bool expected = false;
        if (texture_pool_[index].in_use.compare_exchange_strong(expected, true)) {
            return index;
        }
    }
    return {};
}

// void DxgiVideoCapturer::done_with_frame()
//{
//     // 在shared_texture()已经copy过一次，马上救DoneWithFrame()了，这里不应该再调用
//     //impl_->DoneWithFrame();
// }

void DxgiVideoCapturer::waitForVblank() {
    impl_->WaitForVBlank();
}

VideoCapturer::Backend DxgiVideoCapturer::backend() const {
    return Backend::Dxgi;
}

int64_t DxgiVideoCapturer::luid() {
    return luid_;
}

} // namespace lt