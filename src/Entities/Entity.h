#pragma once

#include "Misc/Coords.h"
#include <iostream>
#include <ncurses.h>
#include <string>

namespace Entities
{

class Entity
{
public:
    /**
     * @brief Constructor
     * 
     * @param name name
     * @param description description (default: empty)
     * @param icon icon (default: set to first character of name)
     * @param isBlocking blocking attribute (default: true)
     */
    Entity(const std::string& name,
           const std::string& description = "",
           chtype icon = 0,
           bool isBlocking = true);

    Entity(const Entity&) = delete;

    Entity(Entity&&) = delete;

    Entity& operator=(const Entity&) = delete;

    Entity& operator=(Entity&&) = delete;

    /**
     * @brief Destructor
     */
    virtual ~Entity() = default;

    /**
     * @brief Set the coordinates
     * 
     * @param value
     */
    void SetCoords(Coords value);

    /**
     * @brief Get the coordinates
     * 
     * @return coordinates
     */
    Coords GetCoords() const;

    /**
     * @brief Check if this entity is blocking (can't overlap with other entities)
     * 
     * @return true if blocking
     */
    bool IsBlocking() const;

    /**
     * @brief Get the icon
     * 
     * @return char icon
     */
    chtype GetIcon() const;

    /**
     * @brief Get the name
     * 
     * @return const std::string& name
     */
    const std::string& GetName() const;

    /**
     * @brief Get the description
     * 
     * @return const std::string& description
     */
    const std::string& GetDescription() const;

protected:
    std::string m_Name;
    std::string m_Description;
    chtype m_Icon;
    bool m_Blocking;
    Coords m_Coords;
};

} /* namespace Entities */