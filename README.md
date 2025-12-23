# Video Simulation System (MyTube)

This project is a console-based video streaming simulation written in **C++**.  

The system models the core pieces of a video platform—users, channels, videos, comments, and playlists—and shows how these parts interact in a structured and maintainable way.

---

## What the Project Does

The application allows users to:
- Register and log in
- Create and manage channels
- Upload and watch videos
- Add comments and likes
- Create playlists and play them
- Search videos by title

All functionality runs in-memory and is designed to be easy to reason about and extend.

---

## How the System Is Designed

The project follows a clean object-oriented design where each class has a clear purpose:

- **User** handles subscriptions, watch history, and playlists  
- **Channel** owns videos and manages subscribers  
- **Video** tracks playback, views, and comments  
- **Comment** represents user feedback with likes and timestamps  
- **Playlist** groups videos together using lightweight references  

This separation keeps the code readable and avoids unnecessary coupling between components.

---

## Memory Ownership & Data Choices

A major focus of this project is making ownership explicit and predictable:

- Channels **own** their videos using unique ownership
- Videos are referenced elsewhere using IDs instead of shared pointers
- Frequently accessed data is stored in cache-friendly containers
- Thread-safe ID generation is used to avoid collisions

These decisions help keep the system simple, efficient, and easy to reason about.

---

## Error Handling & Reliability

Operations return clear results that describe:
- Whether an action succeeded or failed
- Why it failed, if applicable
- Any relevant identifiers

This makes control flow easier to follow and improves overall robustness.

---

## Performance Awareness

While the project is a simulation, it includes lightweight timing and measurement utilities to help understand how different operations behave.  
This encourages thinking about performance early, even in small systems.

---

## Author

**Kavya Chauhan**
