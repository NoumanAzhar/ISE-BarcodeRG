#include "order.h"

#include <chrono>
#include <cstdio>
#include <ctime>
using namespace std;

namespace pos {
	string toString(OrderStatus status) {
		switch (status) {
		case OrderStatus::InProgress:
			return "in_progress";
		case OrderStatus::OnHold:
			return "on_hold";
		case OrderStatus::Paid:
			return "paid";
		case OrderStatus::Archived:
			return "archived";
		}
		return "in_progress";
	}

	optional <OrderStatus> parseOrderStatus(const string& text) {
		if (text == "in_progress")
			return OrderStatus::InProgress;
		else if (text == "on_hold")
			return OrderStatus::OnHold;
		else if (text == "paid")
			return OrderStatus::Paid;
		else if (text == "archived")
			return OrderStatus::Archived;
		else
			return nullopt;
	}

	string toString(PaymentMode mode) {
		switch (mode) {
		case PaymentMode::None:
			return "none";
		case PaymentMode::Cash:
			return "cash";
		case PaymentMode::Card:
			return "card";
		case PaymentMode::Wallet:
			return "wallet";
		}
		return "none";
	}

	optional <PaymentMode> parsePaymentMode(const string& text) {
		if (text == "none")
			return PaymentMode::None;
		else if (text == "cash")
			return PaymentMode::Cash;
		else if (text == "card")
			return PaymentMode::Card;
		else if (text == "wallet")
			return PaymentMode::Wallet;
		else
			return nullopt;
	}

	string makeOrderId(int sequence, int dayOfMonth)
	{
		char buffer[16];
		snprintf(buffer, sizeof(buffer), "%06d%02d", sequence, dayOfMonth);
		return buffer;
	}

	namespace {
		tm localNow()
		{
			time_t now = chrono::system_clock::to_time_t(chrono::system_clock::now());
			tm result{};
		#ifdef _WIN32
			localtime_s(&result, &now);
		#else
			localtime_r(&now, &result);
		#endif
			return result;
		}

		string formatNow(const char* pattern)
		{
			tm t = localNow();
			char buffer[32];
			strftime(buffer, sizeof(buffer), pattern, &t);
			return buffer;
		}
	}
	string SystemClock::date() const
	{
		return formatNow("%Y-%m-%d");
	}
	string SystemClock::time() const
	{
		return formatNow("%H:%M:%S");
	}
	int SystemClock::dayOfMonth() const
	{
		return localNow().tm_mday;
	}
}