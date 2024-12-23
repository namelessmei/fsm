// Testfile for include/fsm.hpp

#include <cstdint>
#include <iostream>
#include "include/fsm.hpp"

int main()
{
    enum class PlayerState : uint8_t
    {
        IDLE,
        WALKING,
        RUNNING
    };

    const uint32_t entity_id = 1;
    StateMachine<PlayerState, float> fsm;

    fsm.add_state(PlayerState::IDLE, entity_id);
    fsm.add_state(PlayerState::WALKING, entity_id);
    fsm.add_state(PlayerState::RUNNING, entity_id);

    fsm.add_transition(
        PlayerState::IDLE,
        PlayerState::WALKING,
        [](uint32_t id, float speed)
        {
            std::cout << "Checking IDLE->WALKING: speed=" << speed << "\n";
            return speed > 0.1f;
        }
    );

    fsm.add_transition(
        PlayerState::WALKING,
        PlayerState::RUNNING,
        [](uint32_t id, float speed)
        {
            std::cout << "Checking WALKING->RUNNING: speed=" << speed << "\n";
            return speed > 5.0f;
        }
    );

    fsm.set_callback(
        PlayerState::WALKING,
        [](uint32_t id)
        {
            std::cout << "Entered WALKING\n";
        },
        [](uint32_t id)
        {
            std::cout << "Exited WALKING state\n";
        }
    );

    fsm.set_callback(
        PlayerState::RUNNING,
        [](uint32_t id)
        {
            std::cout << "Entered RUNNING state\n";
        },
        [](uint32_t id)
        {
            std::cout << "Exited RUNNING state\n";
        }
    );

    std::cout << "Starting FSM in IDLE state...\n";
    fsm.start();

    std::cout << "\nTesting transition IDLE->WALKING\n";
    fsm.update(0.2f);

    std::cout << "\nTesting cached transition with same speed\n";
    fsm.update(0.2f);

    std::cout << "\nTesting transition WALKING->RUNNING\n";
    fsm.update(6.0f);

    std::cout << "\nDone testing!\n";
    return 0;
}
