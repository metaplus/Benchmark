## Memory Mapped File 
Usage approach  
1. Treat as a raw byte array, get underlying buffer from *data( )*, utilize the iterator 
interface same with STL container.
2. Wrap the device into a stream
```cpp
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/iostreams/stream.hpp>           
boost::iostreams::mapped_file_source mmap{"test.bin"};
boost::iostreams::stream<mapped_file_source> istream(mmap, std::ios::binary)
```
Note
```cpp
static int mapped_file::alignment();
```
Returns the operating system's virtual memory allocation granularity. 
Reference value 65536.



## Synopsis
class **mapped_file**
```cpp
#include <boost/iostreams/device/mapped_file.hpp>

namespace boost { namespace iostreams {
class mapped_file {
public:
    typedef char                      char_type;
    typedef [implementation-defined]  category;
    enum mapmode { readonly, readwrite, priv };
    mapped_file();
    explicit mapped_file(mapped_file_params params);
    explicit mapped_file( const std::string& path,
                          std::ios_base::openmode mode =
                              std::ios_base::in | std::ios_base::out,
                          size_type length = max_length,
                          boost::intmax_t offset = 0 );
    explicit mapped_file( const std::string& path,
                          mapmode mode,
                          size_type length = max_length,
                          boost::intmax_t offset = 0 );
    void open(mapped_file_params params);
    void open( const std::string& path,
               std::ios_base::openmode mode = 
                   std::ios_base | std::ios_base,
               size_type length = max_length,
               boost::intmax_t offset = 0 );
    void open( const std::string& path,
               mapmode mode,
               size_type length = max_length,
               boost::intmax_t offset = 0 );
    bool is_open() const;
    mapmode flags() const;
    void close();
    size_type size() const;
    char* data() const;
    const char* const_data() const;
    iterator begin() const;
    const_iterator const_begin() const;
    iterator end() const;
    const_iterator const_end() const;
    static int alignment();
};

} } // End namespace boost::iostreams
```