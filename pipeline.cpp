#include <iostream>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>
#include <chrono>
#include <atomic>
#include <opencv2/opencv.hpp>
#include <onnxruntime_cxx_api.h>

class StreamFilterEngine {
private:
    cv::Mat spatial_mask_;
    float alpha_temporal_;
    cv::Mat temporal_history_;

public:
    StreamFilterEngine(int width, int height, float alpha = 0.3f)
        : alpha_temporal_(alpha) {
        spatial_mask_ = cv::Mat::ones(height, width, CV_8UC1);
        cv::rectangle(spatial_mask_, cv::Point(0, 0), cv::Point(width / 4, height / 4), cv::Scalar(0), -1);
    }

    void applySpatialMask(cv::Mat& frame) {
        cv::bitwise_and(frame, frame, frame, spatial_mask_);
    }

    cv::Mat applyTemporalSmoothing(const cv::Mat& current_frame) {
        if (temporal_history_.empty()) {
            current_frame.convertTo(temporal_history_, CV_32FC3);
            return current_frame;
        }

        cv::Mat current_float;
        current_frame.convertTo(current_float, CV_32FC3);
        cv::accumulateWeighted(current_float, temporal_history_, alpha_temporal_);

        cv::Mat smoothed_output;
        temporal_history_.convertTo(smoothed_output, CV_8UC3);
        return smoothed_output;
    }
};

class MultiStreamPipeline {
private:
    std::vector<std::string> stream_sources_;
    std::vector<std::queue<cv::Mat>> frame_queues_;
    std::vector<std::mutex> queue_mutexes_;
    std::atomic<bool> is_running_{false};
    std::vector<std::thread> worker_threads_;

    void captureLoop(size_t channel_idx, const std::string& source) {
        cv::VideoCapture cap(source);
        if (!cap.isOpened()) return;

        cv::Mat frame;
        while (is_running_) {
            if (!cap.read(frame) || frame.empty()) {
                cap.set(cv::CAP_PROP_POS_FRAMES, 0);
                continue;
            }

            std::lock_guard<std::mutex> lock(queue_mutexes_[channel_idx]);
            if (frame_queues_[channel_idx].size() >= 10) {
                frame_queues_[channel_idx].pop();
            }
            frame_queues_[channel_idx].push(frame.clone());
        }
        cap.release();
    }

public:
    MultiStreamPipeline(const std::vector<std::string>& sources)
        : stream_sources_(sources),
          frame_queues_(sources.size()),
          queue_mutexes_(sources.size()) {}

    void start() {
        is_running_ = true;
        for (size_t i = 0; i < stream_sources_.size(); ++i) {
            worker_threads_.emplace_back(&MultiStreamPipeline::captureLoop, this, i, stream_sources_[i]);
        }
    }

    bool getLatestFrame(size_t channel_idx, cv::Mat& out_frame) {
        std::lock_guard<std::mutex> lock(queue_mutexes_[channel_idx]);
        if (frame_queues_[channel_idx].empty()) return false;
        out_frame = frame_queues_[channel_idx].front();
        frame_queues_[channel_idx].pop();
        return true;
    }

    void stop() {
        is_running_ = false;
        for (auto& t : worker_threads_) {
            if (t.joinable()) t.join();
        }
    }

    ~MultiStreamPipeline() { stop(); }
};

int main(int argc, char** argv) {
    std::vector<std::string> streams = {"stream1.mp4", "stream2.mp4"};
    MultiStreamPipeline pipeline(streams);
    pipeline.start();

    StreamFilterEngine filter(640, 640);
    uint64_t processed_frames = 0;
    auto start_time = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 500; ++i) {
        for (size_t ch = 0; ch < streams.size(); ++ch) {
            cv::Mat frame;
            if (pipeline.getLatestFrame(ch, frame)) {
                filter.applySpatialMask(frame);
                cv::Mat filtered = filter.applyTemporalSmoothing(frame);
                processed_frames++;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start_time;

    std::cout << "[+] Pipeline Run Complete.\n";
    std::cout << "Processed: " << processed_frames << " frames in " << elapsed.count() << "s (" << (elapsed.count() > 0 ? processed_frames / elapsed.count() : 0) << " FPS)\n";

    pipeline.stop();
    return 0;
}
