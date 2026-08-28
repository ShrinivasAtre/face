#include "DmsTemporalEvents.hpp"

#include <algorithm>
#include <cmath>

namespace dms
{
namespace
{
bool finite01(float value) noexcept
{
    return std::isfinite(value) && value >= 0.0F && value <= 1.0F;
}
bool usable(ObservationUsability value) noexcept
{
    return value == ObservationUsability::Usable;
}
} // namespace

bool YawnConfig::validate(std::string &error) const noexcept
{
    error.clear();
    if (!finite01(openThreshold) || !finite01(closeThreshold) || closeThreshold >= openThreshold ||
        minimumDuration <= MonotonicTime::zero() || confirmation <= MonotonicTime::zero())
    {
        error = "invalid yawn thresholds or durations";
        return false;
    }
    return true;
}
YawnFsm::YawnFsm(YawnConfig config) : config_(config)
{
    valid_ = config_.validate(error_);
}
void YawnFsm::reset() noexcept
{
    hasTimestamp_ = false;
    last_ = {};
    active_ = false;
    candidate_.reset();
    opened_.reset();
    count_ = 0;
}
YawnResult YawnFsm::update(MonotonicTime timestamp, ObservationUsability quality, std::optional<float> mouth) noexcept
{
    YawnResult out{active_, false, count_};
    if (!valid_ || (hasTimestamp_ && timestamp <= last_))
    {
        out.active = false;
        return out;
    }
    hasTimestamp_ = true;
    last_ = timestamp;
    if (!usable(quality) || !mouth || !finite01(*mouth))
    {
        active_ = false;
        candidate_.reset();
        opened_.reset();
        out.active = false;
        return out;
    }
    if (!active_)
    {
        if (*mouth >= config_.openThreshold)
        {
            if (!candidate_)
                candidate_ = timestamp;
            if (timestamp - *candidate_ >= config_.confirmation)
            {
                active_ = true;
                opened_ = *candidate_;
                candidate_.reset();
            }
        }
        else
            candidate_.reset();
    }
    else if (*mouth <= config_.closeThreshold)
    {
        if (!candidate_)
            candidate_ = timestamp;
        if (timestamp - *candidate_ >= config_.confirmation)
        {
            if (opened_ && *candidate_ - *opened_ >= config_.minimumDuration)
            {
                ++count_;
                out.event = true;
            }
            active_ = false;
            opened_.reset();
            candidate_.reset();
        }
    }
    else
        candidate_.reset();
    out.active = active_;
    out.count = count_;
    return out;
}

bool HeadPoseConfig::validate(std::string &error) const noexcept
{
    error.clear();
    if (!std::isfinite(yawEnterDegrees) || !std::isfinite(yawExitDegrees) || !std::isfinite(pitchEnterDegrees) ||
        !std::isfinite(pitchExitDegrees) || yawExitDegrees < 0 || yawEnterDegrees <= yawExitDegrees ||
        pitchExitDegrees < 0 || pitchEnterDegrees <= pitchExitDegrees || confirmation <= MonotonicTime::zero())
    {
        error = "invalid head-pose thresholds or confirmation";
        return false;
    }
    return true;
}
HeadPoseFsm::HeadPoseFsm(HeadPoseConfig config) : config_(config)
{
    valid_ = config_.validate(error_);
}
void HeadPoseFsm::reset() noexcept
{
    hasTimestamp_ = false;
    last_ = {};
    zone_ = HeadZone::Unknown;
    candidateZone_ = HeadZone::Unknown;
    candidateSince_.reset();
    neutralArmed_ = true;
    left_ = right_ = up_ = down_ = 0;
}
HeadPoseResult HeadPoseFsm::update(MonotonicTime timestamp, ObservationUsability quality, std::optional<float> yaw,
                                   std::optional<float> pitch) noexcept
{
    HeadPoseResult out{zone_, false, left_, right_, up_, down_};
    if (!valid_ || (hasTimestamp_ && timestamp <= last_))
    {
        out.zone = HeadZone::Unknown;
        return out;
    }
    hasTimestamp_ = true;
    last_ = timestamp;
    if (!usable(quality) || !yaw || !pitch || !std::isfinite(*yaw) || !std::isfinite(*pitch))
    {
        zone_ = HeadZone::Unknown;
        candidateSince_.reset();
        out.zone = zone_;
        return out;
    }
    HeadZone raw = HeadZone::Neutral;
    const float yawLimit =
        zone_ == HeadZone::Left || zone_ == HeadZone::Right ? config_.yawExitDegrees : config_.yawEnterDegrees;
    const float pitchLimit =
        zone_ == HeadZone::Up || zone_ == HeadZone::Down ? config_.pitchExitDegrees : config_.pitchEnterDegrees;
    if (std::abs(*yaw) >= yawLimit)
        raw = *yaw < 0 ? HeadZone::Left : HeadZone::Right;
    else if (std::abs(*pitch) >= pitchLimit)
        raw = *pitch < 0 ? HeadZone::Up : HeadZone::Down;
    if (raw != candidateZone_)
    {
        candidateZone_ = raw;
        candidateSince_ = timestamp;
    }
    if (candidateSince_ && timestamp - *candidateSince_ >= config_.confirmation && zone_ != candidateZone_)
    {
        zone_ = candidateZone_;
        if (zone_ == HeadZone::Neutral)
            neutralArmed_ = true;
        else if (neutralArmed_)
        {
            neutralArmed_ = false;
            out.movementEvent = true;
            if (zone_ == HeadZone::Left)
                ++left_;
            else if (zone_ == HeadZone::Right)
                ++right_;
            else if (zone_ == HeadZone::Up)
                ++up_;
            else if (zone_ == HeadZone::Down)
                ++down_;
        }
    }
    out.zone = zone_;
    out.leftCount = left_;
    out.rightCount = right_;
    out.upCount = up_;
    out.downCount = down_;
    return out;
}

bool DistractionConfig::validate(std::string &error) const noexcept
{
    error.clear();
    if (!finite01(horizontalEnter) || !finite01(horizontalExit) || horizontalExit >= horizontalEnter ||
        !finite01(verticalEnter) || !finite01(verticalExit) || verticalExit >= verticalEnter ||
        gazeConfirmation <= MonotonicTime::zero() || distractionDuration <= MonotonicTime::zero() ||
        recoveryDuration <= MonotonicTime::zero())
    {
        error = "invalid gaze/distraction thresholds or durations";
        return false;
    }
    return true;
}
DistractionFsm::DistractionFsm(DistractionConfig config) : config_(config)
{
    valid_ = config_.validate(error_);
}
void DistractionFsm::reset() noexcept
{
    hasTimestamp_ = false;
    last_ = {};
    gaze_ = GazeZone::Unknown;
    candidate_ = GazeZone::Unknown;
    candidateSince_.reset();
    awaySince_.reset();
    recoverySince_.reset();
    distracted_ = false;
}
DistractionResult DistractionFsm::update(MonotonicTime timestamp, ObservationUsability quality,
                                         std::optional<float> horizontal, std::optional<float> vertical,
                                         HeadZone head) noexcept
{
    DistractionResult out{gaze_, distracted_, false};
    if (!valid_ || (hasTimestamp_ && timestamp <= last_))
    {
        out.gaze = GazeZone::Unknown;
        return out;
    }
    hasTimestamp_ = true;
    last_ = timestamp;
    if (!usable(quality) || !horizontal || !vertical || !std::isfinite(*horizontal) || !std::isfinite(*vertical))
    {
        gaze_ = GazeZone::Unknown;
        candidateSince_.reset();
        awaySince_.reset();
        recoverySince_.reset();
        out.gaze = gaze_;
        out.distracted = false;
        return out;
    }
    const float hlim =
        (gaze_ == GazeZone::Left || gaze_ == GazeZone::Right) ? config_.horizontalExit : config_.horizontalEnter;
    const float vlim =
        (gaze_ == GazeZone::Up || gaze_ == GazeZone::Down) ? config_.verticalExit : config_.verticalEnter;
    GazeZone raw = GazeZone::Forward;
    if (std::abs(*horizontal) >= hlim)
        raw = *horizontal < 0 ? GazeZone::Left : GazeZone::Right;
    else if (std::abs(*vertical) >= vlim)
        raw = *vertical < 0 ? GazeZone::Up : GazeZone::Down;
    if (raw != candidate_)
    {
        candidate_ = raw;
        candidateSince_ = timestamp;
    }
    if (candidateSince_ && timestamp - *candidateSince_ >= config_.gazeConfirmation)
        gaze_ = candidate_;
    const bool away = gaze_ != GazeZone::Forward || head == HeadZone::Left || head == HeadZone::Right ||
                      head == HeadZone::Up || head == HeadZone::Down;
    if (away)
    {
        recoverySince_.reset();
        if (!awaySince_)
            awaySince_ = timestamp;
        if (!distracted_ && timestamp - *awaySince_ >= config_.distractionDuration)
        {
            distracted_ = true;
            out.event = true;
        }
    }
    else
    {
        awaySince_.reset();
        if (distracted_)
        {
            if (!recoverySince_)
                recoverySince_ = timestamp;
            if (timestamp - *recoverySince_ >= config_.recoveryDuration)
            {
                distracted_ = false;
                recoverySince_.reset();
            }
        }
    }
    out.gaze = gaze_;
    out.distracted = distracted_;
    return out;
}

bool PresenceConfig::validate(std::string &error) const noexcept
{
    error.clear();
    if (presentConfirmation <= MonotonicTime::zero() || absentConfirmation <= MonotonicTime::zero())
    {
        error = "invalid presence confirmation durations";
        return false;
    }
    return true;
}
DriverPresenceFsm::DriverPresenceFsm(PresenceConfig config) : config_(config)
{
    valid_ = config_.validate(error_);
}

bool MonitoringAvailabilityConfig::validate(std::string &error) const noexcept
{
    error.clear();
    if (recordAfter < MonotonicTime::zero() || notifyAfter < recordAfter)
    {
        error = "invalid monitoring availability durations";
        return false;
    }
    return true;
}
MonitoringAvailabilityFsm::MonitoringAvailabilityFsm(MonitoringAvailabilityConfig config) : config_(config)
{
    std::string error;
    valid_ = config_.validate(error);
}
void MonitoringAvailabilityFsm::reset() noexcept
{
    hasTimestamp_ = recorded_ = notified_ = false;
    last_ = unavailableSince_ = {};
    count_ = 0;
}
MonitoringAvailabilityResult MonitoringAvailabilityFsm::update(MonotonicTime timestamp,
                                                                ObservationUsability usability) noexcept
{
    MonitoringAvailabilityResult out;
    out.episodeCount = count_;
    if (!valid_ || (hasTimestamp_ && timestamp <= last_)) return out;
    hasTimestamp_ = true;
    last_ = timestamp;
    if (usable(usability))
    {
        unavailableSince_ = {};
        recorded_ = notified_ = false;
        return out;
    }
    if (unavailableSince_ == MonotonicTime::zero()) unavailableSince_ = timestamp;
    out.unavailable = true;
    out.unavailableDuration = timestamp - unavailableSince_;
    if (!recorded_ && out.unavailableDuration >= config_.recordAfter)
    {
        recorded_ = true;
        ++count_;
        out.recordEvent = true;
    }
    if (!notified_ && out.unavailableDuration >= config_.notifyAfter)
    {
        notified_ = true;
        out.notifyEvent = true;
    }
    out.notify = notified_;
    out.episodeCount = count_;
    return out;
}
void DriverPresenceFsm::reset() noexcept
{
    hasTimestamp_ = false;
    last_ = {};
    state_ = PresenceState::Unknown;
    candidate_ = PresenceState::Unknown;
    since_.reset();
}
PresenceState DriverPresenceFsm::update(MonotonicTime timestamp, ObservationUsability quality, bool detected) noexcept
{
    if (!valid_ || (hasTimestamp_ && timestamp <= last_))
        return PresenceState::Unknown;
    hasTimestamp_ = true;
    last_ = timestamp;
    PresenceState raw = PresenceState::Unknown;
    if (detected && usable(quality))
        raw = PresenceState::Present;
    else if (!detected && (usable(quality) || quality == ObservationUsability::Missing))
        raw = PresenceState::Absent;
    if (raw == PresenceState::Unknown)
    {
        candidate_ = raw;
        since_.reset();
        return state_ = raw;
    }
    if (raw != candidate_)
    {
        candidate_ = raw;
        since_ = timestamp;
    }
    const auto required = raw == PresenceState::Present ? config_.presentConfirmation : config_.absentConfirmation;
    if (since_ && timestamp - *since_ >= required)
        state_ = raw;
    return state_;
}

bool DrowsinessConfig::validate(std::string &error) const noexcept
{
    error.clear();
    if (!finite01(perclosWarning) || !finite01(perclosDrowsy) || perclosWarning >= perclosDrowsy ||
        yawnEvidenceWindow <= MonotonicTime::zero() || yawnsForWarning == 0 ||
        minimumAlertHold < MonotonicTime::zero() || recoveryDuration <= MonotonicTime::zero())
    {
        error = "invalid drowsiness thresholds or durations";
        return false;
    }
    return true;
}
DrowsinessFsm::DrowsinessFsm(DrowsinessConfig config) : config_(config)
{
    valid_ = config_.validate(error_);
}
void DrowsinessFsm::reset() noexcept
{
    hasTimestamp_ = false;
    last_ = {};
    state_ = DrowsinessState::Unknown;
    recoverySince_.reset();
    alertSince_.reset();
    yawns_.clear();
}
DrowsinessResult DrowsinessFsm::update(const DrowsinessInput &input) noexcept
{
    DrowsinessResult out{state_, static_cast<std::uint32_t>(yawns_.size())};
    if (!valid_ || (hasTimestamp_ && input.timestamp <= last_))
    {
        out.state = DrowsinessState::Unknown;
        return out;
    }
    hasTimestamp_ = true;
    last_ = input.timestamp;
    while (!yawns_.empty() && input.timestamp - yawns_.front() > config_.yawnEvidenceWindow)
        yawns_.pop_front();
    if (!usable(input.usability) || input.presence != PresenceState::Present)
    {
        state_ = DrowsinessState::Unknown;
        recoverySince_.reset();
        return {state_, static_cast<std::uint32_t>(yawns_.size())};
    }
    if (input.yawnEvent)
        yawns_.push_back(input.timestamp);
    DrowsinessState evidence = DrowsinessState::Normal;
    if (input.prolongedClosure || (input.perclos && *input.perclos >= config_.perclosDrowsy))
        evidence = DrowsinessState::Drowsy;
    else if (input.longBlinkEvent || (input.perclos && *input.perclos >= config_.perclosWarning) ||
             yawns_.size() >= config_.yawnsForWarning)
        evidence = DrowsinessState::Warning;
    if (evidence != DrowsinessState::Normal && !alertSince_)
        alertSince_ = input.timestamp;
    if (evidence == DrowsinessState::Normal &&
        (state_ == DrowsinessState::Warning || state_ == DrowsinessState::Drowsy))
    {
        if (alertSince_ && input.timestamp - *alertSince_ < config_.minimumAlertHold)
            return {state_, static_cast<std::uint32_t>(yawns_.size())};
        if (!recoverySince_)
            recoverySince_ = input.timestamp;
        if (input.timestamp - *recoverySince_ >= config_.recoveryDuration)
        {
            state_ = evidence;
            alertSince_.reset();
        }
    }
    else
    {
        recoverySince_.reset();
        state_ = evidence;
    }
    return {state_, static_cast<std::uint32_t>(yawns_.size())};
}
} // namespace dms
