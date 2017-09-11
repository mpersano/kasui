#ifndef ACTION_H_
#define ACTION_H_

struct abstract_action {
	virtual ~abstract_action()
	{ }

	virtual void step(int dt) = 0;
	virtual bool done() const = 0;

	virtual void set_properties() const = 0; // XXX remove this later
	virtual void reset() = 0;
};

struct timed_action : abstract_action {
	timed_action(int tics)
	: tics(tics), cur_tic(0)
	{ }

	void step(int dt);
	bool done() const;

	void reset()
	{ cur_tic = 0; }

	int tics, cur_tic;
};

struct delay_action : timed_action {
	delay_action(int tics)
	: timed_action(tics)
	{ }

	void execute(float) const
	{ }

	void set_properties() const
	{ }
};

template <class tweening_functor>
struct property_change_action : timed_action {
	typedef typename tweening_functor::type type;

	property_change_action(type *property, const type& from, const type& to, int tics)
	: timed_action(tics), from(from), to(to), property(property)
	{ }

	void set_properties() const
	{ 
		const float t = static_cast<float>(cur_tic)/tics;
		*property = tweener(from, to, t);
	}

	type from, to;
	type *property;
	tweening_functor tweener;
};

struct action_group : abstract_action {
	action_group()
	: head(0), tail(&head)
	{ }

	~action_group()
	{ if (head) delete head; }

	action_group *add(abstract_action *action);

	bool done() const;
	void reset();

	struct action_list_node {
		action_list_node(abstract_action *action)
		: action(action), next(0)
		{ }

		virtual ~action_list_node()
		{ delete action; if (next) delete next; }

		abstract_action *action;
		action_list_node *next;
	};

	action_list_node *head, **tail;
};

struct parallel_action_group : action_group {
	void step(int dt);
	void set_properties() const;
};

struct sequential_action_group : action_group {
	void step(int dt);
	void set_properties() const;
};

#endif // ACTION_H_
