# BarcodeRG backend (Increment 3)

## Data files (all in `data/`)
One record per line, fields separated by a single space, line ends with `#`.
A space inside a value is written as `_`; an empty value is written as `-`.
Statuses are `in-progress`, `on-hold`, `paid`, `archived`.

| File | Fields |
|---|---|
| PCategory.txt | categoryId name acronym |
| Product.txt | id barcode acronym name unit price tax discount quantity barcodeFile categoryId |
| Seasons.txt | startDate endDate percent isActive |
| Order.txt | orderId date time customer contact status amount paymentMode merchant |
| OrderDetail.txt | orderId productId barcode quantity (one line per item) |
| Bill.txt | billId customer orderId withoutTax tax discount payable received balance status seasonalDiscount specialDiscount date time |
| BillDetail.txt | billId acronym unitPrice quantity discountOnLine (one line per product) |
| Approval.txt | orderId requestDate requestTime status decisionDate decisionTime |

Bills are written to `data/bills/BILLID.pdf`. Generated at run time (keep out of git):
Order.txt, OrderDetail.txt, Bill.txt, BillDetail.txt, Approval.txt, data/bills/.
Delete old-layout copies of Bill.txt, BillDetail.txt and OrderDetail.txt before the first run.

## Layers
Models -> storage (one repository per file, `Data_Files` is the only code that knows the line format)
-> services -> GUI. The GUI talks to `AppServices` only. Qt is never used below the GUI.

## Rules worth knowing
* Edit window: a paid order can be edited up to 7 days after its order date (day 7 counts) and only
  after a manager approves the request. Archived orders are never editable. On-hold orders are free.
* Discounts: base = without tax + tax - product discounts; seasonal and special are percentages of
  base; special is at most 8 percent; overlapping seasons use the highest active one.
* Checkout order: validate -> change stock (undone if anything fails) -> save order -> make bill.

## Tests
`Testing/test_support.h` holds the in-memory fakes and fake clock, `Testing/Test.cpp` is the runner,
`Testing/*_tests.cpp` hold the tests. Each member adds `Testing/AREA_tests.cpp` with `TEST(name) { CHECK(...); }`.
Build (no Qt, no OpenCV needed):

    g++ -std=c++17 -I. *.cpp Testing/*.cpp -o tests     # leave out AppServices.cpp, barcode_assign.cpp and the GUI files
    ./tests
