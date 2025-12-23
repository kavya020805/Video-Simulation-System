#ifndef VIDEO_H
#define VIDEO_H

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <chrono>
#include <atomic>

using namespace std;

// We use this enum to handle errors consistently instead of just printing messages everywhere
enum class OpStatus {
    SUCCESS,
    NOT_FOUND,
    ALREADY_EXISTS,
    PERMISSION_DENIED,
    INVALID_INPUT,
    NOT_LOGGED_IN
};

// This wraps operation results so we can return both status and useful info
struct OpResult {
    OpStatus status;
    string message;
    long long id = -1;
    
    OpResult(OpStatus s, const string& m = "", long long i = -1);
    bool isSuccess() const;
};

// Simple timer to measure how long operations take
class PerfTimer {
private:
    chrono::high_resolution_clock::time_point start;
    string operation;
    bool enabled;
public:
    PerfTimer(const string& op, bool enable = true);
    ~PerfTimer();
};

// Toggle this to see performance measurements
extern bool PERF_LOGGING;

// Thread-safe ID generator using atomic operations
class IdGen {
private:
    static atomic<long long> counter;
public:
    static long long next();
};

// Centralized logging to keep output consistent
class Logger {
public:
    enum Level { INFO, WARNING, ERROR, PERF };
    
    static void log(Level level, const string& msg);
    static void info(const string& msg);
    static void warn(const string& msg);
    static void error(const string& msg);
};

// Represents a comment on a video with likes and timestamp
class Comment {
private:
    long long id;
    string author;
    string text;
    int likes;
    long long ts;
public:
    Comment();
    Comment(const string& a, const string& t);

    long long getId() const;
    const string& getAuthor() const;
    const string& getText() const;
    int getLikes() const;
    void like();
};

// Video class handles playback, views, and comments
class Video {
private:
    long long id;
    string title;
    string uploader;
    int durationSec;
    long long views;
    bool playing;
    vector<Comment> comments;

public:
    Video();
    Video(const string& t, const string& u, int d);

    long long getId() const;
    const string& getTitle() const;
    const string& getUploader() const;
    long long getViews() const;

    OpResult play();
    OpResult pause();
    OpResult addComment(const string& user, const string& text);
    OpResult likeComment(long long cid);
    OpResult removeComment(long long cid, const string& requester, const string& channelOwner);
    void listComments() const;
};

// Channel owns videos and manages subscribers
class Channel {
private:
    string name;
    string owner;
    string description;
    vector<unique_ptr<Video>> uploads;
    unordered_set<string> subscribers;

public:
    Channel();
    Channel(const string& n, const string& o, const string& d = "");
    
    // Enable move semantics for efficiency
    Channel(Channel&&) = default;
    Channel& operator=(Channel&&) = default;

    const string& getName() const;
    const string& getOwner() const;

    Video* upload(const string& title, int dur);
    OpResult subscribe(const string& user);
    OpResult unsubscribe(const string& user);
    void listUploads() const;
};

// Playlist stores video IDs instead of pointers to avoid ownership issues
class Playlist {
private:
    string name;
    vector<long long> videoIds;

public:
    Playlist();
    Playlist(const string& n);

    void add(long long videoId, const string& videoTitle);
    const vector<long long>& getVideoIds() const;
    const string& getName() const;
    void show(const unordered_map<long long, Video*>& videoMap) const;
};

#endif
