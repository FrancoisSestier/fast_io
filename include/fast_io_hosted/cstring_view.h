#pragma once

namespace fast_io
{
struct null_terminated_t
{};
inline constexpr null_terminated_t null_terminated{};

template<typename ch_type,typename tr_type = std::char_traits<ch_type>>
class basic_cstring_view:public std::basic_string_view<ch_type,tr_type>
{
public:
	using string_view_type = std::basic_string_view<ch_type,tr_type>;
	using typename std::basic_string_view<ch_type,tr_type>::traits_type;
	using typename std::basic_string_view<ch_type,tr_type>::value_type;
	using typename std::basic_string_view<ch_type,tr_type>::pointer;
	using typename std::basic_string_view<ch_type,tr_type>::const_pointer;
	using typename std::basic_string_view<ch_type,tr_type>::reference;
	using typename std::basic_string_view<ch_type,tr_type>::const_reference;
	using typename std::basic_string_view<ch_type,tr_type>::const_iterator;
	using typename std::basic_string_view<ch_type,tr_type>::iterator;
	using typename std::basic_string_view<ch_type,tr_type>::const_reverse_iterator;
	using typename std::basic_string_view<ch_type,tr_type>::reverse_iterator;
	using typename std::basic_string_view<ch_type,tr_type>::size_type;
	using typename std::basic_string_view<ch_type,tr_type>::difference_type;
	constexpr basic_cstring_view() noexcept=default;
	constexpr basic_cstring_view(const_pointer cstr) noexcept:string_view_type(cstr){}
	constexpr basic_cstring_view(null_terminated_t, const_pointer c, size_type len) noexcept:string_view_type(c,len){}
	constexpr basic_cstring_view(null_terminated_t, string_view_type sv) noexcept:string_view_type(sv){}

	template<std::ranges::contiguous_range crg>
	requires (std::same_as<std::ranges::range_value_t<crg>, ch_type>&&requires(crg cont)
	{
		{cont.c_str()}->std::same_as<const_pointer>;
	})
	constexpr basic_cstring_view(crg const& cont):string_view_type(std::ranges::data(cont),std::ranges::size(cont)){}

#if __cplusplus > 201703L && __cpp_lib_concepts
      template<std::contiguous_iterator It, std::sized_sentinel_for<It> End>
	requires std::same_as<std::iter_value_t<It>, ch_type>
	  && (!std::convertible_to<End, size_type>)
	constexpr basic_cstring_view(null_terminated_t, It first, End last): string_view_type(first,last){}
#endif

	constexpr string_view_type substr(size_type pos=0, size_type n=std::basic_string_view<ch_type,tr_type>::npos) const = delete;

	constexpr void remove_suffix(size_type n)=delete;

	constexpr const_pointer c_str() const noexcept
	{
		return this->data();
	}
};

using cstring_view = basic_cstring_view<char>;
using wcstring_view = basic_cstring_view<wchar_t>;

}