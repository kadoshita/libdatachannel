# This code is based on the libdatachannel media-sender example.
# https://github.com/paullouisageneau/libdatachannel/tree/master/examples/media-sender
# The license of the media-sender code is as follows.

# libdatachannel media sender example
# Copyright (c) 2020 Staz Modrzynski
# Copyright (c) 2020 Paul-Louis Ageneau
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at https://mozilla.org/MPL/2.0/.

cmake_minimum_required(VERSION 3.7)

add_executable(datachannel-whip main.cpp)

set_target_properties(datachannel-whip PROPERTIES
    CXX_STANDARD 17
    OUTPUT_NAME whip)

set_target_properties(datachannel-whip PROPERTIES
	XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER com.github.paullouisageneau.libdatachannel.examples.whip)

find_package(Threads REQUIRED)
target_link_libraries(datachannel-whip LibDataChannel::LibDataChannel Threads::Threads nlohmann_json::nlohmann_json)
target_include_directories(datachannel-whip PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cpp-httplib)
