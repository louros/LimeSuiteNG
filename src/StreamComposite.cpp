#include "limesuiteng/StreamComposite.h"
#include <algorithm>
#include "limesuiteng/types.h"
#include "limesuiteng/StreamConfig.h"
#include "limesuiteng/SDRDescriptor.h"
#include "limesuiteng/RFSOCDescriptor.h"
#include <assert.h>
namespace lime {

StreamComposite::StreamComposite(const std::vector<StreamAggregate>& aggregate)
    : mAggregate(aggregate)
{
}

OpStatus StreamComposite::StreamSetup(const StreamConfig& config)
{
    mActiveAggregates.clear();
    StreamConfig subConfig = config;

    std::size_t rxNeed = config.channels.at(TRXDir::Rx).size();
    std::size_t txNeed = config.channels.at(TRXDir::Tx).size();

    for (auto& aggregate : mAggregate)
    {
        subConfig.channels.at(TRXDir::Rx).clear();
        subConfig.channels.at(TRXDir::Tx).clear();
        assert(aggregate.device);
        const SDRDescriptor& desc = aggregate.device->GetDescriptor();
        std::size_t aggregateChannelCount = aggregate.channels.size();

        std::size_t channelCount = std::min(aggregateChannelCount, rxNeed);
        for (std::size_t j = 0; j < channelCount; ++j)
        {
            if (aggregate.channels[j] >= desc.rfSOC[aggregate.streamIndex].channelCount)
                return OpStatus::OutOfRange;

            subConfig.channels.at(TRXDir::Rx).push_back(aggregate.channels[j]);
        }
        rxNeed -= channelCount;

        channelCount = std::min(aggregateChannelCount, txNeed);
        for (std::size_t j = 0; j < channelCount; ++j)
        {
            if (aggregate.channels[j] >= desc.rfSOC[aggregate.streamIndex].channelCount)
                return OpStatus::OutOfRange;

            subConfig.channels.at(TRXDir::Tx).push_back(aggregate.channels[j]);
        }
        txNeed -= channelCount;

        OpStatus status = aggregate.device->StreamSetup(subConfig, aggregate.streamIndex);
        if (status != OpStatus::Success)
            return status;

        mActiveAggregates.push_back(aggregate);

        if (rxNeed == 0 && txNeed == 0)
        {
            break;
        }
    }
    return OpStatus::Success;
}

void StreamComposite::StreamStart()
{
    std::unordered_map<SDRDevice*, std::vector<uint8_t>> groups;
    for (auto& a : mActiveAggregates)
        groups[a.device].push_back(a.streamIndex);
    for (auto& g : groups)
        g.first->StreamStart(g.second);
}

void StreamComposite::StreamStop()
{
    std::unordered_map<SDRDevice*, std::vector<uint8_t>> groups;
    for (auto& a : mActiveAggregates)
        groups[a.device].push_back(a.streamIndex);
    for (auto& g : groups)
        g.first->StreamStop(g.second);
}

template<class T> uint32_t StreamComposite::StreamRx(T** samples, uint32_t count, StreamMeta* meta)
{
    T** dest = samples;
    for (auto& a : mActiveAggregates)
    {
        uint32_t ret = a.device->StreamRx(a.streamIndex, dest, count, meta);
        if (ret != count)
        {
            return ret;
        }

        dest += a.channels.size();
    }
    return count;
}

template<class T> uint32_t StreamComposite::StreamTx(const T* const* samples, uint32_t count, const StreamMeta* meta)
{
    const T* const* src = samples;
    for (auto& a : mActiveAggregates)
    {
        uint32_t ret = a.device->StreamTx(a.streamIndex, src, count, meta);
        if (ret != count)
        {
            return ret;
        }

        src += a.channels.size();
    }
    return count;
}

// force instantiate functions with these types
template LIME_API uint32_t StreamComposite::StreamRx<lime::complex16_t>(
    lime::complex16_t** samples, uint32_t count, StreamMeta* meta);
template LIME_API uint32_t StreamComposite::StreamRx<lime::complex32f_t>(
    lime::complex32f_t** samples, uint32_t count, StreamMeta* meta);
template LIME_API uint32_t StreamComposite::StreamTx<lime::complex16_t>(
    const lime::complex16_t* const* samples, uint32_t count, const StreamMeta* meta);
template LIME_API uint32_t StreamComposite::StreamTx<lime::complex32f_t>(
    const lime::complex32f_t* const* samples, uint32_t count, const StreamMeta* meta);

} // namespace lime
