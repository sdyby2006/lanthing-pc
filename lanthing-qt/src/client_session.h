#pragma once
#include <cstdint>
#include <functional>
#include <memory>
#include <ltlib/threads.h>
#include <ltrtc/lttypes.h>

namespace lt
{

namespace ui
{

// TODO: �ĳ� ClientSession::create()����ʽ
class ClientSession
{
public:
    struct Params
    {
        std::string client_id;
        std::string room_id;
        std::string auth_token;
        std::string p2p_username;
        std::string p2p_password;
        std::string signaling_addr;
        int32_t signaling_port;
        ltrtc::VideoCodecType video_codec_type;
        uint32_t width;
        uint32_t height;
        uint32_t refresh_rate;
        bool enable_gamepad;
        bool enable_driver_input;
        std::function<void()> on_exited;
    };

public:
    ClientSession(const Params& params);
    ~ClientSession();
    bool start();
    std::string client_id() const;

private:
    void main_loop(const std::function<void()>& i_am_alive);

private:
    Params params_;
    int64_t process_id_;
    void* handle_;
    std::unique_ptr<ltlib::BlockingThread> thread_;
    bool stoped_ = true;
};

} // namespace ui

} // namespace lt