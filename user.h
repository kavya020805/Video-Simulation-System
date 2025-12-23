#ifndef USER_H
#define USER_H

#include "video.h"

// User can watch videos, comment, and manage playlists
class User {
private:
    string username;
    unordered_set<string> subscriptions;
    vector<long long> historyIds;  // Track watch history by video ID
    unordered_map<string, Playlist> playlists;

public:
    User();
    User(const string& n);

    const string& getUsername() const;

    OpResult watch(Video* v);
    OpResult addComment(Video* v, const string& text);
    OpResult likeComment(Video* v, long long cid);
    OpResult createPlaylist(const string& pname);
    Playlist* getPlaylist(const string& pname);
    OpResult subscribeChannel(Channel& ch);
};

#endif
