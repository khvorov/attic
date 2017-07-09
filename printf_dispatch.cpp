#include <chrono>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>

//
template <typename F, typename Tuple, std::size_t... S>
auto invoke_helper(F&& f, Tuple&& args, std::index_sequence<S...>)
{
	return std::forward<F>(f)(std::get<S>(std::forward<Tuple>(args))...);
}

template <typename F, typename Tuple>
auto invoke(F&& f, Tuple&& args)
{
	constexpr auto tuple_size = std::tuple_size<std::decay_t<Tuple>>{};
	return invoke_helper(std::forward<F>(f), std::forward<Tuple>(args), std::make_index_sequence<tuple_size>{});
}

//
using raw_args_buffer_t = std::uint8_t[];
using args_buffer_t = std::unique_ptr<raw_args_buffer_t>;
using formatter_dispatch_t = int (args_buffer_t&&);

template <typename... Args>
struct args_traits_t
{
	using args_t = std::tuple<std::decay_t<Args>...>;

	static constexpr std::size_t args_align = alignof(args_t);
	static constexpr std::size_t args_offset = (sizeof(formatter_dispatch_t *) + args_align - 1) / args_align * args_align;
	static constexpr std::size_t frame_size = args_offset + sizeof(args_t);
};

template <typename T>
T&& preprocess(T&& v)
{
	return std::forward<T>(v);
}

const char * preprocess(std::string&& v)
{
	return v.c_str();
}

template <typename... Args>
int formatter(Args&&... args)
{
	return ::printf(preprocess(std::forward<Args>(args))...);
}

template <typename... Args>
int unpack(args_buffer_t&& buffer)
{
	using traits = args_traits_t<Args...>;
	using args_t = typename traits::args_t;

	args_t & args = *reinterpret_cast<args_t *>(buffer.get() + traits::args_offset);
	invoke(&formatter<Args...>, std::move(args));

	args.~args_t();

	return 0;
}

template <typename... Args>
args_buffer_t pack(Args&&... args)
{
	using traits = args_traits_t<Args...>;

	auto ptr = std::make_unique<raw_args_buffer_t>(traits::frame_size);
	auto mem_blk = ptr.get();

	*reinterpret_cast<formatter_dispatch_t **>(mem_blk) = &unpack<std::decay_t<Args>...>;
	new (mem_blk + traits::args_offset) typename traits::args_t(std::forward<Args>(args)...);

	return std::move(ptr);
}

void dispatch(args_buffer_t&& buffer)
{
	std::thread([buffer = std::move(buffer)] () mutable
	{
		formatter_dispatch_t * pFormatter = *reinterpret_cast<formatter_dispatch_t **>(buffer.get());
		pFormatter(std::move(buffer));
	}).detach();
}

template <typename... Args>
void write(Args&&... args)
{
	dispatch(pack(std::forward<Args>(args)...));
}

int main(int argc, char * argv[])
{
	using namespace std::chrono_literals;

	write("%s, %d\n", "123", argc);

	std::this_thread::sleep_for(0.5s);

	return 0;
}

