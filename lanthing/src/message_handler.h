#pragma once
#include <cstdint>
#include <functional>
#include <memory>

#include <google/protobuf/message_lite.h>

namespace lt {

using MessageHandler = std::function<void(const std::shared_ptr<google::protobuf::MessageLite>&)>;

} // namespace lt