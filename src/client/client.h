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

#pragma once
#include <cstdint>

#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <string>

#include <ltlib/io/client.h>
#include <ltlib/io/ioloop.h>
#include <ltlib/settings.h>
#include <ltlib/threads.h>
#include <ltlib/time_sync.h>
#include <transport/transport.h>

#include <audio/player/audio_player.h>
#include <inputs/capturer/input_capturer.h>
#include <plat/pc_sdl.h>
#include <plat/video_device.h>
#include <video/drpipeline/video_decode_render_pipeline.h>

namespace lt {

namespace cli {

struct SignalingParams {
    SignalingParams(const std::string& _client_id, const std::string& _room_id,
                    const std::string& _addr, uint16_t _port)
        : client_id(_client_id)
        , room_id(_room_id)
        , addr(_addr)
        , port(_port) {}
    std::string client_id;
    std::string room_id;
    std::string addr;
    uint16_t port;
};

class Client {
public:
    struct Params {
        std::string client_id;
        std::string room_id;
        std::string auth_token;
        std::string user;
        std::string pwd;
        std::string signaling_addr;
        uint16_t signaling_port;
        std::string codec;
        uint32_t width;
        uint32_t height;
        uint32_t screen_refresh_rate;
        uint32_t audio_freq;
        uint32_t audio_channels;
        uint32_t rotation;
        int32_t transport_type;
        bool enable_driver_input;
        bool enable_gamepad;
        std::vector<std::string> reflex_servers;
    };

public:
    static std::unique_ptr<Client> create(std::map<std::string, std::string> options);
    ~Client();
    int32_t loop();

private:
    Client(const Params& params);
    bool init();
    bool initSettings();
    bool initSignalingClient();
    bool initAppClient();
    void ioLoop(const std::function<void()>& i_am_alive);
    void onPlatformRenderTargetReset();
    void postTask(const std::function<void()>& task);
    void postDelayTask(int64_t delay_ms, const std::function<void()>& task);
    void syncTime();
    void toggleFullscreen();
    void switchMouseMode();
    void checkWorkerTimeout();
    void tellAppKeepAliveTimeout();

    // app
    void onAppConnected();
    void onAppDisconnected();
    void onAppReconnecting();
    void onAppMessage(uint32_t type, std::shared_ptr<google::protobuf::MessageLite> msg);
    void onAppClipboard(std::shared_ptr<google::protobuf::MessageLite> msg);

    // 信令.
    void onSignalingNetMessage(uint32_t type, std::shared_ptr<google::protobuf::MessageLite> msg);
    void onSignalingDisconnected();
    void onSignalingReconnecting();
    void onSignalingConnected();
    void onJoinRoomAck(std::shared_ptr<google::protobuf::MessageLite> msg);
    void onSignalingMessage(std::shared_ptr<google::protobuf::MessageLite> msg);
    void onSignalingMessageAck(std::shared_ptr<google::protobuf::MessageLite> msg);
    void dispatchSignalingMessageRtc(std::shared_ptr<google::protobuf::MessageLite> msg);
    void dispatchSignalingMessageCore(std::shared_ptr<google::protobuf::MessageLite> msg);
    void sendKeepaliveToSignalingServer();

    // transport
    bool initTransport();
    tp::Client* createTcpClient();
    tp::Client* createRtcClient();
    tp::Client* createRtc2Client();
    static void onTpData(void* user_data, const uint8_t* data, uint32_t size, bool is_reliable);
    static void onTpVideoFrame(void* user_data, const lt::VideoFrame& frame);
    static void onTpAudioData(void* user_data, const lt::AudioData& audio_data);
    static void onTpConnected(void* user_data, lt::LinkType link_type);
    static void onTpConnChanged(void* user_data, lt::LinkType old_type, lt::LinkType new_type);
    static void onTpFailed(void* user_data);
    static void onTpDisconnected(void* user_data);
    static void onTpSignalingMessage(void* user_data, const char* key, const char* value);

    // 数据通道.
    void dispatchRemoteMessage(uint32_t type,
                               const std::shared_ptr<google::protobuf::MessageLite>& msg);
    void sendKeepAlive();
    void onKeepAliveAck();
    bool sendMessageToHost(uint32_t type, const std::shared_ptr<google::protobuf::MessageLite>& msg,
                           bool reliable);
    void sendMessageToHostFromOtherModule(uint32_t type,
                                          const std::shared_ptr<google::protobuf::MessageLite>& msg,
                                          bool reliable);
    void onStartTransmissionAck(const std::shared_ptr<google::protobuf::MessageLite>& msg);
    void onTimeSync(std::shared_ptr<google::protobuf::MessageLite> msg);
    void onSendSideStat(std::shared_ptr<google::protobuf::MessageLite> msg);
    void onCursorInfo(std::shared_ptr<google::protobuf::MessageLite> msg);
    void onChangeStreamingParams(std::shared_ptr<google::protobuf::MessageLite> msg);
    void onRemoteClipboard(std::shared_ptr<google::protobuf::MessageLite> msg);
    void onRemotePullFile(std::shared_ptr<google::protobuf::MessageLite> msg);
    void onRemoteFileChunk(std::shared_ptr<google::protobuf::MessageLite> msg);
    void onRemoteFileChunkAck(std::shared_ptr<google::protobuf::MessageLite> msg);
    void onUserSwitchStretch();
    void resetVideoPipeline();

private:
    std::unique_ptr<ltlib::Settings> settings_;
    std::string auth_token_;
    std::string p2p_username_;
    std::string p2p_password_;
#if LT_WINDOWS
    std::atomic<bool> is_stretch_{false};
#else  // LT_WINDOWS
    std::atomic<bool> is_stretch_{true};
#endif // LT_WINDOWS
    SignalingParams signaling_params_;
    input::Capturer::Params input_params_{};
    video::DecodeRenderPipeline::Params video_params_;
    audio::Player::Params audio_params_{};
    std::vector<std::string> reflex_servers_;
    int32_t transport_type_;
    std::unique_ptr<plat::VideoDevice> video_device_;
    std::mutex dr_mutex_;
    std::unique_ptr<video::DecodeRenderPipeline> video_pipeline_;
    std::unique_ptr<input::Capturer> input_capturer_;
    std::unique_ptr<audio::Player> audio_player_;
    std::shared_mutex ioloop_mutex_;
    std::unique_ptr<ltlib::IOLoop> ioloop_;
    std::unique_ptr<ltlib::Client> signaling_client_;
    std::unique_ptr<ltlib::Client> app_client_;
    lt::tp::Client* tp_client_ = nullptr;
    std::unique_ptr<lt::plat::PcSdl> sdl_;
    std::unique_ptr<ltlib::BlockingThread> io_thread_;
    std::condition_variable exit_cv_;
    ltlib::TimeSync time_sync_;
    int64_t rtt_ = 0;
    int64_t time_diff_ = 0;
    bool windowed_fullscreen_ = true;
    int64_t status_color_ = -1;
    bool signaling_keepalive_inited_ = false;
    lt::LinkType link_type_ = LinkType::Unknown;
    bool absolute_mouse_ = true;
    bool last_w_or_h_is_0_ = false;
    int64_t last_received_keepalive_ = 0;
    bool connected_to_app_ = false;
    std::string ignored_nic_;
    bool stoped_ = false;
    std::map<int32_t, lt::CursorInfo> cursors_;
    std::mutex cursor_mtx_;
};

} // namespace cli

} // namespace lt