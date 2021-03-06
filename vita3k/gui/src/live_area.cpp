﻿// Vita3K emulator project
// Copyright (C) 2020 Vita3K team
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

#include "private.h"

#include <gui/functions.h>

#include <io/VitaIoDevice.h>
#include <io/vfs.h>
#include <util/log.h>

#include <pugixml.hpp>

#include <SDL_keycode.h>

#include <chrono>
#include <sstream>
#include <stb_image.h>

namespace gui {

void draw_information_bar(GuiState &gui) {
    const auto display_size = ImGui::GetIO().DisplaySize;
    const auto SCAL = ImVec2(display_size.x / 960.0f, display_size.y / 544.0f);
    const auto MENUBAR_HEIGHT = 32.f * SCAL.y;
    ImU32 DEFAUL_BAR_COLOR = 4278190080; // Black
    ImU32 DEFAUL_INDICATOR_COLOR = 4294967295; // White

    const auto is_theme_color = (gui.live_area.start_screen || gui.live_area.live_area_screen);

    ImGui::SetNextWindowPos(ImVec2(0.f, 0.f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(display_size.x, MENUBAR_HEIGHT), ImGuiCond_Always);
    ImGui::Begin("##information_bar", nullptr, ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings);

    const auto scal_font = 19.2f / ImGui::GetFontSize();

    const auto now = std::chrono::system_clock::now();
    const auto tt = std::chrono::system_clock::to_time_t(now);
    const auto local = *localtime(&tt);

    ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(0.f, 0.f), ImVec2(display_size.x, MENUBAR_HEIGHT), is_theme_color ? gui.information_bar_color["bar"] : DEFAUL_BAR_COLOR, 0.f, ImDrawCornerFlags_All);
    ImGui::GetForegroundDrawList()->AddText(gui.live_area_font, (20.f * scal_font) * SCAL.x, ImVec2(display_size.x - (122.f * SCAL.x), 6.f * SCAL.y), is_theme_color ? gui.information_bar_color["indicator"] : DEFAUL_INDICATOR_COLOR, fmt::format("{:0>2d}:{:0>2d}", local.tm_hour, local.tm_min).c_str());
    ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(display_size.x - (54.f * SCAL.x), 12.f * SCAL.y), ImVec2(display_size.x - 50.f, 20 * SCAL.y), IM_COL32(81.f, 169.f, 32.f, 255.f), 0.f, ImDrawCornerFlags_All);
    ImGui::GetForegroundDrawList()->AddRectFilled(ImVec2(display_size.x - (50.f * SCAL.x), 5.f * SCAL.y), ImVec2(display_size.x - 12.f, 27 * SCAL.y), IM_COL32(81.f, 169.f, 32.f, 255.f), 2.f, ImDrawCornerFlags_All);

    ImGui::End();
}

#ifdef _WIN32
static const char OS_PREFIX[] = "start ";
#elif __APPLE__
static const char OS_PREFIX[] = "open ";
#else
static const char OS_PREFIX[] = "xdg-open ";
#endif

struct FRAME {
    std::string id;
    std::string multi;
    SceUInt64 autoflip;
};

struct STR {
    std::string color;
    float size;
    std::string text;
};

static std::map<std::string, std::vector<FRAME>> frames;
static std::map<std::string, std::map<std::string, std::vector<STR>>> str;
static std::map<std::string, std::map<std::string, std::map<std::string, std::map<std::string, std::pair<SceInt32, std::string>>>>> liveitem;
static std::map<std::string, std::map<std::string, std::map<std::string, std::map<std::string, ImVec2>>>> items;
static std::map<std::string, std::map<std::string, std::map<std::string, ImVec2>>> items_pos;
static std::map<std::string, std::map<std::string, std::string>> target;
static std::map<std::string, std::map<std::string, uint64_t>> current_item, last_time;
static std::map<std::string, std::string> type;
static std::string start;
static int32_t current_app;
static std::vector<gui::App> app_type;

void init_live_area(GuiState &gui, HostState &host) {
    std::string user_lang;
    const auto sys_lang = static_cast<SceSystemParamLang>(host.cfg.sys_lang);
    switch (sys_lang) {
    case SCE_SYSTEM_PARAM_LANG_JAPANESE: user_lang = "ja", start = u8"はじめる"; break;
    case SCE_SYSTEM_PARAM_LANG_ENGLISH_US: user_lang = "en", start = "Start"; break;
    case SCE_SYSTEM_PARAM_LANG_FRENCH: user_lang = "fr", start = "Demarrer"; break;
    case SCE_SYSTEM_PARAM_LANG_SPANISH: user_lang = "es", start = "Iniciar"; break;
    case SCE_SYSTEM_PARAM_LANG_GERMAN: user_lang = "de", start = "Starten"; break;
    case SCE_SYSTEM_PARAM_LANG_ITALIAN: user_lang = "it", start = "Avvia"; break;
    case SCE_SYSTEM_PARAM_LANG_DUTCH: user_lang = "nl", start = "Starten"; break;
    case SCE_SYSTEM_PARAM_LANG_PORTUGUESE_PT: user_lang = "pt", start = "Iniciar"; break;
    case SCE_SYSTEM_PARAM_LANG_RUSSIAN: user_lang = "ru", start = u8"Запуск"; break;
    case SCE_SYSTEM_PARAM_LANG_KOREAN: user_lang = "ko", start = u8"시작"; break;
    case SCE_SYSTEM_PARAM_LANG_CHINESE_T: user_lang = "ch", start = u8"开始"; break;
    case SCE_SYSTEM_PARAM_LANG_CHINESE_S: user_lang = "zh", start = u8"啟動"; break;
    case SCE_SYSTEM_PARAM_LANG_FINNISH: user_lang = "fi", start = "Rozpocznij"; break;
    case SCE_SYSTEM_PARAM_LANG_SWEDISH: user_lang = "sv", start = "Starta"; break;
    case SCE_SYSTEM_PARAM_LANG_DANISH: user_lang = "da", start = "Start"; break;
    case SCE_SYSTEM_PARAM_LANG_NORWEGIAN: user_lang = "no", start = "Start"; break;
    case SCE_SYSTEM_PARAM_LANG_POLISH: user_lang = "pl", start = "Rozpocznij"; break;
    case SCE_SYSTEM_PARAM_LANG_PORTUGUESE_BR: user_lang = "pt-br", start = "Iniciar"; break;
    case SCE_SYSTEM_PARAM_LANG_ENGLISH_GB: user_lang = "en-gb", start = "Start"; break;
    case SCE_SYSTEM_PARAM_LANG_TURKISH: user_lang = "tr", start = u8"Başlat"; break;
    default: break;
    }

    // Init type
    if (items_pos.empty()) {
        // A1
        items_pos["a1"]["gate"]["pos"] = ImVec2(620.f, 364.f);
        items_pos["a1"]["frame1"]["pos"] = ImVec2(900.f, 415.f);
        items_pos["a1"]["frame1"]["size"] = ImVec2(260.f, 260.f);
        items_pos["a1"]["frame2"]["pos"] = ImVec2(320.f, 415.f);
        items_pos["a1"]["frame2"]["size"] = ImVec2(260.f, 260.f);
        items_pos["a1"]["frame3"]["pos"] = ImVec2(900.f, 155.f);
        items_pos["a1"]["frame3"]["size"] = ImVec2(840.f, 150.f);
        // A2
        items_pos["a2"]["gate"]["pos"] = ImVec2(620.f, 390.f);
        items_pos["a2"]["frame1"]["pos"] = ImVec2(900.f, 405.f);
        items_pos["a2"]["frame1"]["size"] = ImVec2(260.f, 400.f);
        items_pos["a2"]["frame2"]["pos"] = ImVec2(320.f, 405.f);
        items_pos["a2"]["frame2"]["size"] = ImVec2(260.f, 400.f);
        items_pos["a2"]["frame3"]["pos"] = ImVec2(640.f, 205.f);
        items_pos["a2"]["frame3"]["size"] = ImVec2(320.f, 200.f);
        // A3
        items_pos["a3"]["gate"]["pos"] = ImVec2(620.f, 394.f);
        items_pos["a3"]["frame1"]["pos"] = ImVec2(900.f, 415.f);
        items_pos["a3"]["frame1"]["size"] = ImVec2(260.f, 200.f);
        items_pos["a3"]["frame2"]["pos"] = ImVec2(320.f, 415.f);
        items_pos["a3"]["frame2"]["size"] = ImVec2(260.f, 200.f);
        items_pos["a3"]["frame3"]["pos"] = ImVec2(900.f, 215.f);
        items_pos["a3"]["frame3"]["size"] = ImVec2(260.f, 210.f);
        items_pos["a3"]["frame4"]["pos"] = ImVec2(640.f, 215.f);
        items_pos["a3"]["frame4"]["size"] = ImVec2(320.f, 210.f);
        items_pos["a3"]["frame5"]["pos"] = ImVec2(320.f, 215.f);
        items_pos["a3"]["frame5"]["size"] = ImVec2(260.f, 210.f);
        // A4
        items_pos["a4"]["gate"]["pos"] = ImVec2(620.f, 394.f);
        items_pos["a4"]["frame1"]["pos"] = ImVec2(900.f, 415.f);
        items_pos["a4"]["frame1"]["size"] = ImVec2(260.f, 200.f);
        items_pos["a4"]["frame2"]["pos"] = ImVec2(320.f, 415.f);
        items_pos["a4"]["frame2"]["size"] = ImVec2(260.f, 200.f);
        items_pos["a4"]["frame3"]["pos"] = ImVec2(900.f, 215.f);
        items_pos["a4"]["frame3"]["size"] = ImVec2(840.f, 70.f);
        items_pos["a4"]["frame4"]["pos"] = ImVec2(900.f, 145.f);
        items_pos["a4"]["frame4"]["size"] = ImVec2(840.f, 70.f);
        items_pos["a4"]["frame5"]["pos"] = ImVec2(900.f, 75.f);
        items_pos["a4"]["frame5"]["size"] = ImVec2(840.f, 70.f);
        // A5
        items_pos["a5"]["gate"]["pos"] = ImVec2(380.f, 388.f);
        items_pos["a5"]["frame1"]["pos"] = ImVec2(900.f, 414.f);
        items_pos["a5"]["frame1"]["size"] = ImVec2(480.f, 68.f);
        items_pos["a5"]["frame2"]["pos"] = ImVec2(900.f, 345.f);
        items_pos["a5"]["frame2"]["size"] = ImVec2(480.f, 68.f);
        items_pos["a5"]["frame3"]["pos"] = ImVec2(900.f, 278.f);
        items_pos["a5"]["frame3"]["size"] = ImVec2(480.f, 68.f);
        items_pos["a5"]["frame4"]["pos"] = ImVec2(900.f, 210.f);
        items_pos["a5"]["frame4"]["size"] = ImVec2(480.f, 68.f);
        items_pos["a5"]["frame5"]["pos"] = ImVec2(900.f, 142.f);
        items_pos["a5"]["frame5"]["size"] = ImVec2(480.f, 68.f);
        items_pos["a5"]["frame6"]["pos"] = ImVec2(900.f, 74.f);
        items_pos["a5"]["frame6"]["size"] = ImVec2(480.f, 68.f);
        items_pos["a5"]["frame7"]["pos"] = ImVec2(420.f, 220.f);
        items_pos["a5"]["frame7"]["size"] = ImVec2(360.f, 210.f);
        // psmobile
        items_pos["psmobile"]["gate"]["pos"] = ImVec2(380.f, 339.f);
        //items_pos["psmobile"]["frame1"]["pos"] = ImVec2(900.f, 414.f);
        //items_pos["psmobile"]["frame1"]["size"] = ImVec2(480.f, 68.f);
        items_pos["psmobile"]["frame2"]["pos"] = ImVec2(865.f, 215.f);
        items_pos["psmobile"]["frame2"]["size"] = ImVec2(440.f, 68.f);
        items_pos["psmobile"]["frame3"]["pos"] = ImVec2(865.f, 113.f);
        items_pos["psmobile"]["frame3"]["size"] = ImVec2(440.f, 34.f);
        items_pos["psmobile"]["frame4"]["pos"] = ImVec2(865.f, 39.f);
        items_pos["psmobile"]["frame4"]["size"] = ImVec2(440.f, 34.f);
    }

    const VitaIoDevice app_device = host.io.title_id.find("NPXS") != std::string::npos ? VitaIoDevice::vs0 : VitaIoDevice::ux0;
    app_type = app_device == VitaIoDevice::ux0 ? gui.app_selector.apps : gui.app_selector.sys_apps;

    const auto app_index = std::find_if(app_type.begin(), app_type.end(), [&](const App &a) {
        return a.title_id == host.io.title_id;
    });
    current_app = int32_t(std::distance(app_type.begin(), app_index));

    if (gui.live_area_contents.find(host.io.title_id) == gui.live_area_contents.end()) {
        auto default_contents = false;
        const auto fw_path{ fs::path(host.pref_path) / "vs0" };
        const auto default_fw_contents{ fw_path / "data/internal/livearea/default/sce_sys/livearea/contents/template.xml" };
        auto template_xml{ fs::path(host.pref_path) / app_device._to_string() / "app" / host.io.title_id / "sce_sys/livearea/contents/template.xml" };

        pugi::xml_document doc;

        if (!doc.load_file(template_xml.c_str())) {
            if ((host.io.title_id.find("PCS") != std::string::npos) || (host.io.title_id.find("NPXS") != std::string::npos))
                LOG_WARN("Live Area Contents is corrupted or missing for title: {} '{}'.", host.io.title_id, host.app_title);
            if (doc.load_file(default_fw_contents.c_str())) {
                template_xml = default_fw_contents;
                default_contents = true;
                LOG_INFO("Using default firmware contents.");
            } else {
                type[host.io.title_id] = "a1";
                LOG_ERROR("Default firmware contents is corrupted or missing.");
                return;
            }
        }

        if (doc.load_file(template_xml.c_str())) {
            std::map<std::string, std::string> name;

            type[host.io.title_id].clear();
            type[host.io.title_id] = doc.child("livearea").attribute("style").as_string();

            if (!doc.child("livearea").child("livearea-background").child("image").child("lang").text().empty()) {
                for (const auto &livearea_background : doc.child("livearea").child("livearea-background")) {
                    if (livearea_background.child("lang").text().as_string() == user_lang) {
                        name["livearea-background"] = livearea_background.text().as_string();
                        break;
                    } else if (livearea_background.attribute("default").as_string() == std::string("on")) {
                        name["livearea-background"] = livearea_background.text().as_string();
                        break;
                    }
                }
            } else
                name["livearea-background"] = doc.child("livearea").child("livearea-background").child("image").text().as_string();

            if (name["livearea-background"].empty())
                name["livearea-background"] = doc.child("livearea").child("livearea-background").child("image").text().as_string();

            if (!default_contents) {
                for (const auto &gate : doc.child("livearea").child("gate")) {
                    if (gate.child("lang")) {
                        if (gate.child("lang").text().as_string() == user_lang) {
                            name["gate"] = gate.text().as_string();
                            break;
                        } else if (gate.attribute("default").as_string() == std::string("on"))
                            name["gate"] = gate.text().as_string();
                        else
                            for (const auto &lang : gate)
                                if (lang.text().as_string() == user_lang) {
                                    name["gate"] = gate.text().as_string();
                                    break;
                                }
                    } else if (gate.child("cntry")) {
                        if (gate.child("cntry").attribute("lang").as_string() == user_lang) {
                            name["gate"] = gate.text().as_string();
                            break;
                        } else
                            name["gate"] = gate.text().as_string();
                    } else
                        name["gate"] = gate.text().as_string();
                }
                if (name["gate"].find('\n') != std::string::npos)
                    name["gate"].erase(remove(name["gate"].begin(), name["gate"].end(), '\n'), name["gate"].end());
                name["gate"].erase(remove_if(name["gate"].begin(), name["gate"].end(), isspace), name["gate"].end());
            }

            if (name["livearea-background"].find('\n') != std::string::npos)
                name["livearea-background"].erase(remove(name["livearea-background"].begin(), name["livearea-background"].end(), '\n'), name["livearea-background"].end());
            name["livearea-background"].erase(remove_if(name["livearea-background"].begin(), name["livearea-background"].end(), isspace), name["livearea-background"].end());

            for (const auto &contents : name) {
                int32_t width = 0;
                int32_t height = 0;
                vfs::FileBuffer buffer;

                if (default_contents)
                    vfs::read_file(VitaIoDevice::vs0, buffer, host.pref_path, "data/internal/livearea/default/sce_sys/livearea/contents/" + contents.second);
                else if (app_device == VitaIoDevice::vs0)
                    vfs::read_file(VitaIoDevice::vs0, buffer, host.pref_path, "app/" + host.io.title_id + "/sce_sys/livearea/contents/" + contents.second);
                else
                    vfs::read_app_file(buffer, host.pref_path, host.io.title_id, "sce_sys/livearea/contents/" + contents.second);

                if (buffer.empty()) {
                    if ((host.io.title_id.find("PCS") != std::string::npos) || (host.io.title_id.find("NPXS") != std::string::npos))
                        LOG_WARN("Contents {} '{}' Not found for title {} [{}].", contents.first, contents.second, host.io.title_id, host.app_title);
                    continue;
                }
                stbi_uc *data = stbi_load_from_memory(&buffer[0], static_cast<int>(buffer.size()), &width, &height, nullptr, STBI_rgb_alpha);
                if (!data) {
                    LOG_ERROR("Invalid Live Area Contents for title {}.", host.io.title_id);
                    continue;
                }

                gui.live_area_contents[host.io.title_id][contents.first].init(gui.imgui_state.get(), data, width, height);
                stbi_image_free(data);
            }

            std::map<std::string, std::map<std::string, std::vector<std::string>>> items_name;

            frames[host.io.title_id].clear();
            liveitem[host.io.title_id].clear();
            str[host.io.title_id].clear();
            target[host.io.title_id].clear();

            for (const auto &livearea : doc.child("livearea")) {
                if (livearea.attribute("id")) {
                    frames[host.io.title_id].push_back({ livearea.attribute("id").as_string(), livearea.attribute("multi").as_string(), livearea.attribute("autoflip").as_uint() });

                    std::string frame = frames[host.io.title_id].back().id;

                    // Position background
                    if (livearea.child("liveitem").child("background")) {
                        if (livearea.child("liveitem").child("background").attribute("x"))
                            liveitem[host.io.title_id][frame]["background"]["x"].first = livearea.child("liveitem").child("background").attribute("x").as_int();
                        if (livearea.child("liveitem").child("background").attribute("y"))
                            liveitem[host.io.title_id][frame]["background"]["y"].first = livearea.child("liveitem").child("background").attribute("y").as_int();
                        if (livearea.child("liveitem").child("background").attribute("align"))
                            liveitem[host.io.title_id][frame]["background"]["align"].second = livearea.child("liveitem").child("background").attribute("align").as_string();
                        if (livearea.child("liveitem").child("background").attribute("valign"))
                            liveitem[host.io.title_id][frame]["background"]["valign"].second = livearea.child("liveitem").child("background").attribute("valign").as_string();
                    }

                    // Position image
                    if (livearea.child("liveitem").child("image")) {
                        if (livearea.child("liveitem").child("image").attribute("x"))
                            liveitem[host.io.title_id][frame]["image"]["x"].first = livearea.child("liveitem").child("image").attribute("x").as_int();
                        if (livearea.child("liveitem").child("image").attribute("y"))
                            liveitem[host.io.title_id][frame]["image"]["y"].first = livearea.child("liveitem").child("image").attribute("y").as_int();
                        if (livearea.child("liveitem").child("image").attribute("align"))
                            liveitem[host.io.title_id][frame]["image"]["align"].second = livearea.child("liveitem").child("image").attribute("align").as_string();
                        if (livearea.child("liveitem").child("image").attribute("valign"))
                            liveitem[host.io.title_id][frame]["image"]["valign"].second = livearea.child("liveitem").child("image").attribute("valign").as_string();
                        if (livearea.child("liveitem").child("image").attribute("origin"))
                            liveitem[host.io.title_id][frame]["image"]["origin"].second = livearea.child("liveitem").child("image").attribute("origin").as_string();
                    }

                    if (livearea.child("liveitem").child("text")) {
                        // SceInt32
                        if (livearea.child("liveitem").child("text").attribute("width"))
                            liveitem[host.io.title_id][frame]["text"]["width"].first = livearea.child("liveitem").child("text").attribute("width").as_int();
                        if (livearea.child("liveitem").child("text").attribute("height"))
                            liveitem[host.io.title_id][frame]["text"]["height"].first = livearea.child("liveitem").child("text").attribute("height").as_int();
                        if (livearea.child("liveitem").child("text").attribute("x"))
                            liveitem[host.io.title_id][frame]["text"]["x"].first = livearea.child("liveitem").child("text").attribute("x").as_int();
                        if (livearea.child("liveitem").child("text").attribute("y"))
                            liveitem[host.io.title_id][frame]["text"]["y"].first = livearea.child("liveitem").child("text").attribute("y").as_int();
                        if (livearea.child("liveitem").child("text").attribute("margin-top"))
                            liveitem[host.io.title_id][frame]["text"]["margin-top"].first = livearea.child("liveitem").child("text").attribute("margin-top").as_int();
                        if (livearea.child("liveitem").child("text").attribute("margin-buttom"))
                            liveitem[host.io.title_id][frame]["text"]["margin-buttom"].first = livearea.child("liveitem").child("text").attribute("margin-buttom").as_int();
                        if (livearea.child("liveitem").child("text").attribute("margin-left"))
                            liveitem[host.io.title_id][frame]["text"]["margin-left"].first = livearea.child("liveitem").child("text").attribute("margin-left").as_int();
                        if (livearea.child("liveitem").child("text").attribute("margin-right"))
                            liveitem[host.io.title_id][frame]["text"]["margin-right"].first = livearea.child("liveitem").child("text").attribute("margin-right").as_int();

                        // String
                        if (livearea.child("liveitem").child("text").attribute("align"))
                            liveitem[host.io.title_id][frame]["text"]["align"].second = livearea.child("liveitem").child("text").attribute("align").as_string();
                        if (livearea.child("liveitem").child("text").attribute("valign"))
                            liveitem[host.io.title_id][frame]["text"]["valign"].second = livearea.child("liveitem").child("text").attribute("valign").as_string();
                        if (livearea.child("liveitem").child("text").attribute("origin"))
                            liveitem[host.io.title_id][frame]["text"]["origin"].second = livearea.child("liveitem").child("text").attribute("origin").as_string();
                        if (livearea.child("liveitem").child("text").attribute("line-align"))
                            liveitem[host.io.title_id][frame]["text"]["line-align"].second = livearea.child("liveitem").child("text").attribute("line-align").as_string();
                        if (livearea.child("liveitem").child("text").attribute("text-align"))
                            liveitem[host.io.title_id][frame]["text"]["text-align"].second = livearea.child("liveitem").child("text").attribute("text-align").as_string();
                        if (livearea.child("liveitem").child("text").attribute("text-valign"))
                            liveitem[host.io.title_id][frame]["text"]["text-valign"].second = livearea.child("liveitem").child("text").attribute("text-valign").as_string();
                        if (livearea.child("liveitem").child("text").attribute("word-wrap"))
                            liveitem[host.io.title_id][frame]["text"]["word-wrap"].second = livearea.child("liveitem").child("text").attribute("word-wrap").as_string();
                        if (livearea.child("liveitem").child("text").attribute("word-scroll"))
                            liveitem[host.io.title_id][frame]["text"]["word-scroll"].second = livearea.child("liveitem").child("text").attribute("word-scroll").as_string();
                        if (livearea.child("liveitem").child("text").attribute("ellipsis"))
                            liveitem[host.io.title_id][frame]["text"]["ellipsis"].second = livearea.child("liveitem").child("text").attribute("ellipsis").as_string();
                    }

                    for (const auto &frame_id : livearea) {
                        if ((strncmp(frame_id.name(), "liveitem", 8) == 0) && (frame_id.child("cntry"))) {
                            for (const auto &cntry : frame_id) {
                                if ((strncmp(cntry.name(), "cntry", 5) == 0) && (cntry.attribute("lang").as_string() == user_lang)) {
                                    if (frame_id.child("background"))
                                        items_name[frame]["background"].push_back({ frame_id.child("background").text().as_string() });
                                    if (frame_id.child("image"))
                                        items_name[frame]["image"].push_back({ frame_id.child("image").text().as_string() });
                                    if (frame_id.child("target"))
                                        target[host.io.title_id][frame] = frame_id.child("target").text().as_string();
                                    if (frame_id.child("text").child("str")) {
                                        for (const auto &str_child : frame_id.child("text"))
                                            str[host.io.title_id][frame].push_back({ str_child.attribute("color").as_string(), str_child.attribute("size").as_float(), str_child.text().as_string() });
                                    }
                                    break;
                                }
                            }
                        } else if ((strncmp(frame_id.name(), "liveitem", 8) == 0) && frame_id.child("exclude-lang")) {
                            bool exclude_lang = false;
                            for (const auto &liveitem : frame_id)
                                if ((strncmp(liveitem.name(), "exclude-lang", 12) == 0) && (liveitem.text().as_string() == user_lang)) {
                                    exclude_lang = true;
                                    break;
                                }
                            if (!exclude_lang) {
                                if (frame_id.child("background"))
                                    items_name[frame]["background"].push_back({ frame_id.child("background").text().as_string() });
                                if (frame_id.child("image"))
                                    items_name[frame]["image"].push_back({ frame_id.child("image").text().as_string() });
                                if (frame_id.child("target"))
                                    target[host.io.title_id][frame] = frame_id.child("target").text().as_string();
                                if (frame_id.child("text").child("str")) {
                                    for (const auto &str_child : frame_id.child("text"))
                                        str[host.io.title_id][frame].push_back({ str_child.attribute("color").as_string(), str_child.attribute("size").as_float(), str_child.text().as_string() });
                                }
                                break;
                            }
                        } else if ((strncmp(frame_id.name(), "liveitem", 8) == 0) && (frame_id.child("lang").text().as_string() == user_lang)) {
                            if (frame_id.child("background"))
                                items_name[frame]["background"].push_back({ frame_id.child("background").text().as_string() });
                            if (frame_id.child("image"))
                                items_name[frame]["image"].push_back({ frame_id.child("image").text().as_string() });
                            if (frame_id.child("target"))
                                target[host.io.title_id][frame] = frame_id.child("target").text().as_string();
                            if (frame_id.child("text").child("str")) {
                                for (const auto &str_child : frame_id.child("text"))
                                    str[host.io.title_id][frame].push_back({ str_child.attribute("color").as_string(), str_child.attribute("size").as_float(), str_child.text().as_string() });
                            }
                            break;
                        } else if ((strncmp(frame_id.name(), "liveitem", 8) == 0) && !frame_id.child("exclude-lang") && !frame_id.child("lang") && !frame_id.child("cntry")) {
                            if (frame_id.child("background"))
                                items_name[frame]["background"].push_back({ frame_id.child("background").text().as_string() });
                            if (frame_id.child("image"))
                                items_name[frame]["image"].push_back({ frame_id.child("image").text().as_string() });
                            if (frame_id.child("target"))
                                target[host.io.title_id][frame] = frame_id.child("target").text().as_string();
                            if (frame_id.child("text").child("str")) {
                                for (const auto &str_child : frame_id.child("text"))
                                    str[host.io.title_id][frame].push_back({ str_child.attribute("color").as_string(), str_child.attribute("size").as_float(), str_child.text().as_string() });
                            }
                        }
                    }
                    if (target[host.io.title_id][frame].find('\n') != std::string::npos)
                        target[host.io.title_id][frame].erase(remove(target[host.io.title_id][frame].begin(), target[host.io.title_id][frame].end(), '\n'), target[host.io.title_id][frame].end());
                    target[host.io.title_id][frame].erase(remove_if(target[host.io.title_id][frame].begin(), target[host.io.title_id][frame].end(), isspace), target[host.io.title_id][frame].end());
                }
            }

            for (auto &item : items_name) {
                current_item[host.io.title_id][item.first] = 0;
                if (!item.second["background"].empty()) {
                    for (auto &bg_name : item.second["background"])
                        gui.live_items[host.io.title_id][item.first]["background"].push_back({});
                }
                if (!item.second["image"].empty()) {
                    for (auto &img_name : item.second["image"])
                        gui.live_items[host.io.title_id][item.first]["image"].push_back({});
                }
            }

            auto pos = 0;
            std::map<std::string, std::string> current_frame;
            for (auto &item : items_name) {
                if (!item.second.empty()) {
                    for (auto &bg_name : item.second["background"]) {
                        if (!bg_name.empty()) {
                            if (current_frame["background"] != item.first) {
                                current_frame["background"] = item.first;
                                pos = 0;
                            } else
                                ++pos;

                            if (bg_name.find('\n') != std::string::npos)
                                bg_name.erase(remove(bg_name.begin(), bg_name.end(), '\n'), bg_name.end());
                            bg_name.erase(remove_if(bg_name.begin(), bg_name.end(), isspace), bg_name.end());

                            int32_t width = 0;
                            int32_t height = 0;
                            vfs::FileBuffer buffer;

                            if (app_device == VitaIoDevice::vs0)
                                vfs::read_file(VitaIoDevice::vs0, buffer, host.pref_path, "app/" + host.io.title_id + "/sce_sys/livearea/contents/" + bg_name);
                            else
                                vfs::read_app_file(buffer, host.pref_path, host.io.title_id, "sce_sys/livearea/contents/" + bg_name);

                            if (buffer.empty()) {
                                if ((host.io.title_id.find("PCS") != std::string::npos) || (host.io.title_id.find("NPXS") != std::string::npos))
                                    LOG_WARN("background, Id: {}, Name: '{}', Not found for title: {} [{}].", item.first, bg_name, host.io.title_id, host.app_title);
                                continue;
                            }
                            stbi_uc *data = stbi_load_from_memory(&buffer[0], static_cast<int>(buffer.size()), &width, &height, nullptr, STBI_rgb_alpha);
                            if (!data) {
                                LOG_ERROR("Frame: {}, Invalid Live Area Contents for title: {} [{}].", item.first, host.io.title_id, host.app_title);
                                continue;
                            }

                            items[host.io.title_id][item.first]["background"]["size"] = ImVec2(float(width), float(height));
                            gui.live_items[host.io.title_id][item.first]["background"][pos].init(gui.imgui_state.get(), data, width, height);
                            stbi_image_free(data);
                        }
                    }

                    for (auto &img_name : item.second["image"]) {
                        if (!img_name.empty()) {
                            if (current_frame["image"] != item.first) {
                                current_frame["image"] = item.first;
                                pos = 0;
                            } else
                                ++pos;

                            if (img_name.find('\n') != std::string::npos)
                                img_name.erase(remove(img_name.begin(), img_name.end(), '\n'), img_name.end());
                            img_name.erase(remove_if(img_name.begin(), img_name.end(), isspace), img_name.end());

                            int32_t width = 0;
                            int32_t height = 0;
                            vfs::FileBuffer buffer;

                            if (app_device == VitaIoDevice::vs0)
                                vfs::read_file(VitaIoDevice::vs0, buffer, host.pref_path, "app/" + host.io.title_id + "/sce_sys/livearea/contents/" + img_name);
                            else
                                vfs::read_app_file(buffer, host.pref_path, host.io.title_id, "sce_sys/livearea/contents/" + img_name);

                            if (buffer.empty()) {
                                if ((host.io.title_id.find("PCS") != std::string::npos) || (host.io.title_id.find("NPXS") != std::string::npos))
                                    LOG_WARN("Image, Id: {} Name: '{}', Not found for title {} [{}].", item.first, img_name, host.io.title_id, host.app_title);
                                continue;
                            }
                            stbi_uc *data = stbi_load_from_memory(&buffer[0], static_cast<int>(buffer.size()), &width, &height, nullptr, STBI_rgb_alpha);
                            if (!data) {
                                LOG_ERROR("Frame: {}, Invalid Live Area Contents for title: {} [{}].", item.first, host.io.title_id, host.app_title);
                                continue;
                            }

                            items[host.io.title_id][item.first]["image"]["size"] = ImVec2(float(width), float(height));
                            gui.live_items[host.io.title_id][item.first]["image"][pos].init(gui.imgui_state.get(), data, width, height);
                            stbi_image_free(data);
                        }
                    }
                }
            }
        }
    }
    if (type[host.io.title_id].empty())
        type[host.io.title_id] = "a1";
}

inline uint64_t current_time() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch())
        .count();
}

void draw_live_area_screen(GuiState &gui, HostState &host) {
    if (!gui.live_area.manual)
        draw_information_bar(gui);

    const ImVec2 display_size = ImGui::GetIO().DisplaySize;
    const auto scal = ImVec2(display_size.x / 960.0f, display_size.y / 544.0f);
    const VitaIoDevice app_device = host.io.title_id.find("NPXS") != std::string::npos ? VitaIoDevice::vs0 : VitaIoDevice::ux0;

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(display_size, ImGuiCond_Always);
    ImGui::Begin("##live_area", &gui.live_area.live_area_screen, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_AlwaysAutoResize);

    const auto icon_scal_pos = ImVec2(display_size.x - (496.0f * scal.x), display_size.y - (544.f * scal.y));
    const auto icon_scal_size = ImVec2(icon_scal_pos.x + (32.0f * scal.x), icon_scal_pos.y + (32.f * scal.y));

    if (!gui.live_area.manual && (gui.app_selector.icons.find(host.io.title_id) != gui.app_selector.icons.end())) {
        ImGui::GetForegroundDrawList()->AddImage(gui.app_selector.icons[host.io.title_id], icon_scal_pos, icon_scal_size);
    }

    const auto background_pos = ImVec2(900.0f * scal.x, 500.0f * scal.y);
    const auto pos_bg = ImVec2(display_size.x - background_pos.x, display_size.y - background_pos.y);
    const auto background_size = ImVec2(840.0f * scal.x, 500.0f * scal.y);

    if (gui.live_area_contents[host.io.title_id].find("livearea-background") != gui.live_area_contents[host.io.title_id].end())
        ImGui::GetWindowDrawList()->AddImage(gui.live_area_contents[host.io.title_id]["livearea-background"],
            pos_bg, ImVec2(pos_bg.x + background_size.x, pos_bg.y + background_size.y));

    for (const auto &frame : frames[host.io.title_id]) {
        if (frame.autoflip != 0) {
            if (last_time[host.io.title_id][frame.id] == 0)
                last_time[host.io.title_id][frame.id] = current_time();

            while (last_time[host.io.title_id][frame.id] + frame.autoflip < current_time()) {
                last_time[host.io.title_id][frame.id] += frame.autoflip;

                if (gui.live_items[host.io.title_id][frame.id].find("background") != gui.live_items[host.io.title_id][frame.id].end()) {
                    if (current_item[host.io.title_id][frame.id] != int(gui.live_items[host.io.title_id][frame.id]["background"].size()) - 1)
                        ++current_item[host.io.title_id][frame.id];
                    else
                        current_item[host.io.title_id][frame.id] = 0;
                } else if (gui.live_items[host.io.title_id][frame.id].find("image") != gui.live_items[host.io.title_id][frame.id].end()) {
                    if (current_item[host.io.title_id][frame.id] != int(gui.live_items[host.io.title_id][frame.id]["image"].size()) - 1)
                        ++current_item[host.io.title_id][frame.id];
                    else
                        current_item[host.io.title_id][frame.id] = 0;
                }
            }
        }

        if (type[host.io.title_id] == "psmobile") {
            LOG_WARN_IF(items_pos[type[host.io.title_id]][frame.id].empty(), "Info not found for {}, with {}, on title: {}",
                type[host.io.title_id], frame.id, host.io.title_id);
        }

        const auto FRAME_SIZE = items_pos[type[host.io.title_id]][frame.id]["size"];

        auto FRAME_POS = ImVec2(items_pos[type[host.io.title_id]][frame.id]["pos"].x * scal.x,
            items_pos[type[host.io.title_id]][frame.id]["pos"].y * scal.y);

        auto bg_size = items[host.io.title_id][frame.id]["background"]["size"];

        // Resize items
        const auto bg_resize = ImVec2(bg_size.x / FRAME_SIZE.x, bg_size.y / FRAME_SIZE.y);
        if (bg_size.x > FRAME_SIZE.x)
            bg_size.x /= bg_resize.x;
        if (bg_size.y > FRAME_SIZE.y)
            bg_size.y /= bg_resize.y;

        auto img_size = items[host.io.title_id][frame.id]["image"]["size"];
        const auto img_resize = ImVec2(img_size.x / FRAME_SIZE.x, img_size.y / FRAME_SIZE.y);
        if (img_size.x > FRAME_SIZE.x)
            img_size.x /= img_resize.x;
        if (img_size.y > FRAME_SIZE.y)
            img_size.y /= img_resize.y;

        // Items pos init (Center)
        auto bg_pos_init = ImVec2((FRAME_SIZE.x - bg_size.x) / 2.f, (FRAME_SIZE.y - bg_size.y) / 2.f);
        auto img_pos_init = ImVec2((FRAME_SIZE.x - img_size.x) / 2.f, (FRAME_SIZE.y - img_size.y) / 2.f);

        // Allign items
        if ((liveitem[host.io.title_id][frame.id]["background"]["align"].second == "left") && (liveitem[host.io.title_id][frame.id]["background"]["x"].first >= 0))
            bg_pos_init.x = 0.0f;
        else if ((liveitem[host.io.title_id][frame.id]["background"]["align"].second == "right") && (liveitem[host.io.title_id][frame.id]["background"]["x"].first <= 0))
            bg_pos_init.x = FRAME_SIZE.x - bg_size.x;
        else
            bg_pos_init.x += liveitem[host.io.title_id][frame.id]["background"]["x"].first;

        if ((liveitem[host.io.title_id][frame.id]["image"]["align"].second == "left") && (liveitem[host.io.title_id][frame.id]["image"]["x"].first >= 0))
            img_pos_init.x = 0.0f;
        else if ((liveitem[host.io.title_id][frame.id]["image"]["align"].second == "right") && (liveitem[host.io.title_id][frame.id]["image"]["x"].first <= 0))
            img_pos_init.x = FRAME_SIZE.x - img_size.x;
        else
            img_pos_init.x += liveitem[host.io.title_id][frame.id]["image"]["x"].first;

        // Valign items
        if ((liveitem[host.io.title_id][frame.id]["background"]["valign"].second == "top") && (liveitem[host.io.title_id][frame.id]["background"]["y"].first <= 0))
            bg_pos_init.y = 0.0f;
        else if ((liveitem[host.io.title_id][frame.id]["background"]["valign"].second == "bottom") && (liveitem[host.io.title_id][frame.id]["background"]["y"].first >= 0))
            bg_pos_init.y = FRAME_SIZE.y - bg_size.y;
        else
            bg_pos_init.y -= liveitem[host.io.title_id][frame.id]["background"]["y"].first;

        if ((liveitem[host.io.title_id][frame.id]["image"]["valign"].second == "top") && (liveitem[host.io.title_id][frame.id]["image"]["y"].first <= 0))
            img_pos_init.y = 0.0f;
        else if ((liveitem[host.io.title_id][frame.id]["image"]["valign"].second == "bottom") && (liveitem[host.io.title_id][frame.id]["image"]["y"].first >= 0))
            img_pos_init.y = FRAME_SIZE.y - img_size.y;
        else
            img_pos_init.y -= liveitem[host.io.title_id][frame.id]["image"]["y"].first;

        // Set items pos
        auto bg_pos = ImVec2((display_size.x - FRAME_POS.x) + (bg_pos_init.x * scal.x), (display_size.y - FRAME_POS.y) + (bg_pos_init.y * scal.y));

        auto img_pos = ImVec2((display_size.x - FRAME_POS.x) + (img_pos_init.x * scal.x), (display_size.y - FRAME_POS.y) + (img_pos_init.y * scal.y));

        if (bg_size.x == FRAME_SIZE.x)
            bg_pos.x = display_size.x - FRAME_POS.x;
        if (bg_size.y == FRAME_SIZE.y)
            bg_pos.y = display_size.y - FRAME_POS.y;

        if (img_size.x == FRAME_SIZE.x)
            img_pos.x = display_size.x - FRAME_POS.x;
        if (img_size.y == FRAME_SIZE.y)
            img_pos.y = display_size.y - FRAME_POS.y;

        // Scal size items
        const auto bg_scal_size = ImVec2(bg_size.x * scal.x, bg_size.y * scal.y);
        const auto img_scal_size = ImVec2(img_size.x * scal.x, img_size.y * scal.y);

        const auto pos_frame = ImVec2(display_size.x - FRAME_POS.x, display_size.y - FRAME_POS.y);

        // Scal size frame
        const auto scal_size_frame = ImVec2(FRAME_SIZE.x * scal.x, FRAME_SIZE.y * scal.y);

        // Reset position if get outside frame
        if ((bg_pos.x + bg_scal_size.x) > (pos_frame.x + scal_size_frame.x))
            bg_pos.x += (pos_frame.x + scal_size_frame.x) - (bg_pos.x + bg_scal_size.x);
        if (bg_pos.x < pos_frame.x)
            bg_pos.x += pos_frame.x - bg_pos.x;

        if ((bg_pos.y + bg_scal_size.y) > (pos_frame.y + scal_size_frame.y))
            bg_pos.y += (pos_frame.y + scal_size_frame.y) - (bg_pos.y + bg_scal_size.y);
        if (bg_pos.y < pos_frame.y)
            bg_pos.y += pos_frame.y - bg_pos.y;

        if ((img_pos.x + img_scal_size.x) > (pos_frame.x + scal_size_frame.x))
            img_size.x += (pos_frame.x + scal_size_frame.x) - (img_pos.x + img_scal_size.x);
        if (img_pos.x < pos_frame.x)
            img_pos.x += pos_frame.x - img_pos.x;

        if ((img_pos.y + img_scal_size.y) > (pos_frame.y + scal_size_frame.y))
            img_pos.y += (pos_frame.y + scal_size_frame.y) - (img_pos.y + img_scal_size.y);
        if (img_pos.y < pos_frame.y)
            img_pos.y += pos_frame.y - img_pos.y;

        // Display items
        if (gui.live_items[host.io.title_id][frame.id].find("background") != gui.live_items[host.io.title_id][frame.id].end()) {
            ImGui::SetCursorPos(bg_pos);
            ImGui::Image(gui.live_items[host.io.title_id][frame.id]["background"][current_item[host.io.title_id][frame.id]], bg_scal_size);
        }
        if (gui.live_items[host.io.title_id][frame.id].find("image") != gui.live_items[host.io.title_id][frame.id].end()) {
            ImGui::SetCursorPos(img_pos);
            ImGui::Image(gui.live_items[host.io.title_id][frame.id]["image"][current_item[host.io.title_id][frame.id]], img_scal_size);
        }

        // Target link
        if (!target[host.io.title_id][frame.id].empty() && (target[host.io.title_id][frame.id].find("psts:") == std::string::npos)) {
            ImGui::SetCursorPos(pos_frame);
            ImGui::PushID(frame.id.c_str());
            if (ImGui::Selectable("##target_link", ImGuiSelectableFlags_None, false, scal_size_frame))
                system((OS_PREFIX + target[host.io.title_id][frame.id]).c_str());
            ImGui::PopID();
        }

        // Text
        for (const auto &str_tag : str[host.io.title_id][frame.id]) {
            if (!str_tag.text.empty()) {
                std::vector<ImVec4> str_color;

                if (!str_tag.color.empty()) {
                    int color;

                    if (frame.autoflip != 0)
                        sscanf(str[host.io.title_id][frame.id][current_item[host.io.title_id][frame.id]].color.c_str(), "#%x", &color);
                    else
                        sscanf(str_tag.color.c_str(), "#%x", &color);

                    str_color.push_back(ImVec4(float((color >> 16) & 0xFF), float((color >> 8) & 0xFF), float((color >> 0) & 0xFF), 255.f));
                } else
                    str_color.push_back(ImVec4(255.f, 255.f, 255.f, 255.f));

                auto str_size = scal_size_frame, text_pos = pos_frame;

                // Origin
                if (liveitem[host.io.title_id][frame.id]["text"]["origin"].second.empty() || (liveitem[host.io.title_id][frame.id]["text"]["origin"].second == "background")) {
                    if (gui.live_items[host.io.title_id][frame.id].find("background") != gui.live_items[host.io.title_id][frame.id].end())
                        str_size = bg_scal_size, text_pos = bg_pos;
                    else if (!liveitem[host.io.title_id][frame.id]["text"]["origin"].second.empty() && (gui.live_items[host.io.title_id][frame.id].find("image") != gui.live_items[host.io.title_id][frame.id].end()))
                        str_size = img_scal_size, text_pos = img_pos;
                } else if (liveitem[host.io.title_id][frame.id]["text"]["origin"].second == "image") {
                    if (gui.live_items[host.io.title_id][frame.id].find("image") != gui.live_items[host.io.title_id][frame.id].end())
                        str_size = img_scal_size, text_pos = img_pos;
                    else if (gui.live_items[host.io.title_id][frame.id].find("background") != gui.live_items[host.io.title_id][frame.id].end())
                        str_size = bg_scal_size, text_pos = bg_pos;
                }

                auto size_text = ImGui::GetFontSize();
                if (str_tag.size != 0)
                    size_text = str_tag.size; // TODO multiple size on same frame

                auto str_wrap = scal_size_frame.x;
                if (liveitem[host.io.title_id][frame.id]["text"]["allign"].second == "outside-right")
                    str_wrap = str_size.x;

                if (liveitem[host.io.title_id][frame.id]["text"]["width"].first > 0) {
                    if (liveitem[host.io.title_id][frame.id]["text"]["word-wrap"].second != "off")
                        str_wrap = float(liveitem[host.io.title_id][frame.id]["text"]["width"].first) * scal.x;
                    text_pos.x += (str_size.x - (float(liveitem[host.io.title_id][frame.id]["text"]["width"].first) * scal.x)) / 2.f;
                    str_size.x = float(liveitem[host.io.title_id][frame.id]["text"]["width"].first) * scal.x;
                }

                if ((liveitem[host.io.title_id][frame.id]["text"]["height"].first > 0)
                    && ((liveitem[host.io.title_id][frame.id]["text"]["word-scroll"].second == "on" || liveitem[host.io.title_id][frame.id]["text"]["height"].first <= FRAME_SIZE.y))) {
                    text_pos.y += (str_size.y - (float(liveitem[host.io.title_id][frame.id]["text"]["height"].first) * scal.y)) / 2.f;
                    str_size.y = float(liveitem[host.io.title_id][frame.id]["text"]["height"].first) * scal.y;
                }

                const auto scal_font_size = size_text / ImGui::GetFontSize();
                ImGui::SetWindowFontScale(scal_font_size * scal.x);

                // Calcule text pixel size
                ImVec2 calc_text_size;
                if (frame.autoflip > 0) {
                    if (liveitem[host.io.title_id][frame.id]["text"]["word-wrap"].second != "off")
                        calc_text_size = ImGui::CalcTextSize(str[host.io.title_id][frame.id][current_item[host.io.title_id][frame.id]].text.c_str(), 0, false, str_wrap);
                    else
                        calc_text_size = ImGui::CalcTextSize(str[host.io.title_id][frame.id][current_item[host.io.title_id][frame.id]].text.c_str());
                } else {
                    if (liveitem[host.io.title_id][frame.id]["text"]["word-wrap"].second != "off")
                        calc_text_size = ImGui::CalcTextSize(str_tag.text.c_str(), 0, false, str_wrap);
                    else
                        calc_text_size = ImGui::CalcTextSize(str_tag.text.c_str());
                }

                /*if (liveitem[host.io.title_id][frame.id]["text"]["ellipsis"].second == "on") {
                    // TODO ellipsis
                }*/

                ImVec2 str_pos_init;

                // Allign
                if (liveitem[host.io.title_id][frame.id]["text"]["align"].second.empty()) {
                    if (liveitem[host.io.title_id][frame.id]["text"]["text-align"].second.empty()) {
                        if (liveitem[host.io.title_id][frame.id]["text"]["line-align"].second == "left")
                            str_pos_init.x = 0.f;
                        else if (liveitem[host.io.title_id][frame.id]["text"]["line-align"].second == "right")
                            str_pos_init.x = str_size.x - calc_text_size.x;
                        else if (liveitem[host.io.title_id][frame.id]["text"]["line-align"].second == "outside-right") {
                            text_pos.x += str_size.x;
                            if (liveitem[host.io.title_id][frame.id]["text"]["origin"].second == "image") {
                                if (gui.live_items[host.io.title_id][frame.id].find("background") != gui.live_items[host.io.title_id][frame.id].end())
                                    str_size.x = bg_scal_size.x - img_scal_size.x - (img_pos.x - bg_pos.x);
                                else
                                    str_size = scal_size_frame, text_pos = pos_frame;
                            }
                        } else
                            str_pos_init.x = (str_size.x - calc_text_size.x) / 2.0f;
                    } else {
                        if ((liveitem[host.io.title_id][frame.id]["text"]["text-align"].second == "center")
                            || ((liveitem[host.io.title_id][frame.id]["text"]["text-align"].second == "left") && (liveitem[host.io.title_id][frame.id]["text"]["x"].first < 0)))
                            str_pos_init.x = (str_size.x - calc_text_size.x) / 2.0f;
                        else if (liveitem[host.io.title_id][frame.id]["text"]["text-align"].second == "left")
                            str_pos_init.x = 0.f;
                        else if (liveitem[host.io.title_id][frame.id]["text"]["text-align"].second == "right")
                            str_pos_init.x = str_size.x - calc_text_size.x;
                        else if (liveitem[host.io.title_id][frame.id]["text"]["text-align"].second == "outside-right") {
                            text_pos.x += str_size.x;
                            if (liveitem[host.io.title_id][frame.id]["text"]["origin"].second == "image") {
                                if (gui.live_items[host.io.title_id][frame.id].find("background") != gui.live_items[host.io.title_id][frame.id].end())
                                    str_size.x = bg_scal_size.x - img_scal_size.x - (img_pos.x - bg_pos.x);
                                else
                                    str_size = scal_size_frame, text_pos = pos_frame;
                            }
                        }
                    }
                } else {
                    if (liveitem[host.io.title_id][frame.id]["text"]["align"].second == "center")
                        str_pos_init.x = (str_size.x - calc_text_size.x) / 2.0f;
                    else if (liveitem[host.io.title_id][frame.id]["text"]["align"].second == "left")
                        str_pos_init.x = 0.f;
                    else if (liveitem[host.io.title_id][frame.id]["text"]["align"].second == "right")
                        str_pos_init.x = str_size.x - calc_text_size.x;
                    else if (liveitem[host.io.title_id][frame.id]["text"]["align"].second == "outside-right") {
                        text_pos.x += str_size.x;
                        if (liveitem[host.io.title_id][frame.id]["text"]["origin"].second == "image") {
                            if (gui.live_items[host.io.title_id][frame.id].find("background") != gui.live_items[host.io.title_id][frame.id].end())
                                str_size.x = bg_scal_size.x - img_scal_size.x - (img_pos.x - bg_pos.x);
                            else
                                str_size = scal_size_frame, text_pos = pos_frame;
                        }
                    }
                }

                // Valign
                if (liveitem[host.io.title_id][frame.id]["text"]["valign"].second.empty()) {
                    if (liveitem[host.io.title_id][frame.id]["text"]["text-valign"].second.empty() || (liveitem[host.io.title_id][frame.id]["text"]["text-valign"].second == "center"))
                        str_pos_init.y = (str_size.y - calc_text_size.y) / 2.f;
                    else if (liveitem[host.io.title_id][frame.id]["text"]["text-valign"].second == "bottom")
                        str_pos_init.y = str_size.y - calc_text_size.y;
                    else if (liveitem[host.io.title_id][frame.id]["text"]["text-valign"].second == "top")
                        str_pos_init.y = 0.f;
                } else {
                    if ((liveitem[host.io.title_id][frame.id]["text"]["valign"].second == "center")
                        || ((liveitem[host.io.title_id][frame.id]["text"]["valign"].second == "bottom") && (liveitem[host.io.title_id][frame.id]["text"]["y"].first != 0))
                        || ((liveitem[host.io.title_id][frame.id]["text"]["valign"].second == "top") && (liveitem[host.io.title_id][frame.id]["text"]["y"].first != 0)))
                        str_pos_init.y = (str_size.y - calc_text_size.y) / 2.f;
                    else if (liveitem[host.io.title_id][frame.id]["text"]["valign"].second == "bottom")
                        str_pos_init.y = str_size.y - calc_text_size.y;
                    else if (liveitem[host.io.title_id][frame.id]["text"]["valign"].second == "top") {
                        str_pos_init.y = 0.f;
                    } else if (liveitem[host.io.title_id][frame.id]["text"]["valign"].second == "outside-top") {
                        str_pos_init.y = 0.f;
                    }
                }

                auto pos_str = ImVec2(str_pos_init.x, str_pos_init.y - (liveitem[host.io.title_id][frame.id]["text"]["y"].first * scal.y));

                if (liveitem[host.io.title_id][frame.id]["text"]["x"].first > 0) {
                    text_pos.x += liveitem[host.io.title_id][frame.id]["text"]["x"].first * scal.x;
                    str_size.x -= liveitem[host.io.title_id][frame.id]["text"]["x"].first * scal.x;
                }

                if ((liveitem[host.io.title_id][frame.id]["text"]["margin-left"].first > 0) && !liveitem[host.io.title_id][frame.id]["text"]["width"].first) {
                    text_pos.x += liveitem[host.io.title_id][frame.id]["text"]["margin-left"].first * scal.x;
                    str_size.x -= liveitem[host.io.title_id][frame.id]["text"]["margin-left"].first * scal.x;
                }

                if (liveitem[host.io.title_id][frame.id]["text"]["margin-right"].first > 0)
                    str_size.x -= liveitem[host.io.title_id][frame.id]["text"]["margin-right"].first * scal.x;

                if (liveitem[host.io.title_id][frame.id]["text"]["margin-top"].first > 0) {
                    text_pos.y += liveitem[host.io.title_id][frame.id]["text"]["margin-top"].first * scal.y;
                    str_size.y -= liveitem[host.io.title_id][frame.id]["text"]["margin-top"].first * scal.y;
                }

                // Text Display
                // TODO Multible color support on same frame, used by eg: Asphalt: Injection
                // TODO Correct display few line on same frame, used by eg: Asphalt: Injection
                ImGui::SetNextWindowPos(text_pos);
                ImGui::BeginChild(frame.id.c_str(), str_size, false, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoSavedSettings);
                if (liveitem[host.io.title_id][frame.id]["text"]["word-wrap"].second != "off")
                    ImGui::PushTextWrapPos(str_wrap);
                if (liveitem[host.io.title_id][frame.id]["text"]["word-scroll"].second == "on") {
                    static std::map<std::string, std::map<std::string, std::pair<bool, float>>> scroll;
                    if (liveitem[host.io.title_id][frame.id]["text"]["word-wrap"].second == "off") {
                        if (scroll[host.io.title_id][frame.id].first) {
                            if (scroll[host.io.title_id][frame.id].second >= 0.f)
                                scroll[host.io.title_id][frame.id].second -= 0.5f;
                            else
                                scroll[host.io.title_id][frame.id].first = false;
                        } else {
                            if (scroll[host.io.title_id][frame.id].second <= ImGui::GetScrollMaxX())
                                scroll[host.io.title_id][frame.id].second += 0.5f;
                            else
                                scroll[host.io.title_id][frame.id].first = true;
                        }
                        ImGui::SetScrollX(scroll[host.io.title_id][frame.id].second);
                    } else {
                        if (scroll[host.io.title_id][frame.id].first) {
                            if (scroll[host.io.title_id][frame.id].second >= 0.f)
                                scroll[host.io.title_id][frame.id].second -= 0.5f;
                            else
                                scroll[host.io.title_id][frame.id].first = false;
                        } else {
                            if (scroll[host.io.title_id][frame.id].second <= ImGui::GetScrollMaxY())
                                scroll[host.io.title_id][frame.id].second += 0.5f;
                            else
                                scroll[host.io.title_id][frame.id].first = true;
                        }
                        ImGui::SetScrollY(scroll[host.io.title_id][frame.id].second);
                    }
                }
                ImGui::SetCursorPos(pos_str);
                if (frame.autoflip > 0)
                    ImGui::TextColored(str_color[0], "%s", str[host.io.title_id][frame.id][current_item[host.io.title_id][frame.id]].text.c_str());
                else
                    ImGui::TextColored(str_color[0], "%s", str_tag.text.c_str());
                if (liveitem[host.io.title_id][frame.id]["text"]["word-wrap"].second != "off")
                    ImGui::PopTextWrapPos();
                ImGui::EndChild();
                ImGui::SetWindowFontScale(1.f);
            }
        }
    }

    const auto GATE_SIZE = ImVec2(280.0f * scal.x, 158.0f * scal.y);
    const auto GATE_POS = ImVec2(display_size.x - (items_pos[type[host.io.title_id]]["gate"]["pos"].x * scal.x), display_size.y - (items_pos[type[host.io.title_id]]["gate"]["pos"].y * scal.y));
    const auto scal_font_size = 25.0f / ImGui::GetFontSize();
    const auto START_SIZE = ImVec2((ImGui::CalcTextSize(start.c_str()).x * scal_font_size), (ImGui::CalcTextSize(start.c_str()).y * scal_font_size));
    const auto START_BUTTON_SIZE = ImVec2((START_SIZE.x + 26.0f) * scal.x, (START_SIZE.y + 5.0f) * scal.y);
    const auto POS_BUTTON = ImVec2((GATE_POS.x + (GATE_SIZE.x - START_BUTTON_SIZE.x) / 2.0f), (GATE_POS.y + (GATE_SIZE.y - START_BUTTON_SIZE.y) / 1.08f));
    const auto POS_START = ImVec2(POS_BUTTON.x + (START_BUTTON_SIZE.x - (START_SIZE.x * scal.x)) / 2,
        POS_BUTTON.y + (START_BUTTON_SIZE.y - (START_SIZE.y * scal.y)) / 2);
    const auto SELECT_SIZE = ImVec2(GATE_SIZE.x - (10.f * scal.x), GATE_SIZE.y - (5.f * scal.y));
    const auto SELECT_POS = ImVec2(GATE_POS.x + (5.f * scal.y), GATE_POS.y + (2.f * scal.y));
    const auto SIZE_GATE = ImVec2(GATE_POS.x + GATE_SIZE.x, GATE_POS.y + GATE_SIZE.y);

    const auto BUTTON_SIZE = ImVec2(76.f * scal.x, 30.f * scal.y);

    if (gui.live_area_contents[host.io.title_id].find("gate") != gui.live_area_contents[host.io.title_id].end()) {
        ImGui::SetCursorPos(GATE_POS);
        ImGui::Image(gui.live_area_contents[host.io.title_id]["gate"], GATE_SIZE);
    }
    ImGui::PushID(host.io.title_id.c_str());
    ImGui::GetWindowDrawList()->AddRectFilled(POS_BUTTON, ImVec2(POS_BUTTON.x + START_BUTTON_SIZE.x, POS_BUTTON.y + START_BUTTON_SIZE.y), IM_COL32(20, 168, 222, 255), 10.0f * scal.x, ImDrawCornerFlags_All);
    ImGui::GetWindowDrawList()->AddText(gui.live_area_font, 25.0f * scal.x, POS_START, IM_COL32(255, 255, 255, 255), start.c_str());
    ImGui::SetCursorPos(SELECT_POS);
    if (ImGui::Selectable("##gate", ImGuiSelectableFlags_None, false, SELECT_SIZE) || ImGui::IsKeyPressed(host.cfg.keyboard_button_cross)) {
        run_app(gui, host);
        gui.live_area.live_area_screen = false;
    }
    ImGui::PopID();
    ImGui::GetWindowDrawList()->AddRect(GATE_POS, SIZE_GATE, IM_COL32(192, 192, 192, 255), 15.f, ImDrawCornerFlags_All, 12.f);

    if (app_device == VitaIoDevice::ux0) {
        const auto widget_scal_size = ImVec2(80.0f * scal.x, 80.f * scal.y);
        const auto manual_path{ fs::path(host.pref_path) / "ux0/app" / host.io.title_id / "sce_sys/manual/" };
        const auto scal_widget_font_size = 23.0f / ImGui::GetFontSize();

        auto search_pos = ImVec2(578.0f * scal.x, 505.0f * scal.y);
        if (!fs::exists(manual_path) || fs::is_empty(manual_path))
            search_pos = ImVec2(520.0f * scal.x, 505.0f * scal.y);

        const auto pos_scal_search = ImVec2(display_size.x - search_pos.x, display_size.y - search_pos.y);

        const std::string SEARCH = "Search";
        const auto SEARCH_SCAL_SIZE = ImVec2((ImGui::CalcTextSize(SEARCH.c_str()).x * scal_widget_font_size) * scal.x, (ImGui::CalcTextSize(SEARCH.c_str()).y * scal_widget_font_size) * scal.y);
        const auto POS_STR_SEARCH = ImVec2(pos_scal_search.x + ((widget_scal_size.x / 2.f) - (SEARCH_SCAL_SIZE.x / 2.f)),
            pos_scal_search.y + ((widget_scal_size.x / 2.f) - (SEARCH_SCAL_SIZE.y / 2.f)));
        ImGui::GetWindowDrawList()->AddRectFilled(pos_scal_search, ImVec2(pos_scal_search.x + widget_scal_size.x, pos_scal_search.y + widget_scal_size.y), IM_COL32(20, 168, 222, 255), 12.0f * scal.x, ImDrawCornerFlags_All);
        ImGui::GetWindowDrawList()->AddText(gui.live_area_font, 23.0f * scal.x, POS_STR_SEARCH, IM_COL32(255, 255, 255, 255), SEARCH.c_str());
        ImGui::SetCursorPos(pos_scal_search);
        if (ImGui::Selectable("##Search", ImGuiSelectableFlags_None, false, widget_scal_size)) {
            auto search_url = "http://www.google.com/search?q=" + host.app_title;
            std::replace(search_url.begin(), search_url.end(), ' ', '+');
            system((OS_PREFIX + search_url).c_str());
        }

        if (fs::exists(manual_path) && !fs::is_empty(manual_path)) {
            const auto manaul_pos = ImVec2(463.0f * scal.x, 505.0f * scal.y);
            const auto pos_scal_manual = ImVec2(display_size.x - manaul_pos.x, display_size.y - manaul_pos.y);

            const std::string MANUAL_STR = "Manual";
            const auto MANUAL_STR_SCAL_SIZE = ImVec2((ImGui::CalcTextSize(MANUAL_STR.c_str()).x * scal_widget_font_size) * scal.x, (ImGui::CalcTextSize(MANUAL_STR.c_str()).y * scal_widget_font_size) * scal.y);
            const auto MANUAl_STR_POS = ImVec2(pos_scal_manual.x + ((widget_scal_size.x / 2.f) - (MANUAL_STR_SCAL_SIZE.x / 2.f)),
                pos_scal_manual.y + ((widget_scal_size.x / 2.f) - (MANUAL_STR_SCAL_SIZE.y / 2.f)));
            ImGui::GetWindowDrawList()->AddRectFilled(pos_scal_manual, ImVec2(pos_scal_manual.x + widget_scal_size.x, pos_scal_manual.y + widget_scal_size.y), IM_COL32(202, 0, 106, 255), 12.0f * scal.x, ImDrawCornerFlags_All);
            ImGui::GetWindowDrawList()->AddText(gui.live_area_font, 23.0f * scal.x, MANUAl_STR_POS, IM_COL32(255, 255, 255, 255), MANUAL_STR.c_str());
            ImGui::SetCursorPos(pos_scal_manual);
            if (ImGui::Selectable("##manual", ImGuiSelectableFlags_None, false, widget_scal_size)) {
                if (init_manual(gui, host))
                    gui.live_area.manual = true;
                else
                    LOG_ERROR("Error opening Manual");
            }
        }
    }

    if (!gui.live_area.manual) {
        const auto wheel_counter = ImGui::GetIO().MouseWheel;

        if (gui.app_selector.selected_title_id.empty()) {
            if (ImGui::IsKeyPressed(host.cfg.keyboard_button_up) || ImGui::IsKeyPressed(host.cfg.keyboard_leftstick_up) || (wheel_counter == 1)) {
                if (current_app > 0)
                    --current_app;
                else
                    current_app = int(app_type.size()) - 1;
            } else if (ImGui::IsKeyPressed(host.cfg.keyboard_button_down) || ImGui::IsKeyPressed(host.cfg.keyboard_leftstick_down) || (wheel_counter == -1)) {
                if (current_app < int(app_type.size()) - 1)
                    ++current_app;
                else
                    current_app = 0;
            }

            if (host.io.title_id != app_type[current_app].title_id) {
                host.io.title_id = app_type[current_app].title_id;
                host.app_title = app_type[current_app].title;
                init_live_area(gui, host);
            }

            ImGui::SetCursorPos(ImVec2(display_size.x - (60.0f * scal.x), 44.0f * scal.y));
            ImGui::VSliderInt("##slider_current_app", ImVec2(60.f, 500.f * scal.y), &current_app, int32_t(app_type.size()) - 1, 0, fmt::format("{}\n_____\n\n{}", current_app + 1, int32_t(app_type.size())).c_str());
        }

        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.f);
        ImGui::SetCursorPos(ImVec2(display_size.x - (136.0f * scal.x), 44.0f * scal.y));
        if (ImGui::Button("Esc", BUTTON_SIZE) || ImGui::IsKeyPressed(host.cfg.keyboard_button_circle))
            gui.live_area.live_area_screen = false;

        ImGui::SetCursorPos(ImVec2(60.f * scal.x, 44.0f * scal.y));
        if (ImGui::Button("Help", BUTTON_SIZE))
            ImGui::OpenPopup("Live Area Help");
        ImGui::SetNextWindowPos(ImVec2(display_size.x / 2.f, display_size.y / 2.f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
        if (ImGui::BeginPopupModal("Live Area Help", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoSavedSettings)) {
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2.f - (ImGui::CalcTextSize("Help").x / 2.f));
            ImGui::TextColored(GUI_COLOR_TEXT_TITLE, "Help");
            ImGui::Spacing();
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2.f - (ImGui::CalcTextSize("Using configuration set for keyboard in control setting").x / 2.f));
            ImGui::TextColored(GUI_COLOR_TEXT, "Using configuration set for keyboard in control setting");
            if (gui.modules.empty()) {
                ImGui::Spacing();
                ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2.f - (ImGui::CalcTextSize("Firmware not detected, Install it is recomanded for font text in Livea Area").x / 2.f));
                ImGui::TextColored(GUI_COLOR_TEXT, "Firmware not detected, Install it is recomanded for font text in Livea Area");
            }
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextColored(GUI_COLOR_TEXT_TITLE, "Live Area Help");
            ImGui::Spacing();
            ImGui::TextColored(GUI_COLOR_TEXT, "%-16s    %-16s", "Browse in app list", "D-pad, Left Stick, Wheel in Up/Down or using Slider");
            ImGui::TextColored(GUI_COLOR_TEXT, "%-16s    %-16s", "Start App", "Click on Start or Press on Cross");
            ImGui::TextColored(GUI_COLOR_TEXT, "%-16s    %-16s", "Show/Hide Live Area durring app run", "Press on PS");
            ImGui::TextColored(GUI_COLOR_TEXT, "%-16s    %-16s", "Exit Live Area", "Click on Esc or Press on Circle");
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextColored(GUI_COLOR_TEXT_TITLE, "Manual Help");
            ImGui::Spacing();
            ImGui::TextColored(GUI_COLOR_TEXT, "%-16s    %-16s", "Browse page", "D-pad, Left Stick in Left/Right or Wheel in Up/Down or Click on </>");
            ImGui::TextColored(GUI_COLOR_TEXT, "%-16s    %-16s", "Hide/Show button", "Right Click");
            ImGui::TextColored(GUI_COLOR_TEXT, "%-16s    %-16s", "Zoom if available", "Double left click");
            ImGui::TextColored(GUI_COLOR_TEXT, "%-16s    %-16s", "Scroll in zoom", "Wheel Up/Down");
            ImGui::TextColored(GUI_COLOR_TEXT, "%-16s    %-16s", "Exit Manual", "Click on X or Press on PS");
            ImGui::Spacing();
            ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2.f - (BUTTON_SIZE.x / 2.f));
            if (ImGui::Button("Ok", BUTTON_SIZE))
                ImGui::CloseCurrentPopup();

            ImGui::EndPopup();
        }
        ImGui::PopStyleVar();
    }

    ImGui::End();
}

} // namespace gui
