// Prints PASS/FAIL per check and returns 0 only if every check passed.
#include <algorithm>
#include <cctype>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

#include "Data_Files.h"
#include "Product.h"
#include "Category.h"
#include "Inventory.h"
#include "season.h"
#include "order.h"
#include "order_service.h"
#include "OrderTotals.h"
#include "order_edit.h"
#include "Bill.h"
#include "BillStorage.h"
#include "BillGenerator.h"
#include "CheckoutService.h"
#include "access.h"

using namespace pos;
using std::optional;
using std::string;
using std::vector;

// ======================= in-memory fakes =======================

class FakeClock : public IClock {
public:
    string today = "2026-11-23";
    string now = "10:00:00";
    int day = 23;
    string date() const override { return today; }
    string time() const override { return now; }
    int dayOfMonth() const override { return day; }
};

class MemProducts : public IProductRepository {
public:
    vector<Product> items;
    optional<Product> findByBarcode(const string& b) const override {
        for (const Product& p : items) if (p.barcode == b) return p;
        return std::nullopt;
    }
    optional<Product> findById(const string& id) const override {
        for (const Product& p : items) if (p.id == id) return p;
        return std::nullopt;
    }
    vector<Product> all() const override { return items; }
    bool update(const Product& p) override {
        for (Product& x : items) if (x.id == p.id) { x = p; return true; }
        return false;
    }
    bool add(const Product& p) override {
        Product copy = p;
        if (copy.id.empty()) copy.id = nextId();
        if (findById(copy.id)) return false;
        items.push_back(copy);
        return true;
    }
    optional<Product> findByAcronym(const string& a) const override {
        for (const Product& p : items) if (p.acronym == a) return p;
        return std::nullopt;
    }
    bool remove(const string& id) override {
        auto it = std::remove_if(items.begin(), items.end(), [&](const Product& p) { return p.id == id; });
        if (it == items.end()) return false;
        items.erase(it, items.end());
        return true;
    }
    string nextId() const override {
        string id = std::to_string(items.size() + 1);
        return string(4 - std::min<size_t>(4, id.size()), '0') + id;
    }
};

class MemCategories : public ICategoryRepository {
public:
    vector<Category> items;
    optional<Category> findById(const string& id) const override {
        for (const Category& c : items) if (c.id == id) return c;
        return std::nullopt;
    }
    vector<Category> all() const override { return items; }
    bool add(const Category& c) override { items.push_back(c); return true; }
    bool update(const Category& c) override {
        for (Category& x : items) if (x.id == c.id) { x = c; return true; }
        return false;
    }
};

class MemSeasons : public ISeasonRepository {
public:
    vector<Season> items;
    vector<Season> all() const override { return items; }
    bool add(const Season& s) override { items.push_back(s); return true; }
    bool setActive(int i, bool a) override {
        if (i < 0 || i >= (int)items.size()) return false;
        items[i].active = a;
        return true;
    }
};

class MemOrders : public IOrderRepository {
public:
    vector<Order> items;
    bool save(const Order& o) override {          // upsert
        for (Order& x : items) if (x.id == o.id) { x = o; return true; }
        items.push_back(o);
        return true;
    }
    optional<Order> findById(const string& id) const override {
        for (const Order& o : items) if (o.id == id) return o;
        return std::nullopt;
    }
    vector<Order> all() const override { return items; }
    int count() const override { return (int)items.size(); }
    bool containsProduct(const string& pid) const override {
        for (const Order& o : items)
            for (const OrderItem& i : o.items) if (i.productId == pid) return true;
        return false;
    }
};

class MemBills : public IBillRepository {
public:
    vector<Bill> items;
    bool save(const Bill& b) override {
        if (findById(b.getBillId())) return false;
        items.push_back(b);
        return true;
    }
    bool replace(const Bill& b) override {
        for (Bill& x : items) if (x.getBillId() == b.getBillId()) { x = b; return true; }
        return false;
    }
    optional<Bill> findById(const string& id) const override {
        for (const Bill& b : items) if (b.getBillId() == id) return b;
        return std::nullopt;
    }
    optional<Bill> findByOrderId(const string& oid) const override {
        for (const Bill& b : items) if (b.getOrderId() == oid) return b;
        return std::nullopt;
    }
    vector<Bill> all() const override { return items; }
    string nextId() const override {
        string id = std::to_string(items.size() + 1);
        return string(9 - id.size(), '0') + id;
    }
};

// ======================= tiny check helper =======================

static int g_checks = 0, g_failed = 0;

#define CHECK(cond) do { ++g_checks; \
    if (cond) std::cout << "  PASS  " << #cond << "\n"; \
    else { ++g_failed; std::cout << "  FAIL  " << #cond << "   (line " << __LINE__ << ")\n"; } \
} while (0)

// ======================= one shared test world =======================
// Product 0001: barcode 100001, price 100, tax 5, discount 10, stock 100 (count)
// Product 0002: barcode 100002, price  50, tax 0, discount  0, stock   3 (count)

struct World {
    FakeClock clock;
    MemProducts products;
    MemCategories categories;
    MemSeasons seasonRepo;
    MemOrders orders;
    MemBills bills;
    InventoryService inventory{ products, categories };
    SeasonService seasons{ seasonRepo };
    BillGenerator generator{ orders, products, bills };
    OrderEditPolicy policy{ clock };
    OrderEditService edit{ orders, bills, policy };
    CheckoutService checkout{ orders, products, inventory, seasons, edit, clock, generator };
    OrderService draft{ products };
    OrderHoldService hold{ draft, orders, clock };

    World() {
        products.items.push_back(Product("0001", "100001", "LAYS", "Lays", MeasureUnit::Count, 100, 5, 10, 100, ""));
        products.items.push_back(Product("0002", "100002", "MILK", "Milk", MeasureUnit::Count, 50, 0, 0, 3, ""));
        products.items[0].categoryId = "10";
        products.items[1].categoryId = "10";
        categories.items.push_back({ "10", "Snacks", "SNK" });
    }
};

// ======================= the tests =======================

static void testAccess() {
    std::cout << "[access]\n";
    CHECK(isAllowed(Role::Cashier, Action::RecordOrder));
    CHECK(isAllowed(Role::Cashier, Action::HoldOrder));
    CHECK(!isAllowed(Role::Cashier, Action::ApproveEdit));
    CHECK(!isAllowed(Role::Cashier, Action::ViewReports));
    CHECK(isAllowed(Role::Manager, Action::ApproveEdit));
    CHECK(isAllowed(Role::Manager, Action::ViewReports));
    CHECK(!isAllowed(Role::Manager, Action::RecordOrder));
    CHECK(isAllowed(Role::Cashier, Action::PrintBill));
    CHECK(isAllowed(Role::Manager, Action::PrintBill));
}

static void testTotals() {
    std::cout << "[order totals]\n";
    World w;
    w.draft.addByBarcode("100001", 2);
    OrderTotals t = computeTotals(w.draft.current(), w.products);
    CHECK(t.withoutTax == 200.0);
    CHECK(t.tax == 10.0);
    CHECK(t.discount == 20.0);
    CHECK(t.payable == 190.0);

    OrderTotals s = computeTotals(w.draft.current(), w.products, 10.0, 0.0);   // 10% season
    CHECK(s.seasonalDiscount == 19.0);
    CHECK(s.payable == 171.0);
}

static void testScanAndLoad() {
    std::cout << "[scan and load]\n";
    World w;
    CHECK(w.draft.addByBarcode("100001", 1).status == AddStatus::Added);
    CHECK(w.draft.addByBarcode("999999", 1).status == AddStatus::UnknownBarcode);
    CHECK(w.draft.addByBarcode("100002", 5).status == AddStatus::InsufficientStock);   // stock is 3

    Order archived;
    archived.id = "00000123";
    archived.status = OrderStatus::Archived;
    CHECK(!w.draft.load(archived));                      // archived can never be opened

    Order onHold = archived;
    onHold.status = OrderStatus::OnHold;
    CHECK(w.draft.load(onHold));
    CHECK(w.draft.current().id == "00000123");           // id is kept
}

static void testHoldAndResume() {
    std::cout << "[hold and resume]\n";
    World w;
    CHECK(!w.hold.hold().has_value());                   // empty draft cannot be held

    w.draft.addByBarcode("100001", 2);
    optional<string> id = w.hold.hold();
    CHECK(id.has_value());
    CHECK(id && *id == "00000123");                      // sequence 1 + day 23
    CHECK(w.draft.current().items.empty());              // draft cleared for the next customer

    optional<Order> saved = id ? w.orders.findById(*id) : std::nullopt;
    CHECK(saved && saved->status == OrderStatus::OnHold);
    CHECK(saved && saved->customerName == "Walk-in");    // no name needed
    CHECK(w.products.items[0].availableQty == 100);      // no stock reserved
    CHECK(w.hold.list().size() == 1);

    CHECK(id && w.hold.resume(*id));
    CHECK(w.draft.current().items.size() == 1);
    CHECK(!w.hold.resume("00009999"));                   // unknown id
}

static void testCheckoutResumedOrder() {
    std::cout << "[checkout of a resumed on-hold order]\n";
    World w;
    w.draft.addByBarcode("100001", 2);
    optional<string> id = w.hold.hold();
    w.hold.resume(*id);

    Payment pay;
    pay.mode = PaymentMode::Cash;                        // amountReceived 0 = exact amount
    CheckoutResult r = w.checkout.checkout(w.draft.current(), pay);
    CHECK(r.ok());
    CHECK(r.bill.has_value() && r.bill->getTotalBill() == 190.0);
    CHECK(w.products.items[0].availableQty == 98);       // stock reduced by 2
    CHECK(w.orders.findById(*id)->status == OrderStatus::Paid);
    CHECK(w.bills.items.size() == 1);
}

static void testCheckoutRules() {
    std::cout << "[checkout rules]\n";
    World w;
    w.draft.addByBarcode("100001", 1);

    Payment tooMuchSpecial;
    tooMuchSpecial.mode = PaymentMode::Cash;
    tooMuchSpecial.specialPercent = 9;                   // cap is 8
    CHECK(w.checkout.checkout(w.draft.current(), tooMuchSpecial).status == CheckoutStatus::InvalidSpecialDiscount);

    Payment tooLow;
    tooLow.mode = PaymentMode::Cash;
    tooLow.amountReceived = 1;                           // payable is 95
    CHECK(w.checkout.checkout(w.draft.current(), tooLow).status == CheckoutStatus::AmountTooLow);

    Payment ok;
    ok.mode = PaymentMode::Cash;
    CheckoutResult r = w.checkout.checkout(w.draft.current(), ok);
    CHECK(r.ok());

    // The same paid order, edited after the 7-day window -> refused
    w.clock.today = "2026-12-05";                        // 12 days later
    Order edited = *w.orders.findById(r.orderId);
    CHECK(w.checkout.checkout(edited, ok).status == CheckoutStatus::EditNotAllowed);
}

int main() {
    testAccess();
    testTotals();
    testScanAndLoad();
    testHoldAndResume();
    testCheckoutResumedOrder();
    testCheckoutRules();

    std::cout << "\n" << (g_checks - g_failed) << " / " << g_checks << " checks passed\n";
    return g_failed == 0 ? 0 : 1;
}
