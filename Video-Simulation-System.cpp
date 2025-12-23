#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <chrono>
#include <atomic>
#include <limits>
#include <mutex>
#include <iomanip>

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
    
    OpResult(OpStatus s, const string& m = "", long long i = -1) 
        : status(s), message(m), id(i) {}
    
    bool isSuccess() const { return status == OpStatus::SUCCESS; }
};

// Simple timer to measure how long operations take
class PerfTimer {
private:
    chrono::high_resolution_clock::time_point start;
    string operation;
    bool enabled;
public:
    PerfTimer(const string& op, bool enable = true) 
        : operation(op), enabled(enable) {
        if (enabled) start = chrono::high_resolution_clock::now();
    }
    
    ~PerfTimer() {
        if (enabled) {
            auto end = chrono::high_resolution_clock::now();
            auto duration = chrono::duration_cast<chrono::microseconds>(end - start).count();
            cout << "[PERF] " << operation << ": " << duration << " μs\n";
        }
    }
};

// Toggle this to see performance measurements
static bool PERF_LOGGING = false;

// Thread-safe ID generator using atomic operations
class IdGen {
private:
    static atomic<long long> counter;
public:
    static long long next() { return ++counter; }
};
atomic<long long> IdGen::counter{0LL};

// Centralized logging to keep output consistent
class Logger {
public:
    enum Level { INFO, WARNING, ERROR, PERF };
    
    static void log(Level level, const string& msg) {
        switch(level) {
            case INFO: cout << "[INFO] " << msg << "\n"; break;
            case WARNING: cout << "[WARN] " << msg << "\n"; break;
            case ERROR: cout << "[ERROR] " << msg << "\n"; break;
            case PERF: cout << "[PERF] " << msg << "\n"; break;
        }
    }
    
    static void info(const string& msg) { log(INFO, msg); }
    static void warn(const string& msg) { log(WARNING, msg); }
    static void error(const string& msg) { log(ERROR, msg); }
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
    Comment() = default;
    Comment(const string& a, const string& t)
        : id(IdGen::next()), author(a), text(t), likes(0) {
        ts = chrono::duration_cast<chrono::milliseconds>(
                 chrono::system_clock::now().time_since_epoch()).count();
    }

    long long getId() const { return id; }
    const string& getAuthor() const { return author; }
    const string& getText() const { return text; }
    int getLikes() const { return likes; }
    void like() { likes++; }
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
    vector<Comment> comments;  // Using vector for cache-friendly contiguous memory

public:
    Video() = default;
    Video(const string& t, const string& u, int d)
        : id(IdGen::next()), title(t), uploader(u), durationSec(d), views(0), playing(false) {}

    long long getId() const { return id; }
    const string& getTitle() const { return title; }
    const string& getUploader() const { return uploader; }
    long long getViews() const { return views; }

    OpResult play() {
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

    OpResult pause() {
        if (playing) {
            playing = false;
            return OpResult(OpStatus::SUCCESS, "Paused \"" + title + "\"");
        }
        return OpResult(OpStatus::INVALID_INPUT, "Not playing \"" + title + "\"");
    }

    OpResult addComment(const string& user, const string& text) {
        PerfTimer timer("Video::addComment", PERF_LOGGING);
        
        comments.emplace_back(user, text);
        long long cid = comments.back().getId();
        return OpResult(OpStatus::SUCCESS, 
            "Comment added by " + user, cid);
    }

    OpResult likeComment(long long cid) {
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

    OpResult removeComment(long long cid, const string& requester, const string& channelOwner) {
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

    void listComments() const {
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
};

// Channel owns videos and manages subscribers
class Channel {
private:
    string name;
    string owner;
    string description;
    vector<unique_ptr<Video>> uploads;  // Using unique_ptr for automatic memory management
    unordered_set<string> subscribers;

public:
    Channel() = default;
    Channel(const string& n, const string& o, const string& d = "")
        : name(n), owner(o), description(d) {}
    
    // Enable move semantics for efficiency
    Channel(Channel&&) = default;
    Channel& operator=(Channel&&) = default;

    const string& getName() const { return name; }
    const string& getOwner() const { return owner; }

    Video* upload(const string& title, int dur) {
        PerfTimer timer("Channel::upload", PERF_LOGGING);
        
        auto v = make_unique<Video>(title, name, dur);
        Video* ptr = v.get();
        uploads.push_back(move(v));
        Logger::info("Uploaded \"" + title + "\" (id=" + to_string(ptr->getId()) + 
                     ") to channel " + name);
        return ptr;
    }

    OpResult subscribe(const string& user) {
        if (subscribers.insert(user).second) {
            return OpResult(OpStatus::SUCCESS, user + " subscribed to " + name);
        }
        return OpResult(OpStatus::ALREADY_EXISTS, user + " already subscribed");
    }

    OpResult unsubscribe(const string& user) {
        if (subscribers.erase(user)) {
            return OpResult(OpStatus::SUCCESS, user + " unsubscribed from " + name);
        }
        return OpResult(OpStatus::NOT_FOUND, user + " was not subscribed");
    }

    void listUploads() const {
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
};

// Playlist stores video IDs instead of pointers to avoid ownership issues
class Playlist {
private:
    string name;
    vector<long long> videoIds;

public:
    Playlist() = default;
    Playlist(const string& n): name(n) {}

    void add(long long videoId, const string& videoTitle) {
        videoIds.push_back(videoId);
        Logger::info("Added \"" + videoTitle + "\" to playlist \"" + name + "\"");
    }

    const vector<long long>& getVideoIds() const { return videoIds; }
    const string& getName() const { return name; }
    
    void show(const unordered_map<long long, Video*>& videoMap) const {
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
};

// User can watch videos, comment, and manage playlists
class User {
private:
    string username;
    unordered_set<string> subscriptions;
    vector<long long> historyIds;  // Track watch history by video ID
    unordered_map<string, Playlist> playlists;

public:
    User() = default;
    User(const string& n): username(n) {}

    const string& getUsername() const { return username; }

    OpResult watch(Video* v) {
        if (!v) return OpResult(OpStatus::NOT_FOUND, "Video not found");
        
        historyIds.push_back(v->getId());
        return v->play();
    }

    OpResult addComment(Video* v, const string& text) {
        if (!v) return OpResult(OpStatus::NOT_FOUND, "Video not found");
        return v->addComment(username, text);
    }

    OpResult likeComment(Video* v, long long cid) {
        if (!v) return OpResult(OpStatus::NOT_FOUND, "Video not found");
        return v->likeComment(cid);
    }

    OpResult createPlaylist(const string& pname) {
        if (playlists.find(pname) != playlists.end()) {
            return OpResult(OpStatus::ALREADY_EXISTS, "Playlist exists");
        }
        playlists.emplace(pname, Playlist(pname));
        return OpResult(OpStatus::SUCCESS, "Created playlist \"" + pname + "\"");
    }

    Playlist* getPlaylist(const string& pname) {
        auto it = playlists.find(pname);
        return (it == playlists.end()) ? nullptr : &(it->second);
    }

    OpResult subscribeChannel(Channel& ch) {
        if (subscriptions.insert(ch.getName()).second) {
            return ch.subscribe(username);
        }
        return OpResult(OpStatus::ALREADY_EXISTS, "Already subscribed");
    }
};

// Helper to read a line of input with a prompt
static string readLine(const string& prompt) {
    cout << prompt << flush;
    string s;
    if (!getline(cin, s)) return string();
    return s;
}

// Helper to read an integer with validation
static int readInt(const string& prompt) {
    while (true) {
        string s = readLine(prompt);
        try {
            if (s.empty()) return -1;
            return stoi(s);
        } catch (...) {
            cout << "Invalid number, try again\n";
        }
    }
}

// Helper to read a long long with validation
static long long readLongLong(const string& prompt) {
    while (true) {
        string s = readLine(prompt);
        try {
            if (s.empty()) return -1;
            return stoll(s);
        } catch (...) {
            cout << "Invalid number, try again\n";
        }
    }
}

int main() {
    // Speed up I/O operations
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    // Main data structures
    unordered_map<string, User> users;
    unordered_map<string, Channel> channels;
    unordered_map<long long, Video*> videos;  // Videos are owned by channels

    // Create some default channels
    channels.emplace("KavyaTech", Channel("KavyaTech", "system", "C++ tutorials"));
    channels.emplace("IndieMusic", Channel("IndieMusic", "system", "Music channel"));

    // Add some initial videos
    {
        Video* v = channels["KavyaTech"].upload("C++ OOP Deep Dive", 900);
        videos[v->getId()] = v;
        v = channels["KavyaTech"].upload("Data Structures Overview", 720);
        videos[v->getId()] = v;
        v = channels["IndieMusic"].upload("Chill Loops", 300);
        videos[v->getId()] = v;
    }

    User* current = nullptr;  // Currently logged in user

    // Display the menu
    auto menu = [&]() {
        cout << "\n--- MyTube (Improved) ---\n";
        cout << "0  Show menu\n";
        cout << "1  Register\n";
        cout << "2  Login\n";
        cout << "3  Logout\n";
        cout << "4  Create channel (must be logged in)\n";
        cout << "5  Upload video to your channel (logged in)\n";
        cout << "6  Subscribe to channel (logged in)\n";
        cout << "7  Watch video by id\n";
        cout << "8  Add comment to video (logged in)\n";
        cout << "9  Like comment on video (logged in)\n";
        cout << "10 List comments on video\n";
        cout << "11 Search videos by title\n";
        cout << "12 Create playlist (logged in)\n";
        cout << "13 Add video to playlist (logged in)\n";
        cout << "14 Play playlist (logged in)\n";
        cout << "15 List all videos\n";
        cout << "16 List channel uploads\n";
        cout << "17 Toggle performance logging\n";
        cout << "18 Run performance benchmark\n";
        cout << "99 Exit\n";
    };

    menu();

    // Main command loop
    while (true) {
        string cmdS = readLine("\nAction> ");
        if (cmdS.empty()) continue;
        int cmd = -1;
        try { cmd = stoi(cmdS); } catch(...) { cout << "Enter a number\n"; continue; }

        if (cmd == 0) {
            menu();
        } 
        else if (cmd == 1) {
            // Register a new user
            string uname = readLine("Choose username: ");
            if (uname.empty()) { cout << "Empty name\n"; continue; }
            if (users.find(uname) != users.end()) { cout << "User exists\n"; continue; }
            users.emplace(uname, User(uname));
            cout << "Registered user: " << uname << "\n";
        } 
        else if (cmd == 2) {
            // Login
            string uname = readLine("Username: ");
            auto it = users.find(uname);
            if (it == users.end()) { cout << "No such user. Register first.\n"; continue; }
            current = &(it->second);
            cout << "Logged in as " << current->getUsername() << "\n";
        } 
        else if (cmd == 3) {
            // Logout
            if (!current) cout << "Not logged in\n";
            else { cout << "Logged out " << current->getUsername() << "\n"; current = nullptr; }
        } 
        else if (cmd == 4) {
            // Create a channel
            if (!current) { cout << "Login required\n"; continue; }
            string cname = readLine("Channel name: ");
            if (cname.empty()) { cout << "Empty name\n"; continue; }
            if (channels.find(cname) != channels.end()) { cout << "Channel exists\n"; continue; }
            string desc = readLine("Description: ");
            channels.emplace(cname, Channel(cname, current->getUsername(), desc));
            cout << "Channel \"" << cname << "\" created\n";
        } 
        else if (cmd == 5) {
            // Upload a video
            if (!current) { cout << "Login required\n"; continue; }
            string cname = readLine("Your channel name: ");
            auto cit = channels.find(cname);
            if (cit == channels.end()) { cout << "Channel not found\n"; continue; }
            if (cit->second.getOwner() != current->getUsername()) { 
                cout << "You do not own this channel\n"; continue; 
            }
            string title = readLine("Video title: ");
            int dur = readInt("Duration seconds: ");
            Video* v = cit->second.upload(title, dur);
            videos[v->getId()] = v;
        } 
        else if (cmd == 6) {
            // Subscribe to a channel
            if (!current) { cout << "Login required\n"; continue; }
            string cname = readLine("Channel name to subscribe: ");
            auto cit = channels.find(cname);
            if (cit == channels.end()) { cout << "Channel not found\n"; continue; }
            auto result = current->subscribeChannel(cit->second);
            cout << result.message << "\n";
        } 
        else if (cmd == 7) {
            // Watch a video
            long long vid = readLongLong("Video id to watch: ");
            auto vit = videos.find(vid);
            if (vit == videos.end()) { cout << "Video not found\n"; continue; }
            
            OpResult result = current ? current->watch(vit->second) : vit->second->play();
            cout << result.message << "\n";
        } 
        else if (cmd == 8) {
            // Add a comment
            if (!current) { cout << "Login required\n"; continue; }
            long long vid = readLongLong("Video id to comment on: ");
            auto vit = videos.find(vid);
            if (vit == videos.end()) { cout << "Video not found\n"; continue; }
            string text = readLine("Comment text: ");
            auto result = current->addComment(vit->second, text);
            cout << result.message << "\n";
        } 
        else if (cmd == 9) {
            // Like a comment
            if (!current) { cout << "Login required\n"; continue; }
            long long vid = readLongLong("Video id: ");
            auto vit = videos.find(vid);
            if (vit == videos.end()) { cout << "Video not found\n"; continue; }
            long long cid = readLongLong("Comment id to like: ");
            auto result = current->likeComment(vit->second, cid);
            cout << result.message << "\n";
        } 
        else if (cmd == 10) {
            // List comments on a video
            long long vid = readLongLong("Video id to list comments: ");
            auto vit = videos.find(vid);
            if (vit == videos.end()) { cout << "Video not found\n"; continue; }
            vit->second->listComments();
        } 
        else if (cmd == 11) {
            // Search videos by title
            PerfTimer timer("Search operation", PERF_LOGGING);
            
            string q = readLine("Search keyword: ");
            cout << "Results:\n";
            string low = q;
            transform(low.begin(), low.end(), low.begin(), ::tolower);
            
            for (auto &p : videos) {
                string t = p.second->getTitle();
                string tl = t;
                transform(tl.begin(), tl.end(), tl.begin(), ::tolower);
                if (tl.find(low) != string::npos) {
                    cout << "  [" << p.first << "] " << t 
                         << " (channel: " << p.second->getUploader() << ")\n";
                }
            }
        } 
        else if (cmd == 12) {
            // Create a playlist
            if (!current) { cout << "Login required\n"; continue; }
            string pname = readLine("Playlist name: ");
            auto result = current->createPlaylist(pname);
            cout << result.message << "\n";
        } 
        else if (cmd == 13) {
            // Add video to playlist
            if (!current) { cout << "Login required\n"; continue; }
            string pname = readLine("Playlist name: ");
            Playlist* p = current->getPlaylist(pname);
            if (!p) { cout << "Playlist not found\n"; continue; }
            long long vid = readLongLong("Video id to add: ");
            auto vit = videos.find(vid);
            if (vit == videos.end()) { cout << "Video not found\n"; continue; }
            p->add(vid, vit->second->getTitle());
        } 
        else if (cmd == 14) {
            // Play a playlist
            if (!current) { cout << "Login required\n"; continue; }
            string pname = readLine("Playlist name: ");
            Playlist* p = current->getPlaylist(pname);
            if (!p) { cout << "Playlist not found\n"; continue; }
            
            PerfTimer timer("Playlist playback", PERF_LOGGING);
            p->show(videos);
            
            cout << "Playing playlist \"" << pname << "\"\n";
            for (long long vid : p->getVideoIds()) {
                auto it = videos.find(vid);
                if (it != videos.end()) {
                    it->second->play();
                    it->second->pause();
                }
            }
        } 
        else if (cmd == 15) {
            // List all videos
            PerfTimer timer("List all videos", PERF_LOGGING);
            
            cout << "All videos:\n";
            for (auto &p : videos) {
                cout << "  [" << p.first << "] " << p.second->getTitle() 
                     << " (channel: " << p.second->getUploader() 
                     << ", views: " << p.second->getViews() << ")\n";
            }
        } 
        else if (cmd == 16) {
            // List channel uploads
            string cname = readLine("Channel name: ");
            auto cit = channels.find(cname);
            if (cit == channels.end()) { cout << "Channel not found\n"; continue; }
            cit->second.listUploads();
        } 
        else if (cmd == 17) {
            // Toggle performance logging
            PERF_LOGGING = !PERF_LOGGING;
            cout << "Performance logging " << (PERF_LOGGING ? "ENABLED" : "DISABLED") << "\n";
        } 
        else if (cmd == 18) {
            // Run performance benchmark
            cout << "\n=== PERFORMANCE BENCHMARK ===\n";
            PERF_LOGGING = true;
            
            // Test 1: Video lookup speed
            {
                PerfTimer t("1000 video lookups");
                for (int i = 0; i < 1000; ++i) {
                    volatile auto it = videos.find(1);
                    (void)it;  // Prevent compiler optimization
                }
            }
            
            // Test 2: Comment addition speed
            if (!videos.empty()) {
                Video* testVid = videos.begin()->second;
                PerfTimer t("100 comment additions");
                for (int i = 0; i < 100; ++i) {
                    testVid->addComment("benchuser", "test comment");
                }
            }
            
            // Test 3: Search performance
            {
                PerfTimer t("Video search");
                string query = "c++";
                transform(query.begin(), query.end(), query.begin(), ::tolower);
                int count = 0;
                for (auto &p : videos) {
                    string title = p.second->getTitle();
                    transform(title.begin(), title.end(), title.begin(), ::tolower);
                    if (title.find(query) != string::npos) count++;
                }
            }
            
            PERF_LOGGING = false;
            cout << "=== BENCHMARK COMPLETE ===\n\n";
        } 
        else if (cmd == 99) {
            cout << "Goodbye\n";
            break;
        } 
        else {
            cout << "Unknown command\n";
        }
    }

    return 0;
}
