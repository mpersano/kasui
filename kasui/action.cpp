#include <algorithm>

#include "action.h"

void timed_action::step(int dt)
{
    cur_tic = std::min(cur_tic + dt, tics);
    set_properties();
}

bool timed_action::done() const
{
    return cur_tic == tics;
}

action_group *action_group::add(abstract_action *action)
{
    actions_.emplace_back(action);
    return this;
}

void action_group::reset()
{
    for (auto &p : actions_)
        p->reset();
}

bool action_group::done() const
{
    for (auto &p : actions_) {
        if (!p->done())
            return false;
    }

    return true;
}

void parallel_action_group::step(int dt)
{
    for (auto &p : actions_)
        p->step(dt);
}

void parallel_action_group::set_properties() const
{
    for (auto &p : actions_) {
        if (!p->done())
            p->set_properties();
    }
}

void sequential_action_group::step(int dt)
{
    for (auto &p : actions_) {
        if (!p->done()) {
            p->step(dt);
            break;
        }
    }
}

void sequential_action_group::set_properties() const
{
    for (auto &p : actions_) {
        if (!p->done()) {
            p->set_properties();
            break;
        }
    }
}
