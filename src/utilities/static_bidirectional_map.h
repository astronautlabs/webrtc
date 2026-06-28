#pragma once
#include <vector>
#include <utility>
#include <map>

namespace node_webrtc {

    template <typename A, typename B>
    struct bidirectional_map {
        explicit bidirectional_map(std::vector<std::pair<A, B>> values) {
            for (const auto& pair : values) {
                to_b[pair.first] = pair.second;
                to_a[pair.second] = pair.first;
            }
        }
    private:
        std::map<A, B> to_b {};
        std::map<B, A> to_a {};

    public:

        bool contains(const A& a) { return to_b.contains(a); }
        bool contains(const B& b) { return to_a.contains(b); }

        B operator[](const A& a) {
            return to_b[a];
        }

        A operator[](const B& b) {
            return to_a[b];
        }
    };

} // namespace node_webrtc