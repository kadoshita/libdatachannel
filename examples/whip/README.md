# Example of WHIP

This is a WHIP example using libdatachannel.

This code is based on the libdatachannel media-sender example.

https://github.com/paullouisageneau/libdatachannel/tree/master/examples/media-sender

The license of the media-sender code is as follows.

```
libdatachannel media sender example
Copyright (c) 2020 Staz Modrzynski
Copyright (c) 2020 Paul-Louis Ageneau
 *
This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at https://mozilla.org/MPL/2.0/.
```

## How to use

Set each values in main.cpp and build the application.

- `WHIP_ENDPOINT_DOMAIN`
- `WHIP_ENDPOINT_PORT`
- `WHIP_ENDPOINT_PATH`

Open two termials and run the following commands:

```
$ gst-launch-1.0 autovideosrc name=src0 ! video/x-raw,width=640,height=480 ! videoconvert ! queue ! x264enc tune=zerolatency bitrate=1000 key-int-max=30 ! video/x-h264, profile=constrained-baseline ! rtph264pay pt=96 mtu=1200 ! udpsink host=127.0.0.1 port=6000

$ gst-launch-1.0 audiotestsrc ! audioresample ! audio/x-raw,channels=1,rate=16000 ! opusenc bitrate=20000 ! rtpopuspay pt=111 ! udpsink host=127.0.0.1 port=6001
```

Run appliction.

```sh
$ cd build/examples/whip
$ ./whip
```
