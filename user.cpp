#include "user.h"

User::User() = default;
User::User(const string& n): username(n) {}

const string& User::getUsername() const { return username; }

OpResult User::watch(Video* v) {
    if (!v) return OpResult(OpStatus::NOT_FOUND, "Video not found");
    
    historyIds.push_back(v->getId());
    return v->play();
}

OpResult User::addComment(Video* v, const string& text) {
    if (!v) return OpResult(OpStatus::NOT_FOUND, "Video not found");
    return v->addComment(username, text);
}

OpResult User::likeComment(Video* v, long long cid) {
    if (!v) return OpResult(OpStatus::NOT_FOUND, "Video not found");
    return v->likeComment(cid);
}

OpResult User::createPlaylist(const string& pname) {
    if (playlists.find(pname) != playlists.end()) {
        return OpResult(OpStatus::ALREADY_EXISTS, "Playlist exists");
    }
    playlists.emplace(pname, Playlist(pname));
    return OpResult(OpStatus::SUCCESS, "Created playlist \"" + pname + "\"");
}

Playlist* User::getPlaylist(const string& pname) {
    auto it = playlists.find(pname);
    return (it == playlists.end()) ? nullptr : &(it->second);
}

OpResult User::subscribeChannel(Channel& ch) {
    if (subscriptions.insert(ch.getName()).second) {
        return ch.subscribe(username);
    }
    return OpResult(OpStatus::ALREADY_EXISTS, "Already subscribed");
}
