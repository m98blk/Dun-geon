#pragma once

#include "Battle/ApplyEffectOnlySkill.h"
#include "Battle/AttackSkill.h"
#include "Battle/Battle.h"
#include "Battle/Skill.h"
#include "ColorPairs.h"
#include "Components/LogWindow.h"
#include "Components/Nameplate.h"
#include "Misc/Utils.h"
#include "Screen.h"
#include "Subscreen.h"

namespace Battle
{
class ApplyEffectOnlySkillResult;
}

namespace UI
{

class BattleScreen : public Subscreen
{
public:
    /**
     * @brief Width of the world panel
     */
    constexpr static const int ArenaPanelWidth = 50;

    /**
     * @brief Height of the world panel
     */
    constexpr static const int TopPanelHeight = 17;

    /**
     * @brief Width of the HUD panel
     */
    constexpr static const int LogPanelWidth = Screen::ScreenWidth - ArenaPanelWidth;

    /**
     * @brief Width of a nameplate in the arena panel
     */
    constexpr static const int ArenaNameplateWidth = 36;

    /**
     * @brief Height of the bottom panel
     */
    constexpr static const int BottomPanelHeight = Screen::ScreenHeight - TopPanelHeight;

    /**
     * @brief Constructor
     *
     * @param battle battle
     * @param screen screen
     */
    BattleScreen(Battle::Battle& battle, Screen& screen, InputHandler& inputHandler);

    /**
     * @brief Destructor
     */
    ~BattleScreen();

    /**
     * @brief Initialize the subscreen
     */
    virtual void Init() override;

    /**
     * @brief Clean up after the subscreen
     */
    virtual void Terminate() override;

    /**
     * @brief Select the player action via menu
     *
     * @return int action code
     */
    int SelectPlayerAction(const std::map<int, std::string>& actions);

    /**
     * @brief Select an option within the bottom panel with a hover action
     *
     * @param options options
     * @param hoverAction hover action
     * @return int option code
     */
    int SelectWithHoverAction(const std::map<int, std::string>& options,
                              std::function<void(std::map<int, std::string>::iterator)> hoverAction = {});

    /**
     * @brief Write a message in the upper left corner of the bottom panel
     *
     * @param message message
     */
    void PostMessage(const std::string& message);

    /**
     * @brief Project an attack against the enemy
     *
     * @param attackSkill attack skill to consider
     */
    void ProjectSkillUse(const Battle::AttackSkill& attackSkill);

    /**
     * @brief Clear the projection area
     */
    void ClearProjectionArea();

    /**
     * @brief Clear the skill thumbnail area
     */
    void ClearThumbnailArea();

    /**
     * @brief Animate a player attack
     *
     * @param displayData result data to display
     */
    void AnimatePlayerAttack(const Battle::AttackSkill::AttackSkillResult& displayData);

    /**
     * @brief Animate a player attack
     *
     * @param displayData result data to display
     */
    void AnimatePlayerAttack(const Battle::ApplyEffectOnlySkillResult& displayData);

    /**
     * @brief Animate an enemy attack
     *
     * @param displayData result data to display
     * @param skillName name of skill used
     */
    void AnimateEnemyAttack(const Battle::AttackSkill::AttackSkillResult& displayData, const std::string& skillName);

    /**
     * @brief Animate an enemy attack
     *
     * @param displayData result data to display
     * @param skillName name of skill used
     */
    void AnimateEnemyAttack(const Battle::ApplyEffectOnlySkillResult& displayData, const std::string& skillName);

    /**
     * @brief Display the message after the battle
     *
     * @param result battle result
     */
    void BattleEndMessage(Battle::Battle::Result result);

    /**
     * @brief Display the player stats in the stat window
     */
    void DisplayPlayerStats();

    /**
     * @brief Draw the base of the skill thumbnail for the skill being hovered over
     *
     * @param skill skill being hovered over
     */
    void DrawSkillHoverThumbnailBase(const Battle::Skill& skill);

    template<typename EffectType>
    void PrintSkillHoverThumbnailInfo(const Battle::ApplyEffectOnlySkill<EffectType>& applyEffectOnlySkill)
    {
        DrawSkillHoverThumbnailBase(applyEffectOnlySkill);

        // Target type
        std::string targetTypeName;
        short targetTypeColor;
        switch (applyEffectOnlySkill.GetTargetType())
        {
        case Battle::Skill::Target::Opponent:
            targetTypeName = "Opponent";
            targetTypeColor = ColorPairs::RedOnDefault;
            break;
        case Battle::Skill::Target::Self:
            targetTypeName = "Self";
            targetTypeColor = ColorPairs::GreenOnDefault;
            break;
        case Battle::Skill::Target::Choice:
            targetTypeName = "Choice";
            targetTypeColor = ColorPairs::CyanOnDefault;
            break;
        case Battle::Skill::Target::Both:
            targetTypeName = "Both";
            targetTypeColor = ColorPairs::YellowOnDefault;
            break;
        }
        mvwaddstr(m_BottomPanelWindow, 1, SkillHoverThumbnailXPos + 2, "Target ");
        wattron(m_BottomPanelWindow, A_BOLD | COLOR_PAIR(targetTypeColor));
        waddstr(m_BottomPanelWindow, targetTypeName.c_str());
        wattroff(m_BottomPanelWindow, A_BOLD | A_COLOR);

        auto descriptionLines = SplitStringIntoLines(applyEffectOnlySkill.GetEffectDescription(), ArenaPanelWidth - SkillHoverThumbnailXPos - 4);
        int counter           = 0;
        for (const auto& line : descriptionLines)
        {
            mvwaddstr(m_BottomPanelWindow, 3 + counter, SkillHoverThumbnailXPos + 2, line.c_str());
            counter++;
        }

        wrefresh(m_BottomPanelWindow);
    }

    /**
     * @brief Display additional thumbnail info for a given skill type
     *
     * @param attackSkill attack skill being hovered over
     */
    void PrintSkillHoverThumbnailInfo(const Battle::AttackSkill& attackSkill);

    /**
     * @brief Add a message to the battle log
     *
     * @param message message
     */
    void AppendToLog(const std::string& message);

    /**
     * @brief Display effects active on the player
     */
    void DisplayPlayerActiveEffects();

    /**
     * @brief Display effects active on the enemy
     */
    void DisplayEnemyActiveEffects();

private:
    /**
     * @brief X position of the skill hover thumbnail
     */
    constexpr static const int SkillHoverThumbnailXPos = 23;

    Battle::Battle& m_Battle;
    WINDOW* m_ArenaPanelWindow;
    WINDOW* m_BottomPanelWindow;
    WINDOW* m_StatPanelWindow;
    WINDOW* m_PlayerActiveEffectsWindow;
    WINDOW* m_EnemyActiveEffectsWindow;
    Components::Nameplate m_PlayerNameplate;
    Components::Nameplate m_EnemyNameplate;
    Components::LogWindow m_LogWindow;

    /**
     * @brief Draw the layout of the panels
     */
    void DrawScreenLayout();

    /**
     * @brief Draw the arena panel
     */
    void DrawArenaPanel();

    /**
     * @brief Draw the log panel
     */
    void DrawLogPanel();

    /**
     * @brief Draw the bottom panel
     */
    void DrawBottomPanel();

    /**
     * @brief Draw the stat panel
     */
    void DrawStatPanel();

    /**
     * @brief Clear the bottom panel contents
     */
    void ClearBottomPanel();

    /**
     * @brief Battle start animation
     */
    void AnimateBattleStart();

    /**
     * @brief Battle end animation
     */
    void AnimateBattleEnd();

    /**
     * @brief Log an attack
     */
    void LogAttack(const Battle::AttackSkill::AttackSkillResult& result,
                   const std::string& attacker,
                   const std::string& target);
};

} /* namespace UI */