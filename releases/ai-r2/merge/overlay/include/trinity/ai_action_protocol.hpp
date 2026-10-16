#pragma once
#include "base.hpp"

namespace trinity::ai_action {
constexpr u32 ContractVersion = 1u;
constexpr usize AuditCapacity = 128u;

enum class ProviderId : u32 { None=0, LocalFunctionGemma=1, RemoteOllama=2 };
enum class PermissionClass : u32 { ReadOnly=1, BenignControl=2, Privileged=3 };
enum class ActionId : u32 {
    Invalid=0,
    BatteryStatus=1, NetworkStatus=2, NetworkDiagnostics=3, ProcessMemorySummary=4,
    ActionBrokerStatus=5, AuditSnapshot=6,
    LaunchApp=16, OpenFolder=17, OpenSettings=18, AdjustVolume=19, RadioPlay=20, RadioNext=21,
};
enum class Disposition : u32 { Invalid=0, Allowed=1, Denied=2, Unsupported=3, Failed=4, NotReady=5 };
enum class AppTarget : u32 { Writer=1, Calculator=2, Explorer=3, Settings=4, Browser=5, Media=6, Games=7, Sheets=9, Slides=10, Notes=11 };
enum class FolderTarget : u32 { Home=1, Desktop=2, Documents=3, Downloads=4 };
enum class SettingsSection : u32 { General=1, Network=2, Power=3, Mouse=4, Display=5, Audio=6, AI=7 };

struct Request {
    u32 version{ContractVersion};
    ProviderId provider{ProviderId::None};
    ActionId action{ActionId::Invalid};
    PermissionClass permission{PermissionClass::ReadOnly};
    u64 request_id{0};
    u64 arg0{0};
    u64 arg1{0};
    i64 sarg0{0};
    u32 flags{0};
    u32 reserved{0};
};

struct Result {
    u32 version{ContractVersion};
    Disposition disposition{Disposition::Invalid};
    ActionId action{ActionId::Invalid};
    u32 detail{0};
    u64 request_id{0};
    u64 value0{0};
    u64 value1{0};
};

struct AuditEntry {
    u64 sequence{0};
    u64 timestamp{0};
    u64 request_id{0};
    ProviderId provider{ProviderId::None};
    ActionId action{ActionId::Invalid};
    PermissionClass permission{PermissionClass::ReadOnly};
    Disposition disposition{Disposition::Invalid};
    u32 detail{0};
    u32 reserved{0};
};
struct AuditSnapshot { u32 version{ContractVersion}; u32 count{0}; u64 latest_sequence{0}; AuditEntry entries[AuditCapacity]{}; };

} // namespace trinity::ai_action
