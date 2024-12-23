#pragma once

#include <cstdint>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <functional>
#include <memory>
#include <cassert>
#include <optional>

/**
 * @brief A performant finite state machine with transition caching
 *
 * @tparam T The type used to identify states; typically enum or string
 * @tparam Args Variadic template parameters that will be passed down in condition arguments
 */
template<typename T, typename... Args>
class StateMachine
{
private:
    class State;
    class Transition;

    /**
     * @brief Cache system for optimizing frequent transitions
     *
     * Stores the last successful transition and index for quick access
     * on subsequent updates.
     */
    class TransitionCache
    {
        std::shared_ptr<State> target;
        size_t transition_idx;

    public:
        /**
        * @brief Constructs a new transition cache
        *
        * @param to Target state for the cached transition
        * @param idx Index of the transition in the source state's transition list
        */
        explicit TransitionCache(std::shared_ptr<State> to, size_t idx)
            : target(to), transition_idx(idx) {}

        /**
         * @brief Attempts to use the cached transition
         *
         * @param state Current state to transition from
         * @param entity_id Entity ID for condition checking
         * @param args Additional arguments for condition checking
         * @return true if the cached transition is valid and condition passes
         */
        [[nodiscard]] bool try_transition(
            const State* state,
            uint32_t entity_id,
            Args... args
        ) const
        {
            return state->transitions[transition_idx]->check_condition(entity_id, args...);
        }

        [[nodiscard]] std::shared_ptr<State> get_target() const
        {
            return target;
        }

        [[nodiscard]] size_t get_index() const
        {
            return transition_idx;
        }
    };

    /**
     * @brief Represents a transition between states
     *
     * Holds the target state and condition function that must be satisfied
     * for the transition to occur.
     */
    class Transition
    {
        std::shared_ptr<State> to;
        std::function<bool(uint32_t, Args...)> condition;

    public:

        explicit Transition(
            std::shared_ptr<State> to,
            std::function<bool(uint32_t, Args...)> condition
        ) : to(to), condition(condition) {}

        [[nodiscard]] std::shared_ptr<State> get_target() const
        {
            return to;
        }

        [[nodiscard]] bool check_condition(uint32_t entity_id, Args... args) const
        {
            return condition(entity_id, args...);
        }
    };

    /**
     * @brief Represents a state in the finite state machine
     *
     * Manages outgoing transitions, callbacks, and transition caching
     * for a single state.
     */
    class State
    {
        T name;
        uint32_t entity_id;
        std::vector<std::shared_ptr<Transition>> transitions;
        std::function<void(uint32_t)> on_enter;
        std::function<void(uint32_t)> on_exit;
        mutable std::mutex cache_mutex;
        mutable std::optional<TransitionCache> cache;

        friend class TransitionCache;

    public:
        explicit State(T name, uint32_t entity_id)
            : name(name)
            , entity_id(entity_id)
            , cache(std::nullopt) {}

        void add_transition(std::shared_ptr<Transition> transition)
        {
            transitions.push_back(transition);
        }

        [[nodiscard]] bool try_cached_transition(uint32_t entity_id, Args... args) const
        {
            std::lock_guard lock(cache_mutex);
            if (cache && cache->try_transition(this, entity_id, args...))
                return true;

            cache.reset();
            return false;
        }

        void update_cache(size_t idx, std::shared_ptr<State> target) const
        {
            std::lock_guard lock(cache_mutex);
            cache.emplace(target, idx);
        }

        [[nodiscard]] const auto& get_transitions() const
        {
            return transitions;
        }

        [[nodiscard]] const auto& get_name() const
        {
            return name;
        }

        [[nodiscard]] auto get_entity_id() const
        {
            return entity_id;
        }

        [[nodiscard]] const auto& get_cache() const
        {
            return cache;
        }

        void set_callbacks(
            std::function<void(uint32_t)> enter,
            std::function<void(uint32_t)> exit
        )
        {
            if (enter)
                on_enter = enter;
            if (exit)
                on_exit = exit;
        }

        void enter() const
        {
            if (on_enter)
                on_enter(entity_id);
        }

        void exit() const
        {
            if (on_exit)
                on_exit(entity_id);
        }
    };

    void handle_transition(std::shared_ptr<State> new_state)
    {
        current_state->exit();
        current_state = new_state;
        current_state->enter();
    }

    std::unordered_map<T, std::shared_ptr<State>> states;
    std::shared_ptr<State> current_state;

public:
    // Constructor & destructor
    StateMachine() = default;
    ~StateMachine() = default;

    // No copy
    StateMachine(const StateMachine&) = delete;
    StateMachine& operator=(const StateMachine&) = delete;

    // Move semantics
    StateMachine(StateMachine&&) = default;
    StateMachine& operator=(StateMachine&&) = default;

    std::shared_ptr<State> add_state(T name, uint32_t entity_id)
    {
        if (states.contains(name))
            return states[name];

        auto state = std::make_shared<State>(name, entity_id);
        states[name] = state;
        if (!current_state)
            current_state = state;

        return state;
    }

    void add_transition(
        T from,
        T to,
        std::function<bool(uint32_t, Args...)> condition
    )
    {
        assert(states.contains(from) && states.contains(to));
        auto transition = std::make_shared<Transition>(states[to], condition);
        states[from]->add_transition(transition);
    }

    void set_callback(
        T state,
        std::function<void(uint32_t)> on_enter = nullptr,
        std::function<void(uint32_t)> on_exit = nullptr
    )
    {
        assert(states.contains(state));
        states[state]->set_callbacks(on_enter, on_exit);
    }

    void start()
    {
        assert(current_state);
        current_state->enter();
    }

    void update(Args... args)
    {
        assert(current_state);

        if (current_state->try_cached_transition(
            current_state->get_entity_id(),
            args...
        )) {
            handle_transition(current_state->get_cache()->get_target());
            return;
        }

        const auto& transitions = current_state->get_transitions();
        for (size_t i = 0; i < transitions.size(); ++i) {
            if (transitions[i]->check_condition(
                current_state->get_entity_id(),
                args...
            ))
            {
                auto target = transitions[i]->get_target();
                current_state->update_cache(i, target);
                handle_transition(target);
                break;
            }
        }
    }

public:
    [[nodiscard]] T get_current_state() const
    {
        assert(current_state);
        return current_state->get_name();
    }

    [[nodiscard]] bool can_transition_to(T state, Args... args) const
    {
        assert(current_state && states.contains(state));
        auto target = states.at(state);

        if (current_state->try_cached_transition(
            current_state->get_entity_id(),
            args...
        ))
        {
            return current_state->get_cache()->get_target() == target;
        }

        const auto& transitions = current_state->get_transitions();
        for (size_t i = 0; i < transitions.size(); ++i)
        {
            if (transitions[i]->get_target() == target &&
                transitions[i]->check_condition(
                    current_state->get_entity_id(),
                    args...
                ))
            {
                current_state->update_cache(i, target);
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] bool has_state(T state) const
    {
        return states.contains(state);
    }
};
