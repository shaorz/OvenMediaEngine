#include "rtcp_sr_generator.h"
#include "sender_report.h"

RtcpSRGenerator::RtcpSRGenerator(uint32_t ssrc, uint32_t codec_rate)
{
    _ssrc = ssrc;
	_codec_rate = codec_rate;
    _created_time = std::chrono::system_clock::now();
    _last_generated_time = std::chrono::system_clock::now();
}

void RtcpSRGenerator::AddRTPPacketAndGenerateRtcpSR(const RtpPacket &rtp_packet)
{
    _packet_count ++;
    _octec_count += rtp_packet.PayloadSize();

    // RTCP Interval
    // Send RTCP SR twice a second for the first 10 seconds so the player can sync AV. 
    // After 5 seconds, send RTCP SR once every 5 seconds. 
    if((GetElapsedTimeMSFromCreated() < 10000 && GetElapsedTimeMSFromRtcpSRGenerated() > 500) ||
        GetElapsedTimeMSFromRtcpSRGenerated() > 4999)
    {
		auto report = std::make_shared<SenderReport>();
		
		
		uint32_t msw = 0;
    	uint32_t lsw = 0;

    	//ov::Clock::GetNtpTime(msw, lsw);

		double clock = (double)rtp_packet.Timestamp() / (double)_codec_rate;

		double ipart, fraction;
		fraction = modf(clock, &ipart);
		fraction *= 1000; // to milliseconds

		msw = (uint32_t)(ipart);
		lsw = (uint32_t)((double)(fraction*1000)*(double)(((uint64_t)1)<<32)*1.0e-6);

		// auto reverse = (uint32_t)(((double)lsw/std::numeric_limits<uint32_t>::max())*1000);
		// logc("DEBUG", "[SR] Rate(%d) Timestamp(%u) Clock(%lf) ipart(%lf) fraction(%lf) msw(%u) lsw(%u) reverse(%u)",  _codec_rate, rtp_packet.Timestamp(), clock, ipart, fraction, msw, lsw, reverse);
		
		report->SetSenderSsrc(_ssrc);
		report->SetMsw(msw);
		report->SetLsw(lsw);
		
		report->SetTimestamp(rtp_packet.Timestamp());
		report->SetPacketCount(_packet_count);
		report->SetOctetCount(_octec_count);

		_rtcp_packet = std::make_shared<RtcpPacket>();
		_rtcp_packet->Build(report);

        // Reset RTCP information
        _packet_count = 0;
        _octec_count = 0;
        _last_generated_time = std::chrono::system_clock::now();
        _rtcp_generated_count++;
    }
}

bool RtcpSRGenerator::IsAvailableRtcpSRPacket() const
{
    return (_rtcp_packet != nullptr);
}

std::shared_ptr<RtcpPacket> RtcpSRGenerator::PopRtcpSRPacket()
{
    if(_rtcp_packet == nullptr)
    {
        return nullptr;
    }

    return std::move(_rtcp_packet);
}

uint32_t RtcpSRGenerator::GetElapsedTimeMSFromCreated()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - _created_time).count();
}

uint32_t RtcpSRGenerator::GetElapsedTimeMSFromRtcpSRGenerated()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - _last_generated_time).count();
}