#!/usr/bin/env python3
from pathlib import Path

p = Path("src/user/desktop.cpp")
s = p.read_text()

old = '''// v0.63 modern-material primitives. These intentionally remain cheap enough for
// the software compositor while giving the shell a much stronger depth hierarchy.
static void draw_status_chip(i32 x,i32 y,i32 w,const char* label,Color dot,Color textc){
    draw_round_blend(x,y,w,28,Color{18,31,46},205,9);
    draw_round_rect(x+10,y+9,9,9,dot,5);
    draw_modern_text(x+27,y+8,label,textc,1,true);
}
'''
assert old in s, "expected floating-HUD helper not found"
s = s.replace(old, "", 1)

old = '''    // v0.66 Aether Glass IV: narrow workspace rail with labels only on hover-like
    // selected state; avoids the oversized slab look while keeping orientation.
'''
new = '''    // v0.66 Aether Glass IV: narrow workspace rail with labels only on hover-like
    // selected state; avoids the oversized slab look while keeping orientation.
    // Trinity Control used to occupy a separate 296x118 desktop HUD here. It is
    // now part of the fixed-height top bar so the desktop canvas stays clear.
'''
assert old in s, "expected workspace-rail comment not found"
s = s.replace(old, new, 1)

old = '''
    const u8 gpu_mode=shell_gpu_mode();const bool intel_gpu=gpu_mode==ShellGpuIntel||gpu_mode==ShellGpuIntel3D;
    const i32 w=296,h=118,rx=sw-w-22,ry=82;
    draw_round_blend_gradient(rx,ry,w,h,Color{31,50,67},Color{9,21,34},184,17);draw_border(rx,ry,w,h,Color{61,87,112},1);
    draw_modern_text(rx+18,ry+14,"TRINITY CONTROL",TEXT,1,true);
    draw_status_chip(rx+18,ry+42,80,intel_gpu?"INTEL":"GPU",intel_gpu?ACCENT2:MUTED,intel_gpu?ACCENT2:MUTED);
    draw_status_chip(rx+106,ry+42,78,"USB",ACCENT,ACCENT);
    draw_status_chip(rx+192,ry+42,84,"WIFI",Color{238,195,102},Color{238,195,102});
    draw_round_blend(rx+18,ry+80,260,24,g_settings.theme?Color{224,234,244}:Color{12,23,35},142,8);
    draw_round_rect(rx+28,ry+88,8,8,intel_gpu?ACCENT2:Color{238,181,83},4);
    draw_modern_text(rx+44,ry+84,gpu_mode==ShellGpuIntel3D?"INTEL 3D ACTIVE":(intel_gpu?"INTEL GPU DESKTOP":"CPU FALLBACK READY"),intel_gpu?ACCENT2:Color{238,181,83},1,true);
'''
assert old in s, "expected floating Trinity Control HUD not found"
s = s.replace(old, "", 1)

old = '    draw_round_panel(sw-166,14,142,32,g_settings.theme?Color{229,239,246}:Color{10,27,42},g_settings.theme?Color{190,207,221}:Color{47,72,95},12,false);draw_round_rect(sw-154,25,8,8,ACCENT2,4);draw_modern_text(sw-137,22,"NUCLEUS",ACCENT2,1,true);\n'
new = '''    // Keep both system controls inside the existing 46px top-bar envelope.
    // Trinity Control sits immediately left of Nucleus without increasing the
    // bar height or consuming desktop workspace.
    const i32 nucleus_x=sw-166;const i32 control_x=nucleus_x-152;
    draw_round_panel(control_x,14,142,32,g_settings.theme?Color{229,239,246}:Color{10,27,42},g_settings.theme?Color{190,207,221}:Color{47,72,95},12,false);draw_round_rect(control_x+12,25,8,8,accent_color(),4);draw_modern_text(control_x+29,22,"TRINITY CONTROL",g_settings.theme?Color{49,61,73}:TEXT,1,true);
    draw_round_panel(nucleus_x,14,142,32,g_settings.theme?Color{229,239,246}:Color{10,27,42},g_settings.theme?Color{190,207,221}:Color{47,72,95},12,false);draw_round_rect(nucleus_x+12,25,8,8,ACCENT2,4);draw_modern_text(nucleus_x+29,22,"NUCLEUS",ACCENT2,1,true);
'''
assert old in s, "expected Nucleus top-bar control not found"
s = s.replace(old, new, 1)

old = 'const i32 sw=physical_screen_w(),sx=sw/2-170;if(sw>1100&&abs_in_rect(cursor.x,cursor.y,sx,13,340,32))'
new = 'const i32 sw=physical_screen_w(),sx=sw/2-170;const i32 nucleus_x=sw-166;const i32 control_x=nucleus_x-152;if(abs_in_rect(cursor.x,cursor.y,control_x,14,142,32)){menu=false;g_desktop_context=false;switch_process(4);continue;}if(sw>1100&&abs_in_rect(cursor.x,cursor.y,sx,13,340,32))'
assert old in s, "expected top-bar click-dispatch prefix not found"
s = s.replace(old, new, 1)

p.write_text(s)
print("Applied Trinity Control Taskbar R1 to src/user/desktop.cpp")
