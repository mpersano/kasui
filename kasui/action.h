#pragma once

#include <vector>
#include <memory>

struct abstract_action
{
    virtual ~abstract_action() {}

    virtual void step(int dt) = 0;
    virtual bool done() const = 0;

    virtual void set_properties() const = 0; // XXX remove this later
    virtual void reset() = 0;
};

struct timed_action : abstract_action
{
    timed_action(int tics)
        : tics(tics)
        , cur_tic(0)
    {
    }

    void step(int dt);
    bool done() const;

    void reset() { cur_tic = 0; }

    int tics, cur_tic;
};

struct delay_action : timed_action
{
    delay_action(int tics)
        : timed_action(tics)
    {
    }

    void execute(float) const {}

    void set_properties() const {}
};

template <class tweening_functor>
struct property_change_action : timed_action
{
    using type = typename tweening_functor::type;

    property_change_action(type *property, const type &from, const type &to, int tics)
        : timed_action(tics)
        , from(from)
        , to(to)
        , property(property)
    {
    }

    void set_properties() const
    {
        const float t = static_cast<float>(cur_tic) / tics;
        *property = tweener(from, to, t);
    }

    type from, to;
    type *property;
    tweening_functor tweener;
};

struct action_group : abstract_action
{
    action_group *add(abstract_action *action);

    bool done() const;
    void reset();

    std::vector<std::unique_ptr<abstract_action>> actions_;
};

struct parallel_action_group : action_group
{
    void step(int dt);
    void set_properties() const;
};

struct sequential_action_group : action_group
{
    void step(int dt);
    void set_properties() const;
};
