#include "video_capturer.h"
#include <g3log/g3log.hpp>
#include "dxgi_video_capturer.h"

namespace lt
{

std::unique_ptr<VideoCapturer> VideoCapturer::create(const Params& params)
{
    if (params.backend != Backend::Dxgi) {
        LOG(FATAL) << "Only support dxgi video capturer!";
        return nullptr;
    }
    if (params.on_frame == nullptr) {
        LOG(FATAL) << "Create video capturer without callback!";
        return nullptr;
    }
    auto capturer = std::make_unique<DxgiVideoCapturer>();
    capturer->on_frame_ = params.on_frame;
    if (!capturer->init()) {
        return nullptr;
    }
    return capturer;
}

VideoCapturer::VideoCapturer()
    : stop_promise_ { std::make_unique<std::promise<void>>() }
{
}

VideoCapturer::~VideoCapturer()
{
    stop();
}

bool VideoCapturer::init()
{
    if (!pre_init()) {
        return false;
    }
    thread_ = ltlib::BlockingThread::create(
        "video_capture",
        [this](const std::function<void()>& i_am_alive, void*) {
            main_loop(i_am_alive);
    }, nullptr);
    return true;
}

void VideoCapturer::main_loop(const std::function<void()>& i_am_alive)
{
    stoped_ = false;
    LOG(INFO) << "Video capturer started";
    while (!stoped_) {
        i_am_alive();
        auto frame = capture_one_frame();
        if (frame) {
            //TODO: ��������frame������ֵ
            frame->set_picture_id(frame_no_++);
            on_frame_(frame);
        }
        wait_for_vblank();
    }
    stop_promise_->set_value();
}

void VideoCapturer::stop()
{
    //����stoped_��ԭ��Ҳ��Ӧ����ô��������������ƺ��������һ���߳���������VideoCapture����һ���߳�������stop()
    if (!stoped_) {
        stoped_ = true;
        stop_promise_->get_future().get();
    }
}

} // namespace lt