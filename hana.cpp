#include <boost/any.hpp>
#include <boost/hana.hpp>

#include <cassert>
#include <iostream>
#include <string>
#include <typeindex>

namespace hana = boost::hana;

struct Fish { std::string name; };
struct Cat  { std::string name; };
struct Dog  { std::string name; };

struct Person
{
    BOOST_HANA_DEFINE_STRUCT(Person,
            (std::string, name),
            (int, age)
    );
};

// switch/case
template <typename T>
auto case_ = [](auto f)
{
    return hana::make_pair(hana::type_c<T>, f);
};

struct default_t;
auto default_ = case_<default_t>;

template <typename Any, typename Default>
auto process(Any&, std::type_index const &, Default & default_)
{
    return default_();
}

template <typename Any, typename Default, typename Case, typename... Rest>
auto process(Any & a, std::type_index const & t, Default & default_, Case & case_, Rest&... rest)
{
    using T = typename decltype(+hana::first(case_))::type;
    return t == typeid(T) ? hana::second(case_)(*boost::unsafe_any_cast<T>(&a))
                          : process(a, t, default_, rest...);
}

template <typename Any>
auto switch_(Any& a)
{
    return [&a](auto... cases_)
    {
        auto cases = hana::make_tuple(cases_...);
        auto default_ = hana::find_if(cases, [](auto const & c)
        {
            return hana::first(c) == hana::type_c<default_t>;
        });

        static_assert(default_ != hana::nothing, "switch is missing a default_ case");

        auto rest = hana::filter(cases, [](auto const & c)
        {
            return hana::first(c) != hana::type_c<default_t>;
        });

        return hana::unpack(rest, [&](auto&... rest)
        {
            return process(a, a.type(), hana::second(*default_), rest...);
        });
    };
}

int main()
{
    using namespace hana::literals;

    auto animals = hana::make_tuple(Fish{"Nemo"}, Cat{"Garfield"}, Dog{"Snoopy"});

    auto names = hana::transform(animals, [](auto a) { return a.name; });

    assert(hana::reverse(names) == hana::make_tuple("Snoopy", "Garfield", "Nemo"));

    auto has_name = hana::is_valid([](auto&& x) -> decltype((void) x.name) {});
    static_assert(has_name(animals[1_c]), "");
    static_assert(!has_name(1), "");

    // serialize
    auto serialize = [](std::ostream & os, auto const & object)
    {
        hana::for_each(hana::members(object), [&](auto member)
        {
            os << member << std::endl;
        });
    };

    Person john{"John", 30};
    serialize(std::cout, john);

    // switch/case
    boost::any a = 'x';
    auto r = switch_(a)(
        case_<int>([](auto) -> int { return 1; }),
        case_<char>([](auto) -> long { return 2l; }),
        default_([]() -> long long { return 3ll; })
    );

    // the result type is a common type of all the above types
    static_assert(std::is_same<decltype(r), long long>{}, "");
    assert(r == 2ll);

    return 0;
}

