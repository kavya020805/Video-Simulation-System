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

using namespace std;

class IdGen {
private:
    static atomic<long long> counter;
public:
    static long long next() { return ++counter; }
};
atomic<long long> IdGen::counter{0LL};

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
    string getAuthor() const { return author; }
    string getText() const { return text; }
    int getLikes() const { return likes; }
    void like() { likes++; }
};

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
    Video() = default;
    Video(const string& t, const string& u, int d)
        : id(IdGen::next()), title(t), uploader(u), durationSec(d), views(0), playing(false) {}

    long long getId() const { return id; }
    string getTitle() const { return title; }
    string getUploader() const { return uploader; }
    int getViews() const { return views; }

    void play() {
        if (!playing) {
            playing = true;
            ++views;
            cout << "Playing \"" << title << "\" (views: " << views << ")\n";
        } else {
            cout << "Already playing \"" << title << "\"\n";
        }
    }

    void pause() {
        if (playing) {
            playing = false;
            cout << "Paused \"" << title << "\"\n";
        } else {
            cout << "Not playing \"" << title << "\"\n";
        }
    }

    void addComment(const string& user, const string& text) {
        comments.emplace_back(user, text);
        cout << "Comment added (id=" << comments.back().getId() << ") by " << user << "\n";
    }

    bool likeComment(long long cid) {
        for (auto &c : comments) {
            if (c.getId() == cid) {
                c.like();
                cout << "Liked comment " << cid << " (likes=" << c.getLikes() << ")\n";
                return true;
            }
        }
        return false;
    }

    bool removeComment(long long cid, const string& requester, const string& channelOwner) {
        for (auto it = comments.begin(); it != comments.end(); ++it) {
            if (it->getId() == cid) {
                if (requester == it->getAuthor() || requester == channelOwner) {
                    comments.erase(it);
                    cout << "Comment " << cid << " removed\n";
                    return true;
                } else {
                    cout << "Permission denied to remove comment " << cid << "\n";
                    return false;
                }
            }
        }
        cout << "Comment id " << cid << " not found\n";
        return false;
    }

    void listComments() const {
        if (comments.empty()) {
            cout << "No comments\n";
            return;
        }
        cout << "Comments for \"" << title << "\":\n";
        for (const auto &c : comments) {
            cout << "  [" << c.getId() << "] " << c.getAuthor() << " (" << c.getLikes() << " likes): " << c.getText() << "\n";
        }
    }
};

class Channel {
private:
    string name;
    string owner;
    string description;
    vector<shared_ptr<Video>> uploads;
    unordered_set<string> subscribers;

public:
    Channel() = default;
    Channel(const string& n, const string& o, const string& d = "")
        : name(n), owner(o), description(d) {}

    string getName() const { return name; }
    string getOwner() const { return owner; }

    shared_ptr<Video> upload(const string& title, int dur) {
        auto v = make_shared<Video>(title, name, dur);
        uploads.push_back(v);
        cout << "Uploaded \"" << title << "\" (id=" << v->getId() << ") to channel " << name << "\n";
        return v;
    }

    void subscribe(const string& user) {
        if (subscribers.insert(user).second) {
            cout << user << " subscribed to " << name << "\n";
        } else {
            cout << user << " already subscribed\n";
        }
    }

    void unsubscribe(const string& user) {
        if (subscribers.erase(user)) {
            cout << user << " unsubscribed from " << name << "\n";
        } else {
            cout << user << " was not subscribed\n";
        }
    }

    void listUploads() const {
        if (uploads.empty()) {
            cout << "No uploads\n";
            return;
        }
        cout << "Uploads for channel " << name << ":\n";
        for (auto &v : uploads) {
            cout << "  [" << v->getId() << "] " << v->getTitle() << " (views: " << v->getViews() << ")\n";
        }
    }
};

class Playlist {
private:
    string name;
    vector<shared_ptr<Video>> vids;

public:
    Playlist() = default;
    Playlist(const string& n): name(n) {}

    void add(const shared_ptr<Video>& v) {
        vids.push_back(v);
        cout << "Added \"" << v->getTitle() << "\" to playlist \"" << name << "\"\n";
    }

    void playAll() {
        cout << "Playing playlist \"" << name << "\"\n";
        for (auto &v : vids) {
            if (v->getViews() > 0) v->pause(); // Simplified playing state check
            v->play();
            v->pause();
        }
    }

    void show() const {
        cout << "Playlist: " << name << "\n";
        if (vids.empty()) {
            cout << "  (empty)\n";
            return;
        }
        for (size_t i = 0; i < vids.size(); ++i) {
            cout << "  [" << (i+1) << "] " << vids[i]->getTitle() << " (id=" << vids[i]->getId() << ")\n";
        }
    }
};

class User {
private:
    string username;
    unordered_set<string> subscriptions;
    vector<shared_ptr<Video>> history;
    unordered_map<string, Playlist> playlists;
    unordered_set<string> ownedChannels;

public:
    User() = default;
    User(const string& n): username(n) {}

    string getUsername() const { return username; }

    void watch(const shared_ptr<Video>& v) {
        if (!v) {
            cout << "Video not found\n";
            return;
        }
        history.push_back(v);
        v->play();
    }

    void addComment(const shared_ptr<Video>& v, const string& text) {
        if (!v) {
            cout << "Video not found\n";
            return;
        }
        v->addComment(username, text);
    }

    void likeComment(const shared_ptr<Video>& v, long long cid) {
        if (!v) {
            cout << "Video not found\n";
            return;
        }
        if (!v->likeComment(cid)) {
            cout << "Comment not found\n";
        }
    }

    void createPlaylist(const string& pname) {
        if (playlists.find(pname) != playlists.end()) {
            cout << "Playlist exists\n";
            return;
        }
        playlists.emplace(pname, Playlist(pname));
        cout << "Created playlist \"" << pname << "\"\n";
    }

    Playlist* getPlaylist(const string& pname) {
        auto it = playlists.find(pname);
        if (it == playlists.end()) {
            return nullptr;
        }
        return &(it->second);
    }

    void subscribeChannel(Channel& ch) {
        if (subscriptions.insert(ch.getName()).second) {
            ch.subscribe(username);
        } else {
            cout << "Already subscribed\n";
        }
    }
};

// The rest of the code (readLine, readInt, main) remains unchanged...

static string readLine(const string& prompt) {
    cout << prompt << flush;
    string s;
    if (!getline(cin, s)) return string();
    return s;
}

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

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    unordered_map<string, User> users;
    unordered_map<string, Channel> channels;
    unordered_map<long long, shared_ptr<Video>> videos;

    // Pre-create channels
    channels.emplace("KavyaTech", Channel("KavyaTech", "system", "C++ tutorials"));
    channels.emplace("IndieMusic", Channel("IndieMusic", "system", "Music channel"));

    // Register system videos
    {
        auto v = channels["KavyaTech"].upload("C++ OOP Deep Dive", 900);
        videos[v->getId()] = v;
        v = channels["KavyaTech"].upload("Data Structures Overview", 720);
        videos[v->getId()] = v;
        v = channels["IndieMusic"].upload("Chill Loops", 300);
        videos[v->getId()] = v;
    }

    User* current = nullptr;

    auto menu = [&]() {
        cout << "\n--- MyTube (simple) ---\n";
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
        cout << "99 Exit\n";
    };

    menu();

    while (true) {
        string cmdS = readLine("\nAction> ");
        if (cmdS.empty()) continue;
        int cmd = -1;
        try { cmd = stoi(cmdS); } catch(...) { cout << "Enter a number\n"; continue; }

        if (cmd == 0) {
            menu();
        } else if (cmd == 1) {
            string uname = readLine("Choose username: ");
            if (uname.empty()) { cout << "Empty name\n"; continue; }
            if (users.find(uname) != users.end()) { cout << "User exists\n"; continue; }
            users.emplace(uname, User(uname));
            cout << "Registered user: " << uname << "\n";
        } else if (cmd == 2) {
            string uname = readLine("Username: ");
            auto it = users.find(uname);
            if (it == users.end()) { cout << "No such user. Register first.\n"; continue; }
            current = &(it->second);
            cout << "Logged in as " << current->getUsername() << "\n";
        } else if (cmd == 3) {
            if (!current) cout << "Not logged in\n";
            else { cout << "Logged out " << current->getUsername() << "\n"; current = nullptr; }
        } else if (cmd == 4) {
            if (!current) { cout << "Login required\n"; continue; }
            string cname = readLine("Channel name: ");
            if (cname.empty()) { cout << "Empty name\n"; continue; }
            if (channels.find(cname) != channels.end()) { cout << "Channel exists\n"; continue; }
            string desc = readLine("Description: ");
            channels.emplace(cname, Channel(cname, current->getUsername(), desc));
            cout << "Channel \"" << cname << "\" created\n";
        } else if (cmd == 5) {
            if (!current) { cout << "Login required\n"; continue; }
            string cname = readLine("Your channel name: ");
            auto cit = channels.find(cname);
            if (cit == channels.end()) { cout << "Channel not found\n"; continue; }
            if (cit->second.getOwner() != current->getUsername()) { cout << "You do not own this channel\n"; continue; }
            string title = readLine("Video title: ");
            int dur = readInt("Duration seconds: ");
            auto v = cit->second.upload(title, dur);
            videos[v->getId()] = v;
        } else if (cmd == 6) {
            if (!current) { cout << "Login required\n"; continue; }
            string cname = readLine("Channel name to subscribe: ");
            auto cit = channels.find(cname);
            if (cit == channels.end()) { cout << "Channel not found\n"; continue; }
            current->subscribeChannel(cit->second);
        } else if (cmd == 7) {
            long long vid = readInt("Video id to watch: ");
            auto vit = videos.find(vid);
            if (vit == videos.end()) { cout << "Video not found\n"; continue; }
            if (current) current->watch(vit->second);
            else vit->second->play();
        } else if (cmd == 8) {
            if (!current) { cout << "Login required\n"; continue; }
            long long vid = readInt("Video id to comment on: ");
            auto vit = videos.find(vid);
            if (vit == videos.end()) { cout << "Video not found\n"; continue; }
            string text = readLine("Comment text: ");
            current->addComment(vit->second, text);
        } else if (cmd == 9) {
            if (!current) { cout << "Login required\n"; continue; }
            long long vid = readInt("Video id: ");
            auto vit = videos.find(vid);
            if (vit == videos.end()) { cout << "Video not found\n"; continue; }
            long long cid = readInt("Comment id to like: ");
            current->likeComment(vit->second, cid);
        } else if (cmd == 10) {
            long long vid = readInt("Video id to list comments: ");
            auto vit = videos.find(vid);
            if (vit == videos.end()) { cout << "Video not found\n"; continue; }
            vit->second->listComments();
        } else if (cmd == 11) {
            string q = readLine("Search keyword: ");
            cout << "Results:\n";
            string low = q;
            transform(low.begin(), low.end(), low.begin(), ::tolower);
            for (auto &p : videos) {
                string t = p.second->getTitle();
                string tl = t;
                transform(tl.begin(), tl.end(), tl.begin(), ::tolower);
                if (tl.find(low) != string::npos) {
                    cout << "  [" << p.first << "] " << t << " (channel: " << p.second->getUploader() << ")\n";
                }
            }
        } else if (cmd == 12) {
            if (!current) { cout << "Login required\n"; continue; }
            string pname = readLine("Playlist name: ");
            current->createPlaylist(pname);
        } else if (cmd == 13) {
            if (!current) { cout << "Login required\n"; continue; }
            string pname = readLine("Playlist name: ");
            Playlist* p = current->getPlaylist(pname);
            if (!p) { cout << "Playlist not found\n"; continue; }
            long long vid = readInt("Video id to add: ");
            auto vit = videos.find(vid);
            if (vit == videos.end()) { cout << "Video not found\n"; continue; }
            p->add(vit->second);
        } else if (cmd == 14) {
            if (!current) { cout << "Login required\n"; continue; }
            string pname = readLine("Playlist name: ");
            Playlist* p = current->getPlaylist(pname);
            if (!p) { cout << "Playlist not found\n"; continue; }
            p->show();
            p->playAll();
        } else if (cmd == 15) {
            cout << "All videos:\n";
            for (auto &p : videos) {
                cout << "  [" << p.first << "] " << p.second->getTitle() << " (channel: " << p.second->getUploader() << ", views: " << p.second->getViews() << ")\n";
            }
        } else if (cmd == 16) {
            string cname = readLine("Channel name: ");
            auto cit = channels.find(cname);
            if (cit == channels.end()) { cout << "Channel not found\n"; continue; }
            cit->second.listUploads();
        } else if (cmd == 99) {
            cout << "Goodbye\n";
            break;
        } else {
            cout << "Unknown command\n";
        }
    }

    return 0;
}