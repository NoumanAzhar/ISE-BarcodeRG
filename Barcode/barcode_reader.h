#pragma once
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <opencv2/core.hpp>

// Member 5: single entry point for reading a bar code from an image file.

// Loads the image at `path`, runs extractRuns() (image_prep.h) and then
// decodeRuns() (barcode_decode.h), and returns the 6-digit string.
// Returns std::nullopt if the file cannot be loaded or decoding fails.
std::optional<std::string> readBarcodeFile(const std::string& path);

// Result of one camera frame: the number (if read) and where the bar code
// was found, so the UI can draw a frame around it.
struct CameraScanResult {
    std::optional<std::string> value;   // 6 digits, empty if not read in this frame
    cv::Rect box;                       // area that looks like a bar code, only for aiming (empty = none found)
    cv::Rect readBox;                   // area the number was read from (empty = not read, or read from the strip sweep)
};

// Camera reading. It first finds WHERE the bar code is, then reads it:
//  1. the frame is cut into small cells; cells with strong left-right edges
//     and weak up-down edges look like bars, touching cells form a region
//     (also tried on a 90-degree turned frame for upright bar codes),
//  2. the tilt of each region is found by testing angles and keeping the one
//     that makes the bars sharpest, and the region is rotated level,
//  3. several scan lines are read, each averaged over a few rows and cut to
//     the part with dense bar edges, with a few thresholding variants,
//  4. if nothing is found, thin strips across the whole frame are tried.
// The checksum in decodeRuns() means a wrong number is practically never returned.
CameraScanResult scanCameraFrame(const cv::Mat& frame);

// Same as scanCameraFrame() but only returns the number.
std::optional<std::string> readBarcodeFrame(const cv::Mat& frame);

// ---------------------------------------------------------------------------
// Live webcam reading that does not freeze the UI.
//
// scanCameraFrame() is slow (it can take 100+ ms), so it must not run in the
// UI thread. CameraReader owns the camera and two background threads:
//   - capture thread: opens the camera and keeps grabbing frames,
//   - scan thread:    always takes the NEWEST frame, runs scanCameraFrame() on
//                     it and stores the result (frames that arrive while it
//                     is busy are skipped, so nothing piles up).
// The UI only calls poll() from a timer: it returns at once with the newest
// picture and the newest scan result. No Qt is used in here.
class CameraReader {
public:
    struct Snapshot {
        cv::Mat frame;          // newest picture, 8-bit BGR (empty while the camera starts)
        bool newFrame = false;  // true if `frame` is new since the last poll()
        CameraScanResult scan;  // result of the newest finished scan
        int scanId = 0;         // grows by one for every finished scan
        bool failed = false;    // the camera could not be opened / stopped working
        std::string error;      // message for the user when failed
    };

    CameraReader() = default;
    ~CameraReader();                                   // calls stop()
    CameraReader(const CameraReader&) = delete;
    CameraReader& operator=(const CameraReader&) = delete;

    void start(int cameraIndex = 0);   // returns at once; the camera opens in the background
    void stop();                       // ends both threads (waits for a running scan)
    Snapshot poll();                   // cheap, call it from the UI timer

private:
    void captureLoop(int cameraIndex);
    void scanLoop();

    std::thread m_captureThread;
    std::thread m_scanThread;
    std::atomic<bool> m_stop{ false };

    std::mutex m_mutex;                // protects everything below
    std::condition_variable m_newFrame;
    cv::Mat m_frame;                   // newest picture (never modified once stored)
    int m_frameId = 0;                 // counts stored pictures
    int m_scannedId = 0;               // id of the picture the scan thread took last
    int m_polledId = 0;                // id of the picture poll() returned last
    CameraScanResult m_scan;
    int m_scanId = 0;
    bool m_failed = false;
    std::string m_error;
};
