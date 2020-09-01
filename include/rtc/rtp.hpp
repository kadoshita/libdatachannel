/**
 * Copyright (c) 2020 Staz M
 * Copyright (c) 2020 Paul-Louis Ageneau
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef RTC_RTPL_H
#define RTC_RTPL_H

#include "include.hpp"
#include "log.hpp"
#include "message.hpp"

#include <cmath>
#include <functional>
#include <iostream>
#include <utility>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#ifndef htonll
#define htonll(x)                                                                                  \
	((uint64_t)htonl(((uint64_t)(x)&0xFFFFFFFF) << 32) | (uint64_t)htonl((uint64_t)(x) >> 32))
#endif
#ifndef ntohll
#define ntohll(x) htonll(x)
#endif

namespace rtc {

typedef uint32_t SSRC;

#pragma pack(push, 1)

struct RTCP_ReportBlock {
private:
	SSRC ssrc;
	uint32_t fractionLostAndPacketsLost; // fraction lost is 8-bit, packets lost is 24-bit
	uint16_t seqNoCycles;
	uint16_t highestSeqNo;
	uint32_t arrivalJitter;
	uint32_t lastReport;
	uint32_t delaySinceLastReport;

public:
	void print() {
		std::cout << " ssrc:" << ntohl(ssrc) <<
		    // TODO Implement these reports
		    //			  " fractionLost: " << fractionLost <<
		    //			  " packetsLost: " << packetsLost <<
		    " highestSeqNo:" << getHighestSeqNo() << " seqNoCycles:" << getSeqCycleCount()
		          << " jitter:" << getJitter() << " lastSR:" << getNTPOfSR()
		          << " lastSRDelay:" << getDelaySinceSR();
	}

	void preparePacket(SSRC ssrc, [[maybe_unused]] unsigned int packetsLost,
	                   [[maybe_unused]] unsigned int totalPackets, uint16_t highestSeqNo,
	                   uint16_t seqNoCycles, uint32_t jitter, uint64_t lastSR_NTP,
	                   uint64_t lastSR_DELAY) {
		setSeqNo(highestSeqNo, seqNoCycles);
		setJitter(jitter);
		setSSRC(ssrc);

		// Middle 32 bits of NTP Timestamp
		//		  this->lastReport = lastSR_NTP >> 16u;
		setNTPOfSR(uint32_t(lastSR_NTP));
		setDelaySinceSR(uint32_t(lastSR_DELAY));

		// The delay, expressed in units of 1/65536 seconds
		//		  this->delaySinceLastReport = lastSR_DELAY;
	}

	void inline setSSRC(SSRC ssrc) { this->ssrc = htonl(ssrc); }
	SSRC inline getSSRC() const { return ntohl(ssrc); }

	void inline setPacketsLost([[maybe_unused]] unsigned int packetsLost,
	                           [[maybe_unused]] unsigned int totalPackets) {
		// TODO Implement loss percentages.
		this->fractionLostAndPacketsLost = 0;
	}
	unsigned int inline getLossPercentage() const {
		// TODO Implement loss percentages.
		return 0;
	}
	unsigned int inline getPacketLostCount() const {
		// TODO Implement total packets lost.
		return 0;
	}

	void inline setSeqNo(uint16_t highestSeqNo, uint16_t seqNoCycles) {
		this->highestSeqNo = htons(highestSeqNo);
		this->seqNoCycles = htons(seqNoCycles);
	}

	uint16_t inline getHighestSeqNo() const { return ntohs(this->highestSeqNo); }
	uint16_t inline getSeqCycleCount() const { return ntohs(this->seqNoCycles); }

	uint32_t inline getJitter() const { return ntohl(arrivalJitter); }
	void inline setJitter(uint32_t jitter) { this->arrivalJitter = htonl(jitter); }

	void inline setNTPOfSR(uint32_t ntp) { lastReport = htonl(ntp >> 16u); }
	inline uint32_t getNTPOfSR() const { return ntohl(lastReport) << 16u; }
	inline void setDelaySinceSR(uint32_t sr) {
		// The delay, expressed in units of 1/65536 seconds
		delaySinceLastReport = htonl(sr);
	}
	inline uint32_t getDelaySinceSR() const { return ntohl(delaySinceLastReport); }
};

struct RTCP_HEADER {
private:
	uint8_t version : 2;
	uint8_t padding : 1;
	uint8_t reportCount : 5;
	uint8_t payloadType;
	uint16_t length;

public:
	void prepareHeader(uint8_t payloadType, unsigned int reportCount, uint16_t length) {
		version = 2;
		padding = false;
		this->payloadType = payloadType;
		this->reportCount = reportCount;
		setLength(length);
	}

	inline uint8_t getPayloadType() const { return payloadType; }
	inline void setPayloadType(uint8_t payloadType) { this->payloadType = payloadType; }

	inline uint8_t getReportCount() const { return reportCount; }
	inline void setReportCount(uint8_t reportCount) { this->reportCount = reportCount; }

	inline uint16_t getLength() const { return ntohs(length); }
	inline void setLength(uint16_t length) { this->length = htons(length); }

	void print() {
		std::cout << "version:" << (uint16_t)version << " padding:" << (padding ? "T" : "F")
		          << " reportCount: " << (uint16_t)getReportCount()
		          << " payloadType:" << (uint16_t)getPayloadType() << " length: " << getLength();
	}
};

struct RTCP_SR {
private:
	RTCP_HEADER header;

	SSRC senderSSRC;

	uint64_t ntpTimestamp;
	uint32_t rtpTimestamp;

	uint32_t packetCount;
	uint32_t octetCount;

	RTCP_ReportBlock reportBlocks;

public:
	void print() {
		std::cout << "SR ";
		header.print();
		std::cout << " SSRC:" << ntohl(senderSSRC) << " NTP TS: " << ntohll(ntpTimestamp)
		          << " RTP TS: " << ntohl(rtpTimestamp) << " packetCount: " << ntohl(packetCount)
		          << " octetCount: " << ntohl(octetCount) << std::endl;

		for (int i = 0; i < header.getReportCount(); i++) {
			getReportBlock(i)->print();
			std::cout << std::endl;
		}
	}

	inline void preparePacket(SSRC senderSSRC, uint8_t reportCount) {
		unsigned int length =
		    ((offsetof(RTCP_SR, reportBlocks) + reportCount * sizeof(RTCP_ReportBlock)) / 4) - 1;
		header.prepareHeader(200, reportCount, length);
		this->senderSSRC = senderSSRC;
	}

	RTCP_ReportBlock *getReportBlock(int num) { return &reportBlocks + num; }

	[[nodiscard]] unsigned int getSize() const {
		// "length" in packet is one less than the number of 32 bit words in the packet.
		return sizeof(uint32_t) * (1 + header.getLength());
	}

	inline uint32_t getRTPTS() const { return ntohl(rtpTimestamp); }
	inline uint32_t getNTPTS() const { return ntohll(ntpTimestamp); }

	inline void setRTPTS(uint32_t ts) { this->rtpTimestamp = htonl(ts); }
	inline void setNTPTS(uint32_t ts) { this->ntpTimestamp = htonll(ts); }
};

struct RTCP_RR {
private:
	RTCP_HEADER header;
	SSRC senderSSRC;
	RTCP_ReportBlock reportBlocks;

public:
	void print() {
		std::cout << "RR ";
		header.print();
		std::cout << " SSRC:" << ntohl(senderSSRC) << std::endl;

		for (int i = 0; i < header.getReportCount(); i++) {
			getReportBlock(i)->print();
			std::cout << std::endl;
		}
	}
	RTCP_ReportBlock *getReportBlock(int num) { return &reportBlocks + num; }

	inline RTCP_HEADER &getHeader() { return header; }

	inline SSRC getSenderSSRC() const { return ntohl(senderSSRC); }

	inline void setSenderSSRC(SSRC ssrc) { this->senderSSRC = ssrc; }

	[[nodiscard]] inline unsigned int getSize() const {
		// "length" in packet is one less than the number of 32 bit words in the packet.
		return sizeof(uint32_t) * (1 + header.getLength());
	}

	inline void preparePacket(SSRC senderSSRC, uint8_t reportCount) {
		//		  version = 2;
		//		  padding = false;
		//		  this->reportCount = reportCount;
		//		  payloadType = 201;
		//		  // "length" in packet is one less than the number of 32 bit words in the packet.
		unsigned int length =
		    ((offsetof(RTCP_RR, reportBlocks) + reportCount * sizeof(RTCP_ReportBlock)) / 4) - 1;
		header.prepareHeader(201, reportCount, length);
		this->senderSSRC = htonl(senderSSRC);
	}

	static unsigned inline int sizeWithReportBlocks(int reportCount) {
		return offsetof(RTCP_RR, reportBlocks) + reportCount * sizeof(RTCP_ReportBlock);
	}
};

struct RTP
{
	uint8_t version : 2;
	uint8_t padding : 1;
	uint8_t extension : 1;
	uint8_t csrcCount : 4;
	uint8_t markerBit : 1;
	uint8_t payloadType : 7;
	uint16_t seqNumber;
	uint32_t timestamp;
	SSRC ssrc;
	SSRC csrc[16];

	inline uint32_t getSeqNo() const { return ntohs(seqNumber); }
	inline uint32_t getTS() const { return ntohl(timestamp); }
};

struct RTCP_REMB {
	RTCP_HEADER header;

	SSRC senderSSRC;
	SSRC mediaSourceSSRC;

	// Unique identifier
	const char id[4] = {'R', 'E', 'M', 'B'};

	// Num SSRC, Br Exp, Br Mantissa (bit mask)
	uint32_t bitrate;

	SSRC ssrc[1];

	[[nodiscard]] unsigned int getSize() const {
		// "length" in packet is one less than the number of 32 bit words in the packet.
		return sizeof(uint32_t) * (1 + header.getLength());
	}

	void preparePacket(SSRC senderSSRC, unsigned int numSSRC, unsigned int bitrate) {
		// Report Count becomes the format here.
		header.prepareHeader(206, 15, 0);

		// Always zero.
		mediaSourceSSRC = 0;

		this->senderSSRC = htonl(senderSSRC);
		setBitrate(numSSRC, bitrate);
	}

	void setBitrate(unsigned int numSSRC, unsigned int bitrate) {
		unsigned int exp = 0;
		while (bitrate > pow(2, 18) - 1) {
			exp++;
			bitrate /= 2;
		}

		// "length" in packet is one less than the number of 32 bit words in the packet.
		header.setLength(uint16_t((offsetof(RTCP_REMB, ssrc) / 4) - 1 + numSSRC));

		this->bitrate = htonl((numSSRC << (32u - 8u)) | (exp << (32u - 8u - 6u)) | bitrate);
	}

	// TODO Make this work
	//	  uint64_t getBitrate() const{
	//		  uint32_t ntohed = ntohl(this->bitrate);
	//		  uint64_t bitrate = ntohed & (unsigned int)(pow(2, 18)-1);
	//		  unsigned int exp = ntohed & ((unsigned int)( (pow(2, 6)-1)) << (32u-8u-6u));
	//		  return bitrate * pow(2,exp);
	//	  }
	//
	//	  uint8_t getNumSSRCS() const {
	//		  return ntohl(this->bitrate) & (((unsigned int) pow(2,8)-1) << (32u-8u));
	//	  }

	void print() {
		std::cout << "REMB ";
		header.print();
		std::cout << " SSRC:" << ntohl(senderSSRC);
	}

	void setSSRC(uint8_t iterator, SSRC ssrc) { this->ssrc[iterator] = htonl(ssrc); }

	static unsigned int sizeWithSSRCs(int numSSRC) {
		return (offsetof(RTCP_REMB, ssrc)) + sizeof(SSRC) * numSSRC;
	}
};

#pragma pack(pop)

class RtcpHandler {
public:
	virtual void onOutgoing(std::function<void(rtc::message_ptr)> cb) = 0;
	virtual std::optional<rtc::message_ptr> incoming(rtc::message_ptr ptr) = 0;
};

class RtcpSession : public RtcpHandler {
private:
	std::function<void(RTP)> onPacketCB;
	unsigned int requestedBitrate = 0;
	synchronized_callback<rtc::message_ptr> txCB;
	SSRC ssrc = 0;
	uint32_t greatestSeqNo = 0;
	uint64_t syncRTPTS, syncNTPTS;

public:
	void onOutgoing(std::function<void(rtc::message_ptr)> cb) override { txCB = cb; }

	std::optional<rtc::message_ptr> incoming(rtc::message_ptr ptr) override {
		if (ptr->type == rtc::Message::Type::Binary) {
			RTP *rtp = (RTP *)ptr->data();

			// https://tools.ietf.org/html/rfc3550#appendix-A.1
			if (rtp->version != 2) {
				PLOG_WARNING << "RTP packet is not version 2";

				return std::nullopt;
			}
			if (rtp->payloadType == 201 || rtp->payloadType == 200) {
				PLOG_WARNING << "RTP packet has a payload type indicating RR/SR";

				return std::nullopt;
			}

			// TODO Implement the padding bit
			if (rtp->padding) {
				PLOG_WARNING << "Padding processing not implemented";
			}

			ssrc = ntohl(rtp->ssrc);

			uint32_t seqNo = rtp->getSeqNo();
			// uint32_t rtpTS = rtp->getTS();

			if (greatestSeqNo < seqNo)
				greatestSeqNo = seqNo;

			return ptr;
		}

		assert(ptr->type == rtc::Message::Type::Control);
		auto rr = (RTCP_RR *)ptr->data();
		if (rr->getHeader().getPayloadType() == 201) {
			// RR
			ssrc = rr->getSenderSSRC();
			rr->print();
			std::cout << std::endl;
		} else if (rr->getHeader().getPayloadType() == 200) {
			// SR
			ssrc = rr->getSenderSSRC();
			auto sr = (RTCP_SR *)ptr->data();
			syncRTPTS = sr->getRTPTS();
			syncNTPTS = sr->getNTPTS();
			sr->print();
			std::cout << std::endl;

			// TODO For the time being, we will send RR's/REMB's when we get an SR
			pushRR(0);
			if (requestedBitrate > 0)
				pushREMB(requestedBitrate);
		}
		return std::nullopt;
	}

	void requestBitrate(unsigned int newBitrate) {
		this->requestedBitrate = newBitrate;

		PLOG_DEBUG << "[GOOG-REMB] Requesting bitrate: " << newBitrate << std::endl;
		pushREMB(newBitrate);
	}

private:
	void pushREMB(unsigned int bitrate) {
		rtc::message_ptr msg =
		    rtc::make_message(RTCP_REMB::sizeWithSSRCs(1), rtc::Message::Type::Control);
		auto remb = (RTCP_REMB *)msg->data();
		remb->preparePacket(ssrc, 1, bitrate);
		remb->setSSRC(0, ssrc);
		remb->print();
		std::cout << std::endl;

		tx(msg);
	}

	void pushRR(unsigned int lastSR_delay) {
		//		  std::cout << "size " << RTCP_RR::sizeWithReportBlocks(1) << std::endl;
		auto msg = rtc::make_message(RTCP_RR::sizeWithReportBlocks(1), rtc::Message::Type::Control);
		auto rr = (RTCP_RR *)msg->data();
		rr->preparePacket(ssrc, 1);
		rr->getReportBlock(0)->preparePacket(ssrc, 0, 0, greatestSeqNo, 0, 0, syncNTPTS,
		                                     lastSR_delay);
		rr->print();
		std::cout << std::endl;

		tx(msg);
	}

	void tx(message_ptr msg) {
		try {
			txCB(msg);
		} catch (const std::exception &e) {
			LOG_DEBUG << "RTCP tx failed: " << e.what();
		}
	}
};

} // namespace rtc

#endif // RTC_RTPL_H
