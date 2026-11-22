#include <iostream>
#include <iomanip>
#include <string>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <variant>
#include <vector>
using namespace std;

// ============================================================================
// EXCEPTION HIERARCHY (Used in B2 & B4)
// ============================================================================
class BillingError : public runtime_error {
public:
    BillingError(string msg) : runtime_error(msg) {}
};

class ClaimError : public runtime_error {
public:
    ClaimError(string msg) : runtime_error(msg) {}
};

class UnknownPackageError : public BillingError {
public:
    UnknownPackageError(string msg) : BillingError(msg) {}
};

// ============================================================================
// TASK B1: POLYMORPHIC BILL & PAYER HIERARCHY
// ============================================================================
class Bill {
protected:
    int billId;
    string patientName;

public:
    Bill(int id, string name) : billId(id), patientName(name) {}
    virtual ~Bill() noexcept {}

    virtual double total() const = 0;
    virtual string type() const noexcept = 0;

    virtual void print() const {
        cout << "[" << billId << "] " << type()
             << " | Patient: " << patientName
             << " | Total: " << fixed << setprecision(2) << total() << endl;
    }
};

class OutPatientBill : public Bill {
    double consultation, tests;
public:
    OutPatientBill(int id, string name, double c, double t)
        : Bill(id, name), consultation(c), tests(t) {
        if (c < 0 || t < 0) throw BillingError("Negative OutPatient charge");
    }
    double total() const override { return consultation + tests; }
    string type() const noexcept override { return "OutPatient"; }
};

class InPatientBill : public Bill {
    int days;
    double roomPerDay, procedures, medicines;
public:
    InPatientBill(int id, string name, int d, double room, double proc, double med)
        : Bill(id, name), days(d), roomPerDay(room), procedures(proc), medicines(med) {
        if (d < 0 || room < 0 || proc < 0 || med < 0)
            throw BillingError("Negative InPatient charge");
    }
    double total() const override { return days * roomPerDay + procedures + medicines; }
    string type() const noexcept override { return "InPatient"; }
};

class DayCareBill : public Bill {
    string packageCode;
    double packagePrice;
public:
    DayCareBill(int id, string name, string code, double price)
        : Bill(id, name), packageCode(code), packagePrice(price) {
        if (price < 0) throw BillingError("Negative DayCare price");
    }
    double total() const override { return packagePrice; }
    string type() const noexcept override { return "DayCare"; }
};

class EmergencyBill : public Bill {
    double baseCharge, hourlyRate, hours;
public:
    EmergencyBill(int id, string name, double base, double rate, double hrs)
        : Bill(id, name), baseCharge(base), hourlyRate(rate), hours(hrs) {
        if (base < 0 || rate < 0 || hrs < 0)
            throw BillingError("Negative Emergency charge");
    }
    double total() const override { return baseCharge + hourlyRate * hours; }
    string type() const noexcept override { return "Emergency"; }
};

// Task B4.1 Extension: New Bill type added WITHOUT modifying existing classes
class TeleConsultBill : public Bill {
    double videoFee, ePrescriptionFee;
public:
    TeleConsultBill(int id, string name, double vFee, double rxFee)
        : Bill(id, name), videoFee(vFee), ePrescriptionFee(rxFee) {
        if (vFee < 0 || rxFee < 0) throw BillingError("Negative TeleConsult fee");
    }
    double total() const override { return videoFee + ePrescriptionFee; }
    string type() const noexcept override { return "TeleConsult"; }
};

// Payer Hierarchy
class Payer {
public:
    virtual ~Payer() noexcept {}
    virtual double amountCovered(const Bill& b) const = 0;
    virtual string payerName() const noexcept = 0;
};

class SelfPay : public Payer {
public:
    double amountCovered(const Bill& b) const override { (void)b; return 0.0; }
    string payerName() const noexcept override { return "SelfPay"; }
};

class Insurance : public Payer {
    double coverLimit, copayPercent;
public:
    Insurance(double limit, double copay) : coverLimit(limit), copayPercent(copay) {}
    double amountCovered(const Bill& b) const override {
        double eligible = min(b.total(), coverLimit);
        return eligible * (100.0 - copayPercent) / 100.0;
    }
    string payerName() const noexcept override { return "Insurance"; }
};

class GovernmentScheme : public Payer {
    double fixedRate;
public:
    GovernmentScheme(double rate) : fixedRate(rate) {}
    double amountCovered(const Bill& b) const override {
        return min(b.total(), fixedRate);
    }
    string payerName() const noexcept override { return "GovtScheme"; }
};

// Task B4.1 Extension: New Payer added WITHOUT modifying existing classes
class CorporatePayer : public Payer {
    string company;
    double cap, rate;
public:
    CorporatePayer(string c, double limit, double r) : company(c), cap(limit), rate(r) {}
    double amountCovered(const Bill& b) const override {
        return min(b.total(), cap) * rate;
    }
    string payerName() const noexcept override { return "Corporate(" + company + ")"; }
};

// ============================================================================
// TASK B2: ERROR ENGINEERING & RAII CLEANUP
// ============================================================================
class ScopeGuard {
    string name;
public:
    ScopeGuard(string n) : name(n) {}
    ~ScopeGuard() noexcept {
        cout << "  [Destroyed] " << name << endl;
    }
};

double lookupPackage(string code) {
    if (code == "CATARACT") return 18000.0;
    throw UnknownPackageError("Unknown package code: " + code);
}

double settleClaim(double billed, double cover) {
    if (billed < 0 || cover < 0)
        throw BillingError("Negative charge or cover");
    if (billed > cover)
        throw ClaimError("Claim " + to_string((int)billed) + " exceeds cover " + to_string((int)cover));
    return billed;
}

// ============================================================================
// TASK B3: VIRTUAL vs SWITCH vs VARIANT
// ============================================================================
enum class BillType { OP, IP, DC, ER };

struct SwitchBill {
    BillType type;
    double c = 0, t = 0, r = 0, p = 0, m = 0, pkg = 0, base = 0, rate = 0, hrs = 0;
    int d = 0;
    double total() const {
        switch (type) {
            case BillType::OP: return c + t;
            case BillType::IP: return d * r + p + m;
            case BillType::DC: return pkg;
            case BillType::ER: return base + rate * hrs;
        }
        return 0.0;
    }
};

struct VOP { double c, t; double total() const { return c + t; } };
struct VIP { int d; double r, p, m; double total() const { return d * r + p + m; } };
struct VDC { double pkg; double total() const { return pkg; } };
struct VER { double base, rate, hrs; double total() const { return base + rate * hrs; } };
using VariantBill = variant<VOP, VIP, VDC, VER>;

int main() {
    // --- B1: Validate Polymorphic Bills & Payers ---
    cout << "=== TASK B1: POLYMORPHIC BILLING VALIDATION ===" << endl;
    Bill* bills[4] = {
        new OutPatientBill(101, "Ravi", 500, 1200),
        new InPatientBill(102, "Meena", 3, 2500, 35000, 5000),
        new DayCareBill(103, "Arjun", "CATARACT", 18000),
        new EmergencyBill(104, "Priya", 2000, 750, 4)
    };
    SelfPay sp;
    Insurance ins(40000, 10);
    GovernmentScheme gov(15000);

    double grandTotal = 0;
    for (int i = 0; i < 4; i++) {
        bills[i]->print();
        grandTotal += bills[i]->total();
        cout << "   SelfPay: " << sp.amountCovered(*bills[i])
             << " | Insurance: " << ins.amountCovered(*bills[i])
             << " | Govt: " << gov.amountCovered(*bills[i]) << endl;
    }
    cout << "Grand Total Billed: " << grandTotal << "\n\n";
    for (int i = 0; i < 4; i++) delete bills[i];

    // --- B2: Engineer Error Conditions ---
    cout << "=== TASK B2: ERROR CONDITIONS & RAII UNWINDING ===" << endl;
    for (int t = 1; t <= 5; t++) {
        try {
            ScopeGuard g("Test_" + to_string(t));
            if (t == 1) settleClaim(-500, 50000);             // Negative charge
            if (t == 2) settleClaim(60000, 50000);            // Claim above cover
            if (t == 3) lookupPackage("INVALID_PKG");         // Unknown package
            if (t == 4) { double x = stod("ABC"); (void)x; }  // Invalid text -> invalid_argument
            if (t == 5) throw 999;                            // Unknown exception -> catch(...)
        } catch (const ClaimError& e) {
            cout << "Caught ClaimError: " << e.what() << endl;
        } catch (const BillingError& e) {
            cout << "Caught BillingError: " << e.what() << endl;
        } catch (const invalid_argument& e) {
            cout << "Caught std::invalid_argument: " << e.what() << endl;
        } catch (...) {
            cout << "Caught unknown exception via catch(...) — program stays alive!" << endl;
        }
    }

    // --- B3: Performance Comparison (1,000,000 bills) ---
    cout << "\n=== TASK B3: BENCHMARKS (1,000,000 BILLS) ===" << endl;
    const int N = 1000000;
    OutPatientBill op(1, "A", 500, 1200);
    vector<Bill*> polyVec(N, &op);
    vector<SwitchBill> swVec(N, SwitchBill{BillType::OP, 500, 1200, 0, 0, 0, 0, 0, 0, 0, 0});
    vector<VariantBill> varVec(N, VOP{500, 1200});

    auto t0 = chrono::high_resolution_clock::now();
    volatile double s1 = 0;
    for (int i = 0; i < N; i++) s1 += polyVec[i]->total();
    auto t1 = chrono::high_resolution_clock::now();
    volatile double s2 = 0;
    for (int i = 0; i < N; i++) s2 += swVec[i].total();
    auto t2 = chrono::high_resolution_clock::now();
    volatile double s3 = 0;
    for (int i = 0; i < N; i++) s3 += visit([](const auto& b){ return b.total(); }, varVec[i]);
    auto t3 = chrono::high_resolution_clock::now();
    (void)s1; (void)s2; (void)s3;

    cout << "Virtual: " << chrono::duration<double, milli>(t1 - t0).count() << " ms | "
         << "Switch: " << chrono::duration<double, milli>(t2 - t1).count() << " ms | "
         << "Variant: " << chrono::duration<double, milli>(t3 - t2).count() << " ms" << endl;

    // --- B4: Practical Extension ---
    cout << "\n=== TASK B4: NEW BILL & PAYER EXTENSION ===" << endl;
    Bill* tele = new TeleConsultBill(105, "Karthik", 600, 150);
    CorporatePayer corp("TCS Health", 25000, 0.85);
    tele->print();
    cout << "   " << corp.payerName() << " Covered: " << corp.amountCovered(*tele)
         << " | Patient Pays: " << (tele->total() - corp.amountCovered(*tele)) << endl;
    delete tele;

    return 0;
}
