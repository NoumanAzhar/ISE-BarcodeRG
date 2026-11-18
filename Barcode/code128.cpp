#include "code128.h"
#include <iostream>
using namespace std;

// helpers
namespace
{

    const int StartC = 105;
    const int Stop = 106;
    const int SymbolCount = 107;

    const char* const Widths [SymbolCount] = {
        "212222", "222122", "222221", "121223", "121322", "131222", "122213", "122312",  // 0-7
        "132212", "221213", "221312", "231212", "112232", "122132", "122231", "113222",  // 8-15
        "123122", "123221", "223211", "221132", "221231", "213212", "223112", "312131",  // 16-23
        "311222", "321122", "321221", "312212", "322112", "322211", "212123", "212321",  // 24-31
        "232121", "111323", "131123", "131321", "112313", "132113", "132311", "211313",  // 32-39
        "231113", "231311", "112133", "112331", "132131", "113123", "113321", "133121",  // 40-47
        "313121", "211331", "231131", "213113", "213311", "213131", "311123", "311321",  // 48-55
        "331121", "312113", "312311", "332111", "314111", "221411", "431111", "111224",  // 56-63
        "111422", "121124", "121421", "141122", "141221", "112214", "112412", "122114",  // 64-71
        "122411", "142112", "142211", "241211", "221114", "413111", "241112", "134111",  // 72-79
        "111242", "121142", "121241", "114212", "124112", "124211", "411212", "421112",  // 80-87
        "421211", "212141", "214121", "412121", "111143", "111341", "131141", "114113",  // 88-95
        "114311", "411113", "411311", "113141", "114131", "311141", "411131", "211412",  // 96-103
        "211214", "211232",                                                              // 104-105 (Start B, Start C)
        "2331112"                                                                        // 106 (Stop)
    };

    string Patterns [SymbolCount];
    bool Built = false;

    string widthsToModules(const char* widths)
    {
        string out;
        bool bar = true;
        for ( const char* p = widths; *p != '\0'; ++p ) 
        {
            int count = *p - '0';
            for ( int i = 0; i < count; ++i )
                out += bar ? '1' : '0';
            bar = !bar;
        }
        return out;
    }

    void buildPatterns()
    {
        if ( Built )
            return;
        for ( int i = 0; i < SymbolCount; ++i )
            Patterns [i] = widthsToModules(Widths [i]);
        Built = true;
    }

} // namespace

bool validate6Digits(const string& s)
{
    if ( s.size() != 6 )
        return false;
    for ( size_t i = 0; i < s.size(); ++i )
        if ( s [i] < '0' || s [i] > '9' )
            return false;
    return s [0] != '0';
}

string encodeCode128C(const string& digits)
{
    if ( !validate6Digits(digits) ) 
    {
        cerr << "Invalid input " << digits << endl;
        cerr << " (need 6 digits, first digit non-zero)" << endl;
        return string();
    }

    buildPatterns();

    int values [3];
    for ( int i = 0; i < 3; ++i )
        values [i] = ( digits [2 * i] - '0' ) * 10 + ( digits [2 * i + 1] - '0' );

    // Checksum = (Start value + sum of position * value) mod 103.
    int checksum = StartC;
    for ( int i = 0; i < 3; ++i )
        checksum += values [i] * ( i + 1 );
    checksum %= 103;

    string modules = Patterns [StartC];
    for ( int i = 0; i < 3; ++i )
        modules += Patterns [values [i]];
    modules += Patterns [checksum];
    modules += Patterns [Stop];

	return modules;          // no quiet zone ( quiet zone added by renderer )
}

int patternToSymbol(const string& p11)
{
    buildPatterns();

    for ( int i = 0; i < SymbolCount; ++i )
        if ( Patterns [i] == p11 )
            return i;
    return -1;
}
