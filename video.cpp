#include "video.h"

bool PERF_LOGGING = false;

atomic<long long> IdGen::counter{0LL};

// OpResult implementation
OpResult::OpResult(OpStatus s, const string& m, long long i) 
    : status(s), message(m), id(i) {}

bool OpResult::isSuccess() const { 
    return status == OpStatus::SUCCESS; 
}

// PerfTimer implementation
PerfTimer::PerfTimer(const string& op, bool enable) 
    : operation(op), enabled(enable) {
    if (enabled) start = chrono::high_resolution_clock::now();
}

PerfTimer::~PerfTimer() {
    if (enabled) {
        auto end = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::microseconds>(end - start).count();
        cout << "[PERF] " << operation << ": " << duration << " μs\n";
    }
}

// IdGen implementation
long long IdGen::next() { 
    return ++counter; 
}

// Logger implementation
void Logger::log(Level level, const string& msg) {
    switch(level) {
        case INFO: cout << "[INFO] " << msg << "\n"; break;
        case WARNING: cout << "[WARN] " << msg << "\n"; break;
        case ERROR: cout << "[ERROR] " << msg << "\n"; break;
        case PERF: cout << "[PERF] " << msg << "\n"; break;
    }
}

void Logger::info(const string& msg) { log(INFO, msg); }
void Logger::warn(const string& msg) { log(WARNING, msg); }
void Logger::error(const string& msg) { log(ERROR, msg); }

// Comment implementation
Comment::Comment() = default;

Comment::Comment(const string& a, const string& t)
    : id(IdGen::next()), author(a), text(t), likes(0) {
    ts = chrono::duration_cast<chrono::milliseconds>(
             chrono::system_clock::now().time_since_epoch()).count();
}

long long Comment::getId() const { return id; }
const string& Comment::getAuthor() const { return author; }
const string& Comment::getText() const { return text; }
int Comment::getLikes() const { return likes; }
void Comment::like() { likes++; }

// Video implementation
Video::Video() = default;

Video::Video(const string& t, const string& u, int d)
    : id(IdGen::next()), title(t), uploader(u), durationSec(d), views(0), playing(false) {}

long long Video::getId() const { return id; }
const string& Video::getTitle() const { return title; }
const string& Video::getUploader() const { return uploader; }
long long Video::getViews() const { return views; }

OpResult Video::play() {
    PerfTimer timer("Video::play", PERF_LOGGING);
    
    if (!playing) {
        playing = true;
        ++views;
        return OpResult(OpStatus::SUCCESS, 
            "Playing \"" + title + "\" (views: " + to_string(views) + ")");
    }
    return OpResult(OpStatus::ALREADY_EXISTS, 
        "Already playing \"" + title + "\"");
}

OpResult Video::pause() {
    if (playing) {
        playing = false;
        return OpResult(OpStatus::SUCCESS, "Paused \"" + title + "\"");
    }
    return OpResult(OpStatus::INVALID_INPUT, "Not playing \"" + title + "\"");
}

OpResult Video::addComment(const string& user, const string& text) {
    PerfTimer timer("Video::addComment", PERF_LOGGING);
    
    comments.emplace_back(user, text);
    long long cid = comments.back().getId();
    return OpResult(OpStatus::SUCCESS, 
        "Comment added by " + user, cid);
}

OpResult Video::likeComment(long long cid) {
    PerfTimer timer("Video::likeComment", PERF_LOGGING);
    
    // We do a linear search here, could use a hash map if comment counts get huge
    for (auto &c : comments) {
        if (c.getId() == cid) {
            c.like();
            return OpResult(OpStatus::SUCCESS, 
                "Liked comment " + to_string(cid) + " (likes=" + to_string(c.getLikes()) + ")");
        }
    }
    return OpResult(OpStatus::NOT_FOUND, "Comment not found");
}

OpResult Video::removeComment(long long cid, const string& requester, const string& channelOwner) {
    for (auto it = comments.begin(); it != comments.end(); ++it) {
        if (it->getId() == cid) {
            // Only the comment author or channel owner can delete
            if (requester == it->getAuthor() || requester == channelOwner) {
                comments.erase(it);
                return OpResult(OpStatus::SUCCESS, "Comment removed");
            }
            return OpResult(OpStatus::PERMISSION_DENIED, "Permission denied");
        }
    }
    return OpResult(OpStatus::NOT_FOUND, "Comment not found");
}

void Video::listComments() const {
    if (comments.empty()) {
        cout << "No comments\n";
        return;
    }
    cout << "Comments for \"" << title << "\":\n";
    for (const auto &c : comments) {
        cout << "  [" << c.getId() << "] " << c.getAuthor() 
             << " (" << c.getLikes() << " likes): " << c.getText() << "\n";
    }
}

// Channel implementation
Channel::Channel() = default;

Channel::Channel(const string& n, const string& o, const string& d)
    : name(n), owner(o), description(d) {}

const string& Channel::getName() const { return name; }
const string& Channel::getOwner() const { return owner; }

Video* Channel::upload(const string& title, int dur) {
    PerfTimer timer("Channel::upload", PERF_LOGGING);
    
    auto v = make_unique<Video>(title, name, dur);
    Video* ptr = v.get();
    uploads.push_back(move(v));
    Logger::info("Uploaded \"" + title + "\" (id=" + to_string(ptr->getId()) + 
                 ") to channel " + name);
    return ptr;
}

OpResult Channel::subscribe(const string& user) {
    if (subscribers.insert(user).second) {
        return OpResult(OpStatus::SUCCESS, user + " subscribed to " + name);
    }
    return OpResult(OpStatus::ALREADY_EXISTS, user + " already subscribed");
}

OpResult Channel::unsubscribe(const string& user) {
    if (subscribers.erase(user)) {
        return OpResult(OpStatus::SUCCESS, user + " unsubscribed from " + name);
    }
    return OpResult(OpStatus::NOT_FOUND, user + " was not subscribed");
}

void Channel::listUploads() const {
    if (uploads.empty()) {
        cout << "No uploads\n";
        return;
    }
    cout << "Uploads for channel " << name << ":\n";
    for (const auto &v : uploads) {
        cout << "  [" << v->getId() << "] " << v->getTitle() 
             << " (views: " << v->getViews() << ")\n";
    }
}

// Playlist implementation
Playlist::Playlist() = default;
Playlist::Playlist(const string& n): name(n) {}

void Playlist::add(long long videoId, const string& videoTitle) {
    videoIds.push_back(videoId);
    Logger::info("Added \"" + videoTitle + "\" to playlist \"" + name + "\"");
}

const vector<long long>& Playlist::getVideoIds() const { return videoIds; }
const string& Playlist::getName() const { return name; }

void Playlist::show(const unordered_map<long long, Video*>& videoMap) const {
    cout << "Playlist: " << name << "\n";
    if (videoIds.empty()) {
        cout << "  (empty)\n";
        return;
    }
    for (size_t i = 0; i < videoIds.size(); ++i) {
        auto it = videoMap.find(videoIds[i]);
        if (it != videoMap.end()) {
            cout << "  [" << (i+1) << "] " << it->second->getTitle() 
                 << " (id=" << it->second->getId() << ")\n";
        }
    }
}
