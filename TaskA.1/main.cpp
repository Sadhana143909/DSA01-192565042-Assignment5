#include <iostream>
#include <string>
#include <stdexcept>
#include <algorithm>
using namespace std;

// ============================================================================
// TASK A4: EXCEPTION HIERARCHY & noexcept SPECIFICATIONS
// ============================================================================

// Base exception class derived from std::runtime_error (which inherits std::exception)
class HospitalError : public runtime_error {
public:
    HospitalError(string msg) : runtime_error(msg) {}
    virtual ~HospitalError() noexcept {}
};

// Specific domain exceptions
class BillingError : public HospitalError {
public:
    BillingError(string msg) : HospitalError(msg) {}
};

class ClaimError : public HospitalError {
public:
    ClaimError(string msg) : HospitalError(msg) {}
};

class UnknownPackageError : public BillingError {
public:
    UnknownPackageError(string msg) : BillingError(msg) {}
};

class PatientRecordError : public HospitalError {
public:
    PatientRecordError(string msg) : HospitalError(msg) {}
};

// ============================================================================
// TASK A1 & A2: ABSTRACT Bill BASE CLASS & DERIVED BILL TYPES
// ============================================================================

class Bill {
protected:
    int billId;
    string patientName;

public:
    Bill(int id, string name) : billId(id), patientName(name) {
        if (patientName == "") {
            throw PatientRecordError("Patient name cannot be empty");
        }
    }

    // Pure virtual functions (Task A1.2) -> enable runtime binding (Task A2.1)
    virtual double total() const = 0;
    virtual string type() const noexcept = 0;

    // Non-virtual getters marked noexcept (Task A4.2)
    int getId() const noexcept { return billId; }
    string getPatient() const noexcept { return patientName; }

    // Virtual print function calls virtual type() and total() at runtime
    virtual void print() const {
        cout << "[" << billId << "] " << type()
             << " | Patient: " << patientName
             << " | Bill Amount: " << total() << endl;
    }

    // Virtual destructor (Task A2.3) -> must be noexcept (Task A4.2)
    virtual ~Bill() noexcept {}
};

// 1. Out-Patient Bill: consultation + tests
class OutPatientBill : public Bill {
    double consultation;
    double tests;

public:
    OutPatientBill(int id, string name, double c, double t)
        : Bill(id, name), consultation(c), tests(t) {
        if (consultation < 0 || tests < 0) {
            throw BillingError("Negative charges not allowed in OutPatientBill");
        }
    }

    double total() const override {
        return consultation + tests;
    }

    string type() const noexcept override {
        return "OutPatient";
    }
};

// 2. In-Patient Bill: days * roomPerDay + procedures + medicines
class InPatientBill : public Bill {
    int days;
    double roomPerDay;
    double procedures;
    double medicines;

public:
    InPatientBill(int id, string name, int d, double room, double proc, double med)
        : Bill(id, name), days(d), roomPerDay(room), procedures(proc), medicines(med) {
        if (days < 0 || roomPerDay < 0 || procedures < 0 || medicines < 0) {
            throw BillingError("Negative values not allowed in InPatientBill");
        }
    }

    double total() const override {
        return (days * roomPerDay) + procedures + medicines;
    }

    string type() const noexcept override {
        return "InPatient";
    }
};

// 3. Day-Care Surgery Bill: package price lookup
class DayCareBill : public Bill {
    string packageCode;
    double packagePrice;

public:
    DayCareBill(int id, string name, string code)
        : Bill(id, name), packageCode(code) {
        if (packageCode == "CATARACT") {
            packagePrice = 18000;
        } else if (packageCode == "ENDOSCOPY") {
            packagePrice = 12000;
        } else {
            throw UnknownPackageError("Unknown surgery package: " + packageCode);
        }
    }

    double total() const override {
        return packagePrice;
    }

    string type() const noexcept override {
        return "DayCare";
    }
};

// 4. Emergency Bill: baseCharge + hourlyRate * hours
class EmergencyBill : public Bill {
    double baseCharge;
    double hourlyRate;
    double hours;

public:
    EmergencyBill(int id, string name, double base, double rate, double hrs)
        : Bill(id, name), baseCharge(base), hourlyRate(rate), hours(hrs) {
        if (baseCharge < 0 || hourlyRate < 0 || hours < 0) {
            throw BillingError("Negative values not allowed in EmergencyBill");
        }
    }

    double total() const override {
        return baseCharge + (hourlyRate * hours);
    }

    string type() const noexcept override {
        return "Emergency";
    }
};

// ============================================================================
// TASK A3: ABSTRACT Payer CLASS, DERIVED PAYERS & CORPORATE EXTENSION
// ============================================================================

class Payer {
public:
    virtual double amountCovered(const Bill& b) const = 0;
    virtual string payerName() const noexcept = 0;
    virtual ~Payer() noexcept {}
};

// 1. SelfPay: patient pays full bill
class SelfPay : public Payer {
public:
    double amountCovered(const Bill& b) const override {
        (void)b; // unused parameter
        return 0.0;
    }

    string payerName() const noexcept override {
        return "SelfPay";
    }
};

// 2. Insurance: cover limit + co-payment percentage
class Insurance : public Payer {
    double coverLimit;
    double copayPercent;

public:
    Insurance(double limit, double copay)
        : coverLimit(limit), copayPercent(copay) {
        if (coverLimit < 0 || copayPercent < 0 || copayPercent > 100) {
            throw ClaimError("Invalid insurance policy limit or co-pay");
        }
    }

    double amountCovered(const Bill& b) const override {
        double eligible = min(b.total(), coverLimit);
        return eligible * (100.0 - copayPercent) / 100.0;
    }

    string payerName() const noexcept override {
        return "Insurance";
    }
};

// 3. GovernmentScheme: fixed package cap
class GovernmentScheme : public Payer {
    double fixedRate;

public:
    GovernmentScheme(double rate) : fixedRate(rate) {
        if (fixedRate < 0) {
            throw ClaimError("Government scheme rate cannot be negative");
        }
    }

    double amountCovered(const Bill& b) const override {
        return min(b.total(), fixedRate);
    }

    string payerName() const noexcept override {
        return "GovtScheme";
    }
};

// Task A3.2 Extension: New CorporatePayer added WITHOUT changing existing classes
class CorporatePayer : public Payer {
    string companyName;
    double corporateLimit;

public:
    CorporatePayer(string comp, double limit)
        : companyName(comp), corporateLimit(limit) {}

    double amountCovered(const Bill& b) const override {
        // Covers 80% up to corporateLimit
        double eligible = min(b.total(), corporateLimit);
        return eligible * 0.80;
    }

    string payerName() const noexcept override {
        return "Corporate(" + companyName + ")";
    }
};

// ============================================================================
// TASK A5: RAII GUARD, CATCH-LOG-RETHROW (throw;), AND SAFE RECOVERY
// ============================================================================

// Simple RAII class to prove destructors run during stack unwinding (Task A5.2)
class SessionGuard {
    string label;
public:
    SessionGuard(string lbl) : label(lbl) {
        cout << "  [Open]  " << label << endl;
    }
    ~SessionGuard() noexcept {
        cout << "  [Close] " << label << " (RAII cleanup)" << endl;
    }
};

// Settle a claim: checks policy limit, logs ClaimError, and rethrows with throw;
double settleClaim(const Bill& b, const Payer& p, double maxClaimAllowed) {
    SessionGuard guard("ClaimCheck_Bill_" + to_string(b.getId()));

    try {
        double covered = p.amountCovered(b);
        if (covered > maxClaimAllowed) {
            throw ClaimError("Covered amount " + to_string(covered) +
                             " exceeds allowed claim cap " + to_string(maxClaimAllowed));
        }
        return covered;
    } catch (const ClaimError& e) {
        // Layer 2: Log the audit message and rethrow the EXACT same exception
        cout << "  [AUDIT LOG] Claim flagged: " << e.what() << endl;
        throw; // Bare rethrow (Task A5.1)
    }
}

int main() {
    // --- PART 1: Valid Polymorphic Bills & Payers (Tasks A1, A2, A3) ---
    Bill* bills[4];
    bills[0] = new OutPatientBill(101, "Ravi", 500, 1200);
    bills[1] = new InPatientBill(102, "Meena", 3, 2500, 35000, 5000);
    bills[2] = new DayCareBill(103, "Arjun", "CATARACT");
    bills[3] = new EmergencyBill(104, "Priya", 2000, 750, 4);

    SelfPay selfPay;
    Insurance insurance(40000, 10);       // 40,000 limit, 10% co-pay
    GovernmentScheme govt(15000);         // 15,000 fixed rate
    CorporatePayer corp("TCS Health", 25000); // Extension payer

    Payer* payers[4] = {&selfPay, &insurance, &govt, &corp};

    double dailyTotal = 0;
    cout << "=== DAILY HOSPITAL BILLING & CLAIMS ===" << endl;
    for (int i = 0; i < 4; i++) {
        bills[i]->print();                // Runtime polymorphism (Task A2)
        dailyTotal += bills[i]->total();

        // Settle each bill with its assigned payer inside try-catch recovery
        try {
            double covered = settleClaim(*bills[i], *payers[i], 30000);
            double patientPay = bills[i]->total() - covered;
            cout << "  -> Payer: " << payers[i]->payerName()
                 << " | Covered: " << covered
                 << " | Patient Pays: " << patientPay << endl;
        } catch (const ClaimError& e) {
            cout << "  -> [RECOVERED] Claim rejected: " << e.what() << endl;
        }
    }
    cout << "Day's Grand Total Billed: " << dailyTotal << "\n\n";

    // Clean up heap bills safely using virtual destructor (Task A2.3)
    for (int i = 0; i < 4; i++) {
        delete bills[i];
    }

    // --- PART 2: Error Handling & Stack Unwinding Tests (Tasks A4 & A5) ---
    cout << "=== ERROR DETECTION & RECOVERY TESTS ===" << endl;
    for (int test = 1; test <= 3; test++) {
        try {
            SessionGuard outer("ErrorTest_" + to_string(test));
            if (test == 1) {
                OutPatientBill badBill(201, "Kiran", -300, 500); // Negative charge
            } else if (test == 2) {
                DayCareBill badPkg(202, "Sneha", "LASIK_999");   // Unknown package
            } else if (test == 3) {
                OutPatientBill noName(203, "", 400, 200);        // Missing patient
            }
        } catch (const UnknownPackageError& e) {
            cout << "Caught UnknownPackageError: " << e.what() << endl;
        } catch (const BillingError& e) {
            cout << "Caught BillingError: " << e.what() << endl;
        } catch (const HospitalError&
