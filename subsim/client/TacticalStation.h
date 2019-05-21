#pragma once

#include "../common/EventSystem.h"
#include "../common/SimulationEvents.h"

#include "UI.h"
#include "MockUIEvents.h"

#include <queue>

/// forward declaration
class Terrain;

/*!
 * Class that handles the tactical station. This includes "shitty IRC",
 * sonar, weapons, and repair
 */
class TacticalStation : public EventReceiver, public Renderable
{
public:
    /// Init internal state, with team/unit plus pointer to a terrain object
    TacticalStation(uint32_t team, uint32_t unit, Terrain* terrain_);

    /// Receives a text message from the server
    HandleResult receiveTextMessage(TextMessage* message);

    /// Handles key events from SDL. This enables/disables text input using the enter key
    HandleResult handleKeypress(KeyEvent* keypress);

    /// Handles text inputs from SDL. This is how we accumulate our shitty IRC message
    HandleResult handleText(TextInputEvent* text);

    /// Gets our current UnitState
    HandleResult handleUnitState(UnitState* state);

    /// Gets our current sonar display state
    HandleResult handleSonarDisplay(SonarDisplayState* sonar);

    /// Handles drawing the current state on the renderer.
    virtual void redraw() override;

private:
    /// Renders the tube status in the upper right corner
    void renderTubeState();

    /// Renders the submarine at a given position, at a given heading
    void renderSubmarine(int64_t x, int64_t y, int16_t heading);

    /// Renders the terrain
    void renderTerrain();

    /// Returns if a given x/y point is in bounds of the screen
    bool inBounds(int64_t x, int64_t y);

    /// displayX() and displayY() convert from world coordinates to SDL display
    /// coordinates based on the unit's current location
    int64_t displayX(int64_t x, int64_t y);
    int64_t displayY(int64_t x, int64_t y);

    // displayHeading() converts from a global heading to a SDL angle number
    int16_t displayHeading(int16_t heading);

    uint32_t team;
    uint32_t unit;

    /// Container for the last received text messages
    std::queue<std::string> lastMessages;

    /// Stores if we are receiving text
    bool receivingText;
    
    /// Last received unit state for our unit
    UnitState lastState;
    
    /// Last received sonar state (shared across all units)
    SonarDisplayState lastSonar;

    /// Stores a pointer to the Terrain object we will use
    Terrain* terrain;
};

