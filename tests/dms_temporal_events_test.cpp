#include "DmsTemporalEvents.hpp"

#include <chrono>
#include <iostream>

namespace
{
bool check(bool value, const char *text)
{
    if (!value)
        std::cerr << "FAIL: " << text << '\n';
    return value;
}
} // namespace

int main()
{
    using namespace std::chrono_literals;
    using namespace dms;
    YawnConfig yc;
    yc.confirmation = 100ms;
    yc.minimumDuration = 500ms;
    YawnFsm yawn(yc);
    yawn.update(0ms, ObservationUsability::Usable, 0.2F);
    yawn.update(100ms, ObservationUsability::Usable, 0.8F);
    if (!check(yawn.update(200ms, ObservationUsability::Usable, 0.8F).active, "yawn opens after confirmation"))
        return 1;
    yawn.update(800ms, ObservationUsability::Usable, 0.2F);
    auto yr = yawn.update(900ms, ObservationUsability::Usable, 0.2F);
    if (!check(yr.event && yr.count == 1, "qualified yawn counts once"))
        return 1;
    if (!check(!yawn.update(1000ms, ObservationUsability::Occluded, 0.8F).event, "occlusion cannot count yawn"))
        return 1;

    HeadPoseConfig hc;
    hc.confirmation = 100ms;
    HeadPoseFsm head(hc);
    head.update(0ms, ObservationUsability::Usable, 0.0F, 0.0F);
    head.update(100ms, ObservationUsability::Usable, 0.0F, 0.0F);
    head.update(200ms, ObservationUsability::Usable, -30.0F, 0.0F);
    auto hr = head.update(300ms, ObservationUsability::Usable, -30.0F, 0.0F);
    if (!check(hr.zone == HeadZone::Left && hr.movementEvent && hr.leftCount == 1, "left movement counted"))
        return 1;
    if (!check(!head.update(500ms, ObservationUsability::Usable, 30.0F, 0.0F).movementEvent,
               "direction change requires neutral rearm"))
        return 1;
    head.update(600ms, ObservationUsability::Usable, 0.0F, 0.0F);
    head.update(700ms, ObservationUsability::Usable, 0.0F, 0.0F);
    head.update(800ms, ObservationUsability::Usable, 0.0F, 25.0F);
    hr = head.update(900ms, ObservationUsability::Usable, 0.0F, 25.0F);
    if (!check(hr.downCount == 1, "vertical movement counted after neutral"))
        return 1;

    DistractionConfig dc;
    dc.gazeConfirmation = 100ms;
    dc.distractionDuration = 500ms;
    dc.recoveryDuration = 200ms;
    DistractionFsm distraction(dc);
    distraction.update(0ms, ObservationUsability::Usable, 0.0F, 0.0F, HeadZone::Neutral);
    distraction.update(100ms, ObservationUsability::Usable, 0.0F, 0.0F, HeadZone::Neutral);
    distraction.update(200ms, ObservationUsability::Usable, 0.8F, 0.0F, HeadZone::Neutral);
    distraction.update(300ms, ObservationUsability::Usable, 0.8F, 0.0F, HeadZone::Neutral);
    auto dr = distraction.update(800ms, ObservationUsability::Usable, 0.8F, 0.0F, HeadZone::Neutral);
    if (!check(dr.distracted && dr.event, "sustained gaze creates distraction event"))
        return 1;
    distraction.update(900ms, ObservationUsability::Usable, 0.0F, 0.0F, HeadZone::Neutral);
    distraction.update(1000ms, ObservationUsability::Usable, 0.0F, 0.0F, HeadZone::Neutral);
    dr = distraction.update(1200ms, ObservationUsability::Usable, 0.0F, 0.0F, HeadZone::Neutral);
    if (!check(!dr.distracted, "forward recovery clears distraction"))
        return 1;

    PresenceConfig pc;
    pc.presentConfirmation = 100ms;
    pc.absentConfirmation = 300ms;
    DriverPresenceFsm presence(pc);
    presence.update(0ms, ObservationUsability::Usable, true);
    if (!check(presence.update(100ms, ObservationUsability::Usable, true) == PresenceState::Present,
               "presence confirms"))
        return 1;
    presence.update(200ms, ObservationUsability::Missing, false);
    if (!check(presence.update(500ms, ObservationUsability::Missing, false) == PresenceState::Absent,
               "absence confirms"))
        return 1;

    MonitoringAvailabilityConfig ac;
    ac.recordAfter = 500ms;
    ac.notifyAfter = 2s;
    MonitoringAvailabilityFsm availability(ac);
    availability.update(0ms, ObservationUsability::Usable);
    availability.update(100ms, ObservationUsability::Occluded);
    auto ar = availability.update(600ms, ObservationUsability::Occluded);
    if (!check(ar.recordEvent && ar.episodeCount == 1 && !ar.notify, "unavailable episode recorded")) return 1;
    ar = availability.update(2100ms, ObservationUsability::Occluded);
    if (!check(ar.notifyEvent && ar.notify, "unavailable notification delayed")) return 1;
    ar = availability.update(2200ms, ObservationUsability::Usable);
    if (!check(!ar.unavailable && !ar.notify, "availability recovery clears notification")) return 1;

    DrowsinessConfig dcfg;
    dcfg.recoveryDuration = 500ms;
    dcfg.minimumAlertHold = 0ms;
    DrowsinessFsm drowsy(dcfg);
    DrowsinessInput di{0ms, ObservationUsability::Usable, PresenceState::Present, 0.10F, false, false, false};
    if (!check(drowsy.update(di).state == DrowsinessState::Normal, "normal evidence"))
        return 1;
    di.timestamp = 100ms;
    di.perclos = 0.25F;
    if (!check(drowsy.update(di).state == DrowsinessState::Warning, "PERCLOS warning"))
        return 1;
    di.timestamp = 200ms;
    di.perclos = 0.10F;
    di.prolongedClosure = true;
    if (!check(drowsy.update(di).state == DrowsinessState::Drowsy, "closure is drowsy"))
        return 1;
    di.timestamp = 300ms;
    di.prolongedClosure = false;
    drowsy.update(di);
    di.timestamp = 800ms;
    if (!check(drowsy.update(di).state == DrowsinessState::Normal, "drowsiness recovery debounce"))
        return 1;
    di.timestamp = 900ms;
    di.usability = ObservationUsability::Occluded;
    if (!check(drowsy.update(di).state == DrowsinessState::Unknown, "unknown quality suppresses drowsiness"))
        return 1;
    std::cout << "DMS temporal event tests PASSED\n";
    return 0;
}
