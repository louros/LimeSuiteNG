#include "streaming.h"

#include <chrono>
#include <thread>

using namespace std;
using namespace std::literals::string_literals;

namespace lime::tests {

LimeSuiteWrapper_streaming::LimeSuiteWrapper_streaming()
    : device(nullptr)
{
}

void LimeSuiteWrapper_streaming::SetUp()
{
    int n;
    lms_info_str_t list[16]; //should be large enough to hold all detected devices
    if ((n = LMS_GetDeviceList(list)) <= 0) //NULL can be passed to only get number of devices
        GTEST_SKIP() << "device not connected, skipping"s;

    //open the first device
    ASSERT_EQ(LMS_Open(&device, list[1], NULL), 0);
    ASSERT_EQ(LMS_Init(device), 0);
}

void LimeSuiteWrapper_streaming::TearDown()
{
    LMS_Close(device);
}

int LimeSuiteWrapper_streaming::SetupSampleRate()
{
    const RxStreamParams& params = GetParam();
    int status = LMS_SetSampleRate(device, params.sampleRate, params.oversample);
    EXPECT_EQ(status, 0);
    if (status != 0)
        return status;
    float_type host_Hz, rf_Hz;
    EXPECT_EQ(LMS_GetSampleRate(device, LMS_CH_RX, 0, &host_Hz, &rf_Hz), 0);
    EXPECT_NEAR(host_Hz, params.sampleRate, 100);
    EXPECT_NEAR(rf_Hz, params.sampleRate * params.oversample, 100);
    return status;
}

void LimeSuiteWrapper_streaming::SetupRxStreams(std::vector<lms_stream_t>& channels)
{
    const RxStreamParams& params = GetParam();
    for (size_t i = 0; i < params.channelCount; ++i)
        ASSERT_EQ(LMS_EnableChannel(device, LMS_CH_RX, i, true), 0);

    channels.clear();
    channels.reserve(params.channelCount);
    for (size_t i = 0; i < params.channelCount; ++i)
    {
        lms_stream_t streamId;
        streamId.channel = i;
        streamId.fifoSize = 1024 * 1024;
        streamId.throughputVsLatency = 0.0;
        streamId.isTx = false;
        streamId.dataFmt = lms_stream_t::LMS_FMT_I16;
        ASSERT_EQ(LMS_SetupStream(device, &streamId), 0);
        channels.push_back(streamId);
    }
}

void LimeSuiteWrapper_streaming::ReceiveSamplesVerifySampleRate(std::vector<lms_stream_t>& channels)
{
    const double expectedDuration_us{ 10000 };
    const int samplesToGet = GetParam().sampleRate * expectedDuration_us / 1e6;

    const int samplesBatchSize = 5000;
    int16_t buffer[samplesBatchSize * 2]; //buffer to hold complex values (2*samples))

    auto t1 = chrono::high_resolution_clock::now();

    int samplesRemaining = samplesToGet;
    while (samplesRemaining > 0)
    {
        int toRead = samplesRemaining < samplesBatchSize ? samplesRemaining : samplesBatchSize;
        int samplesGot = 0;
        for (auto& handle : channels)
        {
            samplesGot = LMS_RecvStream(&handle, buffer, toRead, NULL, 1000);
            ASSERT_EQ(samplesGot, toRead);
            if (samplesGot <= 0)
                break;
        }
        if (samplesGot != toRead)
            break;
        samplesRemaining -= toRead;
    }
    auto t2 = chrono::high_resolution_clock::now();
    EXPECT_EQ(samplesRemaining, 0);
    const std::chrono::microseconds streamDuration{ chrono::duration_cast<chrono::microseconds>(t2 - t1) };
    EXPECT_NEAR(streamDuration.count(), expectedDuration_us, 1000);
}

TEST_P(LimeSuiteWrapper_streaming, SetSampleRateIsAccurate)
{
    SetupSampleRate();

    std::vector<lms_stream_t> channels;
    SetupRxStreams(channels);

    for (auto& handle : channels)
        ASSERT_EQ(LMS_StartStream(&handle), 0);

    ReceiveSamplesVerifySampleRate(channels);

    for (auto& handle : channels)
        ASSERT_EQ(LMS_StopStream(&handle), 0);
    for (auto& handle : channels)
        LMS_DestroyStream(device, &handle);
}

INSTANTIATE_TEST_SUITE_P(streamVariations,
    LimeSuiteWrapper_streaming,
    ::testing::Values( //
        RxStreamParams{ 20e6, 4, 1 },
        RxStreamParams{ 20e6, 4, 2 },
        RxStreamParams{ 20e6, 2, 1 },
        RxStreamParams{ 20e6, 2, 2 },
        // RxStreamParams{ 20e6, 1, 1 }, // oversampling x1 not always supported
        // RxStreamParams{ 20e6, 1, 2 },
        RxStreamParams{ 10e6, 2, 1 },
        RxStreamParams{ 5e6, 2, 1 },
        RxStreamParams{ 1e6, 2, 1 },
        RxStreamParams{ 1e6, 2, 2 }),
    ::testing::PrintToStringParamName());

/*
TEST_F(LimeSuiteWrapper_streaming, RepeatedStartStopDontChangeSampleRate)
{
    ASSERT_EQ(SetupSampleRate({20e6, 4, 1, LMS_FMT_I16}), 0);
    std::vector<lms_stream_t> channels;
    SetupRxStreams(channels, channelCount);

    for (auto& handle : channels)
        ASSERT_EQ(LMS_StartStream(&handle), 0);

    for (auto& handle : channels)
        ASSERT_EQ(LMS_StopStream(&handle), 0);

    for (auto& handle : channels)
        ASSERT_EQ(LMS_StartStream(&handle), 0);

    ReceiveSamplesVerifySampleRate(channels);

    for (auto& handle : channels)
        ASSERT_EQ(LMS_StopStream(&handle), 0);
    for (auto& handle : channels)
        ASSERT_EQ(LMS_DestroyStream(device, &handle), 0);
}

TEST_F(LimeSuiteWrapper_streaming, RepeatedSetupDestroyDontChangeSampleRate)
{
    std::vector<lms_stream_t> channels;
    SetupRxStreams(channels, channelCount);

    for (auto& handle : channels)
        ASSERT_EQ(LMS_StartStream(&handle), 0);

    for (auto& handle : channels)
        ASSERT_EQ(LMS_StopStream(&handle), 0);

    for (auto& handle : channels)
        ASSERT_EQ(LMS_DestroyStream(device, &handle), 0);

    SetupRxStreams(channels, channelCount);

    for (auto& handle : channels)
        ASSERT_EQ(LMS_StartStream(&handle), 0);

    ReceiveSamplesVerifySampleRate(channels);

    //Stop streaming
    for (auto& handle : channels)
        ASSERT_EQ(LMS_StopStream(&handle), 0);
    for (auto& handle : channels)
        ASSERT_EQ(LMS_DestroyStream(device, &handle), 0);
}
*/
} // namespace lime::tests
