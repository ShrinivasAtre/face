#include "DmsPolicy.hpp"

#include <chrono>
#include <iostream>

int main()
{
    using namespace std::chrono_literals;
    const auto p = dms::OperationalPolicyProfile::stage20Approved();
    std::string error;
    if (!p.validate(error) || std::string(p.name) != "stage20-approved-2026-08-28" ||
        p.eye.maximumBlinkClosure != 700ms || p.eye.maximumLongBlinkClosure != 1500ms ||
        p.eye.prolongedClosure != 1500ms || p.eye.blinkRefractory != 150ms ||
        p.eye.perclosWindow != 60s || p.eye.minimumPerclosCoverage != 0.80F ||
        p.yawn.minimumDuration != 800ms || p.distraction.distractionDuration != 2s ||
        p.presence.absentConfirmation != 1s || p.availability.recordAfter != 500ms ||
        p.availability.notifyAfter != 2s || p.neutralCalibration != 2s ||
        p.drowsiness.minimumAlertHold != 5s || p.drowsiness.recoveryDuration != 10s)
    {
        std::cerr << "FAIL: approved policy mismatch: " << error << '\n';
        return 1;
    }
    std::cout << "DMS approved policy test PASSED\n";
    return 0;
}
