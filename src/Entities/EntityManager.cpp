#include "EntityManager.h"
#include "Character.h"
#include "Entity.h"
#include "Misc/Direction.h"
#include "Player.h"
#include "Worlds/Field.h"
#include "Worlds/Room.h"
#include "Worlds/WorldManager.h"
#include <memory>

namespace Entities
{

EntityManager::EntityManager(Worlds::WorldManager& worldManager, Player& player)
    : m_WorldManager(worldManager),
      m_Player(player)
{
}

void EntityManager::StoreEntity(Worlds::Room& room, Entity& entity)
{
    // Storing pointers is safe because both rooms and entities are tied to the world's lifespan
    m_EntityStorage[&room].emplace_back(&entity);
}

void EntityManager::CycleEntitiesInCurrentRoom()
{
    CycleEntitiesInRoom(m_WorldManager.GetCurrentRoom());
}

bool EntityManager::TryMovePlayerEntity(Direction dir)
{
    if (CanCharacterMove(m_Player, dir))
    {
        VacateEntityFieldInRoom(m_Player, m_WorldManager.GetCurrentRoom());
        m_Player.Move(dir);
        PlaceEntityInRoom(m_Player, m_WorldManager.GetCurrentRoom());
        CycleEntitiesInCurrentRoom();
        return true;
    }
    else if (m_WorldManager.GetCurrentRoom().IsPositionAtRoomEdge(m_Player.GetCoords(), dir))
    {
        Direction nextRoomEntranceDir = dir.Opposite();
        VacateEntityFieldInRoom(m_Player, m_WorldManager.GetCurrentRoom());
        Worlds::Room& nextRoom = m_WorldManager.SwitchCurrentRoom(dir);
        Coords newCoords = nextRoom
                               .TryGetEntrance(nextRoomEntranceDir)
                               ->GetCoords();
        m_Player.SetCoords(newCoords);
        m_Player.SetLastMoveDirection(dir);
        PlaceEntityInRoom(m_Player, nextRoom);
        CycleEntitiesInCurrentRoom();
        return true;
    }

    m_Player.SetLastMoveDirection(dir);
    return false;
}

const Entity* EntityManager::GetApproachedEntity(const Character& approachingCharacter) const
{
    const Worlds::Field* approachedField = GetFieldNextToEntity(approachingCharacter, approachingCharacter.GetLastMoveDirection());
    return approachedField != nullptr ? approachedField->TryGetForegroundEntity() : nullptr;
}

bool EntityManager::CanCharacterMove(const Character& character, Direction dir) const
{
    if (dir == Direction::None()) return true;

    const Worlds::Field* targetField = GetFieldNextToEntity(character, dir);
    if (targetField != nullptr && targetField->TryGetForegroundEntity() == nullptr)
    {
        return true;
    }

    return false;
}

const std::array<const Worlds::Field*, 4> EntityManager::GetFieldsNextToEntity(
    const Entity& entity) const
{
    Coords coords = entity.GetCoords();
    const Worlds::Room& currentRoom = m_WorldManager.GetCurrentRoom();
    std::array<const Worlds::Field*, 4> fields;
    for (const auto& dir : Direction::All())
    {
        fields[dir.ToInt()] = !currentRoom.IsPositionAtRoomEdge(coords, dir)
                                  ? &currentRoom.GetFieldAt(coords.GetAdjacent(dir))
                                  : nullptr;
    }
    return fields;
}

const Worlds::Field* EntityManager::GetFieldNextToEntity(
    const Entity& entity,
    Direction direction) const
{
    return direction != Direction::None()
               ? GetFieldsNextToEntity(entity)[direction.ToInt()]
               : nullptr;
}

void EntityManager::CycleEntitiesInRoom(Worlds::Room& room)
{
    for (auto& entity : m_EntityStorage[&room])
    {
        VacateEntityFieldInRoom(*entity, room);
        //TODO: Implement NPC behavior
        //TODO: Resolve movement conflicts between player and NPCs or two NPCs
        PlaceEntityInRoom(*entity, room);
    }
}

void EntityManager::PlaceEntityInRoom(Entity& entity, Worlds::Room& room)
{
    auto& field = room.GetFieldAt(entity.GetCoords());
    field.PlaceEntity(entity);
}

void EntityManager::VacateEntityFieldInRoom(Entity& entity, Worlds::Room& room)
{
    auto& field = room.GetFieldAt(entity.GetCoords());
    entity.IsBlocking() ? field.VacateForeground() : field.VacateBackground();
}

} /* namespace Entities */