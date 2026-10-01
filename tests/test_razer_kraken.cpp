#include "devices/razer_kraken_v4.hpp"
#include <algorithm>
#include <deque>
#include <stdexcept>
#include <string>
#include <vector>

namespace headsetcontrol::testing {
namespace {
    void require(bool condition, const char* message)
    {
        if (!condition)
            throw std::runtime_error(message);
    }
    std::vector<uint8_t> packet(std::string_view hex)
    {
        std::vector<uint8_t> result;
        for (size_t i = 0; i < hex.size(); i += 2)
            result.push_back(static_cast<uint8_t>(std::stoul(std::string(hex.substr(i, 2)), nullptr, 16)));
        return result;
    }
    const auto BATTERY_QUERY  = packet("0200600000000400008021000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000c700");
    const auto CHARGING_QUERY = packet("020060000000040000802a000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000cc00");
    const auto BATTERY_44     = packet("020260000000050080802101012c0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000006a00");
    const auto BATTERY_45     = packet("020260000000050080802101012d0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000006b00");
    const auto CHARGING_0     = packet("020260000000050080802a0101000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000004d00");
    const auto CHARGING_1     = packet("020260000000050080802a0101010000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000004c00");
    const auto NOTIFICATION   = packet("020a60560000050080802a0201010000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001100");

    const auto SIDETONE_ON  = packet("02006000000005000080980001010000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000007f00");
    const auto SIDETONE_OFF = packet("02006000000005000080980001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000007e00");
    const std::array SIDETONE_LEVELS {
        packet("02006000000005000080990001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000007f00"),
        packet("02006000000005000080990001010000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000007e00"),
        packet("02006000000005000080990001020000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000007d00"),
        packet("02006000000005000080990001030000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000007c00"),
        packet("02006000000005000080990001040000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000007b00"),
        packet("02006000000005000080990001050000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000007a00"),
        packet("02006000000005000080990001060000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000007900"),
        packet("02006000000005000080990001070000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000007800"),
        packet("02006000000005000080990001080000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000007700"),
        packet("02006000000005000080990001090000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000007600"),
        packet("020060000000050000809900010a0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000007500"),
        packet("020060000000050000809900010b0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000007400"),
        packet("020060000000050000809900010c0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000007300"),
        packet("020060000000050000809900010d0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000007200"),
        packet("020060000000050000809900010e0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000007100"),
        packet("020060000000050000809900010f0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000007000")
    };
    const auto ACK_98 = packet("0202600000000500808098010100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000ff00");
    const auto ACK_99 = packet("0202600000000500808099010100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000fe00");

    const auto LIGHTS_OFF = packet("02006000000005000080c10001000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000002700");
    const auto LIGHTS_ON  = packet("02006000000005000080c10001400000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000006700");
    const auto ACK_C1     = packet("02026000000005008080c1010100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000a600");

    class KrakenMockHID final : public HIDInterface {
    public:
        std::deque<Result<std::vector<uint8_t>>> reads;
        std::vector<std::vector<uint8_t>> writes;
        bool fail_write = false;

        Result<void> write(hid_device*, std::span<const uint8_t> data) override
        {
            writes.emplace_back(data.begin(), data.end());
            if (fail_write)
                return DeviceError::hidError("Simulated HID write error");
            return {};
        }

        Result<void> write(hid_device* handle, std::span<const uint8_t> data, size_t size) override
        {
            std::vector<uint8_t> padded(size);
            std::copy_n(data.begin(), std::min(data.size(), size), padded.begin());
            return write(handle, padded);
        }

        Result<size_t> readTimeout(hid_device*, std::span<uint8_t> data, int) override
        {
            if (reads.empty())
                return DeviceError::timeout("Simulated timeout");

            auto result = std::move(reads.front());
            reads.pop_front();
            if (!result)
                return result.error();

            const auto& response = *result;
            const size_t size    = std::min(response.size(), data.size());
            std::copy_n(response.begin(), size, data.begin());
            return size;
        }

        Result<void> sendFeatureReport(hid_device*, std::span<const uint8_t>) override
        {
            return DeviceError::notSupported("Kraken queries must use Output reports");
        }

        Result<void> sendFeatureReport(hid_device*, std::span<const uint8_t>, size_t) override
        {
            return DeviceError::notSupported("Kraken queries must use Output reports");
        }

        Result<size_t> getFeatureReport(hid_device*, std::span<uint8_t>) override
        {
            return DeviceError::notSupported("Not used by this test");
        }

        Result<size_t> getInputReport(hid_device*, std::span<uint8_t>) override
        {
            return DeviceError::notSupported("Not used by this test");
        }
    };

    class TestableKraken : public RazerKrakenV4 {
    public:
        explicit TestableKraken(KrakenMockHID& hid)
            : hid_(hid)
        {
        }

    protected:
        HIDInterface& getHIDInterface() const override { return hid_; }

    private:
        KrakenMockHID& hid_;
    };
} // namespace

void runAllRazerKrakenTests()
{
    for (bool charging : { false, true }) {
        KrakenMockHID hid;
        TestableKraken device(hid);
        hid.reads.emplace_back(NOTIFICATION);
        hid.reads.emplace_back(charging ? CHARGING_1 : CHARGING_0);
        hid.reads.emplace_back(charging ? BATTERY_45 : BATTERY_44);
        auto result = device.getBattery(nullptr);
        require(result.hasValue(), "Captured Kraken responses should parse");
        require(result->level_percent == (charging ? 45 : 44), "Captured battery should match UI");
        require(result->status == (charging ? BATTERY_CHARGING : BATTERY_AVAILABLE), "Charging should match capture");
        require(hid.writes.size() == 2, "Only two captured queries should be sent");
        require(hid.writes[0] == CHARGING_QUERY && hid.writes[1] == BATTERY_QUERY, "Queries must match captured packets exactly");
    }
    {
        KrakenMockHID hid;
        TestableKraken device(hid);
        auto result = device.getBattery(nullptr);
        require(result.hasError(), "Missing response should fail");
        require(hid.writes.size() == 1, "Timeout should not resend query");
    }
    {
        KrakenMockHID hid;
        TestableKraken device(hid);
        hid.fail_write = true;
        require(device.getBattery(nullptr).hasError(), "Write failure should propagate");
        require(hid.writes.size() == 1, "Write failure should not retry");
    }
    {
        KrakenMockHID hid;
        TestableKraken device(hid);
        auto invalid_charging = CHARGING_0;
        invalid_charging[13]  = 2;
        hid.reads.emplace_back(invalid_charging);
        require(device.getBattery(nullptr).hasError(), "Unknown charging flag should fail");
    }
    for (int invalid : { 101, 255 }) {
        KrakenMockHID hid;
        TestableKraken device(hid);
        auto bad = BATTERY_44;
        bad[13]  = static_cast<uint8_t>(invalid);
        hid.reads.emplace_back(CHARGING_0);
        hid.reads.emplace_back(bad);
        require(device.getBattery(nullptr).hasError(), "Invalid percentage should fail");
    }
    {
        KrakenMockHID hid;
        TestableKraken device(hid);
        auto short_response = CHARGING_0;
        short_response.resize(13);
        hid.reads.emplace_back(short_response);
        require(device.getBattery(nullptr).hasError(), "Truncated matching response should fail");
    }
    // Finite captured settings: never synthesize unobserved HID level bytes.
    for (int level = 0; level <= 128; ++level) {
        KrakenMockHID hid;
        TestableKraken device(hid);
        hid.reads.emplace_back(NOTIFICATION);
        hid.reads.emplace_back(ACK_98);
        if (level)
            hid.reads.emplace_back(ACK_99);
        auto result = device.setSidetone(nullptr, static_cast<uint8_t>(level));
        require(result.hasValue(), "Captured sidetone acknowledgments should succeed");
        const int native = level ? std::max(1, level * 15 / 128) : 0;
        const int actual = level ? (native * 128 + 14) / 15 : 0;
        require(result->current_level == actual && result->device_level == native,
            "Report the actual quantized level");
        require(!level || actual * 15 / 128 == native, "Reported level should select the same captured packet");
        require(result->is_muted == (level == 0), "Zero should disable sidetone");
        require(hid.writes[0] == (level ? SIDETONE_ON : SIDETONE_OFF), "Toggle must exactly match capture");
        require(hid.writes.size() == (level ? 2 : 1), "Send only the required captured settings");
        if (level)
            require(hid.writes[1] == SIDETONE_LEVELS[native], "Level must exactly match capture");
    }
    {
        KrakenMockHID hid;
        TestableKraken device(hid);
        hid.reads.emplace_back(ACK_98);
        hid.reads.emplace_back(DeviceError::timeout("Setting timeout"));
        hid.reads.emplace_back(ACK_98);
        require(device.setSidetone(nullptr, 128).hasError(), "Level timeout should fail");
        require(hid.writes.size() == 3 && hid.writes.back() == SIDETONE_OFF, "Restore OFF when level setting fails");
    }
    {
        KrakenMockHID hid;
        TestableKraken device(hid);
        auto bad_ack = ACK_98;
        bad_ack[13]  = 1;
        hid.reads.emplace_back(bad_ack);
        hid.reads.emplace_back(ACK_98);
        require(device.setSidetone(nullptr, 64).hasError(), "Unknown acknowledgment must not report success");
        require(hid.writes.back() == SIDETONE_OFF, "Restore OFF after uncertain toggle acknowledgment");
    }
    {
        KrakenMockHID hid;
        TestableKraken device(hid);
        require(device.setSidetone(nullptr, 129).hasError(), "Reject out of range sidetone");
        require(hid.writes.empty(), "Reject invalid input before any write");
    }
    for (bool on : { false, true }) {
        KrakenMockHID hid;
        TestableKraken device(hid);
        hid.reads.emplace_back(NOTIFICATION);
        hid.reads.emplace_back(ACK_C1);
        auto result = device.setLights(nullptr, on);
        require(result.hasValue() && result->enabled == on, "Captured light acknowledgment should succeed");
        require(hid.writes.size() == 1 && hid.writes[0] == (on ? LIGHTS_ON : LIGHTS_OFF), "Lights output must exactly match capture");
    }
    {
        KrakenMockHID hid;
        TestableKraken device(hid);
        require(device.setLights(nullptr, false).hasError(), "Missing light acknowledgment should fail");
        require(hid.writes.size() == 1, "Light timeout should not resend");
    }
}
} // namespace headsetcontrol::testing
