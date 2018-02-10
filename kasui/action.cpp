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
    action_list_node *p = new action_list_node(action);

    *tail = p;
    tail = &p->next;

    return this;
}

void action_group::reset()
{
    for (action_list_node *p = head; p; p = p->next)
        p->action->reset();
}

bool action_group::done() const
{
    for (action_list_node *p = head; p; p = p->next) {
        if (!p->action->done())
            return false;
    }

    return true;
}

void parallel_action_group::step(int dt)
{
    for (action_list_node *p = head; p; p = p->next)
        p->action->step(dt);
}

void parallel_action_group::set_properties() const
{
    for (action_list_node *p = head; p; p = p->next) {
        if (!p->action->done())
            p->action->set_properties();
    }
}

void sequential_action_group::step(int dt)
{
    for (action_list_node *p = head; p; p = p->next) {
        if (!p->action->done()) {
            p->action->step(dt);
            break;
        }
    }
}

void sequential_action_group::set_properties() const
{
    for (action_list_node *p = head; p; p = p->next) {
        if (!p->action->done()) {
            p->action->set_properties();
            break;
        }
    }
}
