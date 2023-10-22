/**
 * This code is based on the libdatachannel media-sender example.
 * https://github.com/paullouisageneau/libdatachannel/tree/master/examples/media-sender
 * The license of the media-sender code is as follows.
 */

/**
 * libdatachannel media sender example
 * Copyright (c) 2020 Staz Modrzynski
 * Copyright (c) 2020 Paul-Louis Ageneau
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#include "rtc/rtc.hpp"

#include <cstddef>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>

#include <nlohmann/json.hpp>

#ifdef _WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
typedef int SOCKET;
#endif

#include <httplib.h>

#define WHIP_ENDPOINT_DOMAIN "localhost"
#define WHIP_ENDPOINT_PORT 8080
#define WHIP_ENDPOINT_PATH "/whip"

using nlohmann::json;

const int BUFFER_SIZE = 2048;

int main() {
	try {
		rtc::InitLogger(rtc::LogLevel::Debug);

		httplib::Client cli(WHIP_ENDPOINT_DOMAIN, WHIP_ENDPOINT_PORT);

		auto pc = std::make_shared<rtc::PeerConnection>();

		pc->onStateChange(
		    [](rtc::PeerConnection::State state) { std::cout << "State: " << state << std::endl; });

		pc->onGatheringStateChange([pc, &cli](rtc::PeerConnection::GatheringState state) {
			std::cout << "Gathering State: " << state << std::endl;
			if (state == rtc::PeerConnection::GatheringState::Complete) {
				auto description = pc->localDescription();
				json message = {{"type", description->typeString()},
				                {"sdp", std::string(description.value())}};
				std::cout << description.value() << std::endl;

				auto res = cli.Post(WHIP_ENDPOINT_PATH, std::string(description.value()),
				                    "application/sdp");

				if (res->status != 201)
					throw std::runtime_error("Response status code from WHIP server is not 201");

				rtc::Description answer(res->body, "answer");
				std::cout << answer << std::endl;
				pc->setRemoteDescription(answer);
			}
		});

		SOCKET video_sock = socket(AF_INET, SOCK_DGRAM, 0);
		struct sockaddr_in video_addr = {};
		video_addr.sin_family = AF_INET;
		video_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
		video_addr.sin_port = htons(6000);

		if (bind(video_sock, reinterpret_cast<const sockaddr *>(&video_addr), sizeof(video_addr)) <
		    0)
			throw std::runtime_error("Failed to bind UDP socket on 127.0.0.1:6000");

		int rcvBufSize = 212992;
		setsockopt(video_sock, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char *>(&rcvBufSize),
		           sizeof(rcvBufSize));

		SOCKET audio_sock = socket(AF_INET, SOCK_DGRAM, 0);
		struct sockaddr_in audio_addr = {};
		audio_addr.sin_family = AF_INET;
		audio_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
		audio_addr.sin_port = htons(6001);

		if (bind(audio_sock, reinterpret_cast<const sockaddr *>(&audio_addr), sizeof(audio_addr)) <
		    0)
			throw std::runtime_error("Failed to bind UDP socket on 127.0.0.1:6001");

		setsockopt(audio_sock, SOL_SOCKET, SO_RCVBUF, reinterpret_cast<const char *>(&rcvBufSize),
		           sizeof(rcvBufSize));

		const rtc::SSRC audio_ssrc = 43;
		rtc::Description::Audio media_audio("audio", rtc::Description::Direction::SendOnly);
		media_audio.addOpusCodec(
		    111); // Must match the payload type of the external opus RTP stream
		media_audio.addSSRC(audio_ssrc, "audio-send");
		auto audio_track = pc->addTrack(media_audio);

        const rtc::SSRC video_ssrc = 42;
		rtc::Description::Video media_video("video", rtc::Description::Direction::SendOnly);
		media_video.addH264Codec(96); // Must match the payload type of the external h264 RTP stream
		media_video.addSSRC(video_ssrc, "video-send");
		auto video_track = pc->addTrack(media_video);

		pc->setLocalDescription();

		auto video_loop = std::async(std::launch::async, [&]() {
			// Receive from UDP
			char buffer[BUFFER_SIZE];
			int len;
			while ((len = recv(video_sock, buffer, BUFFER_SIZE, 0)) >= 0) {
				if (len < sizeof(rtc::RtpHeader) || !video_track->isOpen())
					continue;

				auto rtp = reinterpret_cast<rtc::RtpHeader *>(buffer);
				rtp->setSsrc(video_ssrc);

				video_track->send(reinterpret_cast<const std::byte *>(buffer), len);
			}
		});

		auto audio_loop = std::async(std::launch::async, [&]() {
			// Receive from UDP
			char buffer[BUFFER_SIZE];
			int len;
			while ((len = recv(audio_sock, buffer, BUFFER_SIZE, 0)) >= 0) {
				if (len < sizeof(rtc::RtpHeader) || !audio_track->isOpen())
					continue;

				auto rtp = reinterpret_cast<rtc::RtpHeader *>(buffer);
				rtp->setSsrc(audio_ssrc);

				audio_track->send(reinterpret_cast<const std::byte *>(buffer), len);
			}
		});

		video_loop.get();
		audio_loop.get();

	} catch (const std::exception &e) {
		std::cerr << "Error: " << e.what() << std::endl;
	}
}
