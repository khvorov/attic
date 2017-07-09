#include <initializer_list>
#include <memory>
#include <type_traits>

// node
template <typename R>
struct node
{
    using ptr_type = std::shared_ptr<node<R>>;

    std::string m_name;
};

template <typename R>
using node_ptr = typename node<R>::ptr_type;

// source node
template <typename R>
struct source_node : public node<R>
{
    using result_type = R;
};

// compute node
template <typename F, typename... Ts>
struct compute_node : public node<std::result_of_t<F(Ts...)>>
{
public:
    using result_type = std::result_of_t<F(Ts...)>;
    using ptr_type = std::shared_ptr<compute_node<F, Ts...>>;

    compute_node(const std::string & name, F&& f, node_ptr<Ts>&&... ts)
        : m_causes(std::forward<node_ptr<Ts>>(ts)...)
    {
        //
    }

    void add_effect(node_ptr<result_type> e) {}

private:
    using cause_nodes_type = std::tuple<node_ptr<Ts>...>;
    cause_nodes_type m_causes;
};

// helper types for compute node
template <typename F, typename... Ts>
using compute_node_ptr = typename compute_node<F, Ts...>::ptr_type;

// bubble net
class compute_graph
{
public:
    template <typename F, typename... Ts>
    compute_node_ptr<F, Ts...> add_compute_node(F&& f, node_ptr<Ts>&&... ts)
    {
        return add_compute_node("mlc::util::uuid().toString()", std::forward<F>(f), std::forward<Ts>(ts)...);
    }

    template <typename F, typename... Ts>
    compute_node_ptr<F, Ts...> add_compute_node(const std::string & name, F&& f, node_ptr<Ts>&&... ts)
    {
        auto cn = std::make_shared<compute_node<F, Ts...>>(name, std::forward<F>(f), std::forward<node_ptr<Ts>>(ts)...);

        addNamedNode(name, cn);
        std::initializer_list<int>{(ts.add_effect(cn), 0)...};

        return cn;
    }

private:
    template <typename T>
    void addNamedNode(const std::string & name, node_ptr<T> node) { /**/ }
};

int main()
{
    return 0;
}