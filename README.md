# Finite State Machine

A performant, thread-safe finite state machine implementation with transition caching.

## Template Parameters

- `T`: The type used to identify states; typically an enum or string
- `Args...`: Variadic template parameters that will be passed down in condition arguments

## Features

- Thread-safe transition caching
- Move-only semantics
- Const-correct interface
- Early cache-hit returns for O(1) performance
- Compliment entity-component systems (ECS)

## Classes

### `StateMachine`

The main state machine container that manages states and transitions.

### Public API

```cpp
// Creates a new state or returns existing one
std::shared_ptr<State> add_state(T name, uint32_t entity_id);

// Adds a transition between states
void add_transition(T from, T to, std::function<bool(uint32_t, Args...)> condition);

// Sets callbacks for state enter/exit
void set_callback(T state, std::function<void(uint32_t)> on_enter = nullptr,
                          std::function<void(uint32_t)> on_exit = nullptr);

// Starts the state machine
void start();

// Updates the state machine, checking transitions
void update(Args... args);

// Gets current state identifier
[[nodiscard]] T get_current_state() const;

// Checks if transition to target state is possible
[[nodiscard]] bool can_transition_to(T state, Args... args) const;

// Checks if state exists in machine
[[nodiscard]] bool has_state(T state) const;
```

### `TransitionCache`

Internal cache mechanism for optimizing frequently used transitions.

#### Features

- Stores last successful transition
- Thread-safe access with mutex
- Updates automatically on successful transitions

## Usage Example

```cpp
enum class PlayerState
{
    IDLE,
    WALKING,
    RUNNING
};

StateMachine<PlayerState, float> player_fsm;

player_fsm.add_state(PlayerState::IDLE, entity_id);
player_fsm.add_state(PlayerState::WALKING, entity_id);
player_fsm.add_state(PlayerState::RUNNING, entity_id);

player_fsm.add_transition(
    PlayerState::IDLE,
    PlayerState::WALKING,
    [](uint32_t id, float speed)
    {
        return speed > 0.1f; // Transitions with conditions
    }
);

player_fsm.add_transition(
    PlayerState::WALKING,
    PlayerState::RUNNING,
    [](uint32_t id, float speed)
    {
        return speed > 5.0f;
    }
);

// Add callbacks
player_fsm.set_callback(PlayerState::RUNNING,
    [](uint32_t id)
    {
        /* on enter */
    },
    [](uint32_t id)
    {
        /* on exit */
    }
);

// Start and update
player_fsm.start();
player_fsm.update(current_speed);
```

## License

MIT License. For more information, please visit [LICENSE]().

## Acknowledgement

Thanks to @alpluspluss for design ideas & help.
