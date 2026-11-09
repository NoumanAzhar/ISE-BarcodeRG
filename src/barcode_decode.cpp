#include "barcode_decode.h"
#include <cmath>
#include "code128.h"
using namespace std;

static double estimateModuleWidth(const vector<int>& runs)
{
    if (runs.size() < 6) {
        return -1.0; // Invalid number of runs
	}
	double width = (runs[0] + runs[1] + runs[2] + runs[3] + runs[4] + runs[5]) / 11.0;
    if (width <= 0.0) {
        return -1.0; // Invalid module width
	}
	return width;
}

static string runsToPattern(const vector<int>& runs, size_t start, double moduleWidth)
{
    string pattern;
    
    if (start + 6 > runs.size())
    {
		return ""; // Return empty string to avoid out-of-bounds access
    }
    
    for (int i = 0; i < 6; i++)
    {
        int n = (int)lround(runs[start + i] / moduleWidth);
        char c;
        if (i % 2 == 0)
        {
            c = '1'; // bar
        }
        else
        {
            c = '0'; // space
        }
        pattern.append(n, c);
    }
    return pattern;
}

optional<string> decodeRuns(const vector<int>& runs)
{
	double moduleWidth = estimateModuleWidth(runs);
	if (moduleWidth <= 0.0) {
		return nullopt; // Invalid module width
	} 
    if (runs.size() < 37)
    {
        return nullopt;
    }
    vector<int> symbols;

    for (int s = 0; s < 5; s++)
    {
        string pattern = runsToPattern(runs, s * 6, moduleWidth);
        int value = patternToSymbol(pattern);
        if (value < 0)
        {
            return nullopt;
        }
        symbols.push_back(value);
    }
    if (symbols[0] != 105) // start C should always be 105
    {
		return nullopt;
    }
	// calculate checksum
    int sum = symbols[0] + 1 * symbols[1] + 2 * symbols[2] + 3 * symbols[3];
	int checksum = sum % 103;
	if (checksum != symbols[4]) // symbols[4] stores the check digit
    {
		return nullopt;
    }
	return nullopt; // temperory
}