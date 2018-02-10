#ifndef SINGLETON_REGISTRY_H_
#define SINGLETON_REGISTRY_H_

namespace singleton {

template <typename... Types>
class registry;

template <typename Head, typename... Tail>
struct registry<Head, Tail...> : registry<Tail...>
{
    typedef registry<Tail...> base_type;

    static Head &get_instance()
    {
        static Head *instance;

        if (!instance) {
            instance = new Head;
            instance->initialize();
        }

        return *instance;
    }
};

template <>
struct registry<>
{
};

template <typename T, typename Registry>
struct find;

template <typename T, typename Registry>
struct find
{
    typedef typename find<T, typename Registry::base_type>::result result;
};

template <typename T, typename... Tail>
struct find<T, registry<T, Tail...>>
{
    typedef registry<T, Tail...> result;
};
}

#endif // SINGLETON_REGISTRY_H_
