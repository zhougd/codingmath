#!/bin/sh


if [ ! -d webrtc-audio-processing ];then
  git clone git://anongit.freedesktop.org/pulseaudio/webrtc-audio-processing
fi

if [ ! -d webrtc-audio-processing/release ];then
  cd webrtc-audio-processing
  mv RELEASE release.txt >/dev/null
  ./autogen.sh --prefix=`pwd`/release && make && make install && cd -
fi

mkdir -p bin; cd bin; cmake ../; make || exit 1 
cd -

set -e

function anc_func()
{
echo "============= WebRTC ANC ============="
rm -f data/*webrtc_anc*
sox data/addednoise.wav data/addednoise.raw || exit 1;
for x in 0 1 2 3;do
  echo -n "WebRTC ANC $x: "
  ./bin/webrtc-audio-process -anc $x data/addednoise.raw data/addednoise_webrtc_anc_$x.raw || exit 1;
  sox -r 16k -t raw -b 16 -c 1 -e signed-intege data/addednoise_webrtc_anc_$x.raw data/addednoise_webrtc_anc_$x.wav || exit 1;
  rm data/addednoise_webrtc_anc_$x.raw
done
}
function agc_func()
{
echo "============= WebRTC AGC ============="
sox data/speech_16k.wav data/speech_16k.raw
rm -f data/*webrtc_agc*
for x in 0 1 2;do
  echo -n "WebRTC AGC $x: "
  ./bin/webrtc-audio-process -agc $x data/speech_16k.raw data/speech_16k_webrtc_agc_$x.raw || exit 1;
  sox -r 16k -t raw -b 16 -c 1 -e signed-intege data/speech_16k_webrtc_agc_$x.raw \
    data/speech_16k_webrtc_agc_$x.wav || exit 1;
  rm -f data/speech_16k_webrtc_agc_$x.raw
done
}
echo "============= WebRTC AEC ============="
rm -rf data/tmp
rm -rf data/output
rm -rf data/input
rm -rf data/ref
mkdir data/tmp
mkdir data/input
mkdir data/output
mkdir data/ref
delay=100
#sox data/100wakeup.wav -p synth whitenoise vol 0.02 | sox -m data/100wakeup.wav - data/tmp/100wakeup_addednoise.wav
sox data/100wakeup.wav data/tmp/100wakeup_addednoise.wav
cp data/xiayutian.wav data/ref/echo.wav
sox -v 0.5 data/ref/echo.wav data/tmp/music-volume-down.wav
sox data/100ms.wav data/tmp/music-volume-down.wav data/tmp/music-add-delay.wav
sox -m data/tmp/music-add-delay.wav data/tmp/100wakeup_addednoise.wav data/input/speech_16k_echo.wav
sox data/input/speech_16k_echo.wav data/input/speech_16k_echo.raw
sox data/ref/echo.wav data/ref/echo.raw

for anc_l in -1;do
for agc_l in 1;do
for aec_l in 0 1;do
  echo -n "WebRTC anc $anc_l agc $agc_l aec $aec_l: "
  ./bin/webrtc-audio-process -anc $anc_l -agc $agc_l -aec $aec_l $delay data/input/speech_16k_echo.raw data/output/speech_16k_$anc_l-$agc_l-$aec_l.raw \
      data/ref/echo.raw || exit 1;
  sox -r 16k -t raw -b 16 -c 1 -e signed-intege data/output/speech_16k_$anc_l-$agc_l-$aec_l.raw \
      data/output/speech_16k_webrtc_$anc_l-$agc_l-$aec_l.wav || exit 1;
  rm -f data/output/speech_16k_webrtc_$anc_l-$agc_l-$aec_l.raw
done
done
done
