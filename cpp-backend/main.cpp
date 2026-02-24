#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

struct User {
    int id;
    std::string name;
    std::string email;
    std::string role;
};

struct Task {
    int id;
    std::string title;
    std::string status;
    int userId;
};

static const std::vector<User> USERS = {
    {1, "John Doe", "john@example.com", "developer"},
    {2, "Jane Smith", "jane@example.com", "designer"},
    {3, "Bob Johnson", "bob@example.com", "manager"},
};

static const std::vector<Task> TASKS = {
    {1, "Implement authentication", "pending", 1},
    {2, "Design user interface", "in-progress", 2},
    {3, "Review code changes", "completed", 3},
};

std::string urlDecode(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '%' && i + 2 < s.size()) {
            int val = 0;
            std::istringstream iss(s.substr(i + 1, 2));
            if (iss >> std::hex >> val) {
                out.push_back(static_cast<char>(val));
                i += 2;
            } else {
                out.push_back(s[i]);
            }
        } else if (s[i] == '+') {
            out.push_back(' ');
        } else {
            out.push_back(s[i]);
        }
    }
    return out;
}

std::map<std::string, std::string> parseQuery(const std::string& query) {
    std::map<std::string, std::string> params;
    std::istringstream ss(query);
    std::string pair;
    while (std::getline(ss, pair, '&')) {
        auto eq = pair.find('=');
        if (eq != std::string::npos) {
            auto key = urlDecode(pair.substr(0, eq));
            auto val = urlDecode(pair.substr(eq + 1));
            params[key] = val;
        }
    }
    return params;
}

std::string jsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 2);
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out.push_back(c); break;
        }
    }
    return out;
}

std::string buildUsersJson() {
    std::ostringstream os;
    os << "{\"users\":[";
    for (size_t i = 0; i < USERS.size(); ++i) {
        const auto& u = USERS[i];
        if (i > 0) os << ',';
        os << "{\"id\":" << u.id
           << ",\"name\":\"" << jsonEscape(u.name) << "\""
           << ",\"email\":\"" << jsonEscape(u.email) << "\""
           << ",\"role\":\"" << jsonEscape(u.role) << "\"}";
    }
    os << "],\"count\":" << USERS.size() << "}";
    return os.str();
}

std::string buildUserJson(const User& u) {
    std::ostringstream os;
    os << "{\"id\":" << u.id
       << ",\"name\":\"" << jsonEscape(u.name) << "\""
       << ",\"email\":\"" << jsonEscape(u.email) << "\""
       << ",\"role\":\"" << jsonEscape(u.role) << "\"}";
    return os.str();
}

std::string buildTasksJson(const std::vector<Task>& tasks) {
    std::ostringstream os;
    os << "{\"tasks\":[";
    for (size_t i = 0; i < tasks.size(); ++i) {
        const auto& t = tasks[i];
        if (i > 0) os << ',';
        os << "{\"id\":" << t.id
           << ",\"title\":\"" << jsonEscape(t.title) << "\""
           << ",\"status\":\"" << jsonEscape(t.status) << "\""
           << ",\"userId\":" << t.userId << "}";
    }
    os << "],\"count\":" << tasks.size() << "}";
    return os.str();
}

std::string buildStatsJson() {
    int total = static_cast<int>(TASKS.size());
    int pending = 0, inProgress = 0, completed = 0;
    for (const auto& t : TASKS) {
        if (t.status == "pending") pending++;
        else if (t.status == "in-progress") inProgress++;
        else if (t.status == "completed") completed++;
    }
    std::ostringstream os;
    os << "{\"users\":{\"total\":" << USERS.size() << "},"
       << "\"tasks\":{\"total\":" << total
       << ",\"pending\":" << pending
       << ",\"inProgress\":" << inProgress
       << ",\"completed\":" << completed << "}}";
    return os.str();
}

std::string httpResponse(int status, const std::string& body, const std::string& statusText = "OK") {
    std::ostringstream os;
    os << "HTTP/1.1 " << status << ' ' << statusText << "\r\n";
    os << "Content-Type: application/json\r\n";
    os << "Access-Control-Allow-Origin: *\r\n";
    os << "Content-Length: " << body.size() << "\r\n";
    os << "Connection: close\r\n\r\n";
    os << body;
    return os.str();
}

void handleClient(int clientFd) {
    char buffer[4096];
    ssize_t n = recv(clientFd, buffer, sizeof(buffer) - 1, 0);
    if (n <= 0) {
        close(clientFd);
        return;
    }
    buffer[n] = '\0';
    std::string req(buffer);

    std::istringstream iss(req);
    std::string method, target, version;
    iss >> method >> target >> version;

    if (method != "GET") {
        std::string body = "{\"error\":\"Method not allowed\"}";
        auto resp = httpResponse(405, body, "Method Not Allowed");
        send(clientFd, resp.c_str(), resp.size(), 0);
        close(clientFd);
        return;
    }

    std::string path = target;
    std::string query;
    auto qpos = target.find('?');
    if (qpos != std::string::npos) {
        path = target.substr(0, qpos);
        query = target.substr(qpos + 1);
    }

    std::string body;

    if (path == "/health") {
        body = "{\"status\":\"ok\",\"message\":\"C++ backend is running\"}";
        auto resp = httpResponse(200, body);
        send(clientFd, resp.c_str(), resp.size(), 0);
        close(clientFd);
        return;
    }

    if (path == "/api/users") {
        body = buildUsersJson();
        auto resp = httpResponse(200, body);
        send(clientFd, resp.c_str(), resp.size(), 0);
        close(clientFd);
        return;
    }

    if (path.rfind("/api/users/", 0) == 0) {
        std::string idStr = path.substr(std::strlen("/api/users/"));
        int id = std::atoi(idStr.c_str());
        const User* found = nullptr;
        for (const auto& u : USERS) {
            if (u.id == id) { found = &u; break; }
        }
        if (!found) {
            body = "{\"error\":\"User not found\"}";
            auto resp = httpResponse(404, body, "Not Found");
            send(clientFd, resp.c_str(), resp.size(), 0);
            close(clientFd);
            return;
        }
        body = buildUserJson(*found);
        auto resp = httpResponse(200, body);
        send(clientFd, resp.c_str(), resp.size(), 0);
        close(clientFd);
        return;
    }

    if (path == "/api/tasks") {
        auto params = parseQuery(query);
        std::string status = params.count("status") ? params["status"] : "";
        std::string userIdStr = params.count("userId") ? params["userId"] : "";

        std::vector<Task> filtered;
        for (const auto& t : TASKS) {
            bool matchStatus = status.empty() || t.status == status;
            bool matchUser = true;
            if (!userIdStr.empty()) {
                int uid = std::atoi(userIdStr.c_str());
                matchUser = (t.userId == uid);
            }
            if (matchStatus && matchUser) {
                filtered.push_back(t);
            }
        }
        body = buildTasksJson(filtered);
        auto resp = httpResponse(200, body);
        send(clientFd, resp.c_str(), resp.size(), 0);
        close(clientFd);
        return;
    }

    if (path == "/api/stats") {
        body = buildStatsJson();
        auto resp = httpResponse(200, body);
        send(clientFd, resp.c_str(), resp.size(), 0);
        close(clientFd);
        return;
    }

    body = "{\"error\":\"Not found\"}";
    auto resp = httpResponse(404, body, "Not Found");
    send(clientFd, resp.c_str(), resp.size(), 0);
    close(clientFd);
}

int main() {
    int port = 8080;
    const char* env = std::getenv("PORT");
    if (env) {
        int p = std::atoi(env);
        if (p > 0 && p < 65536) port = p;
    }

    int serverFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        std::perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port));

    if (bind(serverFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::perror("bind");
        close(serverFd);
        return 1;
    }

    if (listen(serverFd, 16) < 0) {
        std::perror("listen");
        close(serverFd);
        return 1;
    }

    std::cout << "C++ backend server starting on http://localhost:" << port << std::endl;

    while (true) {
        sockaddr_in client{};
        socklen_t len = sizeof(client);
        int clientFd = accept(serverFd, reinterpret_cast<sockaddr*>(&client), &len);
        if (clientFd < 0) {
            if (errno == EINTR) continue;
            std::perror("accept");
            break;
        }
        std::thread(handleClient, clientFd).detach();
    }

    close(serverFd);
    return 0;
}
