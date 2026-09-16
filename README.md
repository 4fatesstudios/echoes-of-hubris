# Echoes of Hubris

**Metroidvania · Unreal Engine 5 · C++ · Blueprints · Gameplay Systems · Enemy AI**

Echoes of Hubris is a Metroidvania set within a massive, mysterious tower inspired by the Tower of Babel. The protagonist awakens with no memory of who they are, knowing only that they must ascend.

As they climb toward the top, they explore interconnected environments, confront increasingly dangerous enemies, and uncover more about the tower and their place within it.

Developed by a six-person team in Unreal Engine 5, Echoes of Hubris was the first project set within the larger universe later shared with Project Warden.

The project combines C++ and Blueprint development across interconnected gameplay systems including combat, inventory, character movement, enemy behavior, and multi-phase boss encounters.

## Gameplay Systems

### Inventory & Items

A reusable inventory system was developed to support item management throughout the game.

The system includes:

- Item stacking
- Configurable maximum stack sizes
- Inventory slot management
- Support for dozens of item types
- Extensible item behavior

The architecture was designed to support different item types while keeping inventory behavior reusable across the project.

### Character Movement & Abilities

Player systems were developed to support the movement and mobility expected from a Metroidvania.

Development included:

- Core character movement
- Mobility abilities
- Combat interactions
- Gameplay abilities used throughout major levels
- Integration between movement and other gameplay systems

### Combat

Combat systems connect player actions, abilities, enemies, and encounters throughout the game.

Gameplay programming included the implementation and integration of combat interactions used across regular enemies and larger boss encounters.

## Enemy & Boss AI

Enemy behavior was developed to support multiple combat patterns and encounter types.

The AI systems include:

- More than 5 combat behaviors
- Enemy attack and movement logic
- Combat-state behavior
- Boss-specific mechanics
- Multi-phase boss encounters

Boss encounters combine multiple behaviors and phases to allow encounters to evolve throughout a fight rather than relying on a single repeated behavior.

## Unreal Engine Development

Echoes of Hubris combines **C++ and Blueprint workflows** throughout the project.

C++ was used for gameplay implementation and underlying systems, while Blueprints supported engine integration, gameplay configuration, and iteration within Unreal Engine.

The repository includes the project's Unreal source code, content, plugins, configuration, and supporting development infrastructure.

## Team & Development

Echoes of Hubris was developed by a **six-person team** from January through December 2024.

Development was organized around iterative milestones and Agile practices, with Git and GitHub used for version control and collaboration.

More than **400 programming hours** were contributed across development.

## Technologies

**Game Development**
- Unreal Engine 5
- C++
- Blueprints

**Gameplay**
- Inventory & Item Systems
- Character Movement
- Combat Systems
- Enemy AI
- Boss Encounters

**Development**
- Git
- GitHub

## Project Status

**Completed — December 2024**

Active development concluded in December 2024 after approximately one year of development.
