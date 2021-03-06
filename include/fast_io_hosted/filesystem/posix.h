#pragma once
#include <dirent.h>

namespace fast_io
{

class posix_directory_io_observer
{
public:
	using native_handle_type = DIR*;
	native_handle_type dirp{};
	constexpr auto& native_handle() noexcept
	{
		return dirp;
	}
	constexpr auto& native_handle() const noexcept
	{
		return dirp;
	}
	explicit constexpr operator bool() const noexcept
	{
		return dirp;
	}
	operator basic_posix_io_observer<char>() const noexcept
	{
		if(dirfd==nullptr)
		{
			errno=EBADF;
			return {-1};
		}
		return {::dirfd(dirp)};
	}
	constexpr native_handle_type release() noexcept
	{
		auto temp{dirp};
		dirp=nullptr;
		return temp;
	}
};

namespace details
{

inline DIR* sys_dup_dir(DIR* dirp)
{
	if(dirfd==nullptr)
		throw_posix_error(EBADF);
	auto fd{::dirfd(dirp)};
	if(fd==-1)
		throw_posix_error();
	auto newfd{details::sys_dup(fd)};
	auto newdir{::fdopendir(newfd)};
	if(newdir==nullptr)
	{
		details::sys_close(newfd);
		throw_posix_error();
	}
	return newdir;
}

}

class posix_directory_io_handle:public posix_directory_io_observer
{
public:
	using native_handle_type = DIR*;
	constexpr posix_directory_io_handle()=default;
	template<typename native_hd>
	requires std::same_as<native_handle_type,std::remove_cvref_t<native_hd>>
	constexpr posix_directory_io_handle(native_hd dirp):posix_directory_io_observer{dirp}{}
	posix_directory_io_handle(posix_directory_io_handle const& other):posix_directory_io_observer{details::sys_dup_dir(other.dirp)}{}
	posix_directory_io_handle& operator=(posix_directory_io_handle const& other)
	{
		auto newdir{details::sys_dup_dir(other.dirp)};
		if(this->native_handle())[[likely]]
			::closedir(this->native_handle());
		this->native_handle()=newdir;
		return *this;
	}
	constexpr posix_directory_io_handle(posix_directory_io_handle&& other) noexcept:posix_directory_io_observer{other.release()}{}

	posix_directory_io_handle& operator=(posix_directory_io_handle&& other) noexcept
	{
		if(this==std::addressof(other))
			return *this;
		if(this->native_handle())[[likely]]
			::closedir(this->native_handle());
		this->native_handle()=other.release();
		return *this;
	}
	inline void reset(native_handle_type dirp=nullptr) noexcept
	{
		if(this->native_handle())[[likely]]
			::closedir(this->native_handle());
		this->native_handle()=dirp;
	}
	void close()
	{
		if(*this)[[likely]]
		{
			if(::closedir(this->native_handle())==-1)
				throw_posix_error();
			this->native_handle()=nullptr;
		}
	}
};

class posix_directory_file:public posix_directory_io_handle
{
public:
	using native_handle_type = DIR*;
	constexpr posix_directory_file()=default;
	template<typename native_hd>
	requires std::same_as<native_handle_type,std::remove_cvref_t<native_hd>>
	constexpr posix_directory_file(native_hd dirp):posix_directory_io_handle(dirp){}
	posix_directory_file(posix_io_handle&& pioh):posix_directory_io_handle(fdopendir(pioh.native_handle()))
	{
		if(this->native_handle()==nullptr)
			throw_posix_error();
		pioh.release();
	}
	posix_directory_file(cstring_view filename):posix_directory_file(posix_file(filename,open_interface<open_mode::in|open_mode::no_block|open_mode::directory|open_mode::binary|open_mode::large_file>))
	{
/*
https://code.woboq.org/userspace/glibc/sysdeps/posix/opendir.c.html
glibc:
enum {
  opendir_oflags = O_RDONLY|O_NDELAY|O_DIRECTORY|O_LARGEFILE|O_CLOEXEC
};
*/
	}
	posix_directory_file(io_at_t,native_io_observer niob,cstring_view filename):posix_directory_file(posix_file(io_at,niob,filename,open_interface<open_mode::in|open_mode::no_block|open_mode::directory|open_mode::binary|open_mode::large_file>))
	{
	}
	posix_directory_file(posix_directory_file const&) = default;
	posix_directory_file& operator=(posix_directory_file const&) = default;
	constexpr posix_directory_file(posix_directory_file&&) noexcept = default;
	posix_directory_file& operator=(posix_directory_file&&) noexcept = default;
	~posix_directory_file()
	{
		if(this->native_handle())[[likely]]
			::closedir(this->native_handle());
	}
};

inline void rewind(posix_directory_io_observer pdiob) noexcept
{
	::rewinddir(pdiob.dirp);	
}

inline void seek(posix_directory_io_observer pdiob,std::common_type_t<std::int64_t,std::ptrdiff_t> offset) noexcept
{
	if constexpr(sizeof(long)<sizeof(offset))
	{
		if(static_cast<std::common_type_t<std::int64_t,std::ptrdiff_t>>(std::numeric_limits<long>::max())<offset)
			fast_terminate();
	}
	::seekdir(pdiob.dirp,static_cast<long>(offset));
}

inline std::common_type_t<std::uint64_t,std::size_t> tell(posix_directory_io_observer pdiob)
{
	auto ret{::telldir(pdiob.dirp)};
	if(ret==-1)
		throw_posix_error();
	return ret;
}

struct posix_directory_entry
{
	DIR* dirp{};
	struct dirent* entry{};
	operator basic_posix_io_observer<char>() const noexcept
	{
		if(dirfd==nullptr)
		{
			errno=EBADF;
			return {-1};
		}
		return {::dirfd(dirp)};
	}
	constexpr operator posix_directory_io_observer() const noexcept
	{
		return {dirp};
	}
};

inline cstring_view filename(posix_directory_entry pioe) noexcept
{
	return pioe.entry->d_name;
}

inline constexpr std::common_type_t<std::size_t,std::uint64_t> inode(posix_directory_entry pioe) noexcept
{
	return pioe.entry->d_ino;
}

inline constexpr file_type type(posix_directory_entry pioe) noexcept
{
	switch(pioe.entry->d_type)
	{
	case DT_BLK:
		return file_type::block;
	case DT_CHR:
		return file_type::character;
	case DT_DIR:
		return file_type::directory;
	case DT_FIFO:
		return file_type::fifo;
	case DT_LNK:
		return file_type::symlink;
	case DT_REG:
		return file_type::regular;
	case DT_SOCK:
		return file_type::socket;
	case DT_UNKNOWN:
		return file_type::unknown;
	default:
		return file_type::not_found;
	};
}

struct posix_directory_iterator
{
	DIR* dirp{};
	struct dirent* entry{};
};

struct posix_directory_generator
{
	posix_directory_file dir_fl;
};

inline posix_directory_entry operator*(posix_directory_iterator pdit) noexcept
{
	return {pdit.dirp,pdit.entry};
}

inline posix_directory_iterator& operator++(posix_directory_iterator& pdit)
{
	errno=0;
	auto entry{readdir(pdit.dirp)};
	if(entry==nullptr&&errno)
		throw_posix_error();
	pdit.entry=entry;
	return pdit;
}


inline void refresh(posix_directory_iterator& pdit)
{
	::seekdir(pdit.dirp,-1);
	++pdit;
}

inline posix_directory_iterator begin(posix_directory_generator const& pdg)
{
	auto dirp{pdg.dir_fl.dirp};
	::rewinddir(dirp);
	posix_directory_iterator pdit{dirp};
	++pdit;
	return pdit;
}

inline std::default_sentinel_t end(posix_directory_generator const&) noexcept
{
	return {};
}

inline constexpr bool operator==(std::default_sentinel_t, posix_directory_iterator const& b) noexcept
{
	return b.entry == nullptr;
}
inline constexpr bool operator==(posix_directory_iterator const& b, std::default_sentinel_t) noexcept
{
	return b.entry == nullptr;
}
inline constexpr bool operator!=(std::default_sentinel_t, posix_directory_iterator const& b) noexcept
{
	return b.entry;
}
inline constexpr bool operator!=(posix_directory_iterator const& b, std::default_sentinel_t) noexcept
{
	return b.entry;
}
/*
inline posix_directory_generator current(posix_directory_io_observer pdiob) noexcept
{
	::rewinddir(pdiob.dirp);
	return {pdiob.dirp};
}
*/
inline posix_directory_generator current(posix_io_observer piob)
{
	return {.dir_fl=posix_directory_file(posix_file(io_dup,piob))};
}

struct posix_recursive_directory_iterator
{
	DIR* dirp{};
	struct dirent* entry{};
	std::vector<posix_directory_file> stack;
	posix_recursive_directory_iterator()=default;
	posix_recursive_directory_iterator(DIR* dp):dirp(dp){}
	posix_recursive_directory_iterator(posix_recursive_directory_iterator const&)=delete;
	posix_recursive_directory_iterator& operator=(posix_recursive_directory_iterator const&)=delete;
	posix_recursive_directory_iterator(posix_recursive_directory_iterator&&) noexcept=default;
	posix_recursive_directory_iterator& operator=(posix_recursive_directory_iterator&&) noexcept=default;
};

struct posix_recursive_directory_generator
{
	posix_directory_file dir_fl;
};

inline std::size_t depth(posix_recursive_directory_iterator const& prdit) noexcept
{
	return prdit.stack.size();
}

inline posix_recursive_directory_iterator& operator++(posix_recursive_directory_iterator& prdit)
{
	for(;;)
	{
		errno=0;
		if(prdit.stack.empty())
		{
			auto entry{readdir(prdit.dirp)};
			if(entry==nullptr)
			{
				if(errno)
					throw_posix_error();
				prdit.entry=nullptr;
				return prdit;
			}
			prdit.entry=entry;
		}
		else
		{
			auto entry=readdir(prdit.stack.back().dirp);
			if(entry==nullptr)
			{
				if(errno)
					throw_posix_error();
				prdit.stack.pop_back();
				continue;
			}
			prdit.entry=entry;
		}
		if(prdit.entry->d_type==DT_DIR)
		{
			auto name{prdit.entry->d_name};
			if((*name==u8'.'&&name[1]==0)||(*name==u8'.'&&name[1]==u8'.'&&name[2]==0))
				continue;
			prdit.stack.emplace_back(at,posix_directory_io_observer{prdit.stack.empty()?prdit.dirp:prdit.stack.back().dirp},name);
		}
		return prdit;
	}
}

inline void pop(posix_recursive_directory_iterator& prdit)
{
	if(prdit.stack.empty())
		prdit.entry=nullptr;
	else
	{
		prdit.stack.pop_back();
		++prdit;
	}
}

inline posix_recursive_directory_iterator begin(posix_recursive_directory_generator const& pdg)
{
	auto dirp{pdg.dir_fl.dirp};
	::rewinddir(dirp);
	posix_recursive_directory_iterator pdit{dirp};
	++pdit;
	return pdit;
}

inline std::default_sentinel_t end(posix_recursive_directory_generator const&) noexcept
{
	return {};
}

inline posix_directory_entry operator*(posix_recursive_directory_iterator const& prdit) noexcept
{
	return {prdit.stack.empty()?prdit.dirp:prdit.stack.back().dirp,prdit.entry};
}

inline bool operator==(std::default_sentinel_t, posix_recursive_directory_iterator const& b) noexcept
{
	return b.stack.empty()&&b.entry == nullptr;
}

inline bool operator==(posix_recursive_directory_iterator const& b, std::default_sentinel_t sntnl) noexcept
{
	return sntnl==b;
}

inline bool operator!=(std::default_sentinel_t sntnl, posix_recursive_directory_iterator const& b) noexcept
{
	return !(sntnl==b);
}

inline bool operator!=(posix_recursive_directory_iterator const& b, std::default_sentinel_t sntnl) noexcept
{
	return sntnl!=b;
}

inline posix_recursive_directory_generator recursive(posix_io_observer piob)
{
	return {.dir_fl=posix_directory_file(posix_file(io_dup,piob))};
}

using directory_entry = posix_directory_entry;

}