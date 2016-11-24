#include <pre_cc.h>
#include "win_str.h"
#include <cc_array.hpp>
#include <boost/smart_ptr.hpp>
#include <Windows.h>

namespace ccwin
{
    template <class TO, class FROM> TO size_cast( FROM value )              { return static_cast<TO>(value); }

    UINT LCIDToCodePage( DWORD ALcid )
    {
        cclib::array<char, sizeof(DWORD) * 2>       Buffer;

        std::fill( Buffer.begin(), Buffer.end(), char() );
        if ( GetLocaleInfo( ALcid, LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER,
                            reinterpret_cast<LPWSTR>(Buffer.data()), size_cast<unsigned int>(Buffer.size()) ) )
            return *reinterpret_cast<UINT *>(&Buffer.front());
        return CP_ACP;
    }

    /************************************************************
    ********    DefaultUserCodePage
    ************************************************************/
    class DefaultUserCodePage
    {
    private:
        static UINT InitValue()
        {
            DWORD   version = GetVersion();

            // High bit is set for Win95/98/ME
            if ( (version & 0x80000000) != 0x80000000 )
            {
                if ( LOBYTE( version ) > 4 )
                    return CP_THREAD_ACP;                       // 3 - Use CP_THREAD_ACP with Win2K/XP
                else
                    return LCIDToCodePage( GetThreadLocale() ); // Use thread's current locale with NT4
            }
            else
                return LCIDToCodePage( GetThreadLocale() );     // Convert thread's current locale with Win95/98/ME
        }

        DefaultUserCodePage();
    public:
        static UINT Value()
        {
            static UINT the_value = InitValue();

            return the_value;
        }
    };

    int CharFromWChar( const std::wstring& str, std::string& result_str,
                       char *buffer, std::size_t buffer_chars )
    {
        BOOL            used_default_char = FALSE;
        unsigned int    buffer_size = size_cast<unsigned int>(buffer_chars);
        int             result = WideCharToMultiByte( DefaultUserCodePage::Value(), 0,
                                                      str.c_str(), size_cast<unsigned int>(str.length()),
                                                      buffer, buffer_size, 0, &used_default_char );

        //DUMPMSG1_IF( (result == 0), "CharFromWChar failed: %1%", GetLastError() );
        //DUMPMSG_IF( (used_default_char), "CharFromWChar failed on some characters" );
        if ( result > 0 && buffer_size > 0 )
            result_str = std::string( buffer, buffer + result );
        return result;
    }

    int WCharFromChar( const std::string& str, std::wstring& result_str,
                       wchar_t *buffer, std::size_t buffer_chars )
    {
        int     result = MultiByteToWideChar( DefaultUserCodePage::Value(), 0,
                                              str.c_str(), size_cast<unsigned int>(str.length()),
                                              buffer, size_cast<unsigned int>(buffer_chars) );

        if ( result >= 0 )
            result_str = std::wstring( buffer, buffer + result );
        return result;
    }

    std::string NarrowStringStrict( const std::wstring& str )
    {
        std::string     result;

        if ( str.empty() )
            return result;

        cclib::array<char, 4096>    buffer;
        std::size_t                 hype_len = str.length() * 3 + 1;        // overallocate

        if ( hype_len < buffer.size() )
        {
            int     dest_len = CharFromWChar( str, result, buffer.data(), buffer.size() );

            if ( dest_len >= 0 )
                return std::string( buffer.data(), buffer.data() + dest_len );
        }

        boost::scoped_array<char>   bbuff( new char[hype_len] );
        int                         dest_len = CharFromWChar( str, result, bbuff.get(), hype_len );

        if ( dest_len >= 0 )
            return std::string( bbuff.get(), bbuff.get() + dest_len );
        return std::string();
    }
    
    std::wstring WidenStringStrict( const std::string& str )
    {
        std::wstring     result;

        if ( str.empty() )
            return result;

        cclib::array<wchar_t, 2048>     buffer;
        std::size_t                     hype_len = str.length() * 2 + 1;    // overallocate

        if ( hype_len < buffer.size() )
        {
            int     dest_len = WCharFromChar( str, result, buffer.data(), buffer.size() );

            if ( dest_len >= 0 )
                return std::wstring( buffer.data(), buffer.data() + dest_len );
        }

        boost::scoped_array<wchar_t>    bbuff( new wchar_t[hype_len] );
        int                             dest_len = WCharFromChar( str, result, bbuff.get(), hype_len );

        if ( dest_len >= 0 )
            return std::wstring( bbuff.get(), bbuff.get() + dest_len );
        return std::wstring();
    }

}