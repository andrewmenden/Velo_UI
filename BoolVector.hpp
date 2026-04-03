#ifndef BOOL_VECTOR_H
#define BOOL_VECTOR_H

#include <memory>
#include <ranges>

// breaking naming convention for being more STL style

class bool_vector
{
	size_t m_size{};
	std::unique_ptr<bool[]> m_value;

public:
	inline bool* begin()
	{
		return m_value.get();
	}

	inline bool* end()
	{
		return begin() + m_size;
	}

	inline const bool* begin() const
	{
		return m_value.get();
	}

	inline const bool* end() const
	{
		return begin() + m_size;
	}

	inline size_t size() const
	{
		return m_size;
	}

	inline bool empty() const
	{
		return m_size == 0;
	}

	inline bool& operator[](size_t i)
	{
		return m_value[i];
	}

	inline const bool& operator[](size_t i) const
	{
		return m_value[i];
	}

	inline bool_vector() = default;

	inline bool_vector(size_t size) :
		m_size{ size },
		m_value{ std::make_unique<bool[]>(size) } {}

	inline bool_vector(size_t size, bool value) :
		m_size{ size },
		m_value{ std::make_unique_for_overwrite<bool[]>(size) } 
	{
		std::ranges::fill(*this, value);
	}

	template <std::ranges::input_range R> requires
		(std::ranges::sized_range<R> &&
		std::convertible_to<std::ranges::range_value_t<R>, bool>)
	inline bool_vector(R&& right) :
		m_size{ std::ranges::size(right) },
		m_value{ std::make_unique_for_overwrite<bool[]>(m_size) }
	{
		std::ranges::copy(right, begin());
	}

	inline bool_vector(const bool_vector& right) :
		m_size{ right.m_size },
		m_value{ std::make_unique_for_overwrite<bool[]>(m_size) }
	{
		std::ranges::copy(right, begin());
	}

	inline bool_vector(bool_vector&&) noexcept = default;

	inline bool_vector& operator=(const bool_vector& right)
	{
		if (this == &right)
			return *this;

		if (right.m_size == 0)
		{
			m_size = 0;
			m_value.reset();
			return *this;
		}

		if (m_size != right.m_size)
		{
			m_value = std::make_unique_for_overwrite<bool[]>(right.m_size);
			m_size = right.m_size;
		}

		std::ranges::copy(right, begin());
		return *this;
	}

	inline bool_vector& operator=(bool_vector&&) noexcept = default;
};

#endif