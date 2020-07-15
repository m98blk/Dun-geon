#include "Screen.h"
#include "CameraStyle.h"
#include "ColorPairs.h"
#include "Entities/EntityManager.h"
#include "Entities/Player.h"
#include "InputHandler.h"
#include "Misc/Coords.h"
#include "Misc/Exceptions.h"
#include "Misc/RNG.h"
#include "Misc/Utils.h"
#include "WorldMapObjectType.h"
#include "Worlds/Field.h"
#include "Worlds/Generation/RoomLayout.h"
#include "Worlds/Room.h"
#include "Worlds/World.h"
#include "Worlds/WorldManager.h"
#include <cmath>
#include <iostream>
#include <map>
#include <menu.h>
#include <ncurses.h>
#include <sstream>
#include <string>

#define PLAYER_HEALTH_PC std::lround(m_Player.GetStats().health / 1.0 / m_Player.GetStats().healthMax * 100)
#define PLAYER_MANA_PC std::lround(m_Player.GetStats().mana / 1.0 / m_Player.GetStats().manaMax * 100)

namespace UI
{

Screen::Screen(InputHandler& inputHandler,
               const Worlds::WorldManager& worldManager,
               const Entities::EntityManager& entityManager,
               const Entities::Player& player)
    : m_InputHandler(inputHandler),
      m_WorldManager(worldManager),
      m_EntityManager(entityManager),
      m_Player(player),
      m_View(View::MainMenu),
      m_GameWorldWindow(nullptr),
      m_GameHUDWindow(nullptr),
      m_GameMessageWindow(nullptr),
      m_CurrentRoom(nullptr),
      m_IsWorldMapCursorEnabled(true)
{
    Init();
}

Screen::~Screen()
{
    Terminate();
}

void Screen::MainMenu()
{
    m_View = View::MainMenu;
    static const std::vector<std::string> splashMsg = {
        "Speakest not of it.",
        "And you thought you'd seen it all...",
        "Enough dun for everyone.",
        "Not nearly as much mashing required this time.",
        "Have you brought your sporks?",
        "The gate opens, and dun awaits.",
        "Not to be confused with a Dun-gen.",
        "...Maybe this time they'll explain what dun is?",
        "Your dun is no longer your own.",
        "I dun my robe and wizard hat.",
        "The dun shines brighter than ever before.",
        "1500 dun well spent.",
        "Hopefully it will have been worth the wait."
    };
    DrawLogo();
    int splashNumber = RNG::RandomInt(splashMsg.size());
    PrintCenterAt(splashMsg[splashNumber], 13);

    refresh();

    std::map<int, std::string> options = {
        { 0, "Start Game" },
        { 2, "Quit" }
    };
    static const int menuWidth = 20;
    static const int menuHeight = 9;
    int choice = SelectViaMenu(options, { (ScreenWidth - menuWidth) / 2 - 1, 15 }, menuWidth, menuHeight, false, 2, 1, "", true);
    switch (choice)
    {
    case 0:
        erase();
        refresh();
        StartGame();
        break;
    case 2:
        m_InputHandler.SetQuit();
        break;
    }
}

void Screen::Draw()
{
    DrawWorld();
    DrawHUD();
    DrawMessageWindow();
}

Screen::View Screen::GetView() const
{
    return m_View;
}

void Screen::PostMessage(const std::string& message)
{
    m_Message = message;
}

void Screen::ShowMap()
{
    View previousView = m_View;
    m_View = View::Map;

    // Prepare window
    // Every room icon is 1 char wide and has 1 char of spacing on both sides horizontally
    // for drawing "hallways" between rooms. This also helps keep better proportions.
    WINDOW* mapWindow = newwin(WorldMapHeight, WorldMapWidth, WorldMapYPos, WorldMapXPos);

    // Handle map interaction
    Coords cursor = m_CurrentRoom->GetCoords();
    int key = 0;
    bool done = false;
    bool actionTaken = true;
    keypad(mapWindow, 1);

    do
    {
        // Need to redraw the screen to get rid of leftover tooltips
        if (actionTaken)
        {
            DrawWorld();
            DrawHUD();
            DrawMessageWindow(false); // don't post the message

            DrawMap(mapWindow, cursor);
            if (m_IsWorldMapCursorEnabled)
            {
                WorldMapObjectType selectedObjectType = GetWorldMapObjectType(cursor);
                DrawMapTooltip(cursor, selectedObjectType);
            }

            actionTaken = false;
        }
        key = wgetch(mapWindow);

        switch (key)
        {
        case 'w':
        case KEY_UP:
            if (m_IsWorldMapCursorEnabled &&
                !m_WorldManager.GetCurrentWorld().IsPositionAtWorldGridEdge(cursor, Direction::Up()))
            {
                cursor.MoveInDirection(Direction::Up());
                actionTaken = true;
            }
            break;
        case 'd':
        case KEY_RIGHT:
            if (m_IsWorldMapCursorEnabled &&
                !m_WorldManager.GetCurrentWorld().IsPositionAtWorldGridEdge(cursor, Direction::Right()))
            {
                cursor.MoveInDirection(Direction::Right());
                actionTaken = true;
            }
            break;
        case 's':
        case KEY_DOWN:
            if (m_IsWorldMapCursorEnabled &&
                !m_WorldManager.GetCurrentWorld().IsPositionAtWorldGridEdge(cursor, Direction::Down()))
            {
                cursor.MoveInDirection(Direction::Down());
                actionTaken = true;
            }
            break;
        case 'a':
        case KEY_LEFT:
            if (m_IsWorldMapCursorEnabled &&
                !m_WorldManager.GetCurrentWorld().IsPositionAtWorldGridEdge(cursor, Direction::Left()))
            {
                cursor.MoveInDirection(Direction::Left());
                actionTaken = true;
            }
            break;
        case ' ':
            m_IsWorldMapCursorEnabled = !m_IsWorldMapCursorEnabled;
            actionTaken = true;
            break;
        case KEY_ENTER:
        case 10:
        case 27:
        case 'q':
            done = true;
            break;
        }
    } while (!done);

    // Clean up the window
    werase(mapWindow);
    wrefresh(mapWindow);

    delwin(mapWindow);
    m_View = previousView;
}

bool Screen::YesNoMessageBox(const std::string& prompt, const std::string& leftOption, const std::string& rightOption, const std::string& title)
{
    // Split the prompt into lines
    std::vector<std::string> lines;
    size_t neededWidth = 0;
    std::string lineBuf;
    std::istringstream iss(prompt);
    while (std::getline(iss, lineBuf))
    {
        if (lineBuf.size() > neededWidth) neededWidth = lineBuf.size();
        lines.push_back(std::move(lineBuf));
    }

    std::string left = "  " + leftOption + "  ";
    std::string right = "  " + rightOption + "  ";
    // The ncurses menu forces all option boxes to be the same size.
    // Thus we should give the shorter option some padding
    std::string& longer = left.size() > right.size() ? left : right;
    std::string& shorter = left.size() > right.size() ? right : left;
    for (size_t i = 0; i < (longer.size() - shorter.size()) / 2; i++)
    {
        shorter.insert(shorter.begin(), ' ');
    }

    int height = lines.size() + 4;
    int width = neededWidth + 4;

    std::vector<ITEM*> items = { new_item(left.c_str(), left.c_str()),
                                 new_item(right.c_str(), right.c_str()),
                                 nullptr };

    int subXPos = (width - (left.size() + right.size() + 4)) / 2;

    WINDOW* boxWin = newwin(height, width, (ScreenHeight - height) / 2, (ScreenWidth - width) / 2);
    WINDOW* boxSub = derwin(boxWin, 1, width - subXPos - 3, height - 2, subXPos);
    MENU* menu = new_menu(items.data());

    set_menu_win(menu, boxWin);
    set_menu_sub(menu, boxSub);
    keypad(boxWin, 1);
    menu_opts_off(menu, O_SHOWDESC);
    set_menu_mark(menu, "");
    set_menu_format(menu, 1, 2);
    set_menu_spacing(menu, 1, 1, 4);
    box(boxWin, 0, 0);
    if (!title.empty()) PrintCenterAt(boxWin, title, 0);
    for (size_t i = 0; i < lines.size(); i++)
    {
        PrintCenterAt(boxWin, lines[i], i + 1);
    }

    post_menu(menu);
    wrefresh(boxWin);
    bool selectedLeft = true;

    bool pressedEnter = false;
    int key;
    while (!pressedEnter)
    {
        key = wgetch(boxWin);
        switch (key)
        {
        // In a 2-button message box, the behavior depends on the selected option and not the key pressed
        case KEY_RIGHT:
        case 'd':
        case KEY_LEFT:
        case 'a':
            if (selectedLeft)
                menu_driver(menu, REQ_RIGHT_ITEM);
            else
                menu_driver(menu, REQ_LEFT_ITEM);
            selectedLeft = !selectedLeft;
            break;
        // ESC exits with negative response
        case 27:
            selectedLeft = false;
            // drop through
        case KEY_ENTER:
        case 10:
            pressedEnter = true;
            break;
        }
        wrefresh(boxWin);
    }

    unpost_menu(menu);
    free_menu(menu);
    werase(boxWin);
    wrefresh(boxWin);
    delwin(boxSub);
    delwin(boxWin);
    for (auto& item : items)
    {
        free_item(item);
    }

    return selectedLeft;
}

void Screen::Init()
{
    initscr();
    start_color();
    use_default_colors();
    raw();
    keypad(stdscr, true);
    noecho();
    curs_set(0);
    ESCDELAY = 0;

    ColorPairs::InitPairs();
}

void Screen::Terminate()
{
    if (m_GameWorldWindow != nullptr) delwin(m_GameWorldWindow);
    if (m_GameHUDWindow != nullptr) delwin(m_GameHUDWindow);
    if (m_GameMessageWindow != nullptr) delwin(m_GameMessageWindow);
    endwin();
}

void Screen::PrintCenterAt(const std::string& str, int yPos)
{
    int xPos = (ScreenWidth - str.size()) / 2;
    xPos += xPos % 2;
    mvaddstr(yPos, xPos, str.c_str());
    refresh();
}

void Screen::PrintCenterAt(WINDOW* window, const std::string& str, int yPos)
{
    int xPos = (getmaxx(window) - str.size()) / 2;
    mvwaddstr(window, yPos, xPos, str.c_str());
    wrefresh(window);
}

void Screen::DrawLogo(int xPos, int yPos)
{
    attron(A_BOLD);
    mvaddstr(++yPos, xPos, "_____");
    mvaddstr(++yPos, xPos, "|    \\   _   _   _  __         ____   ____   ____   _  __");
    mvaddstr(++yPos, xPos, "| /\\  \\ / \\ / \\ | |/  \\  ___  /    \\ / __ \\ /    \\ | |/  \\");
    mvaddstr(++yPos, xPos, "| \\/  / | |_| | |  _  | |___| | () | | ___/ | () | |  _  |");
    mvaddstr(++yPos, xPos, "\\____/  \\_____| \\_/ \\_|       \\__  | \\____/ \\____/ \\_/ \\_|");
    mvaddstr(++yPos, xPos + 31, "__| |");
    mvaddstr(++yPos, xPos + 30, "|____/  ");
    // Print the flashy version string
    addch('v');
    printw("%d", GameVersionMajor);
    addch('.');
    printw("%d", GameVersionMinor);
    addch('.');
    attron(COLOR_PAIR(ColorPairs::YellowOnDefault));
    printw("%d", GameVersionRevision);
    attroff(A_COLOR);
    attroff(A_BOLD);
}

int Screen::SelectViaMenu(std::map<int, std::string> options, Coords position, int width, int height, bool drawBorder, int padX, int padY, const std::string& title, bool spaceOptions)
{
    if (options.empty())
    {
        std::ostringstream errorMessage;
        errorMessage << "Attempted display of empty "
                     << width << 'x' << height
                     << " menu at "
                     << position;
        if (!title.empty()) errorMessage << " (\"" << title << "\")";
        throw DisplayException(errorMessage.str());
    }

    const size_t subWidth = width - 2 - 2 * padX;

    std::vector<ITEM*> items;
    for (auto& pair : options)
    {
        if (pair.second.size() + 4 > subWidth)
        {
            ShortenString(pair.second, subWidth - 4);
        }
        pair.second = std::string("  ") + pair.second + "  ";
        items.push_back(new_item(pair.second.c_str(), pair.second.c_str()));
    }
    items.push_back(nullptr);
    MENU* menu = new_menu(items.data());
    WINDOW* menuWindow = newwin(height,
                                width,
                                position.GetY(),
                                position.GetX());
    WINDOW* menuSub = derwin(menuWindow,
                             height - 2 - 2 * padY,
                             subWidth,
                             1 + padY,
                             1 + padX);
    set_menu_win(menu, menuWindow);
    set_menu_sub(menu, menuSub);
    keypad(menuWindow, 1);

    menu_opts_off(menu, O_SHOWDESC);
    set_menu_mark(menu, "");
    if (spaceOptions) set_menu_spacing(menu, 1, 2, 1);
    if (drawBorder) box(menuWindow, 0, 0);
    if (!title.empty()) PrintCenterAt(menuWindow, title, 0);

    post_menu(menu);
    wrefresh(menuWindow);

    auto it = options.begin();
    bool selected = false;
    int key;
    while (!selected)
    {
        key = wgetch(menuWindow);
        switch (key)
        {
        case KEY_DOWN:
        case 's':
            menu_driver(menu, REQ_DOWN_ITEM);
            if (++it == options.end())
            {
                menu_driver(menu, REQ_FIRST_ITEM);
                it = options.begin();
            }
            break;
        case KEY_UP:
        case 'w':
            menu_driver(menu, REQ_UP_ITEM);
            if (it == options.begin())
            {
                menu_driver(menu, REQ_LAST_ITEM);
                it = options.end();
            }
            it--;
            break;
        case KEY_ENTER:
        case 10:
            selected = true;
            break;
        }
        wrefresh(menuWindow);
    }

    unpost_menu(menu);
    werase(menuWindow);
    wrefresh(menuWindow);
    delwin(menuSub);
    delwin(menuWindow);
    free_menu(menu);
    for (auto& item : items)
        free_item(item);

    return it->first;
}

void Screen::StartGame()
{
    m_View = View::InGame;
    m_GameWorldWindow = newwin(1, 1, 0, 0);
    ResizeAndRepositionWorldWindow();
    m_GameHUDWindow = newwin(ScreenHeight, HUDPanelWidth, 0, WorldPanelWidth);
    m_GameMessageWindow = newwin(ScreenHeight - WorldPanelHeight, WorldPanelWidth + 1, WorldPanelHeight, 0);
    m_Message = "Welcome to the Dun-geon.";
}

void Screen::ResizeAndRepositionWorldWindow()
{
    werase(m_GameWorldWindow);
    wrefresh(m_GameWorldWindow);
    const Worlds::Room& currentRoom = m_WorldManager.GetCurrentRoom();

    Coords::Scalar windowLines = currentRoom.GetHeight() + 2;
    Coords::Scalar windowColumns = currentRoom.GetWidth() + 2;

    if (currentRoom.GetCameraStyle() != CameraStyle::Fixed &&
        currentRoom.GetVisionRadius() > 0)
    {
        windowLines = currentRoom.GetVisionRadius() * 2 + 3;
        windowColumns = windowLines;
    }

    int windowXPos = (WorldPanelWidth - windowColumns) / 2 - 1;
    int windowYPos = (WorldPanelHeight - windowLines) / 2;
    wresize(m_GameWorldWindow, windowLines, windowColumns);
    mvwin(m_GameWorldWindow, windowYPos, windowXPos);
}

void Screen::DrawWorld()
{
    werase(m_GameWorldWindow);
    if (m_CurrentRoom != &m_WorldManager.GetCurrentRoom())
    {
        m_CurrentRoom = &m_WorldManager.GetCurrentRoom();
        ResizeAndRepositionWorldWindow();
    }
    int worldY, worldX;
    getmaxyx(m_GameWorldWindow, worldY, worldX);
    // How far can we draw vertically or horizontally
    Coords::Scalar rangeX = worldX / 2 - (worldX % 2 ? 0 : 1) - 1;
    Coords::Scalar rangeY = worldY / 2 - (worldY % 2 ? 0 : 1) - 1;
    auto playerCoords = m_Player.GetCoords();
    for (int i = 1; i < worldX - 1; i++)
    {
        for (int j = 1; j < worldY - 1; j++)
        {
            int desiredFieldXPos = 0;
            int desiredFieldYPos = 0;
            switch (m_CurrentRoom->GetCameraStyle())
            {
            case CameraStyle::Fixed:
                desiredFieldXPos = i - 1;
                desiredFieldYPos = j - 1;
                break;
            case CameraStyle::PlayerCentered:
                // These coords are relative to the player's position
                desiredFieldXPos = playerCoords.GetX() + (i - 1) - rangeX;
                desiredFieldYPos = playerCoords.GetY() + (j - 1) - rangeY;
                break;
            }
            if (desiredFieldXPos < 0 ||
                desiredFieldXPos >= m_CurrentRoom->GetWidth() ||
                desiredFieldYPos < 0 ||
                desiredFieldYPos >= m_CurrentRoom->GetHeight())
            {
                mvwaddch(m_GameWorldWindow, j, i, DefaultFieldIcon);
            }
            else
            {
                auto radius = m_CurrentRoom->GetVisionRadius();
                Coords targetCoords(desiredFieldXPos, desiredFieldYPos);
                if (radius > 0 &&
                    playerCoords.CombinedDistanceFrom(targetCoords) >
                        (playerCoords.SharesAxisWith(targetCoords)
                             ? radius - 1
                             : radius))
                {
                    mvwaddch(m_GameWorldWindow, j, i, DefaultFieldIcon);
                }
                else
                {
                    mvwaddch(m_GameWorldWindow, j, i, GetFieldIcon(targetCoords));
                }
            }
        }
    }
    if (m_CurrentRoom->GetCameraStyle() != CameraStyle::Fixed)
    {
        box(m_GameWorldWindow, 0, 0);
    }
    wrefresh(m_GameWorldWindow);
}

void Screen::DrawHUD()
{
    werase(m_GameHUDWindow);
    const auto& stats = m_Player.GetStats();
    mvwprintw(m_GameHUDWindow, 2, 4, "World %d", m_WorldManager.GetCurrentWorld().GetWorldNumber());
    mvwprintw(m_GameHUDWindow, 2, HUDPanelWidth - 10, "Room %d", m_WorldManager.GetCurrentRoom().GetRoomNumber());

    PrintCenterAt(m_GameHUDWindow, m_Player.GetName(), 4);

    mvwprintw(m_GameHUDWindow, 6, 4, "Level %d", stats.level);
    mvwprintw(m_GameHUDWindow, 6, HUDPanelWidth - 11, "XP: %3d%%", stats.XP / stats.XPToNextLevel * 100);

    mvwprintw(m_GameHUDWindow, 8, 4, "HP:  %d/%d", stats.health, stats.healthMax);
    mvwprintw(m_GameHUDWindow, 8, HUDPanelWidth - 11, "(%3d%%)", PLAYER_HEALTH_PC);

    mvwprintw(m_GameHUDWindow, 9, 4, "MP:  %d/%d", stats.mana, stats.manaMax);
    mvwprintw(m_GameHUDWindow, 9, HUDPanelWidth - 11, "(%3d%%)", PLAYER_MANA_PC);

    mvwprintw(m_GameHUDWindow, 11, 4, "Str: %3d", stats.vigor);
    mvwprintw(m_GameHUDWindow, 11, HUDPanelWidth - 11, "Def: %3d", stats.valor);

    mvwprintw(m_GameHUDWindow, 12, 4, "Agi: %3d", stats.haste);
    mvwprintw(m_GameHUDWindow, 12, HUDPanelWidth - 11, "Int: %3d", stats.magic);

    std::string wealthAmountStr = std::to_string(stats.dun);
    int xPos = (HUDPanelWidth - wealthAmountStr.size() - 12) / 2;
    xPos += xPos % 2;
    mvwaddstr(m_GameHUDWindow, 14, xPos, "Wealth: ");
    wattron(m_GameHUDWindow, COLOR_PAIR(ColorPairs::YellowOnDefault) | A_BOLD);
    wprintw(m_GameHUDWindow, "%d", stats.dun);
    wattroff(m_GameHUDWindow, A_COLOR | A_BOLD);
    waddstr(m_GameHUDWindow, " dun");

    mvwaddstr(m_GameHUDWindow, 16, 5, "[i]tems");
    mvwaddstr(m_GameHUDWindow, 16, HUDPanelWidth - 12, "[s]kills");

    mvwaddstr(m_GameHUDWindow, 17, 5, "[m]ap");
    mvwaddstr(m_GameHUDWindow, 17, HUDPanelWidth - 12, "[h]elp");

    PrintCenterAt(m_GameHUDWindow, "[q]uit", 18);

    if (m_EntityManager.GetApproachedEntity(m_Player) != nullptr)
    {
        PrintCenterAt(m_GameHUDWindow, m_EntityManager.GetApproachedEntity(m_Player)->GetName(), WorldPanelHeight + 1);
        PrintCenterAt(m_GameHUDWindow, m_EntityManager.GetApproachedEntity(m_Player)->GetDescription(), WorldPanelHeight + 2);
    }

    box(m_GameHUDWindow, 0, 0);
    mvwhline(m_GameHUDWindow, WorldPanelHeight, 1, 0, HUDPanelWidth - 2);
    mvwaddch(m_GameHUDWindow, WorldPanelHeight, HUDPanelWidth - 1, ACS_RTEE);
    wrefresh(m_GameHUDWindow);
}

void Screen::DrawMessageWindow(bool shouldPostMessage)
{
    werase(m_GameMessageWindow);
    wborder(m_GameMessageWindow, 0, 0, 0, 0, 0, ACS_PLUS, 0, ACS_BTEE);
    if (shouldPostMessage)
    {
        if (m_Message.size() > WorldPanelWidth - 4)
        {
            size_t pos = WorldPanelWidth - 5;
            while (m_Message[pos] != ' ' && pos > 0)
                pos--;
            std::string secondLine = m_Message.substr(pos > 0 ? pos + 1 : WorldPanelWidth - 4);
            m_Message = m_Message.substr(0, pos > 0 ? pos : WorldPanelWidth - 4);
            mvwaddstr(m_GameMessageWindow, 1, 2, m_Message.c_str());
            mvwaddstr(m_GameMessageWindow, 2, 2, secondLine.c_str());
        }
        else
        {
            mvwaddstr(m_GameMessageWindow, 1, 2, m_Message.c_str());
        }
        m_Message.clear();
    }
    wrefresh(m_GameMessageWindow);
}

void Screen::DrawMap(WINDOW* mapWindow, Coords cursor)
{
    werase(mapWindow);
    wattron(mapWindow, COLOR_PAIR(ColorPairs::BlackOnYellow));
    box(mapWindow, 0, 0);
    PrintCenterAt(mapWindow, " [SPACE] to toggle cursor ", WorldMapHeight - 1);
    wattroff(mapWindow, A_COLOR);
    wattron(mapWindow, COLOR_PAIR(ColorPairs::WhiteOnYellow) | A_BOLD);
    PrintCenterAt(mapWindow, " World Map ", 0);
    wattroff(mapWindow, A_COLOR | A_BOLD);

    const auto& world = m_WorldManager.GetCurrentWorld();
    for (Coords::Scalar i = 0; i < Worlds::World::MaximumSpan; i++)
    {
        for (Coords::Scalar j = 0; j < Worlds::World::MaximumSpan; j++)
        {
            Coords current(i, j);
            WorldMapObjectType type = GetWorldMapObjectType(current);

            // Select the icon based on the object type
            chtype icon;
            switch (type)
            {
            default:
            case WorldMapObjectType::Empty:
                icon = ' ';
                break;
            case WorldMapObjectType::Room: {
                const auto& room = world.GetRoomAt(current);
                icon = GetRoomMapIcon(room);
                if (room.TryGetEntrance(Direction::Left()) != nullptr)
                    mvwaddch(mapWindow, j + 1, i * 2, ACS_HLINE);
                if (room.TryGetEntrance(Direction::Right()) != nullptr)
                    mvwaddch(mapWindow, j + 1, i * 2 + 2, ACS_HLINE);
                break;
            }
            case WorldMapObjectType::UndiscoveredRoom:
                icon = '?';
                break;
            }

            // Apply highlighting
            bool isCurrentRoom = m_WorldManager.GetCurrentRoom().GetCoords() == current;
            if (m_IsWorldMapCursorEnabled && cursor == current)
            {
                icon |= isCurrentRoom
                            ? (COLOR_PAIR(ColorPairs::BlackOnRed))
                            : (COLOR_PAIR(ColorPairs::BlackOnYellow));
            }
            else if (isCurrentRoom)
            {
                icon |= (COLOR_PAIR(ColorPairs::RedOnDefault) | A_BOLD);
            }
            mvwaddch(mapWindow, j + 1, i * 2 + 1, icon);
        }
    }
    wrefresh(mapWindow);
}

void Screen::DrawMapTooltip(Coords cursor, WorldMapObjectType objectType)
{
    if (objectType == WorldMapObjectType::Empty) return;

    // Prepare window contents
    std::vector<std::string> lines;
    switch (objectType)
    {
    case WorldMapObjectType::Room: {
        const auto& room = m_WorldManager.GetCurrentWorld().GetRoomAt(cursor);
        bool isCurrentRoom = m_WorldManager.IsCurrentRoom(room);
        std::string locPronoun = isCurrentRoom ? "here" : "there";
        lines.push_back("Room " + std::to_string(room.GetRoomNumber()));
        if (m_WorldManager.IsCurrentRoom(room))
            lines.push_back("* You are here *");
        if (room.GetVisionRadius() > 0)
            lines.push_back("It's dark in " + locPronoun + ".");
        break;
    }
    case WorldMapObjectType::UndiscoveredRoom:
        lines.push_back("Undiscovered room");
        break;
    default:
        break;
    }

    // Scale the window
    int tooltipWidth = 0;
    for (const auto& line : lines)
        if (line.size() > static_cast<size_t>(tooltipWidth))
            tooltipWidth = line.size();
    tooltipWidth += 4;
    int tooltipHeight = lines.size() + 2;

    // Tooltip sticks to the cursor
    // Calculate the real coord on the screen first
    int cursorActualX = cursor.GetX() * 2 + 1 + WorldMapXPos;
    int cursorActualY = cursor.GetY() + 1 + WorldMapYPos;
    WINDOW* tooltipWindow = newwin(tooltipHeight,
                                   tooltipWidth,
                                   // Flip the tooltip to the other side of the cursor
                                   // if it cannot fit on either axis
                                   cursorActualY > tooltipHeight
                                       ? cursorActualY - tooltipHeight
                                       : cursorActualY + 1,
                                   cursorActualX < ScreenWidth - tooltipWidth - 1
                                       ? cursorActualX + 1
                                       : cursorActualX - tooltipWidth);
    // Draw the window
    wattron(tooltipWindow, COLOR_PAIR(ColorPairs::YellowOnDefault) | A_BOLD);
    box(tooltipWindow, 0, 0);
    wattroff(tooltipWindow, A_BOLD);
    for (size_t i = 0; i < lines.size(); i++)
    {
        PrintCenterAt(tooltipWindow, lines[i], i + 1);
    }
    wattroff(tooltipWindow, A_COLOR);
    wrefresh(tooltipWindow);

    // Cleanup
    delwin(tooltipWindow);
}

chtype Screen::GetFieldIcon(const Worlds::Field& field) const
{
    chtype icon = 0;
    bool canHaveHighlight = true;
    if (field.TryGetForegroundEntity() != nullptr)
    {
        icon = field.TryGetForegroundEntity()->GetIcon();
    }
    else if (field.TryGetBackgroundEntity() != nullptr)
    {
        icon = field.TryGetBackgroundEntity()->GetIcon();
    }
    else if (field.IsAccessible() && m_CurrentRoom->GetVisionRadius() > 0)
    {
        icon = '.' | COLOR_PAIR(ColorPairs::WhiteOnDefault);
        canHaveHighlight = false;
    }
    else
    {
        icon = DefaultFieldIcon;
        canHaveHighlight = false;
    }

    // Apply highlight if the player is touching this field
    auto lmd = m_Player.GetLastMoveDirection();
    if (canHaveHighlight &&
        lmd != Direction::None() &&
        !m_CurrentRoom->IsPositionAtRoomEdge(m_Player.GetCoords(), lmd) &&
        field.GetCoords() == m_Player.GetCoords().GetAdjacent(lmd))
    {
        short highlightPair;
        // Highlight the background only if it was a non-default color
        short bgColorPair = (icon & A_COLOR) >> 8;
        short fg, bg;
        pair_content(bgColorPair, &fg, &bg);
        highlightPair = (bg > 0) ? ColorPairs::RedOnRed : ColorPairs::RedOnDefault;

        icon &= ~A_COLOR;
        icon |= COLOR_PAIR(highlightPair) | A_BOLD;
    }

    return icon;
}

chtype Screen::GetFieldIcon(Coords coords) const
{
    return GetFieldIcon(m_WorldManager.GetCurrentRoom().GetFieldAt(coords));
}

chtype Screen::GetRoomMapIcon(const Worlds::Room& room) const
{
    bool up = room.TryGetEntrance(Direction::Up()) != nullptr;
    bool right = room.TryGetEntrance(Direction::Right()) != nullptr;
    bool down = room.TryGetEntrance(Direction::Down()) != nullptr;
    bool left = room.TryGetEntrance(Direction::Left()) != nullptr;

    constexpr static const chtype deadEnd = '#';

    if (up)
    {
        if (right)
        {
            if (down && left) return ACS_PLUS;
            if (down && !left) return ACS_LTEE;
            if (!down && left) return ACS_BTEE;
            if (!down && !left) return ACS_LLCORNER;
        }
        else
        {
            if (down && left) return ACS_RTEE;
            if (down && !left) return ACS_VLINE;
            if (!down && left) return ACS_LRCORNER;
            if (!down && !left) return deadEnd;
        }
    }
    else
    {
        if (right)
        {
            if (down && left) return ACS_TTEE;
            if (down && !left) return ACS_ULCORNER;
            if (!down && left) return ACS_HLINE;
            if (!down && !left) return deadEnd;
        }
        else
        {
            if (down && left) return ACS_URCORNER;
            if (down && !left) return deadEnd;
            if (!down && left) return deadEnd;
        }
    }

    return deadEnd;
}

WorldMapObjectType Screen::GetWorldMapObjectType(Coords coords) const
{
    WorldMapObjectType type = WorldMapObjectType::Empty;
    const auto& world = m_WorldManager.GetCurrentWorld();
    if (!world.RoomExistsAt(coords))
    {
        // If the room is undiscovered, we cannot access it directly, but we can check
        // if its neighbors have any entrances leading here.
        for (const auto& dir : Direction::All())
        {
            if (world.RoomExistsAt(coords.GetAdjacent(dir)) &&
                world.GetRoomAt(coords.GetAdjacent(dir)).TryGetEntrance(dir.Opposite()) != nullptr)
            {
                type = WorldMapObjectType::UndiscoveredRoom;
                break;
            }
        }
    }
    else
    {
        type = WorldMapObjectType::Room;
    }

    return type;
}

} /* namespace UI */
