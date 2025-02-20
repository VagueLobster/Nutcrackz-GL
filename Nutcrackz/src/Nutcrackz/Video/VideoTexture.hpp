#pragma once

#include "Nutcrackz/Utils/Intrusive.hpp"

#include "Nutcrackz/Core/Core.hpp"
#include "Nutcrackz/Renderer/Texture.hpp"
#include "Nutcrackz/Asset/Asset.hpp"

#include "miniaudio.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/error.h>
#include <libavutil/avutil.h>
#include <libswresample/swresample.h>
#include <libavutil/audio_fifo.h>
}

#include <filesystem>

namespace Nutcrackz {

	struct VideoReaderState
	{
		int Width = 0, Height = 0;
		double Duration = 0.0;
		int64_t VideoPacketDuration = 0;
		int64_t AudioPacketDuration = 0;
		int64_t AVPacketDuration = 0;
		int Hours = 0, Mins = 0, Secs = 0, Us = 0;
		double Framerate = 0.0;
		int64_t NumberOfFrames = 0;
		int VideoStreamIndex = -1;
		int AudioStreamIndex = -1;
		int AudioVideoStreamIndex = -1;

		AVRational TimeBase = AVRational();
		AVFormatContext* VideoFormatContext = nullptr;
		AVCodecContext* VideoCodecContext = nullptr;
		AVFrame* VideoFrame = nullptr;
		AVPacket* VideoPacket = nullptr;
		AVStream* VideoStream = nullptr;

		AVFormatContext* AudioFormatContext = nullptr;
		AVCodecContext* AudioCodecContext = nullptr;
		AVFrame* AudioFrame = nullptr;
		AVPacket* AudioPacket = nullptr;
		AVStream* AudioStream = nullptr;
		AVAudioFifo* AudioFifo = nullptr;

		AVFormatContext* AudioVideoFormatContext = nullptr;
		AVCodecContext* AudioVideoCodecContext = nullptr;
		AVFrame* AudioVideoFrame = nullptr;
		AVPacket* AudioVideoPacket = nullptr;
		AVStream* AudioVideoStream = nullptr;
		AVAudioFifo* AudioVideoFifo = nullptr;

		int64_t Pts = 0;
	};

	/*struct AVFrameRingStream final : Utils::ThreadSafeIntrusivePtrEnabled<AVFrameRingStream>
	{
		AVFrameRingStream(float sample_rate, unsigned num_channels, double timebase, bool support_resample = false, bool blocking_mix = false);
		~AVFrameRingStream();

		float sample_rate;
		float out_sample_rate;
		float resampling_ratio = 1.0f;
		unsigned num_channels;
		double timebase;
		double inv_sample_rate_ns;
		bool blocking_mix;

		void set_rate_factor(float factor);
		float get_rate_factor() const noexcept;

		uint32_t get_underflow_counter() const;

		bool setup(float mixer_output_rate, unsigned mixer_channels, size_t max_num_frames);
		size_t accumulate_samples(float* const* channels, const float* gain, size_t num_frames) noexcept;
		size_t accumulate_samples_inner(float* const* channels, const float* gain, size_t num_frames) noexcept;
		unsigned get_num_channels() const;
		float get_sample_rate() const;
		void dispose();

		uint32_t get_current_write_count();

		// Buffering in terms of AVFrame is a little questionable since packet sizes can vary a fair bit,
		// might have to revisit later.
		// In practice, any codec will have a reasonably short packet window (10ms - 20ms),
		// but not too long either.
		enum { Frames = 64, FramesHighWatermark = 48 };
		AVFrame* frames[Frames] = {};
		std::atomic_uint32_t write_count;
		std::atomic_uint32_t read_count;
		std::atomic_uint32_t read_frames_count;
		std::atomic_uint32_t write_frames_count;
		std::atomic_uint32_t rate_factor_u32;
		std::atomic_uint32_t underflows;
		std::atomic_bool complete;
		int packet_frames = 0;
		bool running_state = false;
		unsigned get_num_buffered_audio_frames();
		unsigned get_num_buffered_av_frames();

		enum { MaxChannels = 8 };
		//std::unique_ptr<Audio::DSP::SincResampler> resamplers[MaxChannels];
		std::vector<float> tmp_resampler_buffer[MaxChannels];
		float* tmp_resampler_ptrs[MaxChannels] = {};

		struct
		{
			double pts = -1.0;
			int64_t sampled_ns = 0;
		} progress[Frames];
		std::atomic_uint32_t pts_index;

		AVFrame* acquire_write_frame();
		void mark_uncorked_audio_pts();
		void submit_write_frame();
		void mark_complete();

		std::condition_variable cond;
		std::mutex lock;
	};*/

	class VideoTexture : public Asset
	{
	public:
		VideoTexture(const TextureSpecification& specification, Buffer data = Buffer(), const VideoReaderState& state = VideoReaderState());

		~VideoTexture();

		static AssetType GetStaticType() { return AssetType::Video; }
		virtual AssetType GetType() const { return GetStaticType(); }

		uint32_t GetIDFromTexture(uint8_t* frameData, int64_t* pts, bool& seek, bool isPaused, const std::filesystem::path& filepath, double ts, double& outMilliseconds);
		void DeleteRendererID(const uint32_t& rendererID);

		static bool VideoReaderOpen(VideoReaderState* state, const std::filesystem::path& filepath);
		bool VideoReaderReadFrame(VideoReaderState* state, uint8_t* frameBuffer, int64_t* pts, bool isPaused, double ts, double& outMilliseconds);
		bool VideoReaderSeekFrame(VideoReaderState* state, double ts);
		static bool AudioReaderOpen(VideoReaderState* state, const std::filesystem::path& filepath);
		bool AudioReaderReadFrame(VideoReaderState* state, bool isPaused, int64_t ts);
		bool AudioReaderSeekFrame(VideoReaderState* state, double ts, bool resetAudio = false);
		static bool AVReaderOpen(VideoReaderState* state, const std::filesystem::path& filepath);
		bool AVReaderReadFrame(VideoReaderState* state, uint8_t* frameBuffer, int64_t* pts, bool isPaused, double ts);
		bool AVReaderSeekFrame(VideoReaderState* state, double ts, bool resetAudio = false);
		void PauseAudio(bool isPaused);
		void CloseVideo(VideoReaderState* state);
		void CloseAudio(VideoReaderState* state);

		void ReadAndPlayAudio(VideoReaderState* state, int64_t* pts, double ts, bool& seek, bool isPaused, const std::filesystem::path& filepath);
		void ResetAudioPacketDuration(VideoReaderState* state);

		float GetVolumeFactor() { return m_Volume; }
		void SetVolumeFactor(float volume);

		VideoReaderState& GetVideoState() { return m_VideoState; }
		void SetVideoState(const VideoReaderState& state) { m_VideoState = state; }

		bool HasLoadedAudio() { return m_HasLoadedAudio; }
		uint32_t GetWidth() const { return m_Width; }
		uint32_t GetHeight() const { return m_Height; }
		uint32_t GetRendererID() const { return m_RendererID; }


		void SetRendererID(uint32_t id);
		void SetData(Buffer data);

		void Bind(uint32_t slot = 0);

		bool operator==(const VideoTexture& other) const
		{
			return m_RendererID == other.GetRendererID();
		}

		static RefPtr<VideoTexture> Create(const TextureSpecification& specification, Buffer data = Buffer(), const VideoReaderState& state = VideoReaderState());

	private:
		TextureSpecification m_Specification;
		uint32_t m_Width = 0, m_Height = 0;
		uint32_t m_RendererID = 0;

		VideoReaderState m_VideoState;

		bool m_IsVideoLoaded = false;
		bool m_HasLoadedAudio = false;
		bool m_InitializedAudio = false;

		bool m_SeekToFrame = false;

		std::string m_Filepath = "";

		//std::condition_variable m_VideoCondition;
		//std::mutex m_VideoLock;
		//std::condition_variable m_AudioCondition;
		//std::mutex m_AudioLock;

		double m_SeekToSeconds = 0.0;

		ma_device m_AudioDevice;
		float m_Volume = 100.0f;
	};

}
