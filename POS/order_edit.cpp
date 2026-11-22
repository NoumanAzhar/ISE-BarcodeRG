#include "order_edit.h"
#include "Data_Files.h"

#include <string>
using namespace std;

namespace pos {
	namespace {

		bool isLeap(int y)
		{
			return (y % 4 == 0 && y % 100 != 0) || (y % 400 == 0);
		}

		int daysInMonth(int y, int m)
		{
			static const int days[] = { 31, 28, 31, 30, 31,30,31,31,30,31,30,31 };
			return (m == 2 && isLeap(y)) ? 29 : days[m - 1];
		}

		long long daysFromCivil(int y, int m, int d)
		{
			y -= m <= 2;
			const long long era = (y >= 0 ? y : y - 399) / 400;
			const long long yoe = y - era * 400;
			const long long doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
			const long long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
			return era * 146097 + doe - 719468;
		}

		bool parseDate(const string& text, int& y, int& m, int& d)
		{
			if (text.size() != 10 || text[4] != '-' || text[7] != '-') return false;
			for (size_t i = 0; i < text.size(); i++)
			{
				if (i == 4 || i == 7) continue;
				if (text[i] < '0' || text[i] > '9') return false;
			}
			y = stoi(text.substr(0, 4));
			m = stoi(text.substr(5, 2));
			d = stoi(text.substr(8, 2));
			return m >= 1 && m <= 12 && d >= 1 && d <= daysInMonth(y, m);

		}
	}

	int daysBetween(const string& fromDate, const string& toDate)
	{
		int y1, m1, d1, y2, m2, d2;
		if (!parseDate(fromDate, y1, m1, d1) || !parseDate(toDate, y2, m2, d2))
			return kInvalidDays;
		return static_cast<int>(daysFromCivil(y2, m2, d2) - daysFromCivil(y1, m1, d1));
	}

	OrderEditPolicy::OrderEditPolicy(const IClock& clock) : clock_(clock)
	{
	}

	EditDecision OrderEditPolicy::check(const Order& order, const string& paidDate) const
	{
		switch (order.status) {
		case OrderStatus::InProgress:
		case OrderStatus::OnHold:
			return { true, false, "" };
		case OrderStatus::Archived:
			return { false, false, "Archived orders cannot be edited." };
		case OrderStatus::Paid:
			break;
		}

		const string& since = paidDate.empty() ? order.date : paidDate;
		const int days = daysBetween(since, clock_.date());
		if (days == kInvalidDays || days < 0)
			return { false, false, "The payment date is not valid." };
		if (days > kPaidEditWindowDays)
			return { false, false, "The edit window for a paid order is over." };
		return { true, true, "" };
	}
}