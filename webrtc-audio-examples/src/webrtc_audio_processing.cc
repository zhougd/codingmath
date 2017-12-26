#include <string>
#include <iostream>

#include "webrtc/modules/audio_processing/include/audio_processing.h"
#include "webrtc/modules/interface/module_common_types.h"

#define EXPECT_OP(op, val1, val2)                                       \
  do {                                                                  \
    if (!((val1) op (val2))) {                                          \
      fprintf(stderr, "Check failed: %s %s %s\n", #val1, #op, #val2);   \
      exit(1);                                                          \
    }                                                                   \
  } while (0)

#define EXPECT_EQ(val1, val2)  EXPECT_OP(==, val1, val2)
#define EXPECT_NE(val1, val2)  EXPECT_OP(!=, val1, val2)
#define EXPECT_GT(val1, val2)  EXPECT_OP(>, val1, val2)
#define EXPECT_LT(val1, val2)  EXPECT_OP(<, val1, val2)

int usage() {
    std::cout <<
              "Usage: webrtc-audio-process -anc n -agc n -aec n delay_ms input.wav output.wav echo_ref.wav"
              << std::endl;
    return 1;
}

bool ReadFrame(FILE* file, webrtc::AudioFrame* frame) {
    // The files always contain stereo audio.
    size_t frame_size = frame->samples_per_channel_;
    size_t read_count = fread(frame->data_,
                              sizeof(int16_t),
                              frame_size,
                              file);
    if (read_count != frame_size) {
        // Check that the file really ended.
        EXPECT_NE(0, feof(file));
        return false;  // This is expected.
    }
    return true;
}

bool WriteFrame(FILE* file, webrtc::AudioFrame* frame) {
    // The files always contain stereo audio.
    size_t frame_size = frame->samples_per_channel_;
    size_t read_count = fwrite (frame->data_,
                                sizeof(int16_t),
                                frame_size,
                                file);
    if (read_count != frame_size) {
        return false;  // This is expected.
    }
    return true;
}

int main(int argc, char **argv) {
    if (argc != 11) {
        return usage();
    }

    FILE *echo_in = NULL;
    int anc_level, agc_level, aec_level, delay_ms = -1;
    anc_level = atoi(argv[2]);
    agc_level = atoi(argv[4]);
    aec_level = atoi(argv[6]);
    delay_ms = atoi(argv[7]);
    std::string file_in = argv[8];
    std::string file_out = argv[9];
    std::string file_echo_ref = argv[10];
    bool use_aecm = (std::string(argv[5]) == "-aecm") ? 1:0;
    int ret;

    webrtc::AudioFrame *frame = new webrtc::AudioFrame();
    webrtc::AudioFrame *echo_ref_frame = NULL;
    float frame_step = 10;  // ms
    frame->sample_rate_hz_ = 16000;
    frame->samples_per_channel_ = frame->sample_rate_hz_ * frame_step / 1000.0;
    frame->num_channels_ = 1;

    webrtc::AudioProcessing* apm = webrtc::AudioProcessing::Create();
    apm->high_pass_filter()->Enable(true);

    std::cout << "ANC: level " << anc_level << std::endl;
    if (anc_level >= 0) {
        apm->noise_suppression()->Enable(true);
        switch (anc_level) {
        case 0:
            apm->noise_suppression()->set_level(webrtc::NoiseSuppression::kLow);
            break;
        case 1:
            apm->noise_suppression()->set_level(webrtc::NoiseSuppression::kModerate);
            break;
        case 2:
            apm->noise_suppression()->set_level(webrtc::NoiseSuppression::kHigh);
            break;
        case 3:
            apm->noise_suppression()->set_level(webrtc::NoiseSuppression::kVeryHigh);
            break;
        default:
            apm->noise_suppression()->set_level(webrtc::NoiseSuppression::kVeryHigh);
        }
        apm->voice_detection()->Enable(true);
    }
    std::cout << "AGC: model " << agc_level << std::endl;
    if (agc_level >= 0) {
        apm->gain_control()->Enable(true);
        apm->gain_control()->set_analog_level_limits(0, 255);
        switch (agc_level) {
        case 0:
            apm->gain_control()->set_mode(webrtc::GainControl::kAdaptiveAnalog);
            break;
        case 1:
            apm->gain_control()->set_mode(webrtc::GainControl::kAdaptiveDigital);
            break;
        case 2:
            apm->gain_control()->set_mode(webrtc::GainControl::kFixedDigital);
            break;
        default:
            apm->gain_control()->set_mode(webrtc::GainControl::kAdaptiveAnalog);
        }
    }
	
    std::cout << "AEC: level " << aec_level << std::endl;
    if (aec_level >= 0) {
        if (use_aecm) { 
            webrtc::EchoControlMobile *aecm = apm->echo_control_mobile();
	switch (aec_level) {
                case 0:
	        ret = aecm->set_routing_mode(webrtc::EchoControlMobile::kQuietEarpieceOrHeadset);
	        break;
	    case 1:
                   ret = aecm->set_routing_mode(webrtc::EchoControlMobile::kEarpiece); 
                   break;
                case 2:
                   ret = aecm->set_routing_mode(webrtc::EchoControlMobile::kLoudEarpiece); 
                   break;
                case 3:
                   ret = aecm->set_routing_mode(webrtc::EchoControlMobile::kSpeakerphone); 
                   break;
                case 4:
                   ret = aecm->set_routing_mode(webrtc::EchoControlMobile::kLoudSpeakerphone); 
                   break;
                default:
                    ret = aecm->set_routing_mode(webrtc::EchoControlMobile::kQuietEarpieceOrHeadset);
                    break;
	}
            std::cout << "set_routing_mode ret " << ret << std::endl;
            ret = aecm->enable_comfort_noise(false);
            std::cout << "enable_comfort_noise ret" << ret << std::endl;
	ret = aecm->Enable(true);
            std::cout << "Enable ret =" << ret << std::endl;
        } else {
            webrtc::EchoCancellation *aec = apm->echo_cancellation();
	ret = aec->Enable(true);
	std::cout << "Enable ret =" << ret << std::endl;
	switch (aec_level) {
		case 0:
			aec->set_suppression_level(webrtc::EchoCancellation::kLowSuppression);
			break;
		case 1:
			aec->set_suppression_level(webrtc::EchoCancellation::kModerateSuppression);
			break;
		case 2:
			aec->set_suppression_level(webrtc::EchoCancellation::kHighSuppression);
	}
        }

        apm->set_stream_delay_ms(delay_ms);
        std::cout << "delay " << delay_ms << std::endl;
        EXPECT_NE(echo_in = fopen(file_echo_ref.data(), "rb"), NULL);
        echo_ref_frame = new webrtc::AudioFrame();
        echo_ref_frame->sample_rate_hz_ = 16000;
        echo_ref_frame->num_channels_ = 1;
        echo_ref_frame->samples_per_channel_ = echo_ref_frame->sample_rate_hz_ * frame_step / 1000.0;
    }



    FILE *wav_in = fopen(file_in.data(), "rb");
    FILE *wav_out = fopen(file_out.data(), "wb");
    std::cout << "in " << file_in.data() << " out " <<file_out.data()<< " ref " <<  file_echo_ref.data() << std::endl;
    EXPECT_NE(wav_in, NULL);
    EXPECT_NE(wav_out, NULL);
    int num_frame = 0;
    while (ReadFrame(wav_in, frame)) {
        num_frame += 1;
        if (aec_level >= 0 ) {
            if (ReadFrame(echo_in, echo_ref_frame)) {
                ret = apm->ProcessReverseStream(echo_ref_frame);
                //std::cout << "ProcessReverseStream=" << ret << std::endl;
            }
	apm->set_stream_delay_ms(delay_ms);
        }
        ret = apm->ProcessStream(frame);
        //std::cout << "ProcessStream=" << ret << std::endl;
        WriteFrame(wav_out, frame);
    }
    fclose(wav_in);
    fclose(wav_out);
    if (echo_in) {
        fclose(echo_in);
    }

    delete frame;
    if (echo_ref_frame != NULL)
        delete echo_ref_frame;

    delete apm;

    return 0;
}
