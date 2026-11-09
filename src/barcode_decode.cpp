#include "barcode_decode.h"

static double estimateModuleWidth(const std::vector<int>& runs)
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
std::optional<std::string> decodeRuns(const std::vector<int>& runs)
{
	double moduleWidth = estimateModuleWidth(runs);
	if (moduleWidth <= 0.0) {
		return std::nullopt; // Invalid module width
	}
	return std::nullopt; // temperory
}