#pragma once

namespace fast_io
{

namespace details
{
inline constexpr void report_win32_error(error_reporter& report,std::uint32_t ec)
{
	constexpr std::size_t buffer_size{32768};
	reserve_write(report,buffer_size,[&](auto ptr)
	{
		auto const buffer_length(win32::FormatMessageA(
		0x00000200 | 0x00001000,
		nullptr,
		ec,
		(1 << 10),
		ptr,
		buffer_size,
		nullptr));
		return ptr+buffer_length;
	});
}
}

class win32_error : public fast_io_error
{
	std::uint32_t ec;
public:
	explicit win32_error(std::uint32_t error = win32::GetLastError()):ec(error){}
	constexpr auto code() const noexcept
	{
		return ec;
	}
#if __cpp_constexpr >= 201907L
	constexpr
#endif
	void report(error_reporter& report) const override
	{
		details::report_win32_error(report,ec);
	}
};
inline void throw_win32_error()
{
#ifdef __cpp_exceptions
	throw win32_error();
#else
	fast_terminate();
#endif
}
inline void throw_win32_error(std::uint32_t err)
{
#ifdef __cpp_exceptions
	throw win32_error(err);
#else
	fast_terminate();
#endif
}
}