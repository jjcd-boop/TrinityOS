#pragma once
#include "base.hpp"
#include "ai_action_protocol.hpp"
namespace trinity::ai_provider {
constexpr usize MaxPromptBytes=8192u;
constexpr usize MaxRequestBodyBytes=32u*1024u;
constexpr usize MaxResponseBytes=128u*1024u;
constexpr usize MaxHostBytes=96u;
constexpr usize MaxModelBytes=64u;
constexpr usize MaxUrlBytes=192u;
constexpr usize MaxTextBytes=4096u;
enum class Status:u32 {Disabled=0,Ready=1,Busy=2,RuntimeUnavailable=3,Complete=4,InvalidEndpoint=5,InvalidRequest=6,InvalidResponse=7,Cancelled=8,TransportError=9};
struct RemoteEndpoint {u32 enabled{0};u16 port{11434};u16 reserved{0};char host[MaxHostBytes+1]{};char model[MaxModelBytes+1]{};};
struct Prompt {u32 bytes{0};char text[MaxPromptBytes+1]{};};
struct HttpRequest {char url[MaxUrlBytes]{};char content_type[32]{};u32 body_bytes{0};char body[MaxRequestBodyBytes]{};};
struct Result {Status status{Status::Disabled};u32 error{0};u8 has_action{0};u8 reserved[7]{};ai_action::Request action{};u32 text_bytes{0};char text[MaxTextBytes]{};};
struct State {Status current{Status::Disabled};ai_action::ProviderId provider{ai_action::ProviderId::None};RemoteEndpoint remote{};u64 sequence{0};u8 active{0};u8 reserved[7]{};};
Status status(const State&) noexcept;
bool configure_remote(State&,const RemoteEndpoint&) noexcept;
bool submit(State&,ai_action::ProviderId,const Prompt&,HttpRequest&,Result&) noexcept;
bool consume_response(State&,const char*,usize,Result&) noexcept;
void cancel(State&) noexcept;
namespace detail {
bool endpoint_valid(const RemoteEndpoint&) noexcept;
bool build_ollama_request(const RemoteEndpoint&,const Prompt&,HttpRequest&) noexcept;
bool parse_ollama_response(const char*,usize,Result&) noexcept;
}
}
