#pragma once
#include "ai_action_protocol.hpp"
#include "process.hpp"

namespace trinity::ai_action_broker {
struct Context {
    ai_action::ProviderId provider{ai_action::ProviderId::None};
    process::Id caller{process::Id::Desktop};
    bool provider_originated{false};
};
struct Hooks {
    void* user{nullptr};
    bool (*launch_app)(void*, ai_action::AppTarget){nullptr};
    bool (*open_folder)(void*, ai_action::FolderTarget){nullptr};
    bool (*open_settings)(void*, ai_action::SettingsSection){nullptr};
    bool (*battery_status)(void*, u64&, u64&){nullptr};
    bool (*network_status)(void*, bool diagnostics, u64&, u64&){nullptr};
    bool (*process_memory_summary)(void*, u64&, u64&){nullptr};
    bool (*adjust_volume)(void*, i32 delta, u64&){nullptr};
    bool (*radio)(void*, bool next){nullptr};
};
u32 app_slot(ai_action::AppTarget) noexcept;
const char* folder_path(ai_action::FolderTarget) noexcept;
u32 settings_section_code(ai_action::SettingsSection) noexcept;
ai_action::Result execute(const ai_action::Request&, const Context&, const Hooks&) noexcept;
ai_action::AuditSnapshot audit_snapshot() noexcept;
void reset_for_test() noexcept;
} // namespace trinity::ai_action_broker
