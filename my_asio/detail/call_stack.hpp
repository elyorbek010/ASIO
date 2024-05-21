#ifndef MY_ASIO_DETAIL_CALLSTACK_HPP
#define MY_ASIO_DETAIL_CALLSTACK_HPP

namespace my_asio
{
namespace detail
{
template<typename Key, typename Value = void>
class call_stack
{
public:
	call_stack(const call_stack&) = delete;
	const call_stack& operator=(const call_stack&) = delete;

	class context
	{
	public:
		context(Key* key)
			: key(key)
			, value(reinterpret_cast<void*>(this))
			, next(top)
		{
			top = this;
		}

		context(Key* key, Value* value)
			: key(key)
			, value(value)
			, next(top)
		{
			top = this;
		}

		~context()
		{
			top = next;
		}

	private:
		friend class call_stack<Key, Value>;

		Key* key;
		Value* value;
		context* next;
	};

	static Value* contains(Key* key)
	{
		context* elem = top;
		while (elem)
		{
			if (elem->key == key)
				return elem->value;
			elem = elem->next;
		}
		return nullptr;
	}

	static Value* get_top()
	{
		if (top)
			return top->value;
		return nullptr;
	}

private:
	friend class context;
	thread_local static context* top;
};

template<typename Key, typename Value>
thread_local typename call_stack<Key, Value>::context* call_stack<Key, Value>::top = nullptr;

} // namespace detail
} // namespace my_asio

#endif // MY_ASIO_DETAIL_CALLSTACK_HPP