#include <algorithm>
#include <iostream>
#include <iterator>
#include <map>
#include <string>

using kv_map = std::map<std::string, std::string>;

template <typename InputIterator, typename Container, typename InputValueType = typename std::iterator_traits<InputIterator>::value_type>
void parse_key_value_message(InputIterator f, InputIterator l, Container & o, const InputValueType field_separator, const InputValueType kv_separator)
{
    using key_type = typename Container::key_type;
    using value_type = typename Container::mapped_type;

    for (auto p = std::find(f, l, kv_separator); p != l; p = std::find(f, l, kv_separator))
    {
        auto q = std::find(p, l, field_separator);
        o.emplace(key_type(f, p), value_type(std::next(p), q));
        f = std::next(q);
    }
}

template <typename InputIterator, typename Container>
void parse_amps_message(InputIterator f, InputIterator l, Container & o)
{
    parse_key_value_message(f, l, o, '\x01', '=');
}

int main()
{
    // const std::string msg("key1=value1;key_2=1234;key3=;keyN=valueN;");
    const std::string msg("key1=value1\u0001key_2=1234\u0001key3=\u0001keyN=valueN\u0001");

    kv_map result;
    // parse_key_value_message(msg.begin(), msg.end(), result, ';', '=');
    parse_amps_message(msg.begin(), msg.end(), result);
    for (auto && p : result)
        std::cout << p.first << " ==> " << p.second << std::endl;

    return 0;
}
