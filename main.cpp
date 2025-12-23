#include "user.h"

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
